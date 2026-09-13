#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"
#include "Lostsense/Stats/CombatAttributes.h"
#include "TestHarness.h"

#include <algorithm>
#include <vector>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using Tests::TestSuite;

constexpr ClassId Knight{1U};
constexpr ClassId Wizard{2U};
constexpr EffectId Burning{1U};
constexpr EffectId Stun{2U};
constexpr AbilityId Strike{10U};
constexpr AbilityId ChargedBlast{11U};
constexpr AbilityId GuardStep{12U};
constexpr AbilityId WizardOnly{13U};
constexpr AbilityId AtomicFailure{14U};
constexpr CooldownGroupId MartialGroup{1U};

std::vector<EffectDefinition> EffectDefinitions() {
  EffectDefinition burning;
  burning.Id = Burning;
  burning.StackGroup = {1U};
  burning.Stacking = EffectStackingPolicy::StackMagnitude;
  burning.MaxStacks = 3U;
  burning.DurationSeconds = 4.0;
  burning.TickIntervalSeconds = 1.0;
  burning.DamagePerTick = 4.0;

  EffectDefinition stun;
  stun.Id = Stun;
  stun.StackGroup = {2U};
  stun.Stacking = EffectStackingPolicy::RefreshDuration;
  stun.DurationSeconds = 2.0;
  stun.Restrictions = EffectRestriction::AbilityActivation;
  return {burning, stun};
}

Combat::DamageSpec PhysicalDamage(const double value) {
  Combat::DamageSpec damage;
  damage.BaseDamage[static_cast<std::size_t>(Combat::DamageType::Physical)] =
      value;
  damage.CanCritical = false;
  damage.CanBlock = false;
  return damage;
}

std::vector<AbilityDefinition> AbilityDefinitions() {
  AbilityDefinition strike;
  strike.Id = Strike;
  strike.CooldownSeconds = 1.0;
  strike.TargetRule = AbilityTargetRule::Hostile;
  strike.Damage = PhysicalDamage(20.0);
  strike.DealsDamage = true;

  AbilityDefinition blast;
  blast.Id = ChargedBlast;
  blast.ResourceCost = 20.0;
  blast.CooldownSeconds = 2.0;
  blast.CooldownGroup = MartialGroup;
  blast.MaximumCharges = 2U;
  blast.RechargeSeconds = 5.0;
  blast.TargetRule = AbilityTargetRule::Hostile;
  blast.RequiredClass = Knight;
  blast.Prerequisites = {Strike};
  blast.Damage = PhysicalDamage(30.0);
  blast.DealsDamage = true;
  blast.EffectsOnTarget = {Burning};

  AbilityDefinition step;
  step.Id = GuardStep;
  step.ResourceCost = 5.0;
  step.CooldownSeconds = 1.0;
  step.CooldownGroup = MartialGroup;
  step.TargetRule = AbilityTargetRule::Self;

  AbilityDefinition wizard;
  wizard.Id = WizardOnly;
  wizard.TargetRule = AbilityTargetRule::None;
  wizard.RequiredClass = Wizard;

  AbilityDefinition atomic;
  atomic.Id = AtomicFailure;
  atomic.ResourceCost = 10.0;
  atomic.CooldownSeconds = 1.0;
  atomic.TargetRule = AbilityTargetRule::Hostile;
  atomic.Damage = PhysicalDamage(25.0);
  atomic.DealsDamage = true;
  atomic.EffectsOnTarget = {Burning};

  return {strike, blast, step, wizard, atomic};
}

struct Fixture final {
  Combat::Combatant Owner{Combat::CombatantId{100U},
                          Combat::CombatantKind::Player};
  Combat::Combatant Target{Combat::CombatantId{200U},
                           Combat::CombatantKind::Enemy};
  EffectRuntime OwnerEffects{Owner, EffectDefinitions()};
  EffectRuntime TargetEffects{Target, EffectDefinitions()};
  Core::DeterministicRandom Random{42U, 7U};
  AbilityRuntime Abilities{Owner, OwnerEffects, Random, Knight,
                           AbilityDefinitions()};
};

