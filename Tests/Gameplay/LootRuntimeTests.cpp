#include "ItemTestFixtures.h"
#include "Lostsense/Gameplay/Items/Equipment.h"
#include "TestHarness.h"

#include <cstdint>
#include <limits>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using namespace Lostsense::Gameplay::TestData;
using Tests::TestSuite;

bool SameGeneratedEntry(const GeneratedLootEntry &left,
                        const GeneratedLootEntry &right) {
  if (left.Item != right.Item || left.Quantity != right.Quantity ||
      left.IsItemInstance() != right.IsItemInstance()) {
    return false;
  }
  if (!left.IsItemInstance()) {
    return true;
  }
  const ItemInstance &a = left.Instance;
  const ItemInstance &b = right.Instance;
  if (a.InstanceId != b.InstanceId || a.DefinitionId != b.DefinitionId ||
      a.ItemLevel != b.ItemLevel || a.Rarity != b.Rarity ||
      a.Affixes.size() != b.Affixes.size() ||
      a.SocketedItems != b.SocketedItems) {
    return false;
  }
  for (std::size_t index = 0; index < a.Affixes.size(); ++index) {
    if (a.Affixes[index].Id != b.Affixes[index].Id ||
        a.Affixes[index].Magnitudes != b.Affixes[index].Magnitudes) {
      return false;
    }
  }
  return true;
}

void TestLootCatalogValidation(TestSuite &suite) {
  const ItemCatalog items = BuildCatalog();
  const LootCatalog tables = BuildLootCatalog(items);
  suite.Expect(tables.Validate(items), "representative loot catalog validates");

  LootCatalog empty;
  LootTableDefinition noEntries;
  noEntries.Id = {50U};
  suite.Expect(!empty.AddTable(noEntries), "empty loot table is rejected");

  LootTableDefinition zeroWeight;
  zeroWeight.Id = {51U};
  zeroWeight.Entries = {{Sword, 0U, 1U, 1U, {}}};
  suite.Expect(!empty.AddTable(zeroWeight), "zero entry weight is rejected");

  LootCatalog unknownItem;
  LootTableDefinition missingItem;
  missingItem.Id = {52U};
  missingItem.Entries = {{{999U}, 1U, 1U, 1U, {}}};
  missingItem.Rarities = {{Common, 1U}};
  suite.Expect(unknownItem.AddTable(missingItem),
               "shape-valid unknown item table can register");
  suite.Expect(!unknownItem.Validate(items),
               "loot validation rejects unknown ItemId reference");

  LootCatalog unknownRarity;
  LootTableDefinition missingRarity;
  missingRarity.Id = {53U};
  missingRarity.Entries = {{Sword, 1U, 1U, 1U, {}}};
  missingRarity.Rarities = {{{999U}, 1U}};
  suite.Expect(unknownRarity.AddTable(missingRarity),
               "shape-valid unknown rarity table can register");
  suite.Expect(!unknownRarity.Validate(items),
               "loot validation rejects unknown RarityId reference");

  LootCatalog quantityMismatch;
  LootTableDefinition invalidQuantity;
  invalidQuantity.Id = {54U};
  invalidQuantity.Entries = {{Sword, 1U, 1U, 2U, {}}};
  invalidQuantity.Rarities = {{Common, 1U}};
  suite.Expect(quantityMismatch.AddTable(invalidQuantity),
               "non-stack quantity mismatch is cross-reference validation");
  suite.Expect(!quantityMismatch.Validate(items),
               "non-stackable entry rejects multi-quantity range");

  LootCatalog largeWeights;
  LootTableDefinition large;
  large.Id = {55U};
  large.Entries = {
      {Sword, std::numeric_limits<std::uint32_t>::max(), 1U, 1U, {}},
      {Helmet, 1U, 1U, 1U, {}}};
  large.Rarities = {{Common, 1U}};
  suite.Expect(largeWeights.AddTable(large) && largeWeights.Validate(items),
               "64-bit weighted selection accepts totals above uint32 range");
}

