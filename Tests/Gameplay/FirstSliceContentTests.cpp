#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Items/Equipment.h"
#include "Lostsense/Gameplay/Items/Inventory.h"
#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"
#include "Lostsense/Gameplay/Persistence/CharacterPersistence.h"
#include "Lostsense/Gameplay/Persistence/SaveCodec.h"
#include "Lostsense/Stats/CombatAttributes.h"
#include "TestHarness.h"

#include <string>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using namespace Lostsense::Gameplay::FirstSlice;
using Tests::TestSuite;

struct SliceRuntime final {
  ItemCatalog Items{BuildItemCatalog()};
  LootCatalog Tables{BuildLootCatalog()};
  Combat::Combatant Player{Combat::CombatantId{1U},
                           Combat::CombatantKind::Player};
  EffectRuntime Effects{Player, BuildKnightEffects()};
  Core::DeterministicRandom Random{0xC0FFEEU, 0xBEEFU};
  AbilityRuntime Abilities{Player, Effects, Random, KnightClass,
                           BuildKnightAbilities()};
  AbilityLoadout Loadout{Abilities};
  SkillTreeRuntime Skills{Player, Abilities, Loadout, BuildKnightScarAtlas(),
                          16U};
  Inventory InventoryState{Items, 32U};
  EquipmentRuntime Equipment{Items, Player, KnightClass};
  LootRuntime Loot{Items, Tables, Random};
  CharacterPersistence Persistence{CharacterPersistentId{1U},
                                   KnightClass,
                                   Player,
                                   Random,
                                   Effects,
                                   Abilities,
                                   Skills,
                                   Loadout,
                                   InventoryState,
                                   Equipment,
                                   Loot};

  [[nodiscard]] bool UnlockCoreActions() {
    return Abilities.Unlock(KnightPrimaryAttack) &&
           Abilities.Unlock(KnightHeavyAttack) &&
           Abilities.Unlock(KnightDodge) && Abilities.Unlock(KnightGuard) &&
           Loadout.Equip(AbilityLoadoutSlot::PrimaryAttack,
                         KnightPrimaryAttack) == LoadoutResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::SecondaryAttack,
                         KnightHeavyAttack) == LoadoutResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::Dodge, KnightDodge) ==
               LoadoutResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::ClassMechanic, KnightGuard) ==
               LoadoutResult::Success;
  }

  [[nodiscard]] bool AllocateActivePath() {
    return Skills.Allocate(FirstMeasure) == SkillOperationResult::Success &&
           Skills.Allocate(BellstepNode) == SkillOperationResult::Success &&
           Skills.Allocate(ResolveWell) == SkillOperationResult::Success &&
           Skills.Allocate(GuardDensity) == SkillOperationResult::Success &&
           Skills.Allocate(MeasureBreakerNode) ==
               SkillOperationResult::Success &&
           Skills.Allocate(MarchSweepNode) == SkillOperationResult::Success &&
           Skills.Allocate(AnsweringGuardNode) ==
               SkillOperationResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::Active1, Bellstep) ==
               LoadoutResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::Active2, MeasureBreaker) ==
               LoadoutResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::Active3, MarchSweep) ==
               LoadoutResult::Success &&
           Loadout.Equip(AbilityLoadoutSlot::Active4, AnsweringGuard) ==
               LoadoutResult::Success;
  }
};

void TestProductionDefinitionsValidate(TestSuite &suite) {
  const ItemCatalog items = BuildItemCatalog();
  const LootCatalog tables = BuildLootCatalog();
  suite.Expect(items.Validate(), "first-slice item catalog validates");
  suite.Expect(tables.Validate(items), "first-slice loot tables validate");

  SliceRuntime runtime;

  bool foundPerfectGuard = false;
  for (const EffectDefinition &effect : BuildKnightEffects()) {
    if (effect.Id == PerfectGuardWindow) {
      foundPerfectGuard =
          effect.DurationSeconds > 0.0 && effect.DurationSeconds <= 0.20;
    }
  }
  suite.Expect(foundPerfectGuard,
               "perfect guard has a bounded authored timing window");
  suite.Expect(runtime.Effects.IsValid(), "Knight effect definitions validate");
  suite.Expect(runtime.Abilities.IsValid(),
               "Knight ability definitions validate");
  suite.Expect(runtime.Skills.IsValid(), "Knight Scar Atlas subset validates");
  suite.Expect(runtime.Equipment.IsValid(),
               "Knight equipment authority accepts production catalog");
  suite.Expect(runtime.Loot.IsValid(),
               "first-slice deterministic loot runtime validates");
  suite.Expect(runtime.Persistence.IsValid(),
               "first-slice aggregate persistence is composable");
}

