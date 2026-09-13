#pragma once

#include "Lostsense/Combat/DamageCalculator.h"
#include "Lostsense/Combat/VitalPools.h"
#include "Lostsense/Core/DeterministicRandom.h"
#include "Lostsense/Stats/AttributeSet.h"

#include <compare>
#include <cstddef>
#include <cstdint>

namespace Lostsense::Combat {

struct CombatantId final {
  std::uint64_t Value{0};

  constexpr CombatantId() noexcept = default;
  explicit constexpr CombatantId(const std::uint64_t value) noexcept
      : Value{value} {}

  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const CombatantId &, const CombatantId &) noexcept = default;
};

enum class CombatantKind : std::uint8_t {
  Player,
  Enemy,
  Elite,
  Boss,
};

struct DamageSpec final {
  DamageValues BaseDamage{};
  DamageValues AttackPowerCoefficients{};
  DamageValues SpellPowerCoefficients{};
  bool CanCritical{true};
  bool CanBlock{true};
};

struct CombatRolls final {
  double Critical{0.0};
  double Block{0.0};
};

struct CombatResolution final {
  DamageResult CalculatedDamage{};
  DamageApplication AppliedToHealth{};
};

struct CombatantState final {
  CombatantId Id{};
  CombatantKind Kind{CombatantKind::Enemy};
  Stats::AttributeSetState Attributes{};
  HealthState Health{};
  ResourceState Resource{};
};

class Combatant final {
public:
  Combatant(CombatantId id, CombatantKind kind);
  Combatant(CombatantId id, CombatantKind kind, Stats::AttributeSet attributes);

  [[nodiscard]] CombatantId Id() const noexcept { return id_; }
  [[nodiscard]] CombatantKind Kind() const noexcept { return kind_; }
  [[nodiscard]] const Stats::AttributeSet &Attributes() const noexcept {
    return attributes_;
  }
  [[nodiscard]] const HealthPool &Health() const noexcept { return health_; }
  [[nodiscard]] const ResourcePool &Resource() const noexcept {
    return resource_;
  }

  [[nodiscard]] bool SetBaseAttribute(Stats::AttributeId id,
                                      double value) noexcept;
  [[nodiscard]] bool AddAttributeModifier(Stats::AttributeModifier modifier);
  [[nodiscard]] bool RemoveAttributeModifier(Stats::ModifierId id) noexcept;
  [[nodiscard]] std::size_t
  RemoveAttributeModifiersBySource(Stats::ModifierSource source) noexcept;

  [[nodiscard]] ResourceConsumption TryConsumeResource(double amount) noexcept;
  [[nodiscard]] ResourceRestoration RestoreResource(double amount) noexcept;
  [[nodiscard]] HealingApplication Heal(double amount) noexcept;
  [[nodiscard]] bool Revive(double health) noexcept;
  [[nodiscard]] DamageApplication ApplyDamage(double amount) noexcept;

  [[nodiscard]] CombatResolution
  ResolveAttack(Combatant &target, const DamageSpec &spec,
                CombatRolls rolls) const noexcept;
  // This overload always consumes two samples, keeping stream advancement
  // stable even when a particular attack cannot critically hit or be blocked.
  [[nodiscard]] CombatResolution
  ResolveAttack(Combatant &target, const DamageSpec &spec,
                Core::DeterministicRandom &random) const noexcept;

  [[nodiscard]] CombatantState CaptureState() const;
  [[nodiscard]] bool RestoreState(const CombatantState &state);

private:
  void SynchronizeDerivedPools() noexcept;
  [[nodiscard]] DamageValues
  CalculateScaledBaseDamage(const DamageSpec &spec) const noexcept;
  [[nodiscard]] OffensiveStats BuildOffensiveStats() const noexcept;
  [[nodiscard]] DefensiveStats BuildDefensiveStats() const noexcept;

  CombatantId id_{};
  CombatantKind kind_{CombatantKind::Enemy};
  Stats::AttributeSet attributes_{};
  HealthPool health_{};
  ResourcePool resource_{};
};

} // namespace Lostsense::Combat
