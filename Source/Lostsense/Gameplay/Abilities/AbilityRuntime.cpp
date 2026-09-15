#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <set>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

constexpr double Epsilon = 1.0e-12;
constexpr std::uint32_t MaximumConfiguredCharges = 100'000U;

[[nodiscard]] bool IsFiniteNonNegative(const double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool IsTargetRuleValid(const AbilityTargetRule rule) noexcept {
  switch (rule) {
  case AbilityTargetRule::None:
  case AbilityTargetRule::Self:
  case AbilityTargetRule::Hostile:
  case AbilityTargetRule::Friendly:
    return true;
  }
  return false;
}

[[nodiscard]] bool
IsFatalEffectResult(const EffectApplyResult result) noexcept {
  return result == EffectApplyResult::UnknownEffect ||
         result == EffectApplyResult::InvalidSource ||
         result == EffectApplyResult::InvalidRuntime ||
         result == EffectApplyResult::InternalFailure;
}

} // namespace

AbilityRuntime::AbilityRuntime(Combat::Combatant &owner,
                               EffectRuntime &ownerEffects,
                               Core::DeterministicRandom &random,
                               const ClassId ownerClass,
                               std::vector<AbilityDefinition> definitions)
    : owner_{owner}, ownerEffects_{ownerEffects}, random_{random},
      ownerClass_{ownerClass} {
  if (!owner_.Id().IsValid() || !ownerClass_.IsValid() ||
      !ownerEffects_.IsValid() || ownerEffects_.OwnerId() != owner_.Id()) {
    definitionsValid_ = false;
  }
  for (AbilityDefinition &definition : definitions) {
    if (!IsDefinitionShapeValid(definition) ||
        definitions_.contains(definition.Id)) {
      definitionsValid_ = false;
      continue;
    }
    const AbilityId id = definition.Id;
    states_.emplace(id, AbilityRuntimeEntryState{
                            id, false, definition.MaximumCharges, 0.0, 0.0});
    definitions_.emplace(id, std::move(definition));
  }
  for (const auto &[id, definition] : definitions_) {
    static_cast<void>(id);
    for (const EffectId effect : definition.EffectsOnSelf) {
      if (ownerEffects_.FindDefinition(effect) == nullptr) {
        definitionsValid_ = false;
      }
    }
    for (const EffectId effect : definition.EffectsOnTarget) {
      if (ownerEffects_.FindDefinition(effect) == nullptr) {
        definitionsValid_ = false;
      }
    }
  }
  if (definitions_.size() != definitions.size() || !ValidateDefinitionGraph()) {
    definitionsValid_ = false;
  }
}

const AbilityDefinition *
AbilityRuntime::FindDefinition(const AbilityId id) const noexcept {
  const auto it = definitions_.find(id);
  return it == definitions_.end() ? nullptr : &it->second;
}

bool AbilityRuntime::IsUnlocked(const AbilityId id) const noexcept {
  const auto it = states_.find(id);
  return it != states_.end() && it->second.Unlocked;
}

bool AbilityRuntime::Unlock(const AbilityId id) noexcept {
  if (!definitionsValid_) {
    return false;
  }
  const AbilityDefinition *definition = FindDefinition(id);
  const auto state = states_.find(id);
  if (definition == nullptr || state == states_.end() ||
      state->second.Unlocked || !HasUnlockedPrerequisites(*definition) ||
      (definition->RequiredClass.IsValid() &&
       definition->RequiredClass != ownerClass_)) {
    return false;
  }
  state->second.Unlocked = true;
  return true;
}

bool AbilityRuntime::Revoke(const AbilityId id) noexcept {
  const auto state = states_.find(id);
  if (!definitionsValid_ || state == states_.end() || !state->second.Unlocked) {
    return false;
  }
  for (const auto &[otherId, definition] : definitions_) {
    if (otherId == id || !IsUnlocked(otherId)) {
      continue;
    }
    if (std::find(definition.Prerequisites.begin(),
                  definition.Prerequisites.end(),
                  id) != definition.Prerequisites.end()) {
      return false;
    }
  }
  state->second.Unlocked = false;
  state->second.CooldownRemaining = 0.0;
  state->second.RechargeRemaining = 0.0;
  state->second.Charges = definitions_.at(id).MaximumCharges;
  return true;
}