void TestDeterminismAndRngContract(TestSuite &suite) {
  const ItemCatalog items = BuildCatalog();
  const LootCatalog tables = BuildLootCatalog(items);
  Core::DeterministicRandom randomA{12345U, 7U};
  Core::DeterministicRandom randomB{12345U, 7U};
  LootRuntime runtimeA{items, tables, randomA};
  LootRuntime runtimeB{items, tables, randomB};
  const LootContext context{20U, Knight};

  const Core::RandomState before = randomA.CaptureState();
  const LootGenerationOutcome firstA = runtimeA.Generate(MixedTable, context);
  const LootGenerationOutcome firstB = runtimeB.Generate(MixedTable, context);
  suite.Expect(firstA.Result == LootGenerationResult::Success &&
                   firstB.Result == LootGenerationResult::Success &&
                   firstA.Drops.size() == 1U && firstB.Drops.size() == 1U,
               "same-seed representative loot generation succeeds");
  suite.Expect(SameGeneratedEntry(firstA.Drops.front(), firstB.Drops.front()),
               "same table + context + RNG state produces identical loot");
  suite.Expect(randomA.CaptureState() == randomB.CaptureState(),
               "same loot consumes identical RNG stream advancement");
  suite.Expect(!(randomA.CaptureState() == before),
               "successful loot generation advances authoritative RNG");

  bool variationObserved = false;
  GeneratedLootEntry reference;
  bool haveReference = false;
  for (std::uint64_t seed = 1U; seed <= 8U; ++seed) {
    Core::DeterministicRandom random{seed, 3U};
    LootRuntime runtime{items, tables, random};
    const LootGenerationOutcome generated =
        runtime.Generate(MagicSwordTable, context);
    suite.Expect(generated.Result == LootGenerationResult::Success,
                 "variation seed still generates valid magic loot");
    if (!generated.Drops.empty()) {
      if (!haveReference) {
        reference = generated.Drops.front();
        haveReference = true;
      } else if (!SameGeneratedEntry(reference, generated.Drops.front())) {
        variationObserved = true;
      }
    }
  }
  suite.Expect(variationObserved,
               "different deterministic seeds can produce rolled variation");

  Core::DeterministicRandom rollbackRandom{99U, 5U};
  LootRuntime rollbackRuntime{items, tables, rollbackRandom};
  const Core::RandomState rollbackRng = rollbackRandom.CaptureState();
  const LootRuntimeState rollbackState = rollbackRuntime.CaptureState();
  const LootGenerationOutcome failed =
      rollbackRuntime.Generate(MixedTable, {999U, Knight});
  suite.Expect(failed.Result == LootGenerationResult::NoEligibleEntry,
               "level-filtered table reports no eligible entry");
  suite.Expect(rollbackRandom.CaptureState() == rollbackRng &&
                   rollbackRuntime.CaptureState().NextInstanceValue ==
                       rollbackState.NextInstanceValue,
               "failed generation rolls RNG and instance allocator back");
}

void TestRarityAffixesAndFilters(TestSuite &suite) {
  const ItemCatalog items = BuildCatalog();
  const LootCatalog tables = BuildLootCatalog(items);
  Core::DeterministicRandom random{222U, 11U};
  LootRuntime runtime{items, tables, random};

  const LootGenerationOutcome magic =
      runtime.Generate(MagicSwordTable, {25U, Knight});
  suite.Expect(magic.Result == LootGenerationResult::Success &&
                   magic.Drops.size() == 1U &&
                   magic.Drops.front().IsItemInstance(),
               "magic sword table generates a persisted instance");
  const ItemInstance &instance = magic.Drops.front().Instance;
  suite.Expect(instance.Rarity == Magic && instance.Affixes.size() == 2U,
               "rarity roll enforces authored affix count");
  suite.Expect(instance.Affixes[0].Id != instance.Affixes[1].Id,
               "rolled affixes are unique");
  const AffixDefinition &first = *items.FindAffix(instance.Affixes[0].Id);
  const AffixDefinition &second = *items.FindAffix(instance.Affixes[1].Id);
  suite.Expect(!first.ExclusiveGroup.IsValid() ||
                   !second.ExclusiveGroup.IsValid() ||
                   first.ExclusiveGroup != second.ExclusiveGroup,
               "exclusive affix groups cannot coexist");
  for (const RolledAffix &rolled : instance.Affixes) {
    const AffixDefinition &definition = *items.FindAffix(rolled.Id);
    suite.Expect(rolled.Magnitudes.size() == definition.Modifiers.size(),
                 "rolled affix persists each authored magnitude");
    for (std::size_t i = 0; i < rolled.Magnitudes.size(); ++i) {
      suite.Expect(
          rolled.Magnitudes[i] >= definition.Modifiers[i].MinimumMagnitude &&
              rolled.Magnitudes[i] <= definition.Modifiers[i].MaximumMagnitude,
          "affix magnitude remains inside authored bounds");
    }
  }

  const LootGenerationOutcome wrongClass =
      runtime.Generate(KnightOnlyTable, {25U, Wizard});
  suite.Expect(wrongClass.Result == LootGenerationResult::NoEligibleEntry,
               "class filters reject incompatible loot entries");
  const LootGenerationOutcome rightClass =
      runtime.Generate(KnightOnlyTable, {25U, Knight});
  suite.Expect(rightClass.Result == LootGenerationResult::Success,
               "class-compatible loot entry generates");

  const LootGenerationOutcome unique =
      runtime.Generate(UniqueTable, {25U, Knight});
  suite.Expect(unique.Result == LootGenerationResult::Success &&
                   unique.Drops.front().Instance.Rarity == UniqueRarity &&
                   unique.Drops.front().Instance.Affixes.empty(),
               "unique item uses authored fixed rarity behavior");
  const ItemDefinition &uniqueDefinition = *items.FindItem(UniqueBlade);
  suite.Expect(uniqueDefinition.Unique &&
                   uniqueDefinition.UniqueEffects.front() == UniqueEffect &&
                   uniqueDefinition.AbilityMutations.front() == UniqueMutation,
               "unique definition carries stable effect/mutation hooks");

  LootCatalog impossibleTables;
  LootTableDefinition impossible;
  impossible.Id = {80U};
  impossible.Entries = {{Helmet, 1U, 1U, 1U, {}}};
  impossible.Rarities = {{Magic, 1U}};
  suite.Expect(impossibleTables.AddTable(impossible) &&
                   impossibleTables.Validate(items),
               "affix-impossible table is structurally valid content");
  Core::DeterministicRandom impossibleRandom{5U, 1U};
  LootRuntime impossibleRuntime{items, impossibleTables, impossibleRandom};
  const Core::RandomState beforeImpossible = impossibleRandom.CaptureState();
  const LootGenerationOutcome impossibleResult =
      impossibleRuntime.Generate({80U}, {20U, Knight});
  suite.Expect(impossibleResult.Result == LootGenerationResult::NoEligibleAffix,
               "rarity requiring unavailable affixes fails explicitly");
  suite.Expect(impossibleRandom.CaptureState() == beforeImpossible,
               "affix-generation failure restores RNG transactionally");
}

