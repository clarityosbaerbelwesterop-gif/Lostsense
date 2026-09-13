#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Stats/CombatAttributes.h"
#include "TestHarness.h"

#include <vector>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using Tests::TestSuite;

constexpr EffectId Burning{1U};
constexpr EffectId Regeneration{2U};
constexpr EffectId Stun{3U};
constexpr EffectId BattleFocus{4U};
constexpr EffectId FireWard{5U};
constexpr GameplayTagId FireTag{100U};

std::vector<EffectDefinition> Definitions() {
  EffectDefinition burning;
  burning.Id = Burning;
  burning.StackGroup = {1U};
  burning.Stacking = EffectStackingPolicy::StackMagnitude;
  burning.MaxStacks = 3U;
  burning.DurationSeconds = 6.0;
  burning.TickIntervalSeconds = 2.0;
  burning.DamagePerTick = 10.0;
  burning.Tags = {FireTag};

  EffectDefinition regeneration;
  regeneration.Id = Regeneration;
  regeneration.StackGroup = {2U};
  regeneration.Stacking = EffectStackingPolicy::Independent;
  regeneration.MaxStacks = 1U;
  regeneration.DurationSeconds = 4.0;
  regeneration.TickIntervalSeconds = 1.0;
  regeneration.HealingPerTick = 5.0;
  regeneration.Beneficial = true;

  EffectDefinition stun;
  stun.Id = Stun;
  stun.StackGroup = {3U};
  stun.Stacking = EffectStackingPolicy::RefreshDuration;
  stun.DurationSeconds = 3.0;
  stun.Restrictions = EffectRestriction::AbilityActivation;

  EffectDefinition focus;
  focus.Id = BattleFocus;
  focus.StackGroup = {4U};
  focus.Stacking = EffectStackingPolicy::StackMagnitude;
  focus.MaxStacks = 2U;
  focus.DurationSeconds = 5.0;
  focus.Beneficial = true;
  focus.AttributeModifiers = {{Stats::CombatAttributes::AttackPower,
                               Stats::ModifierOperation::Additive, 10.0}};

  EffectDefinition ward;
  ward.Id = FireWard;
  ward.StackGroup = {5U};
  ward.Stacking = EffectStackingPolicy::Replace;
  ward.MaxStacks = 1U;
  ward.Permanent = true;
  ward.Beneficial = true;
  ward.Cleansable = false;
  ward.Dispellable = false;
  ward.GrantedEffectImmunities = {Burning};
  ward.GrantedTagImmunities = {FireTag};

  return {burning, regeneration, stun, focus, ward};
}

void TestDotHotAndStacking(TestSuite &suite) {
  Combat::Combatant target{Combat::CombatantId{1U},
                           Combat::CombatantKind::Player};
  static_cast<void>(
      target.SetBaseAttribute(Stats::CombatAttributes::MaxHealth, 500.0));
  static_cast<void>(target.Heal(500.0));
  EffectRuntime effects{target, Definitions()};
  suite.Expect(effects.IsValid(), "representative effect catalog is valid");

  const auto first = effects.Apply(Burning, Combat::CombatantId{10U});
  suite.Expect(first.Result == EffectApplyResult::Applied,
               "burning applies as a new effect");
  suite.Expect(effects.AdvanceTime(2.0),
               "effect time advances deterministically");
  suite.ExpectNear(target.Health().Current(), 490.0,
                   "burning deals DOT damage");

  suite.Expect(effects.Apply(Burning, Combat::CombatantId{10U}).Result ==
                   EffectApplyResult::Stacked,
               "burning magnitude stacks");
  suite.Expect(effects.AdvanceTime(2.0), "stacked DOT advances");
  suite.ExpectNear(target.Health().Current(), 470.0,
                   "two stacks scale deterministic DOT magnitude");
  suite.Expect(effects.Apply(Burning, Combat::CombatantId{10U}).Result ==
                   EffectApplyResult::Stacked,
               "third burning stack is accepted");
  suite.Expect(effects.Apply(Burning, Combat::CombatantId{10U}).Result ==
                   EffectApplyResult::AtMaxStacks,
               "burning respects max stacks");

  static_cast<void>(target.ApplyDamage(50.0));
  suite.Expect(effects.Apply(Regeneration, Combat::CombatantId{1U}).Result ==
                   EffectApplyResult::Applied,
               "regeneration applies");
  suite.Expect(effects.AdvanceTime(1.0), "HOT tick advances");
  suite.ExpectNear(target.Health().Current(), 425.0,
                   "HOT and stacked DOT resolve in stable instance order");
}

