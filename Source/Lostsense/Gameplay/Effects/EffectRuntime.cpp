#include "Lostsense/Gameplay/Effects/EffectRuntime.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

constexpr std::uint64_t StatusModifierPrefix = 1ULL << 63U;
constexpr std::uint64_t MaximumEffectInstance = (1ULL << 47U) - 1ULL;
constexpr std::uint32_t MaximumConfiguredStacks = 1'000'000U;
constexpr double Epsilon = 1.0e-12;

[[nodiscard]] bool IsFiniteNonNegative(const double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool
IsStackingPolicyValid(const EffectStackingPolicy policy) noexcept {
  switch (policy) {
  case EffectStackingPolicy::Replace:
  case EffectStackingPolicy::RefreshDuration:
  case EffectStackingPolicy::Independent:
  case EffectStackingPolicy::StackMagnitude:
    return true;
  }
  return false;
}

[[nodiscard]] bool
IsRestrictionValid(const EffectRestriction restriction) noexcept {
  constexpr std::uint32_t KnownMask =
      static_cast<std::uint32_t>(EffectRestriction::AbilityActivation) |
      static_cast<std::uint32_t>(EffectRestriction::Movement);
  return (static_cast<std::uint32_t>(restriction) & ~KnownMask) == 0U;
}

[[nodiscard]] bool
IsModifierOperationValid(const Stats::ModifierOperation operation) noexcept {
  switch (operation) {
  case Stats::ModifierOperation::Additive:
  case Stats::ModifierOperation::Multiplicative:
    return true;
  }
  return false;
}

[[nodiscard]] double SaturatingScale(const double magnitude,
                                     const std::uint32_t stacks) noexcept {
  const long double scaled =
      static_cast<long double>(magnitude) * static_cast<long double>(stacks);
  const long double maximum =
      static_cast<long double>(std::numeric_limits<double>::max());
  if (scaled > maximum) {
    return std::numeric_limits<double>::max();
  }
  if (scaled < -maximum) {
    return -std::numeric_limits<double>::max();
  }
  return static_cast<double>(scaled);
}

[[nodiscard]] bool ContainsTag(const std::vector<GameplayTagId> &tags,
                               const GameplayTagId tag) noexcept {
  return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

} // namespace

EffectRuntime::EffectRuntime(Combat::Combatant &owner,
                             std::vector<EffectDefinition> definitions)
    : owner_{owner} {
  if (!owner_.Id().IsValid()) {
    definitionsValid_ = false;
  }
  for (EffectDefinition &definition : definitions) {
    if (!IsDefinitionValid(definition) ||
        definitions_.contains(definition.Id)) {
      definitionsValid_ = false;
      continue;
    }
    definitions_.emplace(definition.Id, std::move(definition));
  }
  if (definitions_.size() != definitions.size()) {
    definitionsValid_ = false;
  }
  struct StackGroupRule final {
    EffectStackingPolicy Policy{EffectStackingPolicy::Replace};
    std::uint32_t MaxStacks{1U};
    EffectId FirstDefinition{};
  };
  std::map<EffectStackGroupId, StackGroupRule> groupRules;
  for (const auto &[id, definition] : definitions_) {
    const auto [it, inserted] = groupRules.emplace(
        definition.StackGroup,
        StackGroupRule{definition.Stacking, definition.MaxStacks, id});
    if (!inserted && (it->second.Policy != definition.Stacking ||
                      it->second.MaxStacks != definition.MaxStacks ||
                      (definition.Stacking != EffectStackingPolicy::Replace &&
                       it->second.FirstDefinition != id))) {
      definitionsValid_ = false;
    }
    for (const EffectId immunity : definition.GrantedEffectImmunities) {
      if (!definitions_.contains(immunity)) {
        definitionsValid_ = false;
      }
    }
  }
}

const EffectDefinition *
EffectRuntime::FindDefinition(const EffectId id) const noexcept {
  const auto it = definitions_.find(id);
  return it == definitions_.end() ? nullptr : &it->second;
}

bool EffectRuntime::HasEffect(const EffectId id) const noexcept {
  return std::any_of(active_.begin(), active_.end(), [id](const auto &entry) {
    return entry.second.DefinitionId == id;
  });
}

bool EffectRuntime::HasRestriction(
    const EffectRestriction restriction) const noexcept {
  for (const auto &[instanceId, state] : active_) {
    static_cast<void>(instanceId);
    const EffectDefinition *definition = FindDefinition(state.DefinitionId);
    if (definition != nullptr &&
        HasRestrictionFlag(definition->Restrictions, restriction)) {
      return true;
    }
  }
  return false;
}

EffectApplyOutcome EffectRuntime::Apply(const EffectId id,
                                        const Combat::CombatantId source) {
  if (!definitionsValid_) {
    return {EffectApplyResult::InvalidRuntime, {}};
  }
  const EffectDefinition *definition = FindDefinition(id);
  if (definition == nullptr) {
    return {EffectApplyResult::UnknownEffect, {}};
  }
  if (!source.IsValid()) {
    return {EffectApplyResult::InvalidSource, {}};
  }
  if (IsImmuneTo(*definition)) {
    return {EffectApplyResult::Immune, {}};
  }

  std::vector<EffectInstanceId> groupInstances;
  for (const auto &[instanceId, state] : active_) {
    const EffectDefinition *existing = FindDefinition(state.DefinitionId);
    if (existing != nullptr && existing->StackGroup == definition->StackGroup) {
      groupInstances.push_back(instanceId);
    }
  }

  if (definition->Stacking == EffectStackingPolicy::RefreshDuration &&
      !groupInstances.empty()) {
    ActiveEffectState &state = active_.at(groupInstances.front());
    state.RemainingSeconds = definition->DurationSeconds;
    if (definition->TickIntervalSeconds > 0.0) {
      state.TimeUntilNextTick = definition->TickIntervalSeconds;
    }
    return {EffectApplyResult::Refreshed, state.InstanceId};
  }

  if (definition->Stacking == EffectStackingPolicy::StackMagnitude &&
      !groupInstances.empty()) {
    ActiveEffectState &state = active_.at(groupInstances.front());
    if (state.Stacks >= definition->MaxStacks) {
      state.RemainingSeconds = definition->DurationSeconds;
      return {EffectApplyResult::AtMaxStacks, state.InstanceId};
    }
    const std::uint32_t newStacks = state.Stacks + 1U;
    if (!ReinstallModifiers(state, newStacks)) {
      return {EffectApplyResult::InternalFailure, state.InstanceId};
    }
    state.RemainingSeconds = definition->DurationSeconds;
    state.Stacks = newStacks;
    return {EffectApplyResult::Stacked, state.InstanceId};
  }

  EffectApplyResult successfulResult = EffectApplyResult::Applied;
  const Combat::CombatantState ownerBefore = owner_.CaptureState();
  const auto activeBefore = active_;
  const std::uint64_t nextBefore = nextInstanceValue_;
  if (definition->Stacking == EffectStackingPolicy::Replace &&
      !groupInstances.empty()) {
    for (const EffectInstanceId instanceId : groupInstances) {
      static_cast<void>(Remove(instanceId));
    }
    successfulResult = EffectApplyResult::Replaced;
  } else if (definition->Stacking == EffectStackingPolicy::Independent &&
             groupInstances.size() >= definition->MaxStacks) {
    return {EffectApplyResult::AtMaxStacks, {}};
  }

  const EffectInstanceId instanceId = AllocateInstanceId();
  if (!instanceId.IsValid()) {
    active_ = activeBefore;
    nextInstanceValue_ = nextBefore;
    static_cast<void>(owner_.RestoreState(ownerBefore));
    return {EffectApplyResult::InternalFailure, {}};
  }
  ActiveEffectState state{instanceId,
                          definition->Id,
                          source,
                          1U,
                          definition->Permanent ? 0.0
                                                : definition->DurationSeconds,
                          definition->TickIntervalSeconds};
  if (!InstallModifiers(state)) {
    active_ = activeBefore;
    nextInstanceValue_ = nextBefore;
    static_cast<void>(owner_.RestoreState(ownerBefore));
    return {EffectApplyResult::InternalFailure, {}};
  }
  active_.emplace(instanceId, state);
  return {successfulResult, instanceId};
}

bool EffectRuntime::Remove(const EffectInstanceId instanceId) noexcept {
  const auto it = active_.find(instanceId);
  if (it == active_.end()) {
    return false;
  }
  RemoveModifiers(it->second);
  active_.erase(it);
  return true;
}

std::size_t EffectRuntime::Cleanse(const EffectRemovalFilter &filter) noexcept {
  std::vector<EffectInstanceId> removals;
  for (const auto &[instanceId, state] : active_) {
    const EffectDefinition *definition = FindDefinition(state.DefinitionId);
    if (definition != nullptr && definition->Cleansable &&
        MatchesFilter(*definition, filter)) {
      removals.push_back(instanceId);
    }
  }
  for (const EffectInstanceId instanceId : removals) {
    static_cast<void>(Remove(instanceId));
  }
  return removals.size();
}

std::size_t EffectRuntime::Dispel(const EffectRemovalFilter &filter) noexcept {
  std::vector<EffectInstanceId> removals;
  for (const auto &[instanceId, state] : active_) {
    const EffectDefinition *definition = FindDefinition(state.DefinitionId);
    if (definition != nullptr && definition->Dispellable &&
        MatchesFilter(*definition, filter)) {
      removals.push_back(instanceId);
    }
  }
  for (const EffectInstanceId instanceId : removals) {
    static_cast<void>(Remove(instanceId));
  }
  return removals.size();
}

bool EffectRuntime::AdvanceTime(const double seconds) noexcept {
  if (!definitionsValid_ || !IsFiniteNonNegative(seconds)) {
    return false;
  }
  double timeLeft = seconds;
  while (timeLeft > Epsilon && !active_.empty()) {
    double step = timeLeft;
    for (const auto &[instanceId, state] : active_) {
      static_cast<void>(instanceId);
      const EffectDefinition *definition = FindDefinition(state.DefinitionId);
      if (definition == nullptr) {
        return false;
      }
      if (!definition->Permanent) {
        step = std::min(step, std::max(0.0, state.RemainingSeconds));
      }
      if (definition->TickIntervalSeconds > 0.0) {
        step = std::min(step, std::max(0.0, state.TimeUntilNextTick));
      }
    }

    if (step > Epsilon) {
      for (auto &[instanceId, state] : active_) {
        static_cast<void>(instanceId);
        const EffectDefinition *definition = FindDefinition(state.DefinitionId);
        if (definition == nullptr) {
          return false;
        }
        if (!definition->Permanent) {
          state.RemainingSeconds = std::max(0.0, state.RemainingSeconds - step);
        }
        if (definition->TickIntervalSeconds > 0.0) {
          state.TimeUntilNextTick =
              std::max(0.0, state.TimeUntilNextTick - step);
        }
      }
      timeLeft = std::max(0.0, timeLeft - step);
    }

    bool processedEvent = false;
    for (auto &[instanceId, state] : active_) {
      static_cast<void>(instanceId);
      const EffectDefinition *definition = FindDefinition(state.DefinitionId);
      if (definition != nullptr && definition->TickIntervalSeconds > 0.0 &&
          state.TimeUntilNextTick <= Epsilon) {
        if (!ApplyTick(state)) {
          return false;
        }
        state.TimeUntilNextTick += definition->TickIntervalSeconds;
        processedEvent = true;
      }
    }

    std::vector<EffectInstanceId> expired;
    for (const auto &[instanceId, state] : active_) {
      const EffectDefinition *definition = FindDefinition(state.DefinitionId);
      if (definition != nullptr && !definition->Permanent &&
          state.RemainingSeconds <= Epsilon) {
        expired.push_back(instanceId);
      }
    }
    for (const EffectInstanceId instanceId : expired) {
      static_cast<void>(Remove(instanceId));
      processedEvent = true;
    }

    if (step <= Epsilon && !processedEvent) {
      return false;
    }
  }
  return true;
}

EffectRuntimeState EffectRuntime::CaptureState() const {
  EffectRuntimeState state;
  state.NextInstanceValue = nextInstanceValue_;
  state.ActiveEffects.reserve(active_.size());
  for (const auto &[instanceId, active] : active_) {
    static_cast<void>(instanceId);
    state.ActiveEffects.push_back(active);
  }
  return state;
}

bool EffectRuntime::RestoreState(const EffectRuntimeState &state) {
  if (!ValidateState(state)) {
    return false;
  }

  const Combat::CombatantState ownerBefore = owner_.CaptureState();
  const auto activeBefore = active_;
  const std::uint64_t nextBefore = nextInstanceValue_;
  for (const auto &[instanceId, active] : active_) {
    static_cast<void>(instanceId);
    RemoveModifiers(active);
  }
  active_.clear();
  nextInstanceValue_ = state.NextInstanceValue;

  for (const ActiveEffectState &active : state.ActiveEffects) {
    if (!InstallModifiers(active)) {
      active_ = activeBefore;
      nextInstanceValue_ = nextBefore;
      static_cast<void>(owner_.RestoreState(ownerBefore));
      return false;
    }
    active_.emplace(active.InstanceId, active);
  }
  return true;
}

bool EffectRuntime::IsDefinitionValid(
    const EffectDefinition &definition) noexcept {
  if (!definition.Id.IsValid() || !definition.StackGroup.IsValid() ||
      !IsStackingPolicyValid(definition.Stacking) ||
      definition.MaxStacks == 0U ||
      definition.MaxStacks > MaximumConfiguredStacks ||
      !IsFiniteNonNegative(definition.DurationSeconds) ||
      !IsFiniteNonNegative(definition.TickIntervalSeconds) ||
      !IsFiniteNonNegative(definition.DamagePerTick) ||
      !IsFiniteNonNegative(definition.HealingPerTick) ||
      !IsRestrictionValid(definition.Restrictions) ||
      (!definition.Permanent && definition.DurationSeconds <= 0.0) ||
      (definition.Permanent && definition.DurationSeconds != 0.0) ||
      definition.AttributeModifiers.size() > 65535U) {
    return false;
  }
  if (definition.Stacking != EffectStackingPolicy::Independent &&
      definition.Stacking != EffectStackingPolicy::StackMagnitude &&
      definition.MaxStacks != 1U) {
    return false;
  }
  if (definition.TickIntervalSeconds == 0.0 &&
      (definition.DamagePerTick > 0.0 || definition.HealingPerTick > 0.0)) {
    return false;
  }
  std::set<Stats::AttributeId> modifiedAttributes;
  for (const EffectAttributeModifier &modifier :
       definition.AttributeModifiers) {
    if (!modifier.Attribute.IsValid() ||
        !IsModifierOperationValid(modifier.Operation) ||
        !std::isfinite(modifier.Magnitude) ||
        (modifier.Operation == Stats::ModifierOperation::Multiplicative &&
         modifier.Magnitude < 0.0) ||
        !modifiedAttributes.insert(modifier.Attribute).second) {
      return false;
    }
  }
  std::set<EffectId> effectImmunities;
  for (const EffectId immunity : definition.GrantedEffectImmunities) {
    if (!immunity.IsValid() || !effectImmunities.insert(immunity).second) {
      return false;
    }
  }
  std::set<GameplayTagId> tags;
  for (const GameplayTagId tag : definition.Tags) {
    if (!tag.IsValid() || !tags.insert(tag).second) {
      return false;
    }
  }
  std::set<GameplayTagId> tagImmunities;
  for (const GameplayTagId tag : definition.GrantedTagImmunities) {
    if (!tag.IsValid() || !tagImmunities.insert(tag).second) {
      return false;
    }
  }
  return true;
}

bool EffectRuntime::IsImmuneTo(
    const EffectDefinition &definition) const noexcept {
  for (const auto &[instanceId, state] : active_) {
    static_cast<void>(instanceId);
    const EffectDefinition *activeDefinition =
        FindDefinition(state.DefinitionId);
    if (activeDefinition == nullptr) {
      continue;
    }
    if (std::find(activeDefinition->GrantedEffectImmunities.begin(),
                  activeDefinition->GrantedEffectImmunities.end(),
                  definition.Id) !=
        activeDefinition->GrantedEffectImmunities.end()) {
      return true;
    }
    for (const GameplayTagId tag : definition.Tags) {
      if (ContainsTag(activeDefinition->GrantedTagImmunities, tag)) {
        return true;
      }
    }
  }
  return false;
}

bool EffectRuntime::MatchesFilter(
    const EffectDefinition &definition,
    const EffectRemovalFilter &filter) const noexcept {
  if (filter.Polarity == EffectPolarity::Beneficial && !definition.Beneficial) {
    return false;
  }
  if (filter.Polarity == EffectPolarity::Harmful && definition.Beneficial) {
    return false;
  }
  return !filter.RequiredTag.IsValid() ||
         ContainsTag(definition.Tags, filter.RequiredTag);
}

Stats::ModifierId
EffectRuntime::ModifierIdFor(const EffectInstanceId instance,
                             const std::size_t index) const noexcept {
  if (!instance.IsValid() || instance.Value > MaximumEffectInstance ||
      index >= 65535U) {
    return {};
  }
  const std::uint64_t encoded =
      (instance.Value << 16U) | (static_cast<std::uint64_t>(index) + 1U);
  return Stats::ModifierId{StatusModifierPrefix | encoded};
}

bool EffectRuntime::InstallModifiers(const ActiveEffectState &effect) {
  const EffectDefinition *definition = FindDefinition(effect.DefinitionId);
  if (definition == nullptr) {
    return false;
  }
  for (std::size_t index = 0; index < definition->AttributeModifiers.size();
       ++index) {
    const EffectAttributeModifier &source =
        definition->AttributeModifiers[index];
    const Stats::ModifierId modifierId =
        ModifierIdFor(effect.InstanceId, index);
    if (!modifierId.IsValid() || owner_.Attributes().HasModifier(modifierId)) {
      return false;
    }
    double magnitude = source.Magnitude;
    if (definition->Stacking == EffectStackingPolicy::StackMagnitude &&
        effect.Stacks > 1U) {
      if (source.Operation == Stats::ModifierOperation::Additive) {
        magnitude = SaturatingScale(source.Magnitude, effect.Stacks);
      } else {
        const double delta = source.Magnitude - 1.0;
        magnitude = std::max(0.0, 1.0 + SaturatingScale(delta, effect.Stacks));
      }
    }
    if (!owner_.AddAttributeModifier(
            {modifierId, source.Attribute, source.Operation,
             Stats::ModifierSource::StatusEffect, magnitude})) {
      return false;
    }
  }
  return true;
}

void EffectRuntime::RemoveModifiers(const ActiveEffectState &effect) noexcept {
  const EffectDefinition *definition = FindDefinition(effect.DefinitionId);
  if (definition == nullptr) {
    return;
  }
  for (std::size_t index = 0; index < definition->AttributeModifiers.size();
       ++index) {
    const Stats::ModifierId modifierId =
        ModifierIdFor(effect.InstanceId, index);
    if (modifierId.IsValid()) {
      static_cast<void>(owner_.RemoveAttributeModifier(modifierId));
    }
  }
}

bool EffectRuntime::ReinstallModifiers(ActiveEffectState &effect,
                                       const std::uint32_t newStacks) {
  const Combat::CombatantState before = owner_.CaptureState();
  RemoveModifiers(effect);
  ActiveEffectState candidate = effect;
  candidate.Stacks = newStacks;
  if (!InstallModifiers(candidate)) {
    static_cast<void>(owner_.RestoreState(before));
    return false;
  }
  return true;
}

bool EffectRuntime::ApplyTick(const ActiveEffectState &effect) noexcept {
  const EffectDefinition *definition = FindDefinition(effect.DefinitionId);
  if (definition == nullptr) {
    return false;
  }
  if (definition->DamagePerTick > 0.0) {
    const double amount =
        SaturatingScale(definition->DamagePerTick, effect.Stacks);
    if (!owner_.ApplyDamage(amount).WasValid) {
      return false;
    }
  }
  if (definition->HealingPerTick > 0.0) {
    const double amount =
        SaturatingScale(definition->HealingPerTick, effect.Stacks);
    if (!owner_.Heal(amount).WasValid) {
      return false;
    }
  }
  return true;
}

bool EffectRuntime::ValidateState(
    const EffectRuntimeState &state) const noexcept {
  if (!definitionsValid_ || state.NextInstanceValue == 0U ||
      state.NextInstanceValue > MaximumEffectInstance + 1U) {
    return false;
  }
  std::set<EffectInstanceId> instances;
  std::map<EffectStackGroupId, std::size_t> groupCounts;
  std::uint64_t greatestInstance = 0U;
  for (const ActiveEffectState &active : state.ActiveEffects) {
    const EffectDefinition *definition = FindDefinition(active.DefinitionId);
    if (!active.InstanceId.IsValid() || !active.SourceId.IsValid() ||
        definition == nullptr || !instances.insert(active.InstanceId).second ||
        active.Stacks == 0U || active.Stacks > definition->MaxStacks ||
        !IsFiniteNonNegative(active.RemainingSeconds) ||
        !IsFiniteNonNegative(active.TimeUntilNextTick)) {
      return false;
    }
    if (definition->Stacking != EffectStackingPolicy::StackMagnitude &&
        active.Stacks != 1U) {
      return false;
    }
    if (definition->Permanent && active.RemainingSeconds != 0.0) {
      return false;
    }
    if (!definition->Permanent &&
        (active.RemainingSeconds <= Epsilon ||
         active.RemainingSeconds > definition->DurationSeconds + Epsilon)) {
      return false;
    }
    if (definition->TickIntervalSeconds == 0.0 &&
        active.TimeUntilNextTick != 0.0) {
      return false;
    }
    if (definition->TickIntervalSeconds > 0.0 &&
        (active.TimeUntilNextTick <= Epsilon ||
         active.TimeUntilNextTick >
             definition->TickIntervalSeconds + Epsilon)) {
      return false;
    }
    greatestInstance = std::max(greatestInstance, active.InstanceId.Value);
    ++groupCounts[definition->StackGroup];
  }
  if (state.NextInstanceValue <= greatestInstance) {
    return false;
  }
  for (const auto &[group, count] : groupCounts) {
    std::uint32_t maximum = 1U;
    EffectStackingPolicy policy = EffectStackingPolicy::Replace;
    bool found = false;
    for (const auto &[id, definition] : definitions_) {
      static_cast<void>(id);
      if (definition.StackGroup == group) {
        if (!found) {
          maximum = definition.MaxStacks;
          policy = definition.Stacking;
          found = true;
        }
      }
    }
    if (!found ||
        (policy == EffectStackingPolicy::Independent && count > maximum) ||
        (policy != EffectStackingPolicy::Independent && count > 1U)) {
      return false;
    }
  }
  return true;
}

EffectInstanceId EffectRuntime::AllocateInstanceId() noexcept {
  if (nextInstanceValue_ == 0U || nextInstanceValue_ > MaximumEffectInstance) {
    return {};
  }
  return EffectInstanceId{nextInstanceValue_++};
}

} // namespace Lostsense::Gameplay
