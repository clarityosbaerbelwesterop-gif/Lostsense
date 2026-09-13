#include "Lostsense/Gameplay/Persistence/CharacterPersistence.h"

#include "Lostsense/Stats/CombatAttributes.h"

#include <algorithm>
#include <bit>
#include <limits>
#include <set>
#include <vector>

namespace Lostsense::Gameplay {
namespace {

[[nodiscard]] bool IsDerivedSource(const Stats::ModifierSource source) {
  return source == Stats::ModifierSource::Equipment ||
         source == Stats::ModifierSource::SkillTree ||
         source == Stats::ModifierSource::StatusEffect;
}

[[nodiscard]] bool SameModifier(const Stats::AttributeModifier &left,
                                const Stats::AttributeModifier &right) {
  return left.Id == right.Id && left.Attribute == right.Attribute &&
         left.Operation == right.Operation && left.Source == right.Source &&
         std::bit_cast<std::uint64_t>(left.Magnitude) ==
             std::bit_cast<std::uint64_t>(right.Magnitude);
}

[[nodiscard]] std::vector<Stats::AttributeModifier>
DerivedModifiers(const Stats::AttributeSetState &state) {
  std::vector<Stats::AttributeModifier> result;
  for (const Stats::AttributeModifier &modifier : state.Modifiers) {
    if (IsDerivedSource(modifier.Source)) {
      result.push_back(modifier);
    }
  }
  std::sort(
      result.begin(), result.end(),
      [](const Stats::AttributeModifier &left,
         const Stats::AttributeModifier &right) { return left.Id < right.Id; });
  return result;
}

[[nodiscard]] const AbilityRuntimeEntryState *
FindAbilityState(const AbilityRuntimeState &state, const AbilityId id) {
  const auto found = std::find_if(
      state.Abilities.begin(), state.Abilities.end(),
      [id](const AbilityRuntimeEntryState &entry) { return entry.Id == id; });
  return found == state.Abilities.end() ? nullptr : &*found;
}

[[nodiscard]] std::uint64_t
MaximumPersistentInstance(const CharacterSaveState &state, bool &valid) {
  valid = true;
  std::set<ItemInstanceId> ids;
  std::uint64_t maximum = 0U;
  for (const ItemInstance &item : state.Inventory.Instances) {
    if (!item.InstanceId.IsValid() || !ids.insert(item.InstanceId).second) {
      valid = false;
      return 0U;
    }
    maximum = std::max(maximum, item.InstanceId.Value);
  }
  for (const EquippedItemState &entry : state.Equipment.Items) {
    if (!entry.Item.InstanceId.IsValid() ||
        !ids.insert(entry.Item.InstanceId).second) {
      valid = false;
      return 0U;
    }
    maximum = std::max(maximum, entry.Item.InstanceId.Value);
  }
  return maximum;
}

} // namespace

CharacterPersistence::CharacterPersistence(
    const CharacterPersistentId character, const ClassId characterClass,
    Combat::Combatant &combatant, Core::DeterministicRandom &random,
    EffectRuntime &effects, AbilityRuntime &abilities,
    SkillTreeRuntime &skillTree, AbilityLoadout &loadout, Inventory &inventory,
    EquipmentRuntime &equipment, LootRuntime &loot) noexcept
    : character_{character}, characterClass_{characterClass},
      combatant_{combatant}, random_{random}, effects_{effects},
      abilities_{abilities}, skillTree_{skillTree}, loadout_{loadout},
      inventory_{inventory}, equipment_{equipment}, loot_{loot} {
  valid_ = character_.IsValid() && characterClass_.IsValid() &&
           combatant_.Kind() == Combat::CombatantKind::Player &&
           effects_.IsValid() && effects_.OwnerId() == combatant_.Id() &&
           abilities_.IsValid() && abilities_.OwnerClass() == characterClass_ &&
           loadout_.IsValid() && loadout_.AbilityAuthority() == &abilities_ &&
           skillTree_.IsValid() && inventory_.IsValid() &&
           equipment_.IsValid() && loot_.IsValid();
}

CharacterSaveState CharacterPersistence::CaptureState() const {
  CharacterSaveState state;
  state.SchemaVersion = CurrentSaveSchemaVersion;
  state.Character = character_;
  state.CharacterClass = characterClass_;
  state.SkillTreeDefinition = skillTree_.TreeId();
  state.Combatant = combatant_.CaptureState();
  Stats::AttributeSet durableAttributes = combatant_.Attributes();
  static_cast<void>(durableAttributes.RemoveModifiersBySource(
      Stats::ModifierSource::Temporary));
  state.Combatant.Attributes = durableAttributes.CaptureState();
  state.Combatant.Health.Maximum =
      durableAttributes.Get(Stats::CombatAttributes::MaxHealth);
  state.Combatant.Health.Current =
      std::min(state.Combatant.Health.Current, state.Combatant.Health.Maximum);
  state.Combatant.Health.Dead = state.Combatant.Health.Current <= 0.0;
  state.Combatant.Resource.Maximum =
      durableAttributes.Get(Stats::CombatAttributes::MaxResource);
  state.Combatant.Resource.Current = std::min(state.Combatant.Resource.Current,
                                              state.Combatant.Resource.Maximum);
  state.Random = random_.CaptureState();
  state.Effects = effects_.CaptureState();
  state.Abilities = abilities_.CaptureState();
  state.SkillTree = skillTree_.CaptureState();
  state.Loadout = loadout_.CaptureState();
  state.Inventory = inventory_.CaptureState();
  state.Equipment = equipment_.CaptureState();
  state.Loot = loot_.CaptureState();
  return state;
}

bool CharacterPersistence::ValidateState(
    const CharacterSaveState &state) const {
  if (!valid_ || state.SchemaVersion != CurrentSaveSchemaVersion ||
      state.Character != character_ ||
      state.CharacterClass != characterClass_ ||
      state.SkillTreeDefinition != skillTree_.TreeId() ||
      state.Combatant.Id != combatant_.Id() ||
      state.Combatant.Kind != combatant_.Kind()) {
    return false;
  }
  return ValidateEnvelope(state) && ValidateCrossState(state);
}

CharacterRestoreResult
CharacterPersistence::RestoreState(const CharacterSaveState &state) {
  if (!valid_) {
    return CharacterRestoreResult::InvalidRuntime;
  }
  if (state.SchemaVersion != CurrentSaveSchemaVersion) {
    return CharacterRestoreResult::UnsupportedSchema;
  }
  if (state.Character != character_ ||
      state.CharacterClass != characterClass_ ||
      state.SkillTreeDefinition != skillTree_.TreeId() ||
      state.Combatant.Id != combatant_.Id() ||
      state.Combatant.Kind != combatant_.Kind()) {
    return CharacterRestoreResult::IdentityMismatch;
  }
  if (!ValidateEnvelope(state) || !ValidateCrossState(state)) {
    return CharacterRestoreResult::InvalidAggregateState;
  }

  CharacterSaveState before = CaptureState();
  // Persistent capture intentionally removes transient Attribute modifiers, but
  // a failed load must roll back the exact live runtime, including transients.
  before.Combatant = combatant_.CaptureState();
  if (ApplyState(state)) {
    return CharacterRestoreResult::Success;
  }
  if (!ApplyState(before)) {
    return CharacterRestoreResult::RollbackFailed;
  }
  return CharacterRestoreResult::SubsystemRestoreFailed;
}

bool CharacterPersistence::ValidateEnvelope(
    const CharacterSaveState &state) const {
  if (!state.Character.IsValid() || !state.CharacterClass.IsValid() ||
      !state.SkillTreeDefinition.IsValid() ||
      (state.Random.Increment & 1U) == 0U ||
      state.Loot.NextInstanceValue == 0U) {
    return false;
  }
  for (const Stats::AttributeModifier &modifier :
       state.Combatant.Attributes.Modifiers) {
    // Temporary combat modifiers are intentionally transient and must not be
    // resurrected by a persistent save payload.
    if (modifier.Source == Stats::ModifierSource::Temporary) {
      return false;
    }
  }
  return true;
}

bool CharacterPersistence::ValidateCrossState(
    const CharacterSaveState &state) const {
  bool uniqueInstances = false;
  const std::uint64_t maximumInstance =
      MaximumPersistentInstance(state, uniqueInstances);
  if (!uniqueInstances || state.Loot.NextInstanceValue <= maximumInstance) {
    return false;
  }

  for (const AbilityId ability : state.Loadout.Slots) {
    if (!ability.IsValid()) {
      continue;
    }
    const AbilityRuntimeEntryState *entry =
        FindAbilityState(state.Abilities, ability);
    if (entry == nullptr || !entry->Unlocked) {
      return false;
    }
  }

  for (const SkillNodeId nodeId : state.SkillTree.AllocatedNodes) {
    const SkillNodeDefinition *node = skillTree_.FindNode(nodeId);
    if (node == nullptr) {
      return false;
    }
    if (node->UnlockAbility.IsValid()) {
      const AbilityRuntimeEntryState *entry =
          FindAbilityState(state.Abilities, node->UnlockAbility);
      if (entry == nullptr || !entry->Unlocked) {
        return false;
      }
    }
  }
  return true;
}

bool CharacterPersistence::ApplyState(const CharacterSaveState &state) {
  // Emptying the presentation-facing loadout first removes dependency locks
  // while the skill graph transitions between two otherwise valid states.
  const AbilityLoadoutState emptyLoadout{};
  if (!loadout_.RestoreState(emptyLoadout) ||
      !inventory_.RestoreState(state.Inventory) ||
      !equipment_.RestoreState(state.Equipment, inventory_) ||
      !effects_.RestoreState(state.Effects) ||
      !skillTree_.RestoreState(state.SkillTree) ||
      !abilities_.RestoreState(state.Abilities) ||
      !loadout_.RestoreState(state.Loadout) ||
      !loot_.RestoreState(state.Loot) || !random_.RestoreState(state.Random) ||
      !DerivedModifiersMatch(state) ||
      !combatant_.RestoreState(state.Combatant)) {
    return false;
  }
  return true;
}

bool CharacterPersistence::DerivedModifiersMatch(
    const CharacterSaveState &state) const {
  const std::vector<Stats::AttributeModifier> expected =
      DerivedModifiers(state.Combatant.Attributes);
  const std::vector<Stats::AttributeModifier> actual =
      DerivedModifiers(combatant_.CaptureState().Attributes);
  if (expected.size() != actual.size()) {
    return false;
  }
  for (std::size_t index = 0; index < expected.size(); ++index) {
    if (!SameModifier(expected[index], actual[index])) {
      return false;
    }
  }
  return true;
}

} // namespace Lostsense::Gameplay
