#include "Lostsense/Gameplay/Items/Equipment.h"

#include <algorithm>
#include <set>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

constexpr std::uint64_t EquipmentModifierPrefix = 1ULL << 61U;

} // namespace

EquipmentRuntime::EquipmentRuntime(const ItemCatalog &catalog,
                                   Combat::Combatant &owner,
                                   const ClassId ownerClass) noexcept
    : catalog_{catalog}, owner_{owner}, ownerClass_{ownerClass},
      valid_{catalog.Validate() && owner.Id().IsValid() &&
             ownerClass.IsValid()} {}

const ItemInstance *
EquipmentRuntime::EquippedAt(const EquipmentSlotId slot) const noexcept {
  const auto it = equipped_.find(slot);
  return it == equipped_.end() ? nullptr : &it->second;
}

bool EquipmentRuntime::ContainsInstance(
    const ItemInstanceId instance) const noexcept {
  return std::any_of(equipped_.begin(), equipped_.end(),
                     [instance](const auto &entry) {
                       return entry.second.InstanceId == instance;
                     });
}

bool EquipmentRuntime::HasAbilityMutation(
    const AbilityMutationId mutation) const noexcept {
  if (!mutation.IsValid()) {
    return false;
  }
  for (const auto &[slot, instance] : equipped_) {
    static_cast<void>(slot);
    const ItemDefinition *definition = catalog_.FindItem(instance.DefinitionId);
    if (definition != nullptr &&
        std::find(definition->AbilityMutations.begin(),
                  definition->AbilityMutations.end(),
                  mutation) != definition->AbilityMutations.end()) {
      return true;
    }
  }
  return false;
}

bool EquipmentRuntime::GrantsUniqueEffect(
    const EffectId effect) const noexcept {
  if (!effect.IsValid()) {
    return false;
  }
  for (const auto &[slot, instance] : equipped_) {
    static_cast<void>(slot);
    const ItemDefinition *definition = catalog_.FindItem(instance.DefinitionId);
    if (definition != nullptr &&
        std::find(definition->UniqueEffects.begin(),
                  definition->UniqueEffects.end(),
                  effect) != definition->UniqueEffects.end()) {
      return true;
    }
  }
  return false;
}

EquipmentResult
EquipmentRuntime::EquipFromInventory(Inventory &inventory,
                                     const ItemInstanceId instanceId,
                                     const EquipmentSlotId slot) {
  if (!valid_ || !inventory.IsValid()) {
    return EquipmentResult::InvalidRuntime;
  }
  if (!slot.IsValid()) {
    return EquipmentResult::InvalidSlot;
  }
  const ItemInstance *candidate = inventory.FindInstance(instanceId);
  if (candidate == nullptr) {
    return EquipmentResult::MissingItem;
  }
  const ItemDefinition *definition = catalog_.FindItem(candidate->DefinitionId);
  if (definition == nullptr || definition->AllowedEquipmentSlots.empty()) {
    return EquipmentResult::InvalidSlot;
  }
  if (!IsSlotAllowed(*definition, slot)) {
    return EquipmentResult::WrongSlot;
  }
  if (!IsInstanceAllowed(*candidate)) {
    return EquipmentResult::WrongClass;
  }

  const InventoryState inventoryState = inventory.CaptureState();
  const Combat::CombatantState combatState = owner_.CaptureState();
  const auto previousEquipment = equipped_;
  const auto rollback = [&]() {
    static_cast<void>(inventory.RestoreState(inventoryState));
    static_cast<void>(owner_.RestoreState(combatState));
    equipped_ = previousEquipment;
  };

  ItemInstance incoming;
  if (inventory.TakeInstance(instanceId, incoming) !=
      InventoryResult::Success) {
    return EquipmentResult::InventoryFailure;
  }

  const auto current = equipped_.find(slot);
  if (current != equipped_.end()) {
    const ItemInstance replaced = current->second;
    if (!RemoveModifiers(replaced)) {
      rollback();
      return EquipmentResult::ModifierFailure;
    }
    equipped_.erase(current);
    if (inventory.AddInstance(replaced) != InventoryResult::Success) {
      rollback();
      return EquipmentResult::InventoryFailure;
    }
  }

  if (!InstallModifiers(incoming)) {
    rollback();
    return EquipmentResult::ModifierFailure;
  }
  equipped_.emplace(slot, std::move(incoming));
  return EquipmentResult::Success;
}

EquipmentResult
EquipmentRuntime::UnequipToInventory(Inventory &inventory,
                                     const EquipmentSlotId slot) {
  if (!valid_ || !inventory.IsValid()) {
    return EquipmentResult::InvalidRuntime;
  }
  if (!slot.IsValid()) {
    return EquipmentResult::InvalidSlot;
  }
  const auto current = equipped_.find(slot);
  if (current == equipped_.end()) {
    return EquipmentResult::MissingItem;
  }

  const InventoryState inventoryState = inventory.CaptureState();
  const Combat::CombatantState combatState = owner_.CaptureState();
  const auto previousEquipment = equipped_;
  const ItemInstance item = current->second;

  if (inventory.AddInstance(item) != InventoryResult::Success) {
    return EquipmentResult::InventoryFailure;
  }
  if (!RemoveModifiers(item)) {
    static_cast<void>(inventory.RestoreState(inventoryState));
    static_cast<void>(owner_.RestoreState(combatState));
    equipped_ = previousEquipment;
    return EquipmentResult::ModifierFailure;
  }
  equipped_.erase(current);
  return EquipmentResult::Success;
}

