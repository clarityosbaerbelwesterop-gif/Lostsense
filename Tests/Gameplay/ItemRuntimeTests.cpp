#include "ItemTestFixtures.h"
#include "Lostsense/Gameplay/Items/Equipment.h"
#include "TestHarness.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using namespace Lostsense::Gameplay::TestData;
using Tests::TestSuite;

void TestCatalogValidation(TestSuite &suite) {
  const ItemCatalog catalog = BuildCatalog();
  suite.Expect(catalog.Validate(), "representative item catalog validates");

  ItemCatalog duplicateItems;
  suite.Expect(duplicateItems.AddRarity({Common, 1U, 100U, 0U, 0U}),
               "rarity registers once");
  ItemDefinition item;
  item.Id = Sword;
  item.Type = WeaponItem;
  item.AllowedEquipmentSlots = {MainHand};
  item.AllowedRarities = {Common};
  suite.Expect(duplicateItems.AddItem(item), "item registers once");
  suite.Expect(!duplicateItems.AddItem(item), "duplicate ItemId is rejected");

  ItemCatalog duplicateAffixes;
  AffixDefinition affix;
  affix.Id = AttackAffix;
  affix.MinimumItemLevel = 1U;
  affix.MaximumItemLevel = 100U;
  affix.AllowedItemTypes = {WeaponItem};
  affix.Modifiers = {{Stats::CombatAttributes::AttackPower,
                      Stats::ModifierOperation::Additive, 1.0, 2.0}};
  suite.Expect(duplicateAffixes.AddAffix(affix), "affix registers once");
  suite.Expect(!duplicateAffixes.AddAffix(affix),
               "duplicate AffixId is rejected");

  ItemCatalog badReference;
  suite.Expect(badReference.AddRarity({Common, 1U, 100U, 0U, 0U}),
               "reference test rarity registers");
  ItemDefinition unknownRarity = item;
  unknownRarity.Id = {50U};
  unknownRarity.AllowedRarities = {{999U}};
  suite.Expect(badReference.AddItem(unknownRarity),
               "shape-valid item can be registered before cross validation");
  suite.Expect(!badReference.Validate(),
               "catalog rejects unknown RarityId reference");

  ItemCatalog malformed;
  ItemDefinition badSlot = item;
  badSlot.Id = {51U};
  badSlot.AllowedEquipmentSlots = {{}};
  suite.Expect(!malformed.AddItem(badSlot),
               "invalid equipment slot is rejected");

  ItemDefinition badLevel = item;
  badLevel.Id = {52U};
  badLevel.MinimumItemLevel = 20U;
  badLevel.MaximumItemLevel = 10U;
  suite.Expect(!malformed.AddItem(badLevel),
               "invalid item-level bounds reject");

  ItemDefinition badModifier = item;
  badModifier.Id = {53U};
  badModifier.BaseModifiers = {{Stats::CombatAttributes::AttackPower,
                                Stats::ModifierOperation::Additive,
                                std::numeric_limits<double>::quiet_NaN()}};
  suite.Expect(!malformed.AddItem(badModifier),
               "non-finite authored modifier is rejected");

  const ItemDefinition &knight = *catalog.FindItem(KnightBlade);
  suite.Expect(catalog.IsItemAllowedForClass(knight, Knight),
               "class-restricted item accepts intended class");
  suite.Expect(!catalog.IsItemAllowedForClass(knight, Wizard),
               "class-restricted item rejects wrong class");
}