AbilityActivationOutcome AbilityRuntime::Activate(const AbilityId id,
                                                  const AbilityTarget &target) {
  AbilityActivationOutcome outcome;
  if (!definitionsValid_) {
    outcome.Result = AbilityActivationResult::InvalidRuntime;
    return outcome;
  }
  const AbilityDefinition *definition = FindDefinition(id);
  auto state = states_.find(id);
  if (definition == nullptr || state == states_.end()) {
    outcome.Result = AbilityActivationResult::UnknownAbility;
    return outcome;
  }
  if (!state->second.Unlocked) {
    outcome.Result = AbilityActivationResult::NotUnlocked;
    return outcome;
  }
  if (!HasUnlockedPrerequisites(*definition)) {
    outcome.Result = AbilityActivationResult::MissingPrerequisite;
    return outcome;
  }
  if (definition->RequiredClass.IsValid() &&
      definition->RequiredClass != ownerClass_) {
    outcome.Result = AbilityActivationResult::WrongClass;
    return outcome;
  }
  if (owner_.Health().IsDead()) {
    outcome.Result = AbilityActivationResult::OwnerDead;
    return outcome;
  }
  if (ownerEffects_.HasRestriction(EffectRestriction::AbilityActivation)) {
    outcome.Result = AbilityActivationResult::BlockedByEffect;
    return outcome;
  }
  if (!ValidateTarget(*definition, target)) {
    outcome.Result = AbilityActivationResult::InvalidTarget;
    return outcome;
  }
  if (HasActiveCooldown(*definition, state->second)) {
    outcome.Result = AbilityActivationResult::OnCooldown;
    return outcome;
  }
  if (state->second.Charges == 0U) {
    outcome.Result = AbilityActivationResult::NoCharges;
    return outcome;
  }
  if (definition->ResourceCost > owner_.Resource().Current()) {
    outcome.Result = AbilityActivationResult::InsufficientResource;
    return outcome;
  }

  const Combat::CombatantState ownerState = owner_.CaptureState();
  const EffectRuntimeState ownerEffectState = ownerEffects_.CaptureState();
  const AbilityRuntimeState abilityState = CaptureState();
  const Core::RandomState randomState = random_.CaptureState();

  Combat::CombatantState targetStateValue;
  EffectRuntimeState targetEffectStateValue;
  const Combat::CombatantState *targetState = nullptr;
  const EffectRuntimeState *targetEffectState = nullptr;
  if (target.Combatant != nullptr && target.Combatant != &owner_) {
    targetStateValue = target.Combatant->CaptureState();
    targetState = &targetStateValue;
  }
  if (target.Effects != nullptr && target.Effects != &ownerEffects_) {
    targetEffectStateValue = target.Effects->CaptureState();
    targetEffectState = &targetEffectStateValue;
  }

  const auto rollback = [&]() {
    return RollbackActivation(ownerState, ownerEffectState, abilityState,
                              randomState, target.Combatant, targetState,
                              target.Effects, targetEffectState);
  };

  if (!owner_.TryConsumeResource(definition->ResourceCost).Succeeded) {
    static_cast<void>(rollback());
    outcome.Result = AbilityActivationResult::InternalFailure;
    return outcome;
  }

  const bool usesCharges = definition->RechargeSeconds > 0.0;
  if (usesCharges) {
    --state->second.Charges;
    if (state->second.Charges < definition->MaximumCharges &&
        state->second.RechargeRemaining <= Epsilon) {
      state->second.RechargeRemaining = definition->RechargeSeconds;
    }
  }

  if (definition->DealsDamage) {
    if (target.Combatant == nullptr) {
      static_cast<void>(rollback());
      outcome.Result = AbilityActivationResult::InternalFailure;
      return outcome;
    }
    outcome.Damage =
        owner_.ResolveAttack(*target.Combatant, definition->Damage, random_);
    outcome.DamageResolved = true;
  }

  for (const EffectId effectId : definition->EffectsOnSelf) {
    const EffectApplyOutcome effectResult =
        ownerEffects_.Apply(effectId, owner_.Id());
    outcome.EffectResults.push_back(effectResult);
    if (IsFatalEffectResult(effectResult.Result)) {
      static_cast<void>(rollback());
      outcome.Result = AbilityActivationResult::InternalFailure;
      return outcome;
    }
  }
  for (const EffectId effectId : definition->EffectsOnTarget) {
    if (target.Effects == nullptr) {
      static_cast<void>(rollback());
      outcome.Result = AbilityActivationResult::InternalFailure;
      return outcome;
    }
    const EffectApplyOutcome effectResult =
        target.Effects->Apply(effectId, owner_.Id());
    outcome.EffectResults.push_back(effectResult);
    if (IsFatalEffectResult(effectResult.Result)) {
      static_cast<void>(rollback());
      outcome.Result = AbilityActivationResult::InternalFailure;
      return outcome;
    }
  }

  state->second.CooldownRemaining = definition->CooldownSeconds;
  if (definition->CooldownGroup.IsValid() &&
      definition->CooldownSeconds > 0.0) {
    cooldownGroups_[definition->CooldownGroup] = definition->CooldownSeconds;
  }
  outcome.Result = AbilityActivationResult::Success;
  return outcome;
}

