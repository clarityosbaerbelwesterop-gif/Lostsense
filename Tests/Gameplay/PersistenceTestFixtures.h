#pragma once

#include "ItemTestFixtures.h"
#include "Lostsense/Gameplay/Persistence/CharacterPersistence.h"
#include "Lostsense/Stats/CombatAttributes.h"

#include <vector>

namespace Lostsense::Gameplay::PersistenceTestData {

using namespace TestData;

inline constexpr CharacterPersistentId Character{500U};
inline constexpr EffectId PersistentBuff{700U};
inline constexpr AbilityId KnightAbility{710U};
inline constexpr SkillTreeId PersistenceTree{720U};
inline constexpr SkillNodeId AbilityNode{721U};

inline std::vector<EffectDefinition> EffectDefinitions() {
  EffectDefinition buff;
  buff.Id = PersistentBuff;
  buff.StackGroup = EffectStackGroupId{700U};
  buff.Stacking = EffectStackingPolicy::Independent;
  buff.MaxStacks = 1U;
  buff.Permanent = true;
  buff.AttributeModifiers = {{Stats::CombatAttributes::Armor,
                              Stats::ModifierOperation::Additive, 2.5}};
  return {buff};
}

inline std::vector<AbilityDefinition> AbilityDefinitions() {
  AbilityDefinition ability;
  ability.Id = KnightAbility;
  ability.RequiredClass = Knight;
  ability.ResourceCost = 3.0;
  ability.CooldownSeconds = 1.25;
  ability.TargetRule = AbilityTargetRule::None;
  ability.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::Active1);
  return {ability};
}

inline SkillTreeDefinition TreeDefinition() {
  SkillTreeDefinition tree;
  tree.Id = PersistenceTree;
  tree.SupportedClasses = {Knight};

  SkillNodeDefinition node;
  node.Id = AbilityNode;
  node.Category = SkillNodeCategory::Major;
  node.PointCost = 1U;
  node.RequiredClass = Knight;
  node.AttributeModifiers = {{Stats::CombatAttributes::AttackPower,
                              Stats::ModifierOperation::Additive, 4.0}};
  node.UnlockAbility = KnightAbility;
  tree.Nodes = {node};
  return tree;
}

struct RuntimeFixture final {
  ItemCatalog Items;
  LootCatalog LootTables;
  Combat::Combatant Owner;
  EffectRuntime Effects;
  Core::DeterministicRandom Random;
  AbilityRuntime Abilities;
  AbilityLoadout Loadout;
  SkillTreeRuntime Skills;
  Inventory InventoryStateAuthority;
  EquipmentRuntime Equipment;
  LootRuntime Loot;
  CharacterPersistence Persistence;

  RuntimeFixture()
      : Items{BuildCatalog()}, LootTables{BuildLootCatalog(Items)},
        Owner{Combat::CombatantId{1000U}, Combat::CombatantKind::Player},
        Effects{Owner, EffectDefinitions()}, Random{42U, 9U},
        Abilities{Owner, Effects, Random, Knight, AbilityDefinitions()},
        Loadout{Abilities},
        Skills{Owner, Abilities, Loadout, TreeDefinition(), 3U},
        InventoryStateAuthority{Items, 16U}, Equipment{Items, Owner, Knight},
        Loot{Items, LootTables, Random},
        Persistence{Character, Knight, Owner, Random, Effects, Abilities,
                    Skills, Loadout, InventoryStateAuthority, Equipment, Loot} {
  }

  [[nodiscard]] bool PopulatePersistentState() {
    if (!Persistence.IsValid() ||
        !Owner.SetBaseAttribute(Stats::CombatAttributes::AttackPower, 0.1) ||
        Skills.Allocate(AbilityNode) != SkillOperationResult::Success ||
        Loadout.Equip(AbilityLoadoutSlot::Active1, KnightAbility) !=
            LoadoutResult::Success ||
        InventoryStateAuthority.AddStack(Potion, 7U) !=
            InventoryResult::Success ||
        InventoryStateAuthority.AddInstance(
            CommonInstance(ItemInstanceId{100U}, Sword)) !=
            InventoryResult::Success ||
        InventoryStateAuthority.AddInstance(
            CommonInstance(ItemInstanceId{200U}, Helmet)) !=
            InventoryResult::Success ||
        Equipment.EquipFromInventory(InventoryStateAuthority,
                                     ItemInstanceId{100U}, MainHand) !=
            EquipmentResult::Success ||
        Effects.Apply(PersistentBuff, Owner.Id()).Result !=
            EffectApplyResult::Applied ||
        !Loot.RestoreState(LootRuntimeState{1000U})) {
      return false;
    }
    static_cast<void>(Owner.ApplyDamage(12.5));
    static_cast<void>(Owner.TryConsumeResource(7.25));
    static_cast<void>(Random.NextUInt32());
    return true;
  }
};

} // namespace Lostsense::Gameplay::PersistenceTestData
