#include "Lostsense/Combat/DamageCalculator.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using namespace Lostsense::Combat;

int Failures = 0;

void ExpectNear(const double actual, const double expected,
                const std::string_view description) {
  if (std::abs(actual - expected) > 0.000001) {
    std::cerr << "FAIL: " << description << " (expected " << expected
              << ", got " << actual << ")\n";
    ++Failures;
  }
}

void Expect(const bool condition, const std::string_view description) {
  if (!condition) {
    std::cerr << "FAIL: " << description << '\n';
    ++Failures;
  }
}

void TestMixedDamagePipeline() {
  DamageRequest request;
  request.BaseDamage[static_cast<std::size_t>(DamageType::Physical)] = 100.0;
  request.BaseDamage[static_cast<std::size_t>(DamageType::Fire)] = 50.0;
  request.Attacker.IncreasedDamagePercent = 20.0;
  request.Defender.Armor = 100.0;
  request.Defender.Resistances[static_cast<std::size_t>(DamageType::Fire)] =
      0.25;
  request.CanCritical = false;
  request.CanBlock = false;

  const DamageResult result = DamageCalculator::Calculate(request);

  ExpectNear(result.AppliedByType[0], 60.0, "armor mitigates physical damage");
  ExpectNear(result.AppliedByType[1], 45.0, "resistance mitigates fire damage");
  ExpectNear(result.TotalApplied, 105.0, "mixed damage is accumulated");
}

void TestCriticalBlockAndPenetration() {
  DamageRequest request;
  request.BaseDamage[static_cast<std::size_t>(DamageType::Fire)] = 100.0;
  request.Attacker.CriticalChance = 0.5;
  request.Attacker.CriticalMultiplier = 2.0;
  request.Attacker
      .ResistancePenetration[static_cast<std::size_t>(DamageType::Fire)] = 0.25;
  request.Defender.Resistances[static_cast<std::size_t>(DamageType::Fire)] =
      0.5;
  request.Defender.BlockChance = 1.0;
  request.Defender.BlockMitigation = 0.4;
  request.CriticalRoll = 0.49;
  request.BlockRoll = 0.99;

  const DamageResult result = DamageCalculator::Calculate(request);

  Expect(result.WasCritical, "authoritative roll produces a critical hit");
  Expect(result.WasBlocked, "authoritative roll produces a block");
  ExpectNear(result.TotalApplied, 90.0,
             "critical, penetration and block compose in order");
}

void TestResistanceRules() {
  DamageRequest request;
  request.BaseDamage[static_cast<std::size_t>(DamageType::Frost)] = 100.0;
  request.Defender.Resistances[static_cast<std::size_t>(DamageType::Frost)] =
      10.0;
  request.Defender.ResistanceCap = 0.75;
  request.CanCritical = false;
  request.CanBlock = false;
  ExpectNear(DamageCalculator::Calculate(request).TotalApplied, 25.0,
             "positive resistance obeys its cap");

  request.Defender.Resistances[static_cast<std::size_t>(DamageType::Frost)] =
      -0.5;
  ExpectNear(DamageCalculator::Calculate(request).TotalApplied, 150.0,
             "negative resistance increases damage");
}

void TestInvalidInputsAreSafe() {
  DamageRequest request;
  request.BaseDamage[0] = -100.0;
  request.BaseDamage[1] = std::numeric_limits<double>::quiet_NaN();
  request.BaseDamage[2] = 40.0;
  request.Attacker.IncreasedDamagePercent = -200.0;
  request.Attacker.CriticalChance = std::numeric_limits<double>::infinity();

  const DamageResult result = DamageCalculator::Calculate(request);
  ExpectNear(result.TotalApplied, 0.0,
             "invalid and negative values cannot produce invalid damage");
  Expect(std::isfinite(result.TotalApplied), "result remains finite");
}

void TestDamageTypeBonusesAndOverflowSafety() {
  DamageRequest request;
  request.BaseDamage[static_cast<std::size_t>(DamageType::Fire)] = 100.0;
  request.Attacker.IncreasedDamagePercent = 10.0;
  request.Attacker.IncreasedDamageByTypePercent[static_cast<std::size_t>(
      DamageType::Fire)] = 20.0;
  request.CanCritical = false;
  request.CanBlock = false;
  ExpectNear(DamageCalculator::Calculate(request).TotalApplied, 130.0,
             "global and matching type bonuses combine additively");

  request.BaseDamage[static_cast<std::size_t>(DamageType::Fire)] =
      std::numeric_limits<double>::max();
  request.Attacker.IncreasedDamagePercent =
      std::numeric_limits<double>::max();
  const DamageResult saturated = DamageCalculator::Calculate(request);
  Expect(std::isfinite(saturated.TotalApplied),
         "finite extreme inputs saturate instead of overflowing");
  Expect(saturated.TotalApplied == std::numeric_limits<double>::max(),
         "overflow saturation preserves maximum representable damage");
}

} // namespace

int main() {
  TestMixedDamagePipeline();
  TestCriticalBlockAndPenetration();
  TestResistanceRules();
  TestInvalidInputsAreSafe();
  TestDamageTypeBonusesAndOverflowSafety();

  if (Failures == 0) {
    std::cout << "All combat damage tests passed.\n";
  }
  return Failures == 0 ? 0 : 1;
}