void TestRefreshModifierLifecycleAndExpiration(TestSuite &suite) {
  Combat::Combatant target{Combat::CombatantId{2U},
                           Combat::CombatantKind::Player};
  EffectRuntime effects{target, Definitions()};
  suite.Expect(effects.Apply(BattleFocus, Combat::CombatantId{2U}).Result ==
                   EffectApplyResult::Applied,
               "attribute buff applies");
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   10.0, "effect modifier changes AttributeSet");
  const auto stacked = effects.Apply(BattleFocus, Combat::CombatantId{2U});
  suite.Expect(stacked.Result == EffectApplyResult::Stacked,
               "attribute effect stacks magnitude");
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   20.0, "stacking rebuilds exact modifier magnitude");
  suite.Expect(effects.Remove(stacked.InstanceId),
               "explicit removal succeeds");
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   0.0, "removal cleans exact modifier");

  const auto stun = effects.Apply(Stun, Combat::CombatantId{9U});
  suite.Expect(effects.HasRestriction(EffectRestriction::AbilityActivation),
               "stun restriction is active gameplay state");
  suite.Expect(effects.AdvanceTime(1.0), "stun duration advances");
  suite.Expect(effects.Apply(Stun, Combat::CombatantId{9U}).Result ==
                   EffectApplyResult::Refreshed,
               "refresh policy refreshes duration");
  suite.Expect(effects.AdvanceTime(2.5),
               "refreshed duration remains active");
  suite.Expect(effects.HasRestriction(EffectRestriction::AbilityActivation),
               "refreshed stun has not expired");
  suite.Expect(effects.AdvanceTime(0.6), "expiration boundary advances");
  suite.Expect(!effects.HasRestriction(EffectRestriction::AbilityActivation),
               "expired stun removes restriction");
  suite.Expect(!effects.Remove(stun.InstanceId),
               "expired effect is already removed");
}

void TestImmunityCleanseAndDispel(TestSuite &suite) {
  Combat::Combatant target{Combat::CombatantId{3U},
                           Combat::CombatantKind::Player};
  EffectRuntime effects{target, Definitions()};
  suite.Expect(effects.Apply(FireWard, Combat::CombatantId{3U}).Result ==
                   EffectApplyResult::Applied,
               "fire ward applies");
  suite.Expect(effects.Apply(Burning, Combat::CombatantId{4U}).Result ==
                   EffectApplyResult::Immune,
               "specific/tag immunity rejects burning");
  suite.Expect(effects.Cleanse({EffectPolarity::Beneficial, {}}) == 0U,
               "non-cleansable ward survives cleanse");

  suite.Expect(effects.Apply(Stun, Combat::CombatantId{4U}).Result ==
                   EffectApplyResult::Applied,
               "harmful stun applies beside ward");
  suite.Expect(effects.Cleanse({EffectPolarity::Harmful, {}}) == 1U,
               "cleanse removes eligible harmful effect");
  suite.Expect(effects.Apply(Regeneration, Combat::CombatantId{3U}).Result ==
                   EffectApplyResult::Applied,
               "beneficial regeneration applies");
  suite.Expect(effects.Dispel({EffectPolarity::Beneficial, {}}) == 1U,
               "dispel removes eligible beneficial effect but not ward");
}

