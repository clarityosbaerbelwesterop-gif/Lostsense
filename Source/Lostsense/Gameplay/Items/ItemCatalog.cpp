#include "Lostsense/Gameplay/Items/ItemCatalog.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

constexpr std::uint8_t MaximumSockets = 16U;
constexpr std::uint8_t MaximumAffixes = 32U;
constexpr std::size_t MaximumResolvedModifiers = 4096U;

[[nodiscard]] bool
IsOperationValid(const Stats::ModifierOperation operation) noexcept {
  switch (operation) {
  case Stats::ModifierOperation::Additive:
  case Stats::ModifierOperation::Multiplicative:
    return true;
  }
  return false;
}

[[nodiscard]] bool IsAffixKindValid(const AffixKind kind) noexcept {
  switch (kind) {
  case AffixKind::Prefix:
  case AffixKind::Suffix:
    return true;
  }
  return false;
}

template <typename T>
[[nodiscard]] bool HasUniqueValidIds(const std::vector<T> &ids) {
  std::set<T> unique;
  for (const T id : ids) {
    if (!id.IsValid() || !unique.insert(id).second) {
      return false;
    }
  }
  return true;
}

template <typename T>
[[nodiscard]] bool Contains(const std::vector<T> &values, const T value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

bool ItemCatalog::AddRarity(RarityDefinition definition) {
  if (!IsRarityShapeValid(definition) || rarities_.contains(definition.Id)) {
    return false;
  }
  return rarities_.emplace(definition.Id, std::move(definition)).second;
}

bool ItemCatalog::AddAffix(AffixDefinition definition) {
  if (!IsAffixShapeValid(definition) || affixes_.contains(definition.Id)) {
    return false;
  }
  return affixes_.emplace(definition.Id, std::move(definition)).second;
}

bool ItemCatalog::AddItem(ItemDefinition definition) {
  if (!IsItemShapeValid(definition) || items_.contains(definition.Id)) {
    return false;
  }
  return items_.emplace(definition.Id, std::move(definition)).second;
}

bool ItemCatalog::Validate() const noexcept {
  if (rarities_.empty() || items_.empty()) {
    return false;
  }
  for (const auto &[id, item] : items_) {
    static_cast<void>(id);
    for (const RarityId rarity : item.AllowedRarities) {
      const auto found = rarities_.find(rarity);
      if (found == rarities_.end() ||
          item.MaximumItemLevel < found->second.MinimumItemLevel ||
          item.MinimumItemLevel > found->second.MaximumItemLevel) {
        return false;
      }
    }
    if (item.FixedRarity.IsValid()) {
      const auto fixed = rarities_.find(item.FixedRarity);
      if (fixed == rarities_.end() ||
          item.MaximumItemLevel < fixed->second.MinimumItemLevel ||
          item.MinimumItemLevel > fixed->second.MaximumItemLevel) {
        return false;
      }
    }
    if (item.MaximumStackSize == 1U && item.AllowedRarities.empty() &&
        !item.FixedRarity.IsValid()) {
      return false;
    }
    if (item.Unique && !item.FixedRarity.IsValid()) {
      return false;
    }
  }
  return true;
}

const RarityDefinition *
ItemCatalog::FindRarity(const RarityId id) const noexcept {
  const auto it = rarities_.find(id);
  return it == rarities_.end() ? nullptr : &it->second;
}

const AffixDefinition *ItemCatalog::FindAffix(const AffixId id) const noexcept {
  const auto it = affixes_.find(id);
  return it == affixes_.end() ? nullptr : &it->second;
}

const ItemDefinition *ItemCatalog::FindItem(const ItemId id) const noexcept {
  const auto it = items_.find(id);
  return it == items_.end() ? nullptr : &it->second;
}

bool ItemCatalog::IsItemAllowedForClass(
    const ItemDefinition &definition, const ClassId ownerClass) const noexcept {
  return !ownerClass.IsValid() || definition.AllowedClasses.empty() ||
         Contains(definition.AllowedClasses, ownerClass);
}

bool ItemCatalog::IsRarityAllowed(const ItemDefinition &item,
                                  const RarityId rarity) const noexcept {
  if (!rarity.IsValid()) {
    return false;
  }
  if (item.FixedRarity.IsValid()) {
    return item.FixedRarity == rarity;
  }
  return Contains(item.AllowedRarities, rarity);
}

bool ItemCatalog::IsAffixEligible(const AffixDefinition &affix,
                                  const ItemDefinition &item,
                                  const std::uint32_t itemLevel,
                                  const ClassId ownerClass) const noexcept {
  if (itemLevel < affix.MinimumItemLevel ||
      itemLevel > affix.MaximumItemLevel) {
    return false;
  }
  if (!affix.AllowedItemTypes.empty() &&
      !Contains(affix.AllowedItemTypes, item.Type)) {
    return false;
  }
  if (!affix.AllowedWeaponTypes.empty() &&
      (!item.WeaponType.IsValid() ||
       !Contains(affix.AllowedWeaponTypes, item.WeaponType))) {
    return false;
  }
  if (!affix.AllowedEquipmentSlots.empty()) {
    bool slotMatch = false;
    for (const EquipmentSlotId slot : item.AllowedEquipmentSlots) {
      if (Contains(affix.AllowedEquipmentSlots, slot)) {
        slotMatch = true;
        break;
      }
    }
    if (!slotMatch) {
      return false;
    }
  }
  return !ownerClass.IsValid() || affix.AllowedClasses.empty() ||
         Contains(affix.AllowedClasses, ownerClass);
}

bool ItemCatalog::ValidateInstance(const ItemInstance &instance,
                                   const ClassId ownerClass) const noexcept {
  const ItemDefinition *item = FindItem(instance.DefinitionId);
  const RarityDefinition *rarity = FindRarity(instance.Rarity);
  if (!instance.InstanceId.IsValid() ||
      instance.InstanceId.Value > MaximumPersistentItemInstanceId ||
      item == nullptr || item->MaximumStackSize != 1U || rarity == nullptr ||
      !IsItemAllowedForClass(*item, ownerClass) ||
      !IsRarityAllowed(*item, instance.Rarity) ||
      instance.ItemLevel < item->MinimumItemLevel ||
      instance.ItemLevel > item->MaximumItemLevel ||
      instance.ItemLevel < rarity->MinimumItemLevel ||
      instance.ItemLevel > rarity->MaximumItemLevel ||
      instance.Affixes.size() < rarity->MinimumAffixes ||
      instance.Affixes.size() > rarity->MaximumAffixes ||
      instance.SocketedItems.size() > item->SocketCapacity) {
    return false;
  }

  std::set<AffixId> affixIds;
  std::set<AffixExclusiveGroupId> groups;
  for (const RolledAffix &rolled : instance.Affixes) {
    const AffixDefinition *affix = FindAffix(rolled.Id);
    if (affix == nullptr || !affixIds.insert(rolled.Id).second ||
        !IsAffixEligible(*affix, *item, instance.ItemLevel, ownerClass) ||
        rolled.Magnitudes.size() != affix->Modifiers.size()) {
      return false;
    }
    if (affix->ExclusiveGroup.IsValid() &&
        !groups.insert(affix->ExclusiveGroup).second) {
      return false;
    }
    for (std::size_t index = 0; index < rolled.Magnitudes.size(); ++index) {
      const double magnitude = rolled.Magnitudes[index];
      const AffixModifierRollDefinition &roll = affix->Modifiers[index];
      if (!std::isfinite(magnitude) || magnitude < roll.MinimumMagnitude ||
          magnitude > roll.MaximumMagnitude) {
        return false;
      }
    }
  }

  if (!instance.SocketedItems.empty() && item->SocketCapacity == 0U) {
    return false;
  }
  for (const ItemId socketedId : instance.SocketedItems) {
    const ItemDefinition *socketed = FindItem(socketedId);
    if (socketed == nullptr || !socketed->CanBeSocketed) {
      return false;
    }
  }
  return true;
}

bool ItemCatalog::ResolveModifiers(
    const ItemInstance &instance,
    std::vector<ResolvedItemModifier> &modifiers) const {
  if (!ValidateInstance(instance)) {
    return false;
  }
  std::vector<ResolvedItemModifier> resolved;
  const ItemDefinition &item = *FindItem(instance.DefinitionId);
  resolved.reserve(item.BaseModifiers.size() + instance.Affixes.size());
  for (const ItemAttributeModifier &modifier : item.BaseModifiers) {
    resolved.push_back(
        {modifier.Attribute, modifier.Operation, modifier.Magnitude});
  }
  for (const RolledAffix &rolled : instance.Affixes) {
    const AffixDefinition &affix = *FindAffix(rolled.Id);
    for (std::size_t index = 0; index < affix.Modifiers.size(); ++index) {
      resolved.push_back({affix.Modifiers[index].Attribute,
                          affix.Modifiers[index].Operation,
                          rolled.Magnitudes[index]});
    }
  }
  for (const ItemId socketedId : instance.SocketedItems) {
    const ItemDefinition &socketed = *FindItem(socketedId);
    for (const ItemAttributeModifier &modifier : socketed.BaseModifiers) {
      resolved.push_back(
          {modifier.Attribute, modifier.Operation, modifier.Magnitude});
    }
  }
  if (resolved.size() > MaximumResolvedModifiers) {
    return false;
  }
  modifiers = std::move(resolved);
  return true;
}

void ItemCatalog::CollectEligibleAffixes(
    const ItemDefinition &item, const std::uint32_t itemLevel,
    const ClassId ownerClass,
    std::vector<const AffixDefinition *> &affixes) const {
  affixes.clear();
  for (const auto &[id, affix] : affixes_) {
    static_cast<void>(id);
    if (IsAffixEligible(affix, item, itemLevel, ownerClass)) {
      affixes.push_back(&affix);
    }
  }
}

bool ItemCatalog::IsRarityShapeValid(
    const RarityDefinition &definition) noexcept {
  return definition.Id.IsValid() && definition.MinimumItemLevel > 0U &&
         definition.MinimumItemLevel <= definition.MaximumItemLevel &&
         definition.MinimumAffixes <= definition.MaximumAffixes &&
         definition.MaximumAffixes <= MaximumAffixes;
}

bool ItemCatalog::IsAffixShapeValid(
    const AffixDefinition &definition) noexcept {
  if (!definition.Id.IsValid() || !IsAffixKindValid(definition.Kind) ||
      definition.Weight == 0U || definition.MinimumItemLevel == 0U ||
      definition.MinimumItemLevel > definition.MaximumItemLevel ||
      !HasUniqueValidIds(definition.AllowedItemTypes) ||
      !HasUniqueValidIds(definition.AllowedWeaponTypes) ||
      !HasUniqueValidIds(definition.AllowedEquipmentSlots) ||
      !HasUniqueValidIds(definition.AllowedClasses) ||
      definition.Modifiers.empty()) {
    return false;
  }
  for (const AffixModifierRollDefinition &modifier : definition.Modifiers) {
    if (!modifier.Attribute.IsValid() ||
        !IsOperationValid(modifier.Operation) ||
        !std::isfinite(modifier.MinimumMagnitude) ||
        !std::isfinite(modifier.MaximumMagnitude) ||
        modifier.MinimumMagnitude > modifier.MaximumMagnitude ||
        (modifier.Operation == Stats::ModifierOperation::Multiplicative &&
         modifier.MinimumMagnitude < 0.0)) {
      return false;
    }
  }
  return true;
}

bool ItemCatalog::IsItemShapeValid(const ItemDefinition &definition) noexcept {
  if (!definition.Id.IsValid() || !definition.Type.IsValid() ||
      definition.MinimumItemLevel == 0U ||
      definition.MinimumItemLevel > definition.MaximumItemLevel ||
      definition.MaximumStackSize == 0U ||
      definition.SocketCapacity > MaximumSockets ||
      !HasUniqueValidIds(definition.AllowedEquipmentSlots) ||
      !HasUniqueValidIds(definition.AllowedRarities) ||
      !HasUniqueValidIds(definition.AllowedClasses) ||
      !HasUniqueValidIds(definition.Tags) ||
      !HasUniqueValidIds(definition.UniqueEffects) ||
      !HasUniqueValidIds(definition.AbilityMutations)) {
    return false;
  }
  if ((!definition.AllowedEquipmentSlots.empty() ||
       definition.WeaponType.IsValid() || definition.Unique) &&
      definition.MaximumStackSize != 1U) {
    return false;
  }
  if (definition.FixedRarity.IsValid() && !definition.AllowedRarities.empty() &&
      !Contains(definition.AllowedRarities, definition.FixedRarity)) {
    return false;
  }
  if (definition.Unique && !definition.FixedRarity.IsValid()) {
    return false;
  }
  if (definition.CanBeSocketed &&
      (!definition.AllowedEquipmentSlots.empty() ||
       definition.WeaponType.IsValid() || definition.SocketCapacity != 0U ||
       definition.MaximumStackSize == 1U || definition.Unique)) {
    return false;
  }
  for (const ItemAttributeModifier &modifier : definition.BaseModifiers) {
    if (!modifier.Attribute.IsValid() ||
        !IsOperationValid(modifier.Operation) ||
        !std::isfinite(modifier.Magnitude) ||
        (modifier.Operation == Stats::ModifierOperation::Multiplicative &&
         modifier.Magnitude < 0.0)) {
      return false;
    }
  }
  return true;
}

} // namespace Lostsense::Gameplay
