#include "Lostsense/Stats/AttributeSet.h"
#include "Lostsense/Stats/CombatAttributes.h"
#include "TestHarness.h"

#include <limits>

namespace {

using namespace Lostsense::Stats;
using Lostsense::Tests::TestSuite;

constexpr AttributeId TestAttribute{9000U};
constexpr AttributeId UnknownAttribute{9001U};

AttributeSet MakeTestSet() {
  AttributeSet attributes;
  static_cast<void>(
      attributes.Define({TestAttribute, 10.0, -100.0, 100.0}));
  return attributes;
}

void TestDefinitionValidation(TestSuite &suite) {
  AttributeSet attributes;
  suite.Expect(!attributes.Define({AttributeId{}, 1.0, 0.0, 2.0}),
               "zero attribute ID is reserved and rejected");
  suite.Expect(!attributes.Define(
                   {TestAttribute, std::numeric_limits<double>::quiet_NaN(),
                    0.0, 2.0}),
               "non-finite default is rejected");
  suite.Expect(!attributes.Define({TestAttribute, 1.0, 2.0, 1.0}),
               "inverted bounds are rejected");
  suite.Expect(!attributes.Define({TestAttribute, 200.0, 0.0, 100.0}),
               "default outside declared bounds is rejected");
  suite.Expect(attributes.Define({TestAttribute, 50.0, 0.0, 100.0}),
               "valid definition is accepted");
  suite.ExpectNear(attributes.GetBase(TestAttribute), 50.0,
                   "valid default base is preserved");
  suite.Expect(!attributes.Define({TestAttribute, 1.0, 0.0, 100.0}),
               "duplicate attribute definition is rejected");
  suite.ExpectNear(attributes.Get(UnknownAttribute), 0.0,
                   "undefined attribute reads safely as zero");
}

void TestModifierEvaluation(TestSuite &suite) {
  AttributeSet first = MakeTestSet();
  AttributeSet second = MakeTestSet();
  const AttributeModifier flatOne{ModifierId{20U}, TestAttribute,
                                  ModifierOperation::Additive,
                                  ModifierSource::Equipment, 5.0};
  const AttributeModifier flatTwo{ModifierId{10U}, TestAttribute,
                                  ModifierOperation::Additive,
                                  ModifierSource::SkillTree, -3.0};
  const AttributeModifier multiplier{ModifierId{30U}, TestAttribute,
                                    ModifierOperation::Multiplicative,
                                    ModifierSource::StatusEffect, 2.0};

  suite.Expect(first.SetBase(TestAttribute, 20.0), "base value can be set");
  suite.Expect(second.SetBase(TestAttribute, 20.0), "second base can be set");
  suite.Expect(first.AddModifier(multiplier), "multiplier can be added");
  suite.Expect(first.AddModifier(flatOne), "first flat modifier can be added");
  suite.Expect(first.AddModifier(flatTwo), "second flat modifier can be added");
  suite.Expect(second.AddModifier(flatTwo), "reverse first modifier is added");
  suite.Expect(second.AddModifier(flatOne), "reverse second modifier is added");
  suite.Expect(second.AddModifier(multiplier),
               "reverse multiplier modifier is added");

  suite.ExpectNear(first.Get(TestAttribute), 44.0,
                   "flat modifiers apply before multiplicative factors");
  suite.ExpectNear(second.Get(TestAttribute), first.Get(TestAttribute),
                   "modifier insertion order cannot change the result");
  suite.Expect(!first.AddModifier(flatOne),
               "duplicate stable modifier ID is rejected");
  suite.Expect(!first.AddModifier({ModifierId{}, TestAttribute,
                                   ModifierOperation::Additive,
                                   ModifierSource::System, 1.0}),
               "zero modifier ID is rejected");
  suite.Expect(!first.AddModifier({ModifierId{99U}, UnknownAttribute,
                                   ModifierOperation::Additive,
                                   ModifierSource::System, 1.0}),
               "modifier for undefined attribute is rejected");
  suite.Expect(!first.AddModifier({ModifierId{100U}, TestAttribute,
                                   ModifierOperation::Multiplicative,
                                   ModifierSource::System, -1.0}),
               "negative multiplicative factor is rejected");
  suite.Expect(!first.AddModifier(
                   {ModifierId{101U}, TestAttribute,
                    static_cast<ModifierOperation>(255U),
                    ModifierSource::System, 1.0}),
               "unknown serialized modifier operation is rejected");
  suite.Expect(!first.AddModifier(
                   {ModifierId{102U}, TestAttribute,
                    ModifierOperation::Additive,
                    static_cast<ModifierSource>(255U), 1.0}),
               "unknown serialized modifier source is rejected");
}

void TestBoundsAndSourceRemoval(TestSuite &suite) {
  AttributeSet attributes = MakeTestSet();
  suite.Expect(attributes.SetBase(TestAttribute, 99.0),
               "bounded base can be assigned");
  suite.Expect(attributes.AddModifier(
                   {ModifierId{1U}, TestAttribute,
                    ModifierOperation::Additive, ModifierSource::Equipment,
                    1000.0}),
               "large finite modifier remains valid");
  suite.ExpectNear(attributes.Get(TestAttribute), 100.0,
                   "evaluated value obeys its upper bound");
  suite.Expect(attributes.AddModifier(
                   {ModifierId{2U}, TestAttribute,
                    ModifierOperation::Multiplicative,
                    ModifierSource::Temporary, 0.0}),
               "zero factor can suppress an attribute temporarily");
  suite.ExpectNear(attributes.Get(TestAttribute), 0.0,
                   "zero multiplier is applied safely");
  suite.Expect(attributes.AddModifier(
                   {ModifierId{3U}, TestAttribute,
                    ModifierOperation::Additive, ModifierSource::Equipment,
                    -4.0}),
               "second equipment modifier can be added");
  suite.Expect(attributes.RemoveModifiersBySource(ModifierSource::Equipment) ==
                   2U,
               "all modifiers from one source category are removed");
  suite.Expect(attributes.ModifierCount() == 1U,
               "unrelated modifier remains after source removal");
  suite.Expect(attributes.RemoveModifier(ModifierId{2U}),
               "modifier can be removed by stable ID");
  suite.ExpectNear(attributes.Get(TestAttribute), 99.0,
                   "removing modifiers restores the base value");
}

void TestStateRoundTripIsTransactional(TestSuite &suite) {
  AttributeSet attributes = MakeTestSet();
  static_cast<void>(attributes.SetBase(TestAttribute, 12.0));
  static_cast<void>(attributes.AddModifier(
      {ModifierId{7U}, TestAttribute, ModifierOperation::Additive,
       ModifierSource::StatusEffect, 8.0}));
  const AttributeSetState saved = attributes.CaptureState();

  static_cast<void>(attributes.SetBase(TestAttribute, 80.0));
  static_cast<void>(attributes.RemoveModifier(ModifierId{7U}));
  suite.Expect(attributes.RestoreState(saved),
               "valid attribute state round-trips");
  suite.ExpectNear(attributes.Get(TestAttribute), 20.0,
                   "round-trip restores base and modifiers");

  AttributeSetState corrupt = saved;
  corrupt.BaseValues.push_back({UnknownAttribute, 55.0});
  suite.Expect(!attributes.RestoreState(corrupt),
               "unknown serialized attribute is rejected");
  suite.ExpectNear(attributes.Get(TestAttribute), 20.0,
                   "rejected state leaves live attributes unchanged");

  corrupt = saved;
  corrupt.BaseValues.clear();
  suite.Expect(!attributes.RestoreState(corrupt),
               "truncated serialized base state is rejected");
  suite.ExpectNear(attributes.Get(TestAttribute), 20.0,
                   "truncated state rejection is transactional");

  corrupt = saved;
  corrupt.BaseValues.front().Value = 1000.0;
  suite.Expect(!attributes.RestoreState(corrupt),
               "out-of-range serialized base is rejected rather than clamped");
}

void TestCoreCombatDefinitions(TestSuite &suite) {
  const AttributeSet attributes = CombatAttributes::CreateDefaultSet();
  suite.ExpectNear(attributes.Get(CombatAttributes::MaxHealth), 100.0,
                   "core health has a safe default");
  suite.ExpectNear(attributes.Get(CombatAttributes::CriticalMultiplier), 1.5,
                   "core critical multiplier has a useful default");
  suite.ExpectNear(attributes.Get(CombatAttributes::MovementSpeedMultiplier),
                   1.0, "movement multiplier defaults to neutral");
  suite.Expect(CombatAttributes::DamageBonus(
                   Lostsense::Combat::DamageType::Physical) !=
                   CombatAttributes::DamageBonus(
                       Lostsense::Combat::DamageType::Fire),
               "damage types map to distinct stable attributes");
  suite.Expect(!CombatAttributes::Resistance(
                    Lostsense::Combat::DamageType::Count)
                    .IsValid(),
               "sentinel damage type cannot create an attribute ID");
}

} // namespace

int main() {
  TestSuite suite{"attribute set"};
  TestDefinitionValidation(suite);
  TestModifierEvaluation(suite);
  TestBoundsAndSourceRemoval(suite);
  TestStateRoundTripIsTransactional(suite);
  TestCoreCombatDefinitions(suite);
  return suite.Finish();
}
