#include "Lostsense/Gameplay/Progression/SkillGraph.h"
#include "Lostsense/Stats/CombatAttributes.h"
#include "TestHarness.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using Tests::TestSuite;

constexpr ClassId Knight{1U};
constexpr ClassId Wizard{2U};
constexpr AbilityId RootAbility{200U};
constexpr AbilityId TransformAbility{201U};
constexpr SkillTreeId KnightTree{1U};
constexpr SkillNodeId RootNode{1U};
constexpr SkillNodeId TransformNode{2U};
constexpr SkillNodeId ArmorNode{3U};
constexpr SkillNodeId AlternateNode{4U};
constexpr SkillNodeId WizardNode{5U};
constexpr SkillExclusiveGroupId BranchChoice{1U};
constexpr AbilityMutationId TransformMutation{1U};
constexpr AbilityMutationId AlternateMutation{2U};
constexpr GameplayTagId TransformTag{100U};

std::vector<AbilityDefinition> AbilityDefinitions() {
  AbilityDefinition root;
  root.Id = RootAbility;
  root.RequiredClass = Knight;
  root.AllowedLoadoutSlots = AbilityLoadoutSlotBit(AbilityLoadoutSlot::Active1);

  AbilityDefinition transform;
  transform.Id = TransformAbility;
  transform.RequiredClass = Knight;
  transform.Prerequisites = {RootAbility};
  transform.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::Active2);
  return {root, transform};
}

SkillTreeDefinition RepresentativeTree() {
  SkillTreeDefinition tree;
  tree.Id = KnightTree;
  tree.SupportedClasses = {Knight, Wizard};
  tree.ExclusiveGroups = {{BranchChoice, 1U}};

  SkillNodeDefinition root;
  root.Id = RootNode;
  root.Category = SkillNodeCategory::Minor;
  root.PointCost = 1U;
  root.Connections = {TransformNode, ArmorNode, AlternateNode, WizardNode};
  root.AttributeModifiers = {{Stats::CombatAttributes::AttackPower,
                              Stats::ModifierOperation::Additive, 10.0}};
  root.UnlockAbility = RootAbility;

  SkillNodeDefinition transform;
  transform.Id = TransformNode;
  transform.Category = SkillNodeCategory::Transformation;
  transform.PointCost = 2U;
  transform.Prerequisites = {RootNode};
  transform.Connections = {RootNode};
  transform.UnlockAbility = TransformAbility;
  transform.AbilityMutations = {TransformMutation};
  transform.Tags = {TransformTag};

  SkillNodeDefinition armor;
  armor.Id = ArmorNode;
  armor.Category = SkillNodeCategory::Major;
  armor.PointCost = 1U;
  armor.Prerequisites = {RootNode};
  armor.Connections = {RootNode};
  armor.ExclusiveGroup = BranchChoice;
  armor.AttributeModifiers = {{Stats::CombatAttributes::Armor,
                               Stats::ModifierOperation::Additive, 5.0}};

  SkillNodeDefinition alternate;
  alternate.Id = AlternateNode;
  alternate.Category = SkillNodeCategory::Keystone;
  alternate.PointCost = 1U;
  alternate.Prerequisites = {RootNode};
  alternate.Connections = {RootNode};
  alternate.ExclusiveGroup = BranchChoice;
  alternate.AbilityMutations = {AlternateMutation};

  SkillNodeDefinition wizard;
  wizard.Id = WizardNode;
  wizard.Category = SkillNodeCategory::ClassMechanic;
  wizard.PointCost = 1U;
  wizard.RequiredClass = Wizard;
  wizard.Prerequisites = {RootNode};
  wizard.Connections = {RootNode};

  tree.Nodes = {root, transform, armor, alternate, wizard};
  return tree;
}

struct Fixture final {
  Combat::Combatant Owner{Combat::CombatantId{1U},
                          Combat::CombatantKind::Player};
  EffectRuntime Effects{Owner, std::vector<EffectDefinition>{}};
  Core::DeterministicRandom Random{42U, 9U};
  AbilityRuntime Abilities{Owner, Effects, Random, Knight,
                           AbilityDefinitions()};
  AbilityLoadout Loadout{Abilities};
};

