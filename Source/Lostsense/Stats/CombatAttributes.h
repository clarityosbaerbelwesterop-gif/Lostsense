#pragma once

#include "Lostsense/Combat/DamageCalculator.h"
#include "Lostsense/Stats/AttributeSet.h"

#include <cstdint>

namespace Lostsense::Stats::CombatAttributes {

inline constexpr AttributeId MaxHealth{1};
inline constexpr AttributeId MaxResource{2};
inline constexpr AttributeId Strength{3};
inline constexpr AttributeId Dexterity{4};
inline constexpr AttributeId Intelligence{5};
inline constexpr AttributeId AttackPower{6};
inline constexpr AttributeId SpellPower{7};
inline constexpr AttributeId Armor{8};
inline constexpr AttributeId MovementSpeedMultiplier{9};
inline constexpr AttributeId CriticalChance{10};
inline constexpr AttributeId CriticalMultiplier{11};
inline constexpr AttributeId BlockChance{12};
inline constexpr AttributeId BlockMitigation{13};
inline constexpr AttributeId DamageBonusPercent{14};
inline constexpr AttributeId ArmorPenetration{15};
inline constexpr AttributeId ResistanceCap{16};

inline constexpr std::uint32_t DamageBonusBase = 100;
inline constexpr std::uint32_t ResistanceBase = 200;
inline constexpr std::uint32_t ResistancePenetrationBase = 300;

[[nodiscard]] constexpr AttributeId
DamageBonus(const Combat::DamageType type) noexcept {
  const auto index = static_cast<std::uint32_t>(type);
  return index < Combat::DamageTypeCount ? AttributeId{DamageBonusBase + index}
                                         : AttributeId{};
}

[[nodiscard]] constexpr AttributeId
Resistance(const Combat::DamageType type) noexcept {
  const auto index = static_cast<std::uint32_t>(type);
  return index < Combat::DamageTypeCount ? AttributeId{ResistanceBase + index}
                                         : AttributeId{};
}

[[nodiscard]] constexpr AttributeId
ResistancePenetration(const Combat::DamageType type) noexcept {
  const auto index = static_cast<std::uint32_t>(type);
  return index < Combat::DamageTypeCount
             ? AttributeId{ResistancePenetrationBase + index}
             : AttributeId{};
}

[[nodiscard]] AttributeSet CreateDefaultSet();

} // namespace Lostsense::Stats::CombatAttributes