void UnlockCore(Fixture &fixture, TestSuite &suite) {
  suite.Expect(fixture.Abilities.Unlock(Strike), "base strike unlocks");
  suite.Expect(fixture.Abilities.Unlock(ChargedBlast),
               "dependent blast unlocks after prerequisite");
  suite.Expect(fixture.Abilities.Unlock(GuardStep), "guard step unlocks");
  suite.Expect(fixture.Abilities.Unlock(AtomicFailure),
               "atomic failure test ability unlocks");
}

void TestDefinitionGraphAndOwnership(TestSuite &suite) {
  Fixture fixture;
  suite.Expect(fixture.Abilities.IsValid(),
               "representative ability graph is valid");
  suite.Expect(!fixture.Abilities.Unlock(ChargedBlast),
               "unlock rejects missing prerequisite");
  suite.Expect(fixture.Abilities.Unlock(Strike),
               "prerequisite unlock succeeds");
  suite.Expect(fixture.Abilities.Unlock(ChargedBlast),
               "dependent unlock succeeds after prerequisite");
  suite.Expect(!fixture.Abilities.Revoke(Strike),
               "dependency-safe revoke protects prerequisite");
  suite.Expect(!fixture.Abilities.Unlock(WizardOnly),
               "class restriction rejects wrong class");

  AbilityDefinition missing;
  missing.Id = {30U};
  missing.Prerequisites = {{999U}};
  missing.TargetRule = AbilityTargetRule::None;
  Core::DeterministicRandom missingRandom{1U, 1U};
  AbilityRuntime missingRuntime{
      fixture.Owner, fixture.OwnerEffects, missingRandom, Knight, {missing}};
  suite.Expect(!missingRuntime.IsValid(),
               "definition graph rejects missing prerequisite IDs");

  AbilityDefinition first;
  first.Id = {31U};
  first.Prerequisites = {{32U}};
  AbilityDefinition second;
  second.Id = {32U};
  second.Prerequisites = {{31U}};
  Core::DeterministicRandom cycleRandom{2U, 1U};
  AbilityRuntime cycleRuntime{fixture.Owner,
                              fixture.OwnerEffects,
                              cycleRandom,
                              Knight,
                              {first, second}};
  suite.Expect(!cycleRuntime.IsValid(), "definition graph rejects cycles");

  Combat::Combatant otherOwner{Combat::CombatantId{300U},
                               Combat::CombatantKind::Player};
  EffectRuntime otherEffects{otherOwner, EffectDefinitions()};
  Core::DeterministicRandom mismatchRandom{3U, 1U};
  AbilityRuntime mismatchedEffects{fixture.Owner, otherEffects, mismatchRandom,
                                   Knight, AbilityDefinitions()};
  suite.Expect(!mismatchedEffects.IsValid(),
               "ability runtime rejects effects owned by another combatant");
}

void TestActivationDamageCostCooldownAndEffects(TestSuite &suite) {
  Fixture fixture;
  UnlockCore(fixture, suite);
  const AbilityTarget hostile{&fixture.Target, &fixture.TargetEffects,
                              TargetRelation::Hostile};
  const double resourceBefore = fixture.Owner.Resource().Current();
  const auto blast = fixture.Abilities.Activate(ChargedBlast, hostile);
  suite.Expect(blast.Result == AbilityActivationResult::Success,
               "charged blast activates successfully");
  suite.Expect(blast.DamageResolved,
               "ability resolves DamageSpec through combat");
  suite.ExpectNear(fixture.Target.Health().Current(), 70.0,
                   "ability damage reaches target health");
  suite.ExpectNear(fixture.Owner.Resource().Current(), resourceBefore - 20.0,
                   "resource cost commits exactly once");
  suite.Expect(fixture.TargetEffects.HasEffect(Burning),
               "ability applies configured target effect");

  suite.Expect(fixture.Abilities.Activate(ChargedBlast, hostile).Result ==
                   AbilityActivationResult::OnCooldown,
               "per-ability cooldown blocks repeat activation");
  const AbilityTarget self{&fixture.Owner, &fixture.OwnerEffects,
                           TargetRelation::Self};
  suite.Expect(fixture.Abilities.Activate(GuardStep, self).Result ==
                   AbilityActivationResult::OnCooldown,
               "cooldown group blocks sibling ability");

  suite.Expect(fixture.Abilities.AdvanceTime(2.0),
               "cooldowns advance deterministically");
  suite.Expect(fixture.Abilities.Activate(ChargedBlast, hostile).Result ==
                   AbilityActivationResult::Success,
               "second charge activates after cooldown");
  suite.Expect(fixture.Abilities.AdvanceTime(2.0), "cooldown clears again");
  suite.Expect(fixture.Abilities.Activate(ChargedBlast, hostile).Result ==
                   AbilityActivationResult::NoCharges,
               "depleted charges are explicit");
  suite.Expect(fixture.Abilities.AdvanceTime(1.0),
               "recharge reaches first charge across activations");
  suite.Expect(fixture.Abilities.Activate(ChargedBlast, hostile).Result ==
                   AbilityActivationResult::Success,
               "recharged ability can activate again");
}