void TestSnapshotRestoreAndCorruption(TestSuite &suite) {
  Combat::Combatant target{Combat::CombatantId{4U},
                           Combat::CombatantKind::Player};
  EffectRuntime effects{target, Definitions()};
  static_cast<void>(effects.Apply(BattleFocus, Combat::CombatantId{4U}));
  static_cast<void>(effects.Apply(BattleFocus, Combat::CombatantId{4U}));
  static_cast<void>(effects.Apply(Stun, Combat::CombatantId{8U}));
  static_cast<void>(effects.AdvanceTime(1.0));
  const EffectRuntimeState saved = effects.CaptureState();
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   20.0, "snapshot captures active modifier state");

  static_cast<void>(effects.Cleanse({EffectPolarity::Any, {}}));
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   0.0, "mutation removes captured modifier");
  suite.Expect(effects.RestoreState(saved), "valid effect state restores");
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   20.0, "restore recreates exact modifier state");
  suite.Expect(effects.HasRestriction(EffectRestriction::AbilityActivation),
               "restore recreates restrictions");

  EffectRuntimeState corrupt = saved;
  corrupt.ActiveEffects.front().Stacks = 999U;
  const auto beforeRejected = effects.CaptureState();
  suite.Expect(!effects.RestoreState(corrupt),
               "corrupt stack count is rejected transactionally");
  suite.Expect(effects.CaptureState().ActiveEffects.size() ==
                   beforeRejected.ActiveEffects.size(),
               "rejected restore leaves active effect count unchanged");
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   20.0, "rejected restore leaves modifiers unchanged");
}

void TestDeterministicReplay(TestSuite &suite) {
  Combat::Combatant first{Combat::CombatantId{5U},
                          Combat::CombatantKind::Player};
  Combat::Combatant second{Combat::CombatantId{5U},
                           Combat::CombatantKind::Player};
  EffectRuntime firstEffects{first, Definitions()};
  EffectRuntime secondEffects{second, Definitions()};
  static_cast<void>(firstEffects.Apply(Burning, Combat::CombatantId{11U}));
  static_cast<void>(secondEffects.Apply(Burning, Combat::CombatantId{11U}));
  static_cast<void>(firstEffects.Apply(Burning, Combat::CombatantId{11U}));
  static_cast<void>(secondEffects.Apply(Burning, Combat::CombatantId{11U}));
  suite.Expect(firstEffects.AdvanceTime(5.0) && secondEffects.AdvanceTime(5.0),
               "matching effect simulations advance");
  suite.ExpectNear(first.Health().Current(), second.Health().Current(),
                   "matching effect simulations reproduce health");
  const auto firstState = firstEffects.CaptureState();
  const auto secondState = secondEffects.CaptureState();
  suite.Expect(firstState.NextInstanceValue == secondState.NextInstanceValue &&
                   firstState.ActiveEffects.size() ==
                       secondState.ActiveEffects.size(),
               "matching effect simulations reproduce runtime shape");
  suite.ExpectNear(firstState.ActiveEffects.front().RemainingSeconds,
                   secondState.ActiveEffects.front().RemainingSeconds,
                   "matching effect simulations reproduce timers");
}