void TestInstanceAndAffixResolution(TestSuite &suite) {
  const ItemCatalog catalog = BuildCatalog();
  ItemInstance instance{ItemInstanceId{10U},
                        Sword,
                        20U,
                        Magic,
                        {{AttackAffix, {6.0}}, {ArmorAffix, {3.0}}},
                        {Gem}};
  suite.Expect(catalog.ValidateInstance(instance, Knight),
               "rolled instance validates against definitions");

  std::vector<ResolvedItemModifier> modifiers;
  suite.Expect(catalog.ResolveModifiers(instance, modifiers),
               "rolled item resolves persistent affix/socket modifiers");
  suite.Expect(modifiers.size() == 4U,
               "base, affix and socket modifiers are all resolved");
  double attack = 0.0;
  double armor = 0.0;
  for (const ResolvedItemModifier &modifier : modifiers) {
    if (modifier.Attribute == Stats::CombatAttributes::AttackPower) {
      attack += modifier.Magnitude;
    }
    if (modifier.Attribute == Stats::CombatAttributes::Armor) {
      armor += modifier.Magnitude;
    }
  }
  suite.ExpectNear(attack, 11.0,
                   "base + rolled affix + socket attack is stable");
  suite.ExpectNear(armor, 3.0, "rolled armor magnitude is preserved");

  ItemInstance exclusive = instance;
  exclusive.Affixes = {{AttackAffix, {5.0}}, {AlternateAttackAffix, {11.0}}};
  suite.Expect(!catalog.ValidateInstance(exclusive, Knight),
               "two affixes from one exclusive group are rejected");

  ItemInstance wrongClass = instance;
  wrongClass.Affixes = {{AttackAffix, {5.0}}, {WizardAffix, {8.0}}};
  suite.Expect(!catalog.ValidateInstance(wrongClass, Knight),
               "class-incompatible rolled affix is rejected");

  ItemInstance corrupted = instance;
  corrupted.Affixes.front().Magnitudes.front() = 99.0;
  suite.Expect(!catalog.ValidateInstance(corrupted, Knight),
               "persisted magnitude outside authored bounds is rejected");
}

void TestInventoryTransactions(TestSuite &suite) {
  const ItemCatalog catalog = BuildCatalog();
  Inventory inventory{catalog, 4U};
  suite.Expect(inventory.IsValid(), "inventory validates against item catalog");
  suite.Expect(inventory.AddStack(Potion, 15U) == InventoryResult::Success,
               "stackable item adds");
  suite.Expect(inventory.AddStack(Potion, 10U) == InventoryResult::Success,
               "stack quantities merge deterministically");
  suite.Expect(inventory.StackQuantity(Potion) == 25U &&
                   inventory.OccupiedSlots() == 2U,
               "merged quantity occupies authored stack slots");

  const ItemInstance swordA = CommonInstance({20U}, Sword);
  const ItemInstance swordB = CommonInstance({21U}, Sword);
  suite.Expect(inventory.AddInstance(swordA) == InventoryResult::Success,
               "non-stackable instance adds");
  suite.Expect(inventory.AddInstance(swordA) ==
                   InventoryResult::DuplicateInstance,
               "duplicate instance ID rejects");
  suite.Expect(inventory.AddInstance(swordB) == InventoryResult::Success,
               "second distinct instance fills final slot");
  suite.Expect(inventory.AddInstance(CommonInstance({22U}, Sword)) ==
                   InventoryResult::CapacityExceeded,
               "capacity rejects extra instance");

  suite.Expect(inventory.RemoveStack(Potion, 5U) == InventoryResult::Success &&
                   inventory.StackQuantity(Potion) == 20U,
               "stack removal updates quantity exactly");

  Inventory destination{catalog, 1U};
  suite.Expect(destination.AddInstance(CommonInstance({30U}, Helmet)) ==
                   InventoryResult::Success,
               "destination can be intentionally filled");
  const InventoryState beforeSource = inventory.CaptureState();
  const InventoryState beforeDestination = destination.CaptureState();
  suite.Expect(inventory.TransferInstanceTo(destination, {20U}) ==
                   InventoryResult::CapacityExceeded,
               "failed transfer reports destination capacity");
  suite.Expect(inventory.ContainsInstance({20U}) &&
                   destination.ContainsInstance({30U}) &&
                   destination.OccupiedSlots() ==
                       beforeDestination.Instances.size() &&
                   inventory.CaptureState().Instances.size() ==
                       beforeSource.Instances.size(),
               "failed transfer rolls both inventories back");

  Inventory transferTarget{catalog, 4U};
  suite.Expect(inventory.TransferStackTo(transferTarget, Potion, 7U) ==
                   InventoryResult::Success,
               "stack transfer succeeds atomically");
  suite.Expect(inventory.StackQuantity(Potion) == 13U &&
                   transferTarget.StackQuantity(Potion) == 7U,
               "stack transfer moves exact quantity");

  const InventoryState captured = inventory.CaptureState();
  suite.Expect(inventory.RemoveStack(Potion, 13U) == InventoryResult::Success,
               "state mutation before restore succeeds");
  suite.Expect(inventory.RestoreState(captured) &&
                   inventory.StackQuantity(Potion) == 13U,
               "inventory capture/restore returns exact state");

  InventoryState malformed = captured;
  malformed.Instances.push_back(malformed.Instances.front());
  suite.Expect(!inventory.RestoreState(malformed),
               "malformed duplicate instance restore rejects transactionally");
  suite.Expect(inventory.StackQuantity(Potion) == 13U,
               "failed restore leaves live inventory unchanged");

  Inventory ordered{catalog, 4U};
  suite.Expect(ordered.AddInstance(CommonInstance({50U}, Sword)) ==
                       InventoryResult::Success &&
                   ordered.AddInstance(CommonInstance({40U}, Helmet)) ==
                       InventoryResult::Success,
               "out-of-order instance insertion succeeds");
  const InventoryState orderedState = ordered.CaptureState();
  suite.Expect(orderedState.Instances.size() == 2U &&
                   orderedState.Instances[0].InstanceId ==
                       ItemInstanceId{40U} &&
                   orderedState.Instances[1].InstanceId == ItemInstanceId{50U},
               "capture ordering is stable by strong instance ID");
}