void TestKnightSkillAndAbilityLoop(TestSuite &suite) {
  SliceRuntime runtime;
  suite.Expect(runtime.UnlockCoreActions(), "core Knight actions unlock/equip");
  suite.Expect(runtime.AllocateActivePath(),
               "Scar Atlas path unlocks and equips four active abilities");
  suite.Expect(runtime.Loadout.AbilityAt(AbilityLoadoutSlot::Active1) ==
                       Bellstep &&
                   runtime.Loadout.AbilityAt(AbilityLoadoutSlot::Active4) ==
                       AnsweringGuard,
               "active slots use canonical production ability IDs");
  suite.ExpectNear(runtime.Player.Resource().Maximum(), 110.0,
                   "Resolve Well increases portable resource maximum");
  suite.Expect(runtime.Skills.Allocate(StandWhereItFallsNode) ==
                   SkillOperationResult::Success,
               "Knight Vow allocates through real SkillTreeRuntime");
  suite.Expect(runtime.Skills.HasMutation(StandWhereItFalls),
               "allocated Vow exposes stable mutation hook");
  suite.Expect(runtime.Skills.Allocate(HollowMeasureNode) ==
                   SkillOperationResult::ExclusiveGroupLimit,
               "mutually exclusive Oath path is enforced");

  Combat::Combatant enemy{Combat::CombatantId{2U},
                          Combat::CombatantKind::Enemy};
  EffectRuntime enemyEffects{enemy, BuildKnightEffects()};
  const double before = enemy.Health().Current();
  const AbilityTarget hostile{&enemy, &enemyEffects, TargetRelation::Hostile};
  const AbilityActivationOutcome bellstep =
      runtime.Abilities.Activate(Bellstep, hostile);
  suite.Expect(bellstep.Result == AbilityActivationResult::Success &&
                   bellstep.DamageResolved,
               "Bellstep resolves through authoritative AbilityRuntime");
  suite.Expect(enemy.Health().Current() < before,
               "Bellstep damages hostile portable Combatant");
  suite.Expect(runtime.Effects.HasEffect(BellstepArmor),
               "Bellstep applies its authored temporary armor effect");

  AbilityTarget noTarget{};
  const AbilityActivationOutcome guard =
      runtime.Abilities.Activate(AnsweringGuard, noTarget);
  suite.Expect(guard.Result == AbilityActivationResult::Success &&
                   runtime.Effects.HasEffect(AnsweringGuardWindow),
               "Answering Guard applies its authored guard window");
}

