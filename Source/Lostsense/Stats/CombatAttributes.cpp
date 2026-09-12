#include "Lostsense/Stats/CombatAttributes.h"

#include <cstdlib>

namespace Lostsense::Stats::CombatAttributes {
namespace {

constexpr double MaximumMagnitude = 1.0e15;
constexpr double MaximumPercent = 1.0e6;

void DefineOrAbort(AttributeSet &attributes,
                   const AttributeDefinition definition) {
  if (!attributes.Define(definition)) {
    std::abort();
  }
}

} // namespace

AttributeSet CreateDefaultSet() {
  AttributeSet attributes;
  DefineOrAbort(attributes, {MaxHealth, 100.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {MaxResource, 100.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {Strength, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {Dexterity, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {Intelligence, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {AttackPower, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {SpellPower, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {Armor, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {MovementSpeedMultiplier, 1.0, 0.0, 10.0});
  DefineOrAbort(attributes, {CriticalChance, 0.05, 0.0, 1.0});
  DefineOrAbort(attributes, {CriticalMultiplier, 1.5, 1.0, 100.0});
  DefineOrAbort(attributes, {BlockChance, 0.0, 0.0, 1.0});
  DefineOrAbort(attributes, {BlockMitigation, 0.5, 0.0, 1.0});
  DefineOrAbort(attributes, {DamageBonusPercent, 0.0, -100.0, MaximumPercent});
  DefineOrAbort(attributes, {ArmorPenetration, 0.0, 0.0, MaximumMagnitude});
  DefineOrAbort(attributes, {ResistanceCap, 0.75, 0.0, 1.0});

  for (std::size_t index = 0; index < Combat::DamageTypeCount; ++index) {
    const auto type = static_cast<Combat::DamageType>(index);
    DefineOrAbort(attributes, {DamageBonus(type), 0.0, -100.0, MaximumPercent});
    DefineOrAbort(attributes, {Resistance(type), 0.0, -1.0, 1.0});
    DefineOrAbort(attributes, {ResistancePenetration(type), 0.0, 0.0, 2.0});
  }
  return attributes;
}

} // namespace Lostsense::Stats::CombatAttributes
