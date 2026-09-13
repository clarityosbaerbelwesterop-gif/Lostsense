#pragma once

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Core/DeterministicRandom.h"
#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"
#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Gameplay/Items/Equipment.h"
#include "Lostsense/Gameplay/Items/Inventory.h"
#include "Lostsense/Gameplay/Items/LootRuntime.h"
#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"
#include "Lostsense/Gameplay/Progression/SkillGraph.h"

#include <compare>
#include <cstdint>

namespace Lostsense::Gameplay {

inline constexpr std::uint32_t CurrentSaveSchemaVersion = 2U;
inline constexpr std::uint32_t OldestSupportedSaveSchemaVersion = 1U;

struct CharacterPersistentId final {
  std::uint64_t Value{0U};

  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const CharacterPersistentId &,
              const CharacterPersistentId &) noexcept = default;
};

struct CharacterSaveState final {
  std::uint32_t SchemaVersion{CurrentSaveSchemaVersion};
  CharacterPersistentId Character{};
  ClassId CharacterClass{};
  SkillTreeId SkillTreeDefinition{};
  Combat::CombatantState Combatant{};
  Core::RandomState Random{};
  EffectRuntimeState Effects{};
  AbilityRuntimeState Abilities{};
  SkillTreeRuntimeState SkillTree{};
  AbilityLoadoutState Loadout{};
  InventoryState Inventory{};
  EquipmentState Equipment{};
  LootRuntimeState Loot{};
};

enum class CharacterRestoreResult : std::uint8_t {
  Success,
  InvalidRuntime,
  UnsupportedSchema,
  IdentityMismatch,
  InvalidAggregateState,
  SubsystemRestoreFailed,
  RollbackFailed,
};

class CharacterPersistence final {
public:
  CharacterPersistence(CharacterPersistentId character, ClassId characterClass,
                       Combat::Combatant &combatant,
                       Core::DeterministicRandom &random,
                       EffectRuntime &effects, AbilityRuntime &abilities,
                       SkillTreeRuntime &skillTree, AbilityLoadout &loadout,
                       Inventory &inventory, EquipmentRuntime &equipment,
                       LootRuntime &loot) noexcept;

  [[nodiscard]] bool IsValid() const noexcept { return valid_; }
  [[nodiscard]] CharacterSaveState CaptureState() const;
  [[nodiscard]] bool ValidateState(const CharacterSaveState &state) const;
  [[nodiscard]] CharacterRestoreResult
  RestoreState(const CharacterSaveState &state);

private:
  [[nodiscard]] bool ValidateEnvelope(const CharacterSaveState &state) const;
  [[nodiscard]] bool ValidateCrossState(const CharacterSaveState &state) const;
  [[nodiscard]] bool ApplyState(const CharacterSaveState &state);
  [[nodiscard]] bool
  DerivedModifiersMatch(const CharacterSaveState &state) const;

  CharacterPersistentId character_{};
  ClassId characterClass_{};
  Combat::Combatant &combatant_;
  Core::DeterministicRandom &random_;
  EffectRuntime &effects_;
  AbilityRuntime &abilities_;
  SkillTreeRuntime &skillTree_;
  AbilityLoadout &loadout_;
  Inventory &inventory_;
  EquipmentRuntime &equipment_;
  LootRuntime &loot_;
  bool valid_{false};
};

} // namespace Lostsense::Gameplay