void TestTargetResourceAndRestrictionFailures(TestSuite &suite) {
  Fixture fixture;
  UnlockCore(fixture, suite);
  const double resourceBefore = fixture.Owner.Resource().Current();
  const AbilityTarget wrongRelation{&fixture.Target, &fixture.TargetEffects,
                                    TargetRelation::Friendly};
  suite.Expect(fixture.Abilities.Activate(Strike, wrongRelation).Result ==
                   AbilityActivationResult::InvalidTarget,
               "target relation is validated");
  suite.ExpectNear(fixture.Owner.Resource().Current(), resourceBefore,
                   "invalid target cannot consume resource");

  static_cast<void>(fixture.Owner.TryConsumeResource(95.0));
  const AbilityTarget hostile{&fixture.Target, &fixture.TargetEffects,
                              TargetRelation::Hostile};
  suite.Expect(fixture.Abilities.Activate(ChargedBlast, hostile).Result ==
                   AbilityActivationResult::InsufficientResource,
               "insufficient resource is explicit");

  static_cast<void>(fixture.Owner.RestoreResource(100.0));
  static_cast<void>(fixture.OwnerEffects.Apply(Stun, fixture.Target.Id()));
  suite.Expect(fixture.Abilities.Activate(Strike, hostile).Result ==
                   AbilityActivationResult::BlockedByEffect,
               "stun-like effect blocks ability activation");
}

void TestCaptureRestoreAndCorruption(TestSuite &suite) {
  Fixture fixture;
  UnlockCore(fixture, suite);
  const AbilityTarget hostile{&fixture.Target, &fixture.TargetEffects,
                              TargetRelation::Hostile};
  static_cast<void>(fixture.Abilities.Activate(ChargedBlast, hostile));
  const AbilityRuntimeState saved = fixture.Abilities.CaptureState();
  suite.Expect(fixture.Abilities.AdvanceTime(1.0),
               "ability state mutates after capture");
  suite.Expect(fixture.Abilities.RestoreState(saved),
               "valid ability state restores");

  AbilityRuntimeState corrupt = saved;
  for (auto &entry : corrupt.Abilities) {
    if (entry.Id == ChargedBlast) {
      entry.Charges = 99U;
    }
  }
  const AbilityRuntimeState before = fixture.Abilities.CaptureState();
  suite.Expect(!fixture.Abilities.RestoreState(corrupt),
               "corrupt charge state is rejected");
  const AbilityRuntimeState after = fixture.Abilities.CaptureState();
  suite.Expect(after.Abilities.size() == before.Abilities.size(),
               "rejected ability restore is transactional");
}