void TestEquipmentLifecycle(TestSuite &suite) {
  const ItemCatalog catalog = BuildCatalog();
  Combat::Combatant owner{Combat::CombatantId{100U},
                          Combat::CombatantKind::Player};
  Inventory inventory{catalog, 8U};
  EquipmentRuntime equipment{catalog, owner, Knight};
  suite.Expect(equipment.IsValid(), "equipment runtime validates");

  suite.Expect(inventory.AddInstance(CommonInstance({100U}, Sword)) ==
                   InventoryResult::Success,
               "sword enters inventory before equip");
  suite.Expect(equipment.EquipFromInventory(inventory, {100U}, Head) ==
                   EquipmentResult::WrongSlot,
               "wrong equipment slot rejects before mutation");
  suite.Expect(inventory.ContainsInstance({100U}),
               "wrong-slot failure preserves inventory ownership");

  suite.Expect(equipment.EquipFromInventory(inventory, {100U}, MainHand) ==
                   EquipmentResult::Success,
               "owned compatible item equips");
  suite.Expect(!inventory.ContainsInstance({100U}) &&
                   equipment.ContainsInstance({100U}),
               "equipped instance has one authoritative owner");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   3.0, "equip installs exact equipment modifier");

  suite.Expect(equipment.UnequipToInventory(inventory, MainHand) ==
                   EquipmentResult::Success,
               "equipped item returns to inventory");
  suite.Expect(inventory.ContainsInstance({100U}) &&
                   equipment.EquippedCount() == 0U,
               "unequip transfers ownership back to inventory");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   0.0, "unequip removes exact equipment modifier");

  Combat::Combatant wizard{Combat::CombatantId{101U},
                           Combat::CombatantKind::Player};
  Inventory wizardInventory{catalog, 4U};
  EquipmentRuntime wizardEquipment{catalog, wizard, Wizard};
  suite.Expect(wizardInventory.AddInstance(CommonInstance(
                   {101U}, KnightBlade)) == InventoryResult::Success,
               "wrong-class item may exist in inventory");
  suite.Expect(
      wizardEquipment.EquipFromInventory(wizardInventory, {101U}, MainHand) ==
          EquipmentResult::WrongClass,
      "class validation blocks equip");

  Inventory replacementInventory{catalog, 8U};
  EquipmentRuntime replacement{catalog, owner, Knight};
  suite.Expect(
      replacementInventory.AddInstance(CommonInstance({110U}, Sword)) ==
              InventoryResult::Success &&
          replacementInventory.AddInstance(ItemInstance{
              ItemInstanceId{111U}, UniqueBlade, 10U, UniqueRarity, {}, {}}) ==
              InventoryResult::Success,
      "replacement candidates enter inventory");
  suite.Expect(
      replacement.EquipFromInventory(replacementInventory, {110U}, MainHand) ==
          EquipmentResult::Success,
      "initial replacement item equips");
  suite.Expect(
      replacement.EquipFromInventory(replacementInventory, {111U}, MainHand) ==
          EquipmentResult::Success,
      "replacement swaps item atomically");
  suite.Expect(replacementInventory.ContainsInstance({110U}) &&
                   !replacementInventory.ContainsInstance({111U}),
               "replacement returns prior item and consumes incoming item");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   15.0,
                   "replacement removes old modifier before new modifier");
  suite.Expect(replacement.GrantsUniqueEffect(UniqueEffect) &&
                   replacement.HasAbilityMutation(UniqueMutation),
               "unique item exposes stable effect and mutation references");

  const ItemInstance incoming = CommonInstance({112U}, Sword);
  suite.Expect(replacementInventory.AddInstance(incoming) ==
                   InventoryResult::Success,
               "collision candidate enters inventory");
  const Stats::ModifierId collision{(1ULL << 61U) |
                                    (incoming.InstanceId.Value << 16U) | 1ULL};
  suite.Expect(owner.AddAttributeModifier({collision,
                                           Stats::CombatAttributes::AttackPower,
                                           Stats::ModifierOperation::Additive,
                                           Stats::ModifierSource::System, 0.0}),
               "test reserves incoming equipment modifier ID");
  const InventoryState beforeFailure = replacementInventory.CaptureState();
  suite.Expect(
      replacement.EquipFromInventory(replacementInventory, {112U}, MainHand) ==
          EquipmentResult::ModifierFailure,
      "modifier collision rejects replacement");
  suite.Expect(replacement.ContainsInstance({111U}) &&
                   replacementInventory.ContainsInstance({112U}) &&
                   replacementInventory.CaptureState().Instances.size() ==
                       beforeFailure.Instances.size(),
               "failed replacement rolls inventory and equipment back");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   15.0, "failed replacement preserves previous item stats");
  suite.Expect(owner.RemoveAttributeModifier(collision),
               "test collision modifier is removable");

  const EquipmentState saved = replacement.CaptureState();
  suite.Expect(replacement.UnequipToInventory(replacementInventory, MainHand) ==
                   EquipmentResult::Success,
               "equipment can be cleared before restore");
  ItemInstance removed;
  suite.Expect(replacementInventory.TakeInstance({111U}, removed) ==
                   InventoryResult::Success,
               "restored equipped item is removed from inventory ownership");
  suite.Expect(replacement.RestoreState(saved, replacementInventory),
               "equipment state restores transactionally");
  suite.Expect(replacement.ContainsInstance({111U}),
               "restore recovers equipped instance");
  suite.ExpectNear(owner.Attributes().Get(Stats::CombatAttributes::AttackPower),
                   15.0, "restore reinstalls exact modifiers");

  EquipmentState corrupt = saved;
  corrupt.Items.push_back({RingRight, corrupt.Items.front().Item});
  suite.Expect(!replacement.RestoreState(corrupt, replacementInventory),
               "corrupt duplicate equipped instance restore rejects");
  suite.Expect(replacement.ContainsInstance({111U}),
               "failed equipment restore leaves live state unchanged");
}

} // namespace

int main() {
  TestSuite suite{"gameplay.items_inventory_equipment"};
  TestCatalogValidation(suite);
  TestInstanceAndAffixResolution(suite);
  TestInventoryTransactions(suite);
  TestEquipmentLifecycle(suite);
  return suite.Finish();
}