void TestAllocationRefundAndIntegration(TestSuite &suite) {
  Fixture fixture;
  SkillTreeRuntime tree{fixture.Owner, fixture.Abilities, fixture.Loadout,
                        RepresentativeTree(), 5U};
  suite.Expect(tree.IsValid(), "representative skill graph validates");
  suite.Expect(tree.TreeId() == KnightTree, "tree keeps stable tree ID");
  suite.Expect(tree.TotalPoints() == 5U && tree.UnspentPoints() == 5U,
               "initial skill point budget is explicit");

  suite.Expect(tree.Allocate(TransformNode) ==
                   SkillOperationResult::MissingPrerequisite,
               "allocation rejects missing prerequisite");
  suite.Expect(tree.Allocate(RootNode) == SkillOperationResult::Success,
               "root node allocates");
  suite.ExpectNear(
      fixture.Owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
      10.0, "skill modifier uses existing AttributeSet authority");
  suite.Expect(fixture.Abilities.IsUnlocked(RootAbility),
               "skill node unlocks through AbilityRuntime authority");
  suite.Expect(tree.Allocate(TransformNode) == SkillOperationResult::Success,
               "dependent transformation allocates");
  suite.Expect(fixture.Abilities.IsUnlocked(TransformAbility),
               "transformation unlocks dependent ability");
  suite.Expect(tree.HasMutation(TransformMutation),
               "transformation exposes generic ability mutation hook");
  suite.Expect(tree.HasTag(TransformTag),
               "allocated node contributes stable build tag");
  suite.Expect(tree.UnspentPoints() == 2U,
               "allocation spends exact authored point costs");

  suite.Expect(tree.Refund(RootNode) == SkillOperationResult::DependencyActive,
               "refund cannot break an allocated dependency");
  suite.Expect(
      fixture.Loadout.Equip(AbilityLoadoutSlot::Active2, TransformAbility) ==
          LoadoutResult::Success,
      "tree-unlocked ability enters portable loadout");
  suite.Expect(tree.Refund(TransformNode) ==
                   SkillOperationResult::AbilityEquipped,
               "refund cannot revoke an equipped ability");
  suite.Expect(fixture.Loadout.Unequip(AbilityLoadoutSlot::Active2),
               "ability can be unequipped before refund");
  suite.Expect(tree.Refund(TransformNode) == SkillOperationResult::Success,
               "safe dependent refund succeeds");
  suite.Expect(!fixture.Abilities.IsUnlocked(TransformAbility),
               "refund revokes through AbilityRuntime authority");
  suite.Expect(!tree.HasMutation(TransformMutation) &&
                   !tree.HasTag(TransformTag),
               "refund removes derived transformation hooks exactly");

  suite.Expect(tree.Refund(RootNode) == SkillOperationResult::Success,
               "root refunds after dependents are gone");
  suite.ExpectNear(
      fixture.Owner.Attributes().Get(Stats::CombatAttributes::AttackPower), 0.0,
      "refund removes exact skill-owned AttributeSet modifier");
  suite.Expect(!fixture.Abilities.IsUnlocked(RootAbility),
               "root ability ownership is revoked safely");
  suite.Expect(tree.UnspentPoints() == 5U,
               "refund restores exact point budget");
}