void TestStackAndRuntimeState(TestSuite &suite) {
  const ItemCatalog items = BuildCatalog();
  const LootCatalog tables = BuildLootCatalog(items);
  Core::DeterministicRandom random{77U, 13U};
  LootRuntime runtime{items, tables, random};

  const LootGenerationOutcome stack =
      runtime.Generate(StackTable, {10U, Knight});
  suite.Expect(stack.Result == LootGenerationResult::Success &&
                   stack.Drops.size() == 1U &&
                   !stack.Drops.front().IsItemInstance() &&
                   stack.Drops.front().Quantity >= 2U &&
                   stack.Drops.front().Quantity <= 5U,
               "stackable loot rolls deterministic authored quantity range");

  const LootGenerationOutcome first =
      runtime.Generate(SwordOnlyTable, {10U, Knight});
  const LootGenerationOutcome second =
      runtime.Generate(SwordOnlyTable, {10U, Knight});
  suite.Expect(
      first.Result == LootGenerationResult::Success &&
          second.Result == LootGenerationResult::Success &&
          first.Drops.front().Instance.InstanceId == ItemInstanceId{1U} &&
          second.Drops.front().Instance.InstanceId == ItemInstanceId{2U},
      "persistent item instance IDs advance deterministically");
  const LootRuntimeState saved = runtime.CaptureState();
  suite.Expect(!runtime.RestoreState({0U}),
               "zero next-instance state is rejected");
  suite.Expect(!runtime.RestoreState({MaximumPersistentItemInstanceId + 2ULL}),
               "out-of-range next-instance state is rejected");
  suite.Expect(runtime.CaptureState().NextInstanceValue ==
                   saved.NextInstanceValue,
               "failed loot-state restore leaves runtime unchanged");
}

void TestPortableEndToEndChain(TestSuite &suite) {
  const ItemCatalog items = BuildCatalog();
  const LootCatalog tables = BuildLootCatalog(items);
  Core::DeterministicRandom random{404U, 21U};
  LootRuntime loot{items, tables, random};
  Inventory inventory{items, 8U};
  Combat::Combatant owner{Combat::CombatantId{500U},
                          Combat::CombatantKind::Player};
  EquipmentRuntime equipment{items, owner, Knight};
  const double baseline =
      owner.Attributes().Get(Stats::CombatAttributes::AttackPower);

  const LootGenerationOutcome generated =
      loot.Generate(SwordOnlyTable, {15U, Knight});
  suite.Expect(generated.Result == LootGenerationResult::Success &&
                   generated.Drops.size() == 1U &&
                   generated.Drops.front().IsItemInstance(),
               "loot runtime creates representative equippable item");
  const ItemInstance generatedItem = generated.Drops.front().Instance;
  suite.Expect(inventory.AddInstance(generatedItem) == InventoryResult::Success,
               "generated item enters inventory");
  suite.Expect(equipment.EquipFromInventory(inventory, generatedItem.InstanceId,
                                            MainHand) ==
                   EquipmentResult::Success,
               "inventory item equips through equipment runtime");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   baseline + 3.0,
                   "equipped loot changes existing AttributeSet authority");
  suite.Expect(equipment.UnequipToInventory(inventory, MainHand) ==
                   EquipmentResult::Success,
               "equipped loot unequips back to inventory");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   baseline,
                   "unequip returns attribute value to exact baseline");
  suite.Expect(inventory.ContainsInstance(generatedItem.InstanceId),
               "end-to-end item ownership returns to inventory");
}

} // namespace

int main() {
  TestSuite suite{"gameplay.deterministic_loot"};
  TestLootCatalogValidation(suite);
  TestDeterminismAndRngContract(suite);
  TestRarityAffixesAndFilters(suite);
  TestStackAndRuntimeState(suite);
  TestPortableEndToEndChain(suite);
  return suite.Finish();
}
