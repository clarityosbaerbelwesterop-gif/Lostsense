#include "Lostsense/Combat/Combatant.h"

#include "Lostsense/Stats/CombatAttributes.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Lostsense::Combat {
namespace {

[[nodiscard]] double NonNegativeFinite(const double value) noexcept {
  return std::isfinite(value) ? std::max(0.0, value) : 0.0;
}

[[nodiscard]] double SaturatingMultiply(const double left,
                                        const double right) noexcept {
  if (left <= 0.0 || right <= 0.0) {
    return 0.0;
  }
  const double maximum = std::numeric_limits<double>::max();
  return left > maximum / right ? maximum : left * right;
}

[[nodiscard]] double SaturatingAdd(const double left,
                                   const double right) noexcept {
  const double maximum = std::numeric_limits<double>::max();
  return left > maximum - right ? maximum : left + right;
}

} // namespace

Combatant::Combatant(const CombatantId id, const CombatantKind kind)
    : Combatant{id, kind, Stats::CombatAttributes::CreateDefaultSet()} {}

Combatant::Combatant(const CombatantId id, const CombatantKind kind,
                     Stats::AttributeSet attributes)
    : id_{id}, kind_{kind}, attributes_{std::move(attributes)},
      health_{attributes_.Get(Stats::CombatAttributes::MaxHealth)},
      resource_{attributes_.Get(Stats::CombatAttributes::MaxResource)} {}

bool Combatant::SetBaseAttribute(const Stats::AttributeId id,
                                 const double value) noexcept {
  if (!attributes_.SetBase(id, value)) {
    return false;
  }
  SynchronizeDerivedPools();
  return true;
}

bool Combatant::AddAttributeModifier(const Stats::AttributeModifier modifier) {
  if (!attributes_.AddModifier(modifier)) {
    return false;
  }
  SynchronizeDerivedPools();
  return true;
}

bool Combatant::RemoveAttributeModifier(const Stats::ModifierId id) noexcept {
  if (!attributes_.RemoveModifier(id)) {
    return false;
  }
  SynchronizeDerivedPools();
  return true;
}

std::size_t Combatant::RemoveAttributeModifiersBySource(
    const Stats::ModifierSource source) noexcept {
  const std::size_t removed = attributes_.RemoveModifiersBySource(source);
  SynchronizeDerivedPools();
  return removed;
}

ResourceConsumption
Combatant::TryConsumeResource(const double amount) noexcept {
  return resource_.TryConsume(amount);
}

ResourceRestoration Combatant::RestoreResource(const double amount) noexcept {
  return resource_.Restore(amount);
}

HealingApplication Combatant::Heal(const double amount) noexcept {
  return health_.Heal(amount);
}

bool Combatant::Revive(const double health) noexcept {
  return health_.Revive(health);
}

CombatResolution
Combatant::ResolveAttack(Combatant &target, const DamageSpec &spec,
                         const CombatRolls rolls) const noexcept {
  DamageRequest request;
  request.BaseDamage = CalculateScaledBaseDamage(spec);
  request.Attacker = BuildOffensiveStats();
  request.Defender = target.BuildDefensiveStats();
  request.CriticalRoll = rolls.Critical;
  request.BlockRoll = rolls.Block;
  request.CanCritical = spec.CanCritical;
  request.CanBlock = spec.CanBlock;

  CombatResolution result;
  result.CalculatedDamage = DamageCalculator::Calculate(request);
  result.AppliedToHealth =
      target.health_.ApplyDamage(result.CalculatedDamage.TotalApplied);
  return result;
}

CombatResolution
Combatant::ResolveAttack(Combatant &target, const DamageSpec &spec,
                         Core::DeterministicRandom &random) const noexcept {
  const CombatRolls rolls{random.NextUnit(), random.NextUnit()};
  return ResolveAttack(target, spec, rolls);
}