bool AbilityRuntime::AdvanceTime(const double seconds) noexcept {
  if (!definitionsValid_ || !IsFiniteNonNegative(seconds)) {
    return false;
  }
  for (auto &[id, state] : states_) {
    const AbilityDefinition &definition = definitions_.at(id);
    state.CooldownRemaining = std::max(0.0, state.CooldownRemaining - seconds);
    if (state.Charges >= definition.MaximumCharges ||
        definition.RechargeSeconds <= 0.0) {
      state.RechargeRemaining = 0.0;
      continue;
    }
    double remainingAdvance = seconds;
    while (state.Charges < definition.MaximumCharges &&
           remainingAdvance + Epsilon >= state.RechargeRemaining) {
      remainingAdvance -= state.RechargeRemaining;
      ++state.Charges;
      if (state.Charges < definition.MaximumCharges) {
        state.RechargeRemaining = definition.RechargeSeconds;
      } else {
        state.RechargeRemaining = 0.0;
      }
    }
    if (state.Charges < definition.MaximumCharges && remainingAdvance > 0.0) {
      state.RechargeRemaining =
          std::max(0.0, state.RechargeRemaining - remainingAdvance);
    }
  }
  for (auto &[group, remaining] : cooldownGroups_) {
    static_cast<void>(group);
    remaining = std::max(0.0, remaining - seconds);
  }
  return true;
}

AbilityRuntimeState AbilityRuntime::CaptureState() const {
  AbilityRuntimeState state;
  state.Abilities.reserve(states_.size());
  state.CooldownGroups.reserve(cooldownGroups_.size());
  for (const auto &[id, entry] : states_) {
    static_cast<void>(id);
    state.Abilities.push_back(entry);
  }
  for (const auto &[id, remaining] : cooldownGroups_) {
    state.CooldownGroups.push_back({id, remaining});
  }
  return state;
}

bool AbilityRuntime::RestoreState(const AbilityRuntimeState &state) noexcept {
  if (!ValidateState(state)) {
    return false;
  }
  std::map<AbilityId, AbilityRuntimeEntryState> restored;
  std::map<CooldownGroupId, double> restoredGroups;
  for (const AbilityRuntimeEntryState &entry : state.Abilities) {
    restored.emplace(entry.Id, entry);
  }
  for (const CooldownGroupState &group : state.CooldownGroups) {
    restoredGroups.emplace(group.Id, group.Remaining);
  }
  states_ = std::move(restored);
  cooldownGroups_ = std::move(restoredGroups);
  return true;
}