void TestAtomicFailureRollsBackEverything(TestSuite &suite) {
  Fixture fixture;
  UnlockCore(fixture, suite);

  EffectDefinition onlyStun;
  onlyStun.Id = Stun;
  onlyStun.StackGroup = {2U};
  onlyStun.Stacking = EffectStackingPolicy::RefreshDuration;
  onlyStun.DurationSeconds = 2.0;
  onlyStun.Restrictions = EffectRestriction::AbilityActivation;
  EffectRuntime incompatibleTargetEffects{fixture.Target, {onlyStun}};

  const auto ownerBefore = fixture.Owner.CaptureState();
  const auto targetBefore = fixture.Target.CaptureState();
  const auto abilityBefore = fixture.Abilities.CaptureState();
  const auto randomBefore = fixture.Random.CaptureState();
  const AbilityTarget hostile{&fixture.Target, &incompatibleTargetEffects,
                              TargetRelation::Hostile};
  const auto result = fixture.Abilities.Activate(AtomicFailure, hostile);
  suite.Expect(result.Result == AbilityActivationResult::InternalFailure,
               "effect resolution failure becomes internal activation failure");
  suite.ExpectNear(fixture.Owner.Resource().Current(),
                   ownerBefore.Resource.Current,
                   "internal failure rolls back resource cost");
  suite.ExpectNear(fixture.Target.Health().Current(),
                   targetBefore.Health.Current,
                   "internal failure rolls back resolved damage");
  suite.Expect(fixture.Random.CaptureState() == randomBefore,
               "internal failure restores authoritative RNG state");
  const auto abilityAfter = fixture.Abilities.CaptureState();
  suite.Expect(abilityAfter.Abilities.size() == abilityBefore.Abilities.size(),
               "internal failure restores ability runtime shape");
}

void TestRechargeAndRestoreBoundaries(TestSuite &suite) {
  Fixture fixture;
  UnlockCore(fixture, suite);
  const AbilityTarget hostile{&fixture.Target, &fixture.TargetEffects,
                              TargetRelation::Hostile};
  static_cast<void>(fixture.Abilities.Activate(ChargedBlast, hostile));
  static_cast<void>(fixture.Abilities.AdvanceTime(2.0));
  static_cast<void>(fixture.Abilities.Activate(ChargedBlast, hostile));
  suite.Expect(
      fixture.Abilities.AdvanceTime(20.0),
      "large deterministic advance processes multiple recharge intervals");
  const AbilityRuntimeState recharged = fixture.Abilities.CaptureState();
  const auto charged =
      std::find_if(recharged.Abilities.begin(), recharged.Abilities.end(),
                   [](const AbilityRuntimeEntryState &entry) {
                     return entry.Id == ChargedBlast;
                   });
  suite.Expect(charged != recharged.Abilities.end() && charged->Charges == 2U &&
                   charged->RechargeRemaining == 0.0,
               "large advance restores all charges without timer residue");

  AbilityRuntimeState invalidCooldown = recharged;
  for (auto &entry : invalidCooldown.Abilities) {
    if (entry.Id == Strike) {
      entry.CooldownRemaining = -1.0;
    }
  }
  suite.Expect(!fixture.Abilities.RestoreState(invalidCooldown),
               "restore rejects negative cooldown state");

  AbilityRuntimeState oversizedCooldown = recharged;
  for (auto &entry : oversizedCooldown.Abilities) {
    if (entry.Id == Strike) {
      entry.CooldownRemaining = 99.0;
    }
  }
  suite.Expect(!fixture.Abilities.RestoreState(oversizedCooldown),
               "restore rejects cooldown beyond authored duration");

  AbilityRuntimeState invalidRecharge = recharged;
  for (auto &entry : invalidRecharge.Abilities) {
    if (entry.Id == ChargedBlast) {
      entry.Charges = 1U;
      entry.RechargeRemaining = 99.0;
    }
  }
  suite.Expect(!fixture.Abilities.RestoreState(invalidRecharge),
               "restore rejects recharge timer beyond definition");
}

} // namespace

int main() {
  TestSuite suite{"abilities"};
  TestDefinitionGraphAndOwnership(suite);
  TestActivationDamageCostCooldownAndEffects(suite);
  TestTargetResourceAndRestrictionFailures(suite);
  TestCaptureRestoreAndCorruption(suite);
  TestAtomicFailureRollsBackEverything(suite);
  TestRechargeAndRestoreBoundaries(suite);
  return suite.Finish();
}