EquipmentState EquipmentRuntime::CaptureState() const {
  EquipmentState state;
  state.Items.reserve(equipped_.size());
  for (const auto &[slot, item] : equipped_) {
    state.Items.push_back({slot, item});
  }
  return state;
}

bool EquipmentRuntime::RestoreState(const EquipmentState &state,
                                    const Inventory &inventory) {
  if (!valid_ || !inventory.IsValid() || !ValidateState(state, inventory)) {
    return false;
  }
  const Combat::CombatantState combatState = owner_.CaptureState();
  const auto previousEquipment = equipped_;
  const auto rollback = [&]() {
    static_cast<void>(owner_.RestoreState(combatState));
    equipped_ = previousEquipment;
  };

  for (const auto &[slot, item] : equipped_) {
    static_cast<void>(slot);
    if (!RemoveModifiers(item)) {
      rollback();
      return false;
    }
  }
  equipped_.clear();

  for (const EquippedItemState &entry : state.Items) {
    if (!InstallModifiers(entry.Item)) {
      rollback();
      return false;
    }
    equipped_.emplace(entry.Slot, entry.Item);
  }
  return true;
}

bool EquipmentRuntime::IsInstanceAllowed(
    const ItemInstance &instance) const noexcept {
  if (!catalog_.ValidateInstance(instance, ownerClass_)) {
    return false;
  }
  const ItemDefinition &item = *catalog_.FindItem(instance.DefinitionId);
  if (item.AllowedEquipmentSlots.empty() ||
      !catalog_.IsItemAllowedForClass(item, ownerClass_)) {
    return false;
  }
  for (const RolledAffix &rolled : instance.Affixes) {
    const AffixDefinition &affix = *catalog_.FindAffix(rolled.Id);
    if (!catalog_.IsAffixEligible(affix, item, instance.ItemLevel,
                                  ownerClass_)) {
      return false;
    }
  }
  return true;
}

bool EquipmentRuntime::IsSlotAllowed(
    const ItemDefinition &definition,
    const EquipmentSlotId slot) const noexcept {
  return std::find(definition.AllowedEquipmentSlots.begin(),
                   definition.AllowedEquipmentSlots.end(),
                   slot) != definition.AllowedEquipmentSlots.end();
}

Stats::ModifierId
EquipmentRuntime::ModifierIdFor(const ItemInstanceId instance,
                                const std::size_t index) const noexcept {
  if (!instance.IsValid() || instance.Value > MaximumPersistentItemInstanceId ||
      index >= 65535U) {
    return {};
  }
  const std::uint64_t encoded =
      (instance.Value << 16U) | (static_cast<std::uint64_t>(index) + 1U);
  return Stats::ModifierId{EquipmentModifierPrefix | encoded};
}

bool EquipmentRuntime::InstallModifiers(const ItemInstance &instance) {
  std::vector<ResolvedItemModifier> modifiers;
  if (!IsInstanceAllowed(instance) ||
      !catalog_.ResolveModifiers(instance, modifiers)) {
    return false;
  }
  std::size_t installed = 0U;
  for (std::size_t index = 0; index < modifiers.size(); ++index) {
    const Stats::ModifierId id = ModifierIdFor(instance.InstanceId, index);
    const ResolvedItemModifier &source = modifiers[index];
    if (!id.IsValid() || owner_.Attributes().HasModifier(id) ||
        !owner_.AddAttributeModifier({id, source.Attribute, source.Operation,
                                      Stats::ModifierSource::Equipment,
                                      source.Magnitude})) {
      for (std::size_t rollback = 0; rollback < installed; ++rollback) {
        static_cast<void>(owner_.RemoveAttributeModifier(
            ModifierIdFor(instance.InstanceId, rollback)));
      }
      return false;
    }
    ++installed;
  }
  return true;
}

bool EquipmentRuntime::RemoveModifiers(const ItemInstance &instance) noexcept {
  std::vector<ResolvedItemModifier> modifiers;
  if (!catalog_.ResolveModifiers(instance, modifiers)) {
    return false;
  }
  for (std::size_t index = 0; index < modifiers.size(); ++index) {
    const Stats::ModifierId id = ModifierIdFor(instance.InstanceId, index);
    if (!id.IsValid() || !owner_.Attributes().HasModifier(id)) {
      return false;
    }
  }
  for (std::size_t index = 0; index < modifiers.size(); ++index) {
    if (!owner_.RemoveAttributeModifier(
            ModifierIdFor(instance.InstanceId, index))) {
      return false;
    }
  }
  return true;
}

bool EquipmentRuntime::ValidateState(
    const EquipmentState &state, const Inventory &inventory) const noexcept {
  std::set<EquipmentSlotId> slots;
  std::set<ItemInstanceId> instances;
  for (const EquippedItemState &entry : state.Items) {
    const ItemDefinition *definition =
        catalog_.FindItem(entry.Item.DefinitionId);
    if (!entry.Slot.IsValid() || definition == nullptr ||
        !IsSlotAllowed(*definition, entry.Slot) ||
        !slots.insert(entry.Slot).second ||
        !instances.insert(entry.Item.InstanceId).second ||
        inventory.ContainsInstance(entry.Item.InstanceId) ||
        !IsInstanceAllowed(entry.Item)) {
      return false;
    }
  }
  return true;
}

} // namespace Lostsense::Gameplay
