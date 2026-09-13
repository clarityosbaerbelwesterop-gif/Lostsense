#include "Lostsense/Combat/VitalPools.h"
#include "TestHarness.h"

#include <limits>

namespace {

using namespace Lostsense::Combat;
using Lostsense::Tests::TestSuite;

void TestDamageHealingAndDeath(TestSuite &suite) {
  HealthPool health{100.0};
  const DamageApplication first = health.ApplyDamage(30.0);
  suite.Expect(first.WasValid && !first.BecameDead,
               "ordinary damage is valid and non-lethal");
  suite.ExpectNear(first.Applied, 30.0, "ordinary damage is fully applied");
  suite.ExpectNear(health.Current(), 70.0, "damage reduces current health");

  const HealingApplication healed = health.Heal(50.0);
  suite.ExpectNear(healed.Applied, 30.0, "healing fills only missing health");
  suite.ExpectNear(healed.Overheal, 20.0, "excess healing is reported");
  suite.ExpectNear(health.Current(), 100.0, "healing cannot exceed maximum");

  const DamageApplication lethal = health.ApplyDamage(150.0);
  suite.Expect(lethal.BecameDead && health.IsDead(),
               "crossing zero emits one death transition");
  suite.ExpectNear(lethal.Applied, 100.0,
                   "lethal hit applies remaining health");
  suite.ExpectNear(lethal.Overkill, 50.0,
                   "lethal excess is reported as overkill");

  const DamageApplication repeated = health.ApplyDamage(10.0);
  suite.Expect(repeated.WasAlreadyDead && !repeated.BecameDead,
               "repeated damage cannot emit another death transition");
  suite.ExpectNear(repeated.Applied, 0.0,
                   "dead health cannot be reduced again");
  suite.ExpectNear(repeated.Overkill, 10.0,
                   "damage against a dead target remains observable");

  const HealingApplication deadHeal = health.Heal(40.0);
  suite.Expect(deadHeal.WasDead && deadHeal.Applied == 0.0,
               "ordinary healing cannot implicitly resurrect");
  suite.Expect(health.Revive(25.0),
               "explicit revive succeeds with positive health");
  suite.ExpectNear(health.Current(), 25.0, "revive restores requested health");
  suite.Expect(!health.Revive(25.0),
               "living target cannot be revived repeatedly");
}

void TestHealthBoundsAndInvalidValues(TestSuite &suite) {
  HealthPool health{-10.0};
  suite.Expect(health.IsDead() && health.Maximum() == 0.0,
               "invalid constructor maximum produces a safe empty pool");
  suite.Expect(!health.SetMaximum(std::numeric_limits<double>::infinity()),
               "invalid maximum is rejected without mutation");
  suite.Expect(health.SetMaximum(100.0), "maximum can be established later");
  suite.Expect(health.IsDead(), "increasing zero maximum does not resurrect");
  suite.Expect(health.Revive(200.0), "revive clamps to maximum health");
  suite.ExpectNear(health.Current(), 100.0, "revive cannot overflow health");
  suite.Expect(health.SetMaximum(20.0), "maximum can be reduced");
  suite.ExpectNear(health.Current(), 20.0,
                   "maximum reduction clamps current health");
  suite.Expect(health.SetMaximum(0.0) && health.IsDead(),
               "zero maximum creates a deterministic death state");

  const double before = health.Current();
  suite.Expect(!health.ApplyDamage(-1.0).WasValid,
               "negative damage is rejected");
  suite.Expect(!health.Heal(std::numeric_limits<double>::quiet_NaN()).WasValid,
               "non-finite healing is rejected");
  suite.ExpectNear(health.Current(), before,
                   "invalid changes leave current health untouched");
}

void TestHealthStateValidation(TestSuite &suite) {
  HealthPool health{100.0};
  static_cast<void>(health.ApplyDamage(65.0));
  const HealthState saved = health.CaptureState();
  static_cast<void>(health.ApplyDamage(35.0));
  suite.Expect(health.RestoreState(saved), "valid health state restores");
  suite.ExpectNear(health.Current(), 35.0,
                   "health state restores current value");
  suite.Expect(!health.RestoreState({100.0, 25.0, true}),
               "dead flag inconsistent with health is rejected");
  suite.ExpectNear(health.Current(), 35.0,
                   "rejected health state is transactional");
  suite.Expect(!health.RestoreState({10.0, 11.0, false}),
               "current health above maximum is rejected");
}

void TestResourceConsumptionAndRestoration(TestSuite &suite) {
  ResourcePool resource{50.0};
  const ResourceConsumption spent = resource.TryConsume(20.0);
  suite.Expect(spent.WasValid && spent.Succeeded,
               "affordable resource cost succeeds");
  suite.ExpectNear(resource.Current(), 30.0, "resource cost is deducted");

  const ResourceConsumption insufficient = resource.TryConsume(40.0);
  suite.Expect(insufficient.WasValid && !insufficient.Succeeded,
               "unaffordable cost fails atomically");
  suite.ExpectNear(resource.Current(), 30.0,
                   "failed cost cannot underflow resource");
  suite.Expect(!resource.TryConsume(-1.0).WasValid,
               "negative resource cost is rejected");

  const ResourceRestoration restored = resource.Restore(50.0);
  suite.ExpectNear(restored.Restored, 20.0, "restore fills available capacity");
  suite.ExpectNear(restored.Overflow, 30.0, "restore reports overflow");
  suite.ExpectNear(resource.Current(), 50.0, "resource cannot exceed maximum");
  suite.Expect(resource.SetMaximum(10.0), "resource maximum can be reduced");
  suite.ExpectNear(resource.Current(), 10.0,
                   "maximum reduction clamps current resource");
}

void TestResourceStateValidation(TestSuite &suite) {
  ResourcePool resource{80.0};
  static_cast<void>(resource.TryConsume(30.0));
  const ResourceState saved = resource.CaptureState();
  static_cast<void>(resource.TryConsume(10.0));
  suite.Expect(resource.RestoreState(saved), "valid resource state restores");
  suite.ExpectNear(resource.Current(), 50.0,
                   "resource state restores current value");
  suite.Expect(!resource.RestoreState({20.0, 21.0}),
               "resource state above maximum is rejected");
  suite.Expect(
      !resource.RestoreState({std::numeric_limits<double>::quiet_NaN(), 0.0}),
      "non-finite resource state is rejected");
  suite.ExpectNear(resource.Current(), 50.0,
                   "rejected resource state leaves pool unchanged");
}

} // namespace

int main() {
  TestSuite suite{"vital pools"};
  TestDamageHealingAndDeath(suite);
  TestHealthBoundsAndInvalidValues(suite);
  TestHealthStateValidation(suite);
  TestResourceConsumptionAndRestoration(suite);
  TestResourceStateValidation(suite);
  return suite.Finish();
}