bool AbilityRuntime::IsDefinitionShapeValid(
    const AbilityDefinition &definition) noexcept {
  if (!definition.Id.IsValid() ||
      !IsFiniteNonNegative(definition.ResourceCost) ||
      !IsFiniteNonNegative(definition.CooldownSeconds) ||
      definition.MaximumCharges == 0U ||
      definition.MaximumCharges > MaximumConfiguredCharges ||
      !IsFiniteNonNegative(definition.RechargeSeconds) ||
      !IsTargetRuleValid(definition.TargetRule) ||
      (definition.AllowedLoadoutSlots &
       static_cast<AbilityLoadoutSlotMask>(~AllAbilityLoadoutSlots)) != 0U) {
    return false;
  }
  if (definition.MaximumCharges > 1U && definition.RechargeSeconds <= 0.0) {
    return false;
  }
  if (definition.TargetRule == AbilityTargetRule::None &&
      (definition.DealsDamage || !definition.EffectsOnTarget.empty())) {
    return false;
  }
  for (std::size_t index = 0; index < Combat::DamageTypeCount; ++index) {
    if (!std::isfinite(definition.Damage.BaseDamage[index]) ||
        definition.Damage.BaseDamage[index] < 0.0 ||
        !std::isfinite(definition.Damage.AttackPowerCoefficients[index]) ||
        definition.Damage.AttackPowerCoefficients[index] < 0.0 ||
        !std::isfinite(definition.Damage.SpellPowerCoefficients[index]) ||
        definition.Damage.SpellPowerCoefficients[index] < 0.0) {
      return false;
    }
  }
  std::set<AbilityId> prerequisites;
  for (const AbilityId prerequisite : definition.Prerequisites) {
    if (!prerequisite.IsValid() || prerequisite == definition.Id ||
        !prerequisites.insert(prerequisite).second) {
      return false;
    }
  }
  for (const EffectId effect : definition.EffectsOnTarget) {
    if (!effect.IsValid()) {
      return false;
    }
  }
  for (const EffectId effect : definition.EffectsOnSelf) {
    if (!effect.IsValid()) {
      return false;
    }
  }
  return true;
}

bool AbilityRuntime::ValidateDefinitionGraph() const noexcept {
  for (const auto &[id, definition] : definitions_) {
    static_cast<void>(id);
    for (const AbilityId prerequisite : definition.Prerequisites) {
      if (!definitions_.contains(prerequisite)) {
        return false;
      }
    }
  }
  enum class Visit : std::uint8_t { Unvisited, Visiting, Done };
  std::map<AbilityId, Visit> visit;
  for (const auto &[id, definition] : definitions_) {
    static_cast<void>(definition);
    visit[id] = Visit::Unvisited;
  }
  std::function<bool(AbilityId)> dfs = [&](const AbilityId id) {
    if (visit[id] == Visit::Visiting) {
      return false;
    }
    if (visit[id] == Visit::Done) {
      return true;
    }
    visit[id] = Visit::Visiting;
    for (const AbilityId prerequisite : definitions_.at(id).Prerequisites) {
      if (!dfs(prerequisite)) {
        return false;
      }
    }
    visit[id] = Visit::Done;
    return true;
  };
  for (const auto &[id, definition] : definitions_) {
    static_cast<void>(definition);
    if (!dfs(id)) {
      return false;
    }
  }
  return true;
}

bool AbilityRuntime::HasUnlockedPrerequisites(
    const AbilityDefinition &definition) const noexcept {
  return std::all_of(definition.Prerequisites.begin(),
                     definition.Prerequisites.end(),
                     [this](const AbilityId id) { return IsUnlocked(id); });
}

bool AbilityRuntime::ValidateTarget(
    const AbilityDefinition &definition,
    const AbilityTarget &target) const noexcept {
  switch (definition.TargetRule) {
  case AbilityTargetRule::None:
    return target.Combatant == nullptr && target.Effects == nullptr &&
           target.Relation == TargetRelation::None && !definition.DealsDamage &&
           definition.EffectsOnTarget.empty();
  case AbilityTargetRule::Self:
    return target.Combatant == &owner_ &&
           target.Relation == TargetRelation::Self &&
           (definition.EffectsOnTarget.empty() ||
            target.Effects == &ownerEffects_) &&
           (target.Effects == nullptr ||
            target.Effects->OwnerId() == owner_.Id());
  case AbilityTargetRule::Hostile:
    return target.Combatant != nullptr && target.Combatant != &owner_ &&
           target.Relation == TargetRelation::Hostile &&
           (definition.EffectsOnTarget.empty() || target.Effects != nullptr) &&
           (target.Effects == nullptr ||
            target.Effects->OwnerId() == target.Combatant->Id());
  case AbilityTargetRule::Friendly:
    return target.Combatant != nullptr &&
           target.Relation == TargetRelation::Friendly &&
           (definition.EffectsOnTarget.empty() || target.Effects != nullptr) &&
           (target.Effects == nullptr ||
            target.Effects->OwnerId() == target.Combatant->Id());
  }
  return false;
}

