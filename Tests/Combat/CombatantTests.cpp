#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Stats/CombatAttributes.h"
#include "TestHarness.h"

#include <cstddef>

namespace {

using namespace Lostsense::Combat;
using namespace Lostsense::Stats;
using Lostsense::Core::DeterministicRandom;
using Lostsense::Tests::TestSuite;

constexpr std::size_t Physical = static_cast<std::size_t>(DamageType::Physical);
constexpr std::size_t Fire = static_cast<std::size_t>(DamageType::Fire);

void FillHealth(Combatant &combatant, const double maximum) {
  static_cast<void>(
      combatant.SetBaseAttribute(CombatAttributes::MaxHealth, maximum));
  static_cast<void>(combatant.Heal(maximum));
}

void TestAttributesDriveDamageAndHealth(TestSuite &suite) {
  Combatant attacker{CombatantId{1U}, CombatantKind::Player};
  Combatant target{CombatantId{2U}, CombatantKind::Enemy};
  FillHealth(target, 500.0);

  suite.Expect(attacker.SetBaseAttribute(CombatAttributes::AttackPower, 50.0),
               "attack power can be configured");
  suite.Expect(
      attacker.SetBaseAttribute(CombatAttributes::DamageBonusPercent, 20.0),
      "global damage bonus can be configured");
  suite.Expect(attacker.SetBaseAttribute(
                   CombatAttributes::DamageBonus(DamageType::Physical), 10.0),
               "type damage bonus can be configured");
  suite.Expect(target.SetBaseAttribute(CombatAttributes::Armor, 100.0),
               "defender armor can be configured");

  DamageSpec attack;
  attack.BaseDamage[Physical] = 100.0;
  attack.AttackPowerCoefficients[Physical] = 1.0;
  attack.CanCritical = false;
  attack.CanBlock = false;
  const CombatResolution result =
      attacker.ResolveAttack(target, attack, CombatRolls{});

  suite.ExpectNear(result.CalculatedDamage.TotalApplied, 97.5,
                   "scaling, bonuses and mitigation form one damage pipeline");
  suite.ExpectNear(result.AppliedToHealth.Applied, 97.5,
                   "calculated damage is applied to target health");
  suite.ExpectNear(target.Health().Current(), 402.5,
                   "target health reflects resolved attack");
}

void TestCriticalBlockResistanceAndPenetration(TestSuite &suite) {
  Combatant attacker{CombatantId{3U}, CombatantKind::Player};
  Combatant boss{CombatantId{4U}, CombatantKind::Boss};
  FillHealth(boss, 500.0);

  static_cast<void>(
      attacker.SetBaseAttribute(CombatAttributes::CriticalChance, 1.0));
  static_cast<void>(
      attacker.SetBaseAttribute(CombatAttributes::CriticalMultiplier, 2.0));
  static_cast<void>(attacker.SetBaseAttribute(
      CombatAttributes::ResistancePenetration(DamageType::Fire), 0.10));
  static_cast<void>(boss.SetBaseAttribute(
      CombatAttributes::Resistance(DamageType::Fire), 0.25));
  static_cast<void>(boss.SetBaseAttribute(CombatAttributes::BlockChance, 1.0));
  static_cast<void>(
      boss.SetBaseAttribute(CombatAttributes::BlockMitigation, 0.5));

  DamageSpec spell;
  spell.BaseDamage[Fire] = 100.0;
  const CombatResolution result =
      attacker.ResolveAttack(boss, spell, CombatRolls{0.99, 0.99});
  suite.Expect(result.CalculatedDamage.WasCritical,
               "combatant forwards deterministic critical roll");
  suite.Expect(result.CalculatedDamage.WasBlocked,
               "combatant forwards deterministic block roll");
  suite.ExpectNear(result.CalculatedDamage.TotalApplied, 85.0,
                   "critical, resistance, penetration and block compose");
}

void TestDerivedPoolsTrackAttributes(TestSuite &suite) {
  Combatant combatant{CombatantId{5U}, CombatantKind::Elite};
  static_cast<void>(
      combatant.ResolveAttack(combatant,
                              DamageSpec{.BaseDamage =
                                             [] {
                                               DamageValues damage{};
                                               damage[Physical] = 40.0;
                                               return damage;
                                             }(),
                                         .CanCritical = false,
                                         .CanBlock = false},
                              CombatRolls{}));
  suite.ExpectNear(combatant.Health().Current(), 60.0,
                   "setup damage reduces health");

  suite.Expect(combatant.AddAttributeModifier(
                   {ModifierId{500U}, CombatAttributes::MaxHealth,
                    ModifierOperation::Multiplicative,
                    ModifierSource::StatusEffect, 0.5}),
               "maximum-health status modifier is accepted");
  suite.ExpectNear(combatant.Health().Maximum(), 50.0,
                   "derived health maximum updates immediately");
  suite.ExpectNear(combatant.Health().Current(), 50.0,
                   "current health clamps when maximum falls");
  suite.Expect(combatant.RemoveAttributeModifier(ModifierId{500U}),
               "maximum-health modifier can be removed");
  suite.ExpectNear(combatant.Health().Maximum(), 100.0,
                   "removed modifier restores maximum");
  suite.ExpectNear(combatant.Health().Current(), 50.0,
                   "raising maximum does not create free healing");

  suite.Expect(combatant.SetBaseAttribute(CombatAttributes::MaxResource, 30.0),
               "resource maximum attribute can change");
  suite.ExpectNear(combatant.Resource().Current(), 30.0,
                   "resource current clamps to its new maximum");
  suite.Expect(combatant.TryConsumeResource(25.0).Succeeded,
               "combatant exposes atomic resource spending");
  suite.Expect(!combatant.TryConsumeResource(6.0).Succeeded,
               "resource spending cannot underflow");
}

void TestRandomResolutionIsReproducible(TestSuite &suite) {
  Combatant firstAttacker{CombatantId{10U}, CombatantKind::Player};
  Combatant secondAttacker{CombatantId{11U}, CombatantKind::Player};
  Combatant firstTarget{CombatantId{12U}, CombatantKind::Enemy};
  Combatant secondTarget{CombatantId{13U}, CombatantKind::Enemy};
  static_cast<void>(
      firstAttacker.SetBaseAttribute(CombatAttributes::CriticalChance, 0.5));
  static_cast<void>(
      secondAttacker.SetBaseAttribute(CombatAttributes::CriticalChance, 0.5));
  static_cast<void>(
      firstTarget.SetBaseAttribute(CombatAttributes::BlockChance, 0.5));
  static_cast<void>(
      secondTarget.SetBaseAttribute(CombatAttributes::BlockChance, 0.5));

  DamageSpec attack;
  attack.BaseDamage[Physical] = 30.0;
  DeterministicRandom firstRandom{777U, 4U};
  DeterministicRandom secondRandom{777U, 4U};
  const CombatResolution first =
      firstAttacker.ResolveAttack(firstTarget, attack, firstRandom);
  const CombatResolution second =
      secondAttacker.ResolveAttack(secondTarget, attack, secondRandom);
  suite.ExpectNear(first.CalculatedDamage.TotalApplied,
                   second.CalculatedDamage.TotalApplied,
                   "same random stream reproduces damage outcome");
  suite.Expect(first.CalculatedDamage.WasCritical ==
                       second.CalculatedDamage.WasCritical &&
                   first.CalculatedDamage.WasBlocked ==
                       second.CalculatedDamage.WasBlocked,
               "same stream reproduces combat event flags");
  suite.Expect(firstRandom.CaptureState() == secondRandom.CaptureState(),
               "equal combat consumes equal random state");

  DamageSpec deterministicAttack = attack;
  deterministicAttack.CanCritical = false;
  deterministicAttack.CanBlock = false;
  DeterministicRandom usedByCombat{99U, 2U};
  DeterministicRandom expectedAdvance{99U, 2U};
  static_cast<void>(expectedAdvance.NextUnit());
  static_cast<void>(expectedAdvance.NextUnit());
  static_cast<void>(firstAttacker.ResolveAttack(
      firstTarget, deterministicAttack, usedByCombat));
  suite.Expect(usedByCombat.CaptureState() == expectedAdvance.CaptureState(),
               "combat always advances exactly two authoritative samples");
}

void TestDeadAttackerCannotResolveDamage(TestSuite &suite) {
  Combatant attacker{CombatantId{30U}, CombatantKind::Player};
  Combatant target{CombatantId{31U}, CombatantKind::Enemy};
  FillHealth(target, 100.0);

  suite.Expect(attacker.ApplyDamage(1000.0).BecameDead,
               "setup damage kills attacker");
  DamageSpec attack;
  attack.BaseDamage[Physical] = 50.0;
  attack.CanCritical = false;
  attack.CanBlock = false;

  const CombatResolution resolution =
      attacker.ResolveAttack(target, attack, CombatRolls{});
  suite.Expect(!resolution.AppliedToHealth.WasValid,
               "dead attacker cannot produce a valid damage application");
  suite.ExpectNear(target.Health().Current(), 100.0,
                   "dead attacker leaves target health unchanged");

  DeterministicRandom random{303U, 9U};
  DeterministicRandom expected{303U, 9U};
  static_cast<void>(expected.NextUnit());
  static_cast<void>(expected.NextUnit());
  static_cast<void>(attacker.ResolveAttack(target, attack, random));
  suite.Expect(
      random.CaptureState() == expected.CaptureState(),
      "dead deterministic attack still consumes the documented two samples");
}

void TestCombatantStateRoundTrip(TestSuite &suite) {
  Combatant combatant{CombatantId{20U}, CombatantKind::Player};
  Combatant enemy{CombatantId{21U}, CombatantKind::Enemy};
  DamageSpec hit;
  hit.BaseDamage[Physical] = 25.0;
  hit.CanCritical = false;
  hit.CanBlock = false;
  static_cast<void>(enemy.ResolveAttack(combatant, hit, CombatRolls{}));
  static_cast<void>(combatant.TryConsumeResource(40.0));
  const CombatantState saved = combatant.CaptureState();

  static_cast<void>(enemy.ResolveAttack(combatant, hit, CombatRolls{}));
  static_cast<void>(combatant.TryConsumeResource(20.0));
  suite.Expect(combatant.RestoreState(saved),
               "consistent combatant state restores atomically");
  suite.ExpectNear(combatant.Health().Current(), 75.0,
                   "combatant restore includes health");
  suite.ExpectNear(combatant.Resource().Current(), 60.0,
                   "combatant restore includes resource");

  CombatantState corrupt = saved;
  corrupt.Health.Maximum = 101.0;
  suite.Expect(!combatant.RestoreState(corrupt),
               "state inconsistent with derived attributes is rejected");
  suite.ExpectNear(combatant.Health().Current(), 75.0,
                   "rejected combatant state leaves health unchanged");
}

} // namespace

int main() {
  TestSuite suite{"combatant"};
  TestAttributesDriveDamageAndHealth(suite);
  TestCriticalBlockResistanceAndPenetration(suite);
  TestDerivedPoolsTrackAttributes(suite);
  TestRandomResolutionIsReproducible(suite);
  TestDeadAttackerCannotResolveDamage(suite);
  TestCombatantStateRoundTrip(suite);
  return suite.Finish();
}