void TestPointsExclusiveGroupsAndClassRules(TestSuite &suite) {
  Fixture limitedFixture;
  SkillTreeRuntime limited{limitedFixture.Owner, limitedFixture.Abilities,
                           limitedFixture.Loadout, RepresentativeTree(), 1U};
  suite.Expect(limited.Allocate(RootNode) == SkillOperationResult::Success,
               "limited tree can buy root");
  suite.Expect(limited.Allocate(TransformNode) ==
                   SkillOperationResult::InsufficientPoints,
               "insufficient points fail without partial allocation");
  suite.Expect(!limited.IsAllocated(TransformNode) &&
                   !limitedFixture.Abilities.IsUnlocked(TransformAbility),
               "failed allocation leaves ability and graph unchanged");
  suite.Expect(limited.GrantPoints(2U),
               "points can be granted deterministically");
  suite.Expect(limited.Allocate(TransformNode) == SkillOperationResult::Success,
               "granted points enable later allocation");
  suite.Expect(!limited.GrantPoints(0U), "zero point grant is rejected");

  Fixture branchFixture;
  SkillTreeRuntime branches{branchFixture.Owner, branchFixture.Abilities,
                            branchFixture.Loadout, RepresentativeTree(), 4U};
  suite.Expect(branches.Allocate(RootNode) == SkillOperationResult::Success,
               "branch fixture allocates root");
  suite.Expect(branches.Allocate(ArmorNode) == SkillOperationResult::Success,
               "first exclusive branch allocates");
  suite.ExpectNear(
      branchFixture.Owner.Attributes().Get(Stats::CombatAttributes::Armor), 5.0,
      "branch modifier applies");
  suite.Expect(branches.Allocate(AlternateNode) ==
                   SkillOperationResult::ExclusiveGroupLimit,
               "exclusive group blocks conflicting branch");
  suite.Expect(branches.Refund(ArmorNode) == SkillOperationResult::Success,
               "exclusive branch can be refunded");
  suite.ExpectNear(
      branchFixture.Owner.Attributes().Get(Stats::CombatAttributes::Armor), 0.0,
      "branch refund removes modifier");
  suite.Expect(branches.Allocate(AlternateNode) ==
                   SkillOperationResult::Success,
               "exclusive capacity becomes available after refund");
  suite.Expect(branches.HasMutation(AlternateMutation),
               "keystone can expose rule-changing mutation reference");
  suite.Expect(branches.Allocate(WizardNode) ==
                   SkillOperationResult::WrongClass,
               "node class restriction is enforced at allocation");
}

bool IsValidDefinition(SkillTreeDefinition definition) {
  Fixture fixture;
  SkillTreeRuntime runtime{fixture.Owner, fixture.Abilities, fixture.Loadout,
                           std::move(definition), 10U};
  return runtime.IsValid();
}

void TestGraphValidation(TestSuite &suite) {
  {
    SkillTreeDefinition tree = RepresentativeTree();
    tree.Nodes.push_back(tree.Nodes.front());
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "duplicate node IDs are rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    SkillNodeDefinition node;
    node.Id = RootNode;
    node.Prerequisites = {SkillNodeId{999U}};
    tree.Nodes = {node};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "missing prerequisite references are rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    SkillNodeDefinition first;
    first.Id = SkillNodeId{10U};
    first.Prerequisites = {SkillNodeId{11U}};
    first.Connections = {SkillNodeId{11U}};
    SkillNodeDefinition second;
    second.Id = SkillNodeId{11U};
    second.Prerequisites = {SkillNodeId{10U}};
    second.Connections = {SkillNodeId{10U}};
    tree.Nodes = {first, second};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "prerequisite cycles are rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    SkillNodeDefinition node;
    node.Id = RootNode;
    node.PointCost = 0U;
    tree.Nodes = {node};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "invalid zero point cost is rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    SkillNodeDefinition node;
    node.Id = RootNode;
    node.ExclusiveGroup = SkillExclusiveGroupId{999U};
    tree.Nodes = {node};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "missing exclusive group definition is rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    SkillNodeDefinition first;
    first.Id = RootNode;
    first.Connections = {SkillNodeId{2U}};
    SkillNodeDefinition second;
    second.Id = SkillNodeId{2U};
    second.Prerequisites = {RootNode};
    tree.Nodes = {first, second};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "asymmetric graph connection is rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    SkillNodeDefinition node;
    node.Id = RootNode;
    node.UnlockAbility = AbilityId{999U};
    tree.Nodes = {node};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "missing ability reference is rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    tree.SupportedClasses = {Knight};
    SkillNodeDefinition node;
    node.Id = RootNode;
    node.RequiredClass = Wizard;
    tree.Nodes = {node};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "node class outside authored tree classes is rejected");
  }
  {
    SkillTreeDefinition tree;
    tree.Id = KnightTree;
    tree.SupportedClasses = {Knight, Knight};
    SkillNodeDefinition node;
    node.Id = RootNode;
    tree.Nodes = {node};
    suite.Expect(!IsValidDefinition(std::move(tree)),
                 "duplicate class references are rejected");
  }
}

