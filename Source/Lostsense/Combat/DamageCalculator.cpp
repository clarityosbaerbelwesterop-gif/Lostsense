#include "Lostsense/Combat/DamageCalculator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Lostsense::Combat {
namespace {

[[nodiscard]] double FiniteOrZero(const double value) noexcept {
  return std::isfinite(value) ? value : 0.0;
}

[[nodiscard]] double NonNegative(const double value) noexcept {
  return std::max(0.0, FiniteOrZero(value));
}

[[nodiscard]] double UnitInterval(const double value) noexcept {
  return std::clamp(FiniteOrZero(value), 0.0, 1.0);
}

[[nodiscard]] double IncreasedMultiplier(const double globalPercent,
                                         const double typePercent) noexcept {
  const long double percent =
      static_cast<long double>(FiniteOrZero(globalPercent)) +
      static_cast<long double>(FiniteOrZero(typePercent));
  const long double multiplier = std::max(0.0L, 1.0L + percent / 100.0L);
  const long double maximum =
      static_cast<long double>(std::numeric_limits<double>::max());
  return static_cast<double>(std::min(multiplier, maximum));
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

[[nodiscard]] double ResistanceMultiplier(const double resistance,
                                          const double penetration,
                                          const double cap) noexcept {
  const double cappedResistance =
      std::clamp(FiniteOrZero(resistance) - NonNegative(penetration), -1.0,
                 UnitInterval(cap));
  return 1.0 - cappedResistance;
}

[[nodiscard]] double ArmorMultiplier(const double armor,
                                     const double penetration) noexcept {
  const double effectiveArmor =
      std::max(0.0, NonNegative(armor) - NonNegative(penetration));
  // Smoothly scales without making physical damage fully immune to armor.
  return 100.0 / (100.0 + effectiveArmor);
}

} // namespace

DamageResult
DamageCalculator::Calculate(const DamageRequest &request) noexcept {
  DamageResult result;

  result.WasCritical =
      request.CanCritical && UnitInterval(request.CriticalRoll) <
                                 UnitInterval(request.Attacker.CriticalChance);
  const double criticalMultiplier =
      result.WasCritical
          ? std::max(1.0, FiniteOrZero(request.Attacker.CriticalMultiplier))
          : 1.0;

  for (std::size_t index = 0; index < DamageTypeCount; ++index) {
    const double increasedMultiplier = IncreasedMultiplier(
        request.Attacker.IncreasedDamagePercent,
        request.Attacker.IncreasedDamageByTypePercent[index]);
    double mitigationMultiplier =
        ResistanceMultiplier(request.Defender.Resistances[index],
                             request.Attacker.ResistancePenetration[index],
                             request.Defender.ResistanceCap);
    if (index == static_cast<std::size_t>(DamageType::Physical)) {
      mitigationMultiplier *= ArmorMultiplier(
          request.Defender.Armor, request.Attacker.ArmorPenetration);
    }

    double damage = SaturatingMultiply(NonNegative(request.BaseDamage[index]),
                                       increasedMultiplier);
    damage = SaturatingMultiply(damage, criticalMultiplier);
    result.AppliedByType[index] =
        SaturatingMultiply(damage, mitigationMultiplier);
  }

  result.WasBlocked =
      request.CanBlock && UnitInterval(request.BlockRoll) <
                              UnitInterval(request.Defender.BlockChance);
  if (result.WasBlocked) {
    const double blockedMultiplier =
        1.0 - UnitInterval(request.Defender.BlockMitigation);
    for (double &damage : result.AppliedByType) {
      damage *= blockedMultiplier;
    }
  }

  for (const double damage : result.AppliedByType) {
    result.TotalApplied = SaturatingAdd(result.TotalApplied, damage);
  }
  return result;
}

} // namespace Lostsense::Combat