void TestDeterministicSliceLootAndEquipment(TestSuite &suite) {
  const ItemCatalog itemsA = BuildItemCatalog();
  const LootCatalog tablesA = BuildLootCatalog();
  Core::DeterministicRandom randomA{12345U, 99U};
  Core::DeterministicRandom randomB{12345U, 99U};
  LootRuntime lootA{itemsA, tablesA, randomA};
  LootRuntime lootB{itemsA, tablesA, randomB};
  const LootContext context{12U, KnightClass};
  const LootGenerationOutcome a = lootA.Generate(VaurEliteLoot, context);
  const LootGenerationOutcome b = lootB.Generate(VaurEliteLoot, context);
  suite.Expect(a.Result == LootGenerationResult::Success &&
                   b.Result == LootGenerationResult::Success &&
                   a.Drops.size() == b.Drops.size(),
               "elite loot generates successfully from canonical table");
  bool same = a.Drops.size() == b.Drops.size();
  for (std::size_t index = 0U; same && index < a.Drops.size(); ++index) {
    same = a.Drops[index].Item == b.Drops[index].Item &&
           a.Drops[index].Quantity == b.Drops[index].Quantity &&
           a.Drops[index].Instance.DefinitionId ==
               b.Drops[index].Instance.DefinitionId &&
           a.Drops[index].Instance.Rarity == b.Drops[index].Instance.Rarity &&
           a.Drops[index].Instance.Affixes.size() ==
               b.Drops[index].Instance.Affixes.size();
  }
  suite.Expect(same, "same seed produces same first-slice loot identity");

  SliceRuntime runtime;
  LootGenerationOutcome drop = runtime.Loot.Generate(VaurEnemyLoot, context);
  suite.Expect(drop.Result == LootGenerationResult::Success &&
                   drop.Drops.size() == 1U,
               "world enemy emits one canonical loot roll");

  if (!drop.Drops.empty() && drop.Drops.front().IsItemInstance()) {
    ItemInstance instance = drop.Drops.front().Instance;
    const ItemDefinition *definition =
        runtime.Items.FindItem(instance.DefinitionId);
    suite.Expect(definition != nullptr &&
                     !definition->AllowedEquipmentSlots.empty(),
                 "rolled equipment resolves to equippable production item");
    if (definition != nullptr && !definition->AllowedEquipmentSlots.empty()) {
      const double attackBefore =
          runtime.Player.Attributes().Get(Stats::CombatAttributes::AttackPower);
      suite.Expect(runtime.InventoryState.AddInstance(instance) ==
                       InventoryResult::Success,
                   "world drop enters real Inventory authority");
      suite.Expect(runtime.Equipment.EquipFromInventory(
                       runtime.InventoryState, instance.InstanceId,
                       definition->AllowedEquipmentSlots.front()) ==
                       EquipmentResult::Success,
                   "picked-up production item equips transactionally");
      const double attackAfter =
          runtime.Player.Attributes().Get(Stats::CombatAttributes::AttackPower);
      suite.Expect(attackAfter >= attackBefore,
                   "equipment modifier path changes authoritative attributes");
    }
  } else if (!drop.Drops.empty()) {
    suite.Expect(runtime.InventoryState.AddStack(drop.Drops.front().Item,
                                                 drop.Drops.front().Quantity) ==
                     InventoryResult::Success,
                 "stackable world drop enters real Inventory authority");
  }

  LootGenerationOutcome boss = runtime.Loot.Generate(OdranBossLoot, context);
  suite.Expect(
      boss.Result == LootGenerationResult::Success && boss.Drops.size() == 1U &&
          boss.Drops.front().Instance.DefinitionId == OdransClapper,
      "Odran boss table deterministically produces canonical boss weapon");
  if (!boss.Drops.empty()) {
    const ItemInstance clapper = boss.Drops.front().Instance;
    suite.Expect(runtime.InventoryState.AddInstance(clapper) ==
                     InventoryResult::Success,
                 "Odran's Clapper enters inventory");
    suite.Expect(runtime.Equipment.EquipFromInventory(
                     runtime.InventoryState, clapper.InstanceId, MainHand) ==
                     EquipmentResult::Success,
                 "Odran's Clapper can replace current main-hand item");
    suite.Expect(
        runtime.Equipment.HasAbilityMutation(ClappersReturn),
        "boss weapon exposes canonical Clapper's Return mutation hook");
  }
}

void TestSliceSaveRoundTrip(TestSuite &suite) {
  SliceRuntime runtime;
  suite.Expect(runtime.UnlockCoreActions() && runtime.AllocateActivePath(),
               "slice progression prepares persistent state");
  const LootContext context{15U, KnightClass};
  LootGenerationOutcome boss = runtime.Loot.Generate(OdranBossLoot, context);
  suite.Expect(boss.Result == LootGenerationResult::Success &&
                   !boss.Drops.empty(),
               "boss loot exists for save/load integration");
  if (!boss.Drops.empty()) {
    const ItemInstance item = boss.Drops.front().Instance;
    suite.Expect(
        runtime.InventoryState.AddInstance(item) == InventoryResult::Success &&
            runtime.Equipment.EquipFromInventory(runtime.InventoryState,
                                                 item.InstanceId, MainHand) ==
                EquipmentResult::Success,
        "boss weapon state is equipped before save");
  }

  const CharacterSaveState saved = runtime.Persistence.CaptureState();
  suite.Expect(runtime.Persistence.ValidateState(saved),
               "first-slice aggregate state validates");
  std::string payload;
  suite.Expect(SaveCodec::Serialize(saved, payload),
               "first-slice aggregate serializes");

  CharacterSaveState decoded;
  suite.Expect(SaveCodec::Deserialize(payload, decoded).Status ==
                   SaveDecodeStatus::Success,
               "first-slice save payload decodes");

  static_cast<void>(runtime.Loadout.Unequip(AbilityLoadoutSlot::Active1));
  static_cast<void>(
      runtime.Equipment.UnequipToInventory(runtime.InventoryState, MainHand));
  suite.Expect(runtime.Persistence.RestoreState(decoded) ==
                   CharacterRestoreResult::Success,
               "first-slice progression/equipment restores atomically");
  suite.Expect(runtime.Loadout.AbilityAt(AbilityLoadoutSlot::Active1) ==
                       Bellstep &&
                   runtime.Equipment.HasAbilityMutation(ClappersReturn),
               "save/load restores active ability and boss-weapon mutation");
}

} // namespace

int main() {
  TestSuite suite{"gameplay.first_slice_content"};
  TestProductionDefinitionsValidate(suite);
  TestKnightSkillAndAbilityLoop(suite);
  TestDeterministicSliceLootAndEquipment(suite);
  TestSliceSaveRoundTrip(suite);
  return suite.Finish();
}