void TestCaptureRestoreAndCorruption(TestSuite &suite) {
  Fixture fixture;
  SkillTreeRuntime tree{fixture.Owner, fixture.Abilities, fixture.Loadout,
                        RepresentativeTree(), 5U};
  const SkillTreeRuntimeState empty = tree.CaptureState();
  suite.Expect(tree.Allocate(RootNode) == SkillOperationResult::Success,
               "restore fixture allocates root");
  suite.Expect(tree.Allocate(TransformNode) == SkillOperationResult::Success,
               "restore fixture allocates transformation");
  const SkillTreeRuntimeState saved = tree.CaptureState();

  suite.Expect(tree.Refund(TransformNode) == SkillOperationResult::Success,
               "state mutates after capture");
  suite.Expect(tree.Refund(RootNode) == SkillOperationResult::Success,
               "captured root can be removed before restore");
  suite.Expect(tree.RestoreState(saved), "valid skill state restores");
  suite.Expect(tree.IsAllocated(RootNode) && tree.IsAllocated(TransformNode),
               "restore recreates allocations");
  suite.Expect(fixture.Abilities.IsUnlocked(RootAbility) &&
                   fixture.Abilities.IsUnlocked(TransformAbility),
               "restore recreates ability unlocks through authority");
  suite.ExpectNear(
      fixture.Owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
      10.0, "restore recreates exact skill modifier");
  suite.Expect(tree.HasMutation(TransformMutation),
               "restore rebuilds derived mutation state");

  SkillTreeRuntimeState corrupt = saved;
  ++corrupt.UnspentPoints;
  const SkillTreeRuntimeState before = tree.CaptureState();
  suite.Expect(!tree.RestoreState(corrupt),
               "point-accounting corruption is rejected");
  suite.Expect(tree.CaptureState().AllocatedNodes == before.AllocatedNodes &&
                   tree.UnspentPoints() == before.UnspentPoints,
               "rejected restore is transactional");

  SkillTreeRuntimeState duplicate = saved;
  duplicate.AllocatedNodes.push_back(RootNode);
  suite.Expect(!tree.RestoreState(duplicate),
               "duplicate persisted allocation is rejected");

  suite.Expect(
      fixture.Loadout.Equip(AbilityLoadoutSlot::Active2, TransformAbility) ==
          LoadoutResult::Success,
      "restore safety fixture equips transformed ability");
  suite.Expect(!tree.RestoreState(empty),
               "restore cannot silently revoke an equipped ability");
  suite.Expect(tree.IsAllocated(TransformNode),
               "failed equipped-ability restore leaves allocation intact");
}