void TestDeathAndRestoreValidation(TestSuite &suite) {
  Combat::Combatant target{Combat::CombatantId{6U},
                           Combat::CombatantKind::Player};
  static_cast<void>(
      target.SetBaseAttribute(Stats::CombatAttributes::MaxHealth, 15.0));
  EffectRuntime effects{target, Definitions()};
  suite.Expect(effects.Apply(Burning, Combat::CombatantId{12U}).Result ==
                   EffectApplyResult::Applied,
               "lethal DOT applies");
  suite.Expect(effects.AdvanceTime(4.0), "lethal DOT advances");
  suite.Expect(target.Health().IsDead(), "DOT can produce a death transition");

  Combat::Combatant restoredTarget{Combat::CombatantId{7U},
                                   Combat::CombatantKind::Player};
  EffectRuntime restored{restoredTarget, Definitions()};
  static_cast<void>(restored.Apply(Burning, Combat::CombatantId{12U}));
  EffectRuntimeState saved = restored.CaptureState();

  EffectRuntimeState unknownDefinition = saved;
  unknownDefinition.ActiveEffects.front().DefinitionId = EffectId{999U};
  suite.Expect(!restored.RestoreState(unknownDefinition),
               "restore rejects unknown effect definitions");

  EffectRuntimeState invalidDuration = saved;
  invalidDuration.ActiveEffects.front().RemainingSeconds = 99.0;
  suite.Expect(!restored.RestoreState(invalidDuration),
               "restore rejects impossible remaining duration");

  EffectRuntimeState invalidTick = saved;
  invalidTick.ActiveEffects.front().TimeUntilNextTick = 99.0;
  suite.Expect(!restored.RestoreState(invalidTick),
               "restore rejects impossible tick timer");

  EffectRuntimeState expired = saved;
  expired.ActiveEffects.front().RemainingSeconds = 0.0;
  suite.Expect(!restored.RestoreState(expired),
               "restore rejects already-expired active state");

  EffectRuntimeState invalidSource = saved;
  invalidSource.ActiveEffects.front().SourceId = Combat::CombatantId{};
  suite.Expect(!restored.RestoreState(invalidSource),
               "restore rejects invalid effect source identity");
}

void TestModifierCollisionAndReplacePolicy(TestSuite &suite) {
  Combat::Combatant target{Combat::CombatantId{8U},
                           Combat::CombatantKind::Player};
  const Stats::ModifierId reservedStatusId{(1ULL << 63U) | (1ULL << 16U) |
                                           1ULL};
  suite.Expect(target.AddAttributeModifier(
                   {reservedStatusId, Stats::CombatAttributes::AttackPower,
                    Stats::ModifierOperation::Additive,
                    Stats::ModifierSource::System, 3.0}),
               "test reserves the first deterministic status modifier ID");
  EffectRuntime effects{target, Definitions()};
  const auto failed = effects.Apply(BattleFocus, Combat::CombatantId{8U});
  suite.Expect(failed.Result == EffectApplyResult::InternalFailure,
               "modifier ownership collision rejects effect atomically");
  suite.ExpectNear(target.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   3.0, "failed effect leaves foreign modifier unchanged");
  suite.Expect(effects.ActiveCount() == 0U,
               "failed effect does not leak active runtime state");

  Combat::Combatant wardTarget{Combat::CombatantId{9U},
                               Combat::CombatantKind::Player};
  EffectRuntime wardEffects{wardTarget, Definitions()};
  const auto first = wardEffects.Apply(FireWard, wardTarget.Id());
  const auto second = wardEffects.Apply(FireWard, wardTarget.Id());
  suite.Expect(first.Result == EffectApplyResult::Applied &&
                   second.Result == EffectApplyResult::Replaced,
               "replace policy creates one fresh authoritative instance");
  suite.Expect(wardEffects.ActiveCount() == 1U,
               "replace policy leaves exactly one active instance");
  suite.Expect(second.InstanceId != first.InstanceId,
               "replace policy advances deterministic instance identity");
}

} // namespace

int main() {
  TestSuite suite{"gameplay effects"};
  TestDotHotAndStacking(suite);
  TestRefreshModifierLifecycleAndExpiration(suite);
  TestImmunityCleanseAndDispel(suite);
  TestSnapshotRestoreAndCorruption(suite);
  TestDeterministicReplay(suite);
  TestDeathAndRestoreValidation(suite);
  TestModifierCollisionAndReplacePolicy(suite);
  return suite.Finish();
}
