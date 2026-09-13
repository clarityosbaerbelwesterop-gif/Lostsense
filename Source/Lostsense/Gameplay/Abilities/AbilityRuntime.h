#pragma once

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Core/DeterministicRandom.h"
#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Gameplay/GameplayTypes.h"

#include <cstdint>
#include <map>
#include <vector>

namespace Lostsense::Gameplay {

enum class AbilityTargetRule : std::uint8_t {
  None,
  Self,
  Hostile,
  Friendly,
};

enum class TargetRelation : std::uint8_t {
  None,
  Self,
  Hostile,
  Friendly,
};

struct AbilityDefinition final {
  AbilityId Id{};
  double ResourceCost{0.0};
  double CooldownSeconds{0.0};
  CooldownGroupId CooldownGroup{};
  std::uint32_t MaximumCharges{1U};
  double RechargeSeconds{0.0};
  AbilityTargetRule TargetRule{AbilityTargetRule::None};
  ClassId RequiredClass{};
  AbilityLoadoutSlotMask AllowedLoadoutSlots{AllAbilityLoadoutSlots};
  bool AllowDuplicateInLoadout{false};
  std::vector<AbilityId> Prerequisites{};
  Combat::DamageSpec Damage{};
  bool DealsDamage{false};
  std::vector<EffectId> EffectsOnTarget{};
  std::vector<EffectId> EffectsOnSelf{};
};

struct AbilityRuntimeEntryState final {
  AbilityId Id{};
  bool Unlocked{false};
  std::uint32_t Charges{0U};
  double CooldownRemaining{0.0};
  double RechargeRemaining{0.0};
};

struct CooldownGroupState final {
  CooldownGroupId Id{};
  double Remaining{0.0};
};

struct AbilityRuntimeState final {
  std::vector<AbilityRuntimeEntryState> Abilities{};
  std::vector<CooldownGroupState> CooldownGroups{};
};

struct AbilityTarget final {
  Combat::Combatant *Combatant{nullptr};
  EffectRuntime *Effects{nullptr};
  TargetRelation Relation{TargetRelation::None};
};

enum class AbilityActivationResult : std::uint8_t {
  Success,
  UnknownAbility,
  InvalidRuntime,
  NotUnlocked,
  MissingPrerequisite,
  WrongClass,
  InvalidTarget,
  InsufficientResource,
  OnCooldown,
  NoCharges,
  BlockedByEffect,
  InternalFailure,
};

struct AbilityActivationOutcome final {
  AbilityActivationResult Result{AbilityActivationResult::InternalFailure};
  Combat::CombatResolution Damage{};
  bool DamageResolved{false};
  std::vector<EffectApplyOutcome> EffectResults{};
};

class AbilityRuntime final {
public:
  AbilityRuntime(Combat::Combatant &owner, EffectRuntime &ownerEffects,
                 Core::DeterministicRandom &random, ClassId ownerClass,
                 std::vector<AbilityDefinition> definitions);

  [[nodiscard]] bool IsValid() const noexcept { return definitionsValid_; }
  [[nodiscard]] ClassId OwnerClass() const noexcept { return ownerClass_; }
  [[nodiscard]] const AbilityDefinition *
  FindDefinition(AbilityId id) const noexcept;
  [[nodiscard]] bool IsUnlocked(AbilityId id) const noexcept;
  [[nodiscard]] bool Unlock(AbilityId id) noexcept;
  [[nodiscard]] bool Revoke(AbilityId id) noexcept;

  [[nodiscard]] AbilityActivationOutcome Activate(AbilityId id,
                                                  const AbilityTarget &target);
  [[nodiscard]] bool AdvanceTime(double seconds) noexcept;

  [[nodiscard]] AbilityRuntimeState CaptureState() const;
  [[nodiscard]] bool RestoreState(const AbilityRuntimeState &state) noexcept;

private:
  [[nodiscard]] static bool
  IsDefinitionShapeValid(const AbilityDefinition &definition) noexcept;
  [[nodiscard]] bool ValidateDefinitionGraph() const noexcept;
  [[nodiscard]] bool
  HasUnlockedPrerequisites(const AbilityDefinition &definition) const noexcept;
  [[nodiscard]] bool ValidateTarget(const AbilityDefinition &definition,
                                    const AbilityTarget &target) const noexcept;
  [[nodiscard]] bool
  ValidateState(const AbilityRuntimeState &state) const noexcept;
  [[nodiscard]] bool
  HasActiveCooldown(const AbilityDefinition &definition,
                    const AbilityRuntimeEntryState &state) const noexcept;
  [[nodiscard]] bool RollbackActivation(
      const Combat::CombatantState &ownerState,
      const EffectRuntimeState &ownerEffectState,
      const AbilityRuntimeState &abilityState,
      const Core::RandomState &randomState, Combat::Combatant *targetCombatant,
      const Combat::CombatantState *targetState, EffectRuntime *targetEffects,
      const EffectRuntimeState *targetEffectState) noexcept;

  Combat::Combatant &owner_;
  EffectRuntime &ownerEffects_;
  Core::DeterministicRandom &random_;
  ClassId ownerClass_{};
  std::map<AbilityId, AbilityDefinition> definitions_{};
  std::map<AbilityId, AbilityRuntimeEntryState> states_{};
  std::map<CooldownGroupId, double> cooldownGroups_{};
  bool definitionsValid_{true};
};

} // namespace Lostsense::Gameplay