CombatantState Combatant::CaptureState() const {
  return {id_, kind_, attributes_.CaptureState(), health_.CaptureState(),
          resource_.CaptureState()};
}

bool Combatant::RestoreState(const CombatantState &state) {
  if (state.Id != id_ || state.Kind != kind_) {
    return false;
  }

  Stats::AttributeSet restoredAttributes = attributes_;
  HealthPool restoredHealth = health_;
  ResourcePool restoredResource = resource_;
  if (!restoredAttributes.RestoreState(state.Attributes) ||
      !restoredHealth.RestoreState(state.Health) ||
      !restoredResource.RestoreState(state.Resource) ||
      restoredAttributes.Get(Stats::CombatAttributes::MaxHealth) !=
          restoredHealth.Maximum() ||
      restoredAttributes.Get(Stats::CombatAttributes::MaxResource) !=
          restoredResource.Maximum()) {
    return false;
  }

  attributes_ = std::move(restoredAttributes);
  health_ = restoredHealth;
  resource_ = restoredResource;
  return true;
}

void Combatant::SynchronizeDerivedPools() noexcept {
  static_cast<void>(
      health_.SetMaximum(attributes_.Get(Stats::CombatAttributes::MaxHealth)));
  static_cast<void>(resource_.SetMaximum(
      attributes_.Get(Stats::CombatAttributes::MaxResource)));
}

DamageValues
Combatant::CalculateScaledBaseDamage(const DamageSpec &spec) const noexcept {
  const double attackPower =
      attributes_.Get(Stats::CombatAttributes::AttackPower);
  const double spellPower =
      attributes_.Get(Stats::CombatAttributes::SpellPower);
  DamageValues scaled{};
  for (std::size_t index = 0; index < DamageTypeCount; ++index) {
    const double attackContribution = SaturatingMultiply(
        attackPower, NonNegativeFinite(spec.AttackPowerCoefficients[index]));
    const double spellContribution = SaturatingMultiply(
        spellPower, NonNegativeFinite(spec.SpellPowerCoefficients[index]));
    scaled[index] =
        SaturatingAdd(SaturatingAdd(NonNegativeFinite(spec.BaseDamage[index]),
                                    attackContribution),
                      spellContribution);
  }
  return scaled;
}

OffensiveStats Combatant::BuildOffensiveStats() const noexcept {
  OffensiveStats stats;
  stats.IncreasedDamagePercent =
      attributes_.Get(Stats::CombatAttributes::DamageBonusPercent);
  stats.CriticalChance =
      attributes_.Get(Stats::CombatAttributes::CriticalChance);
  stats.CriticalMultiplier =
      attributes_.Get(Stats::CombatAttributes::CriticalMultiplier);
  stats.ArmorPenetration =
      attributes_.Get(Stats::CombatAttributes::ArmorPenetration);
  for (std::size_t index = 0; index < DamageTypeCount; ++index) {
    const auto type = static_cast<DamageType>(index);
    stats.IncreasedDamageByTypePercent[index] =
        attributes_.Get(Stats::CombatAttributes::DamageBonus(type));
    stats.ResistancePenetration[index] =
        attributes_.Get(Stats::CombatAttributes::ResistancePenetration(type));
  }
  return stats;
}

DefensiveStats Combatant::BuildDefensiveStats() const noexcept {
  DefensiveStats stats;
  stats.Armor = attributes_.Get(Stats::CombatAttributes::Armor);
  stats.ResistanceCap = attributes_.Get(Stats::CombatAttributes::ResistanceCap);
  stats.BlockChance = attributes_.Get(Stats::CombatAttributes::BlockChance);
  stats.BlockMitigation =
      attributes_.Get(Stats::CombatAttributes::BlockMitigation);
  for (std::size_t index = 0; index < DamageTypeCount; ++index) {
    stats.Resistances[index] = attributes_.Get(
        Stats::CombatAttributes::Resistance(static_cast<DamageType>(index)));
  }
  return stats;
}

} // namespace Lostsense::Combat
