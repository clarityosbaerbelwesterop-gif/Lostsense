#include "Lostsense/Combat/DamageCalculator.h"

#include <algorithm>
#include <cmath>

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

  const double increasedMultiplier = std::max(
      0.0, 1.0 + FiniteOrZero(request.Attacker.IncreasedDamagePercent) / 100.0);
  result.WasCritical =
      request.CanCritical && UnitInterval(request.CriticalRoll) <
                                 UnitInterval(request.Attacker.CriticalChance);
  const double criticalMultiplier =
      result.WasCritical
          ? std::max(1.0, FiniteOrZero(request.Attacker.CriticalMultiplier))
          : 1.0;

  for (std::size_t index = 0; index < DamageTypeCount; ++index) {
    double mitigationMultiplier =
        ResistanceMultiplier(request.Defender.Resistances[index],
                             request.Attacker.ResistancePenetration[index],
                             request.Defender.ResistanceCap);
    if (index == static_cast<std::size_t>(DamageType::Physical)) {
      mitigationMultiplier *= ArmorMultiplier(
          request.Defender.Armor, request.Attacker.ArmorPenetration);
    }

    result.AppliedByType[index] = NonNegative(request.BaseDamage[index]) *
                                  increasedMultiplier * criticalMultiplier *
                                  mitigationMultiplier;
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
    result.TotalApplied += damage;
  }
  return result;
}

} // namespace Lostsense::Combat