bool AbilityRuntime::ValidateState(
    const AbilityRuntimeState &state) const noexcept {
  if (!definitionsValid_ || state.Abilities.size() != definitions_.size()) {
    return false;
  }
  std::set<AbilityId> seen;
  for (const AbilityRuntimeEntryState &entry : state.Abilities) {
    const AbilityDefinition *definition = FindDefinition(entry.Id);
    if (definition == nullptr || !seen.insert(entry.Id).second ||
        entry.Charges > definition->MaximumCharges ||
        !IsFiniteNonNegative(entry.CooldownRemaining) ||
        entry.CooldownRemaining > definition->CooldownSeconds + Epsilon ||
        !IsFiniteNonNegative(entry.RechargeRemaining) ||
        (entry.Charges == definition->MaximumCharges &&
         entry.RechargeRemaining != 0.0) ||
        (entry.Charges < definition->MaximumCharges &&
         definition->RechargeSeconds <= 0.0 &&
         entry.RechargeRemaining != 0.0) ||
        (entry.Charges < definition->MaximumCharges &&
         definition->RechargeSeconds > 0.0 &&
         entry.RechargeRemaining <= Epsilon) ||
        (definition->RechargeSeconds > 0.0 &&
         entry.RechargeRemaining > definition->RechargeSeconds + Epsilon) ||
        (entry.Unlocked && definition->RequiredClass.IsValid() &&
         definition->RequiredClass != ownerClass_)) {
      return false;
    }
  }
  std::set<CooldownGroupId> groups;
  for (const CooldownGroupState &group : state.CooldownGroups) {
    if (!group.Id.IsValid() || !groups.insert(group.Id).second ||
        !IsFiniteNonNegative(group.Remaining)) {
      return false;
    }
    bool known = false;
    double maximumGroupCooldown = 0.0;
    for (const auto &[id, definition] : definitions_) {
      static_cast<void>(id);
      if (definition.CooldownGroup == group.Id) {
        known = true;
        maximumGroupCooldown =
            std::max(maximumGroupCooldown, definition.CooldownSeconds);
      }
    }
    if (!known || group.Remaining > maximumGroupCooldown + Epsilon) {
      return false;
    }
  }
  for (const AbilityRuntimeEntryState &entry : state.Abilities) {
    if (entry.Unlocked) {
      const AbilityDefinition &definition = definitions_.at(entry.Id);
      for (const AbilityId prerequisite : definition.Prerequisites) {
        const auto prerequisiteState = std::find_if(
            state.Abilities.begin(), state.Abilities.end(),
            [prerequisite](const AbilityRuntimeEntryState &candidate) {
              return candidate.Id == prerequisite;
            });
        if (prerequisiteState == state.Abilities.end() ||
            !prerequisiteState->Unlocked) {
          return false;
        }
      }
    }
  }
  return true;
}

bool AbilityRuntime::HasActiveCooldown(
    const AbilityDefinition &definition,
    const AbilityRuntimeEntryState &state) const noexcept {
  if (state.CooldownRemaining > Epsilon) {
    return true;
  }
  if (!definition.CooldownGroup.IsValid()) {
    return false;
  }
  const auto group = cooldownGroups_.find(definition.CooldownGroup);
  return group != cooldownGroups_.end() && group->second > Epsilon;
}

bool AbilityRuntime::RollbackActivation(
    const Combat::CombatantState &ownerState,
    const EffectRuntimeState &ownerEffectState,
    const AbilityRuntimeState &abilityState,
    const Core::RandomState &randomState, Combat::Combatant *targetCombatant,
    const Combat::CombatantState *targetState, EffectRuntime *targetEffects,
    const EffectRuntimeState *targetEffectState) noexcept {
  bool ok = true;
  if (targetEffects != nullptr && targetEffects != &ownerEffects_ &&
      targetEffectState != nullptr) {
    ok = targetEffects->RestoreState(*targetEffectState) && ok;
  }
  ok = ownerEffects_.RestoreState(ownerEffectState) && ok;
  if (targetCombatant != nullptr && targetCombatant != &owner_ &&
      targetState != nullptr) {
    ok = targetCombatant->RestoreState(*targetState) && ok;
  }
  ok = owner_.RestoreState(ownerState) && ok;
  ok = RestoreState(abilityState) && ok;
  ok = random_.RestoreState(randomState) && ok;
  return ok;
}

} // namespace Lostsense::Gameplay
