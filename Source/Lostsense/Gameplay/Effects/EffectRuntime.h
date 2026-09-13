#pragma once

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/GameplayTypes.h"
#include "Lostsense/Stats/AttributeSet.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace Lostsense::Gameplay {

enum class EffectStackingPolicy : std::uint8_t {
  Replace,
  RefreshDuration,
  Independent,
  StackMagnitude,
};

enum class EffectRestriction : std::uint32_t {
  None = 0U,
  AbilityActivation = 1U << 0U,
  Movement = 1U << 1U,
};

[[nodiscard]] constexpr EffectRestriction
operator|(const EffectRestriction left,
          const EffectRestriction right) noexcept {
  return static_cast<EffectRestriction>(static_cast<std::uint32_t>(left) |
                                        static_cast<std::uint32_t>(right));
}

[[nodiscard]] constexpr bool
HasRestrictionFlag(const EffectRestriction value,
                   const EffectRestriction flag) noexcept {
  return (static_cast<std::uint32_t>(value) &
          static_cast<std::uint32_t>(flag)) != 0U;
}

struct EffectAttributeModifier final {
  Stats::AttributeId Attribute{};
  Stats::ModifierOperation Operation{Stats::ModifierOperation::Additive};
  double Magnitude{0.0};
};

struct EffectDefinition final {
  EffectId Id{};
  EffectStackGroupId StackGroup{};
  EffectStackingPolicy Stacking{EffectStackingPolicy::Independent};
  std::uint32_t MaxStacks{1U};
  double DurationSeconds{0.0};
  bool Permanent{false};
  double TickIntervalSeconds{0.0};
  double DamagePerTick{0.0};
  double HealingPerTick{0.0};
  std::vector<EffectAttributeModifier> AttributeModifiers{};
  EffectRestriction Restrictions{EffectRestriction::None};
  std::vector<EffectId> GrantedEffectImmunities{};
  std::vector<GameplayTagId> Tags{};
  std::vector<GameplayTagId> GrantedTagImmunities{};
  bool Beneficial{false};
  bool Cleansable{true};
  bool Dispellable{true};
};

struct ActiveEffectState final {
  EffectInstanceId InstanceId{};
  EffectId DefinitionId{};
  Combat::CombatantId SourceId{};
  std::uint32_t Stacks{1U};
  double RemainingSeconds{0.0};
  double TimeUntilNextTick{0.0};
};

struct EffectRuntimeState final {
  std::uint64_t NextInstanceValue{1U};
  std::vector<ActiveEffectState> ActiveEffects{};
};

enum class EffectApplyResult : std::uint8_t {
  Applied,
  Replaced,
  Refreshed,
  Stacked,
  Immune,
  AtMaxStacks,
  UnknownEffect,
  InvalidSource,
  InvalidRuntime,
  InternalFailure,
};

struct EffectApplyOutcome final {
  EffectApplyResult Result{EffectApplyResult::InternalFailure};
  EffectInstanceId InstanceId{};
};

enum class EffectPolarity : std::uint8_t { Any, Beneficial, Harmful };

struct EffectRemovalFilter final {
  EffectPolarity Polarity{EffectPolarity::Any};
  GameplayTagId RequiredTag{};
};

class EffectRuntime final {
public:
  EffectRuntime(Combat::Combatant &owner,
                std::vector<EffectDefinition> definitions);

  [[nodiscard]] bool IsValid() const noexcept { return definitionsValid_; }
  [[nodiscard]] Combat::CombatantId OwnerId() const noexcept {
    return owner_.Id();
  }
  [[nodiscard]] const EffectDefinition *
  FindDefinition(EffectId id) const noexcept;
  [[nodiscard]] std::size_t ActiveCount() const noexcept {
    return active_.size();
  }
  [[nodiscard]] bool HasEffect(EffectId id) const noexcept;
  [[nodiscard]] bool
  HasRestriction(EffectRestriction restriction) const noexcept;

  [[nodiscard]] EffectApplyOutcome Apply(EffectId id,
                                         Combat::CombatantId source);
  [[nodiscard]] bool Remove(EffectInstanceId instanceId) noexcept;
  [[nodiscard]] std::size_t Cleanse(const EffectRemovalFilter &filter) noexcept;
  [[nodiscard]] std::size_t Dispel(const EffectRemovalFilter &filter) noexcept;
  [[nodiscard]] bool AdvanceTime(double seconds) noexcept;

  [[nodiscard]] EffectRuntimeState CaptureState() const;
  [[nodiscard]] bool RestoreState(const EffectRuntimeState &state);

private:
  [[nodiscard]] static bool
  IsDefinitionValid(const EffectDefinition &definition) noexcept;
  [[nodiscard]] bool
  IsImmuneTo(const EffectDefinition &definition) const noexcept;
  [[nodiscard]] bool
  MatchesFilter(const EffectDefinition &definition,
                const EffectRemovalFilter &filter) const noexcept;
  [[nodiscard]] Stats::ModifierId
  ModifierIdFor(EffectInstanceId instance, std::size_t index) const noexcept;
  [[nodiscard]] bool InstallModifiers(const ActiveEffectState &effect);
  void RemoveModifiers(const ActiveEffectState &effect) noexcept;
  [[nodiscard]] bool ReinstallModifiers(ActiveEffectState &effect,
                                        std::uint32_t newStacks);
  [[nodiscard]] bool ApplyTick(const ActiveEffectState &effect) noexcept;
  [[nodiscard]] bool
  ValidateState(const EffectRuntimeState &state) const noexcept;
  [[nodiscard]] EffectInstanceId AllocateInstanceId() noexcept;

  Combat::Combatant &owner_;
  std::map<EffectId, EffectDefinition> definitions_{};
  std::map<EffectInstanceId, ActiveEffectState> active_{};
  std::uint64_t nextInstanceValue_{1U};
  bool definitionsValid_{true};
};

} // namespace Lostsense::Gameplay