void TestAuthorityAndModifierCollisionSafety(TestSuite &suite) {
  Fixture fixture;

  Combat::Combatant otherOwner{Combat::CombatantId{77U},
                               Combat::CombatantKind::Player};
  EffectRuntime otherEffects{otherOwner, std::vector<EffectDefinition>{}};
  Core::DeterministicRandom otherRandom{7U, 7U};
  AbilityRuntime otherAbilities{otherOwner, otherEffects, otherRandom, Knight,
                                AbilityDefinitions()};
  AbilityLoadout otherLoadout{otherAbilities};
  SkillTreeRuntime mismatched{fixture.Owner, fixture.Abilities, otherLoadout,
                              RepresentativeTree(), 2U};
  suite.Expect(!mismatched.IsValid(),
               "skill tree rejects loadout from another ability authority");

  SkillTreeRuntime first{fixture.Owner, fixture.Abilities, fixture.Loadout,
                         RepresentativeTree(), 2U};
  SkillTreeRuntime second{fixture.Owner, fixture.Abilities, fixture.Loadout,
                          RepresentativeTree(), 2U};
  suite.Expect(first.IsValid() && second.IsValid(),
               "duplicate runtime fixtures are individually well-formed");
  suite.Expect(first.Allocate(RootNode) == SkillOperationResult::Success,
               "first runtime installs its stable skill modifier");
  suite.Expect(second.Allocate(RootNode) ==
                   SkillOperationResult::ModifierFailure,
               "second runtime detects stable modifier ID collision");
  suite.Expect(
      !second.IsAllocated(RootNode) && second.UnspentPoints() == 2U,
      "modifier collision rolls graph and point state back atomically");
  suite.ExpectNear(
      fixture.Owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
      10.0, "collision rollback preserves original modifier exactly once");
  suite.Expect(
      fixture.Abilities.IsUnlocked(RootAbility),
      "collision rollback preserves pre-existing ability authority state");
}

void TestPointOverflowGuard(TestSuite &suite) {
  Fixture fixture;
  SkillTreeRuntime tree{fixture.Owner, fixture.Abilities, fixture.Loadout,
                        RepresentativeTree(),
                        std::numeric_limits<std::uint32_t>::max()};
  suite.Expect(tree.IsValid(), "maximum point budget remains valid state");
  suite.Expect(!tree.GrantPoints(1U),
               "point grant cannot overflow uint32 state");
}

void TestLargeGraph(TestSuite &suite) {
  constexpr std::uint32_t NodeCount = 2048U;
  Combat::Combatant owner{Combat::CombatantId{55U},
                          Combat::CombatantKind::Player};
  EffectRuntime effects{owner, std::vector<EffectDefinition>{}};
  Core::DeterministicRandom random{100U, 11U};
  AbilityRuntime abilities{owner, effects, random, Knight,
                           std::vector<AbilityDefinition>{}};
  AbilityLoadout loadout{abilities};

  SkillTreeDefinition definition;
  definition.Id = SkillTreeId{2U};
  definition.SupportedClasses = {Knight};
  definition.Nodes.reserve(NodeCount);
  for (std::uint32_t value = 1U; value <= NodeCount; ++value) {
    SkillNodeDefinition node;
    node.Id = SkillNodeId{value};
    if (value > 1U) {
      node.Prerequisites = {SkillNodeId{value - 1U}};
      node.Connections.push_back(SkillNodeId{value - 1U});
    }
    if (value < NodeCount) {
      node.Connections.push_back(SkillNodeId{value + 1U});
    }
    definition.Nodes.push_back(std::move(node));
  }

  SkillTreeRuntime tree{owner, abilities, loadout, std::move(definition),
                        NodeCount};
  suite.Expect(tree.IsValid(), "2048-node synthetic graph validates");
  bool allocationsSucceeded = true;
  for (std::uint32_t value = 1U; value <= NodeCount; ++value) {
    allocationsSucceeded =
        allocationsSucceeded &&
        tree.Allocate(SkillNodeId{value}) == SkillOperationResult::Success;
  }
  suite.Expect(
      allocationsSucceeded,
      "2048-node chain allocates deterministically in dependency order");
  suite.Expect(tree.IsAllocated(SkillNodeId{NodeCount}) &&
                   tree.UnspentPoints() == 0U,
               "large graph reaches final node with exact point accounting");
  const SkillTreeRuntimeState state = tree.CaptureState();
  suite.Expect(state.AllocatedNodes.size() == NodeCount,
               "large graph capture contains every allocated node");
}

} // namespace

int main() {
  TestSuite suite{"skill graph"};
  TestAllocationRefundAndIntegration(suite);
  TestPointsExclusiveGroupsAndClassRules(suite);
  TestGraphValidation(suite);
  TestCaptureRestoreAndCorruption(suite);
  TestAuthorityAndModifierCollisionSafety(suite);
  TestPointOverflowGuard(suite);
  TestLargeGraph(suite);
  return suite.Finish();
}
