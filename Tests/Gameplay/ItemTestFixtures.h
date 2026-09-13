#pragma once

#include "Lostsense/Gameplay/Items/LootRuntime.h"
#include "Lostsense/Stats/CombatAttributes.h"

namespace Lostsense::Gameplay::TestData {

inline constexpr ClassId Knight{1U};
inline constexpr ClassId Wizard{2U};

inline constexpr ItemTypeId WeaponItem{1U};
inline constexpr ItemTypeId ArmorItem{2U};
inline constexpr ItemTypeId ConsumableItem{3U};
inline constexpr ItemTypeId SocketItem{4U};
inline constexpr WeaponTypeId SwordWeapon{1U};

inline constexpr EquipmentSlotId MainHand{1U};
inline constexpr EquipmentSlotId Head{2U};
inline constexpr EquipmentSlotId RingLeft{3U};
inline constexpr EquipmentSlotId RingRight{4U};

inline constexpr RarityId Common{1U};
inline constexpr RarityId Magic{2U};
inline constexpr RarityId UniqueRarity{3U};

inline constexpr AffixId AttackAffix{1U};
inline constexpr AffixId AlternateAttackAffix{2U};
inline constexpr AffixId ArmorAffix{3U};
inline constexpr AffixId WizardAffix{4U};
inline constexpr AffixExclusiveGroupId OffensiveGroup{1U};
inline constexpr AffixExclusiveGroupId DefensiveGroup{2U};
inline constexpr AffixExclusiveGroupId ClassGroup{3U};

inline constexpr ItemId Sword{1U};
inline constexpr ItemId Helmet{2U};
inline constexpr ItemId Potion{3U};
inline constexpr ItemId Gem{4U};
inline constexpr ItemId KnightBlade{5U};
inline constexpr ItemId UniqueBlade{6U};
inline constexpr ItemId Ring{7U};

inline constexpr EffectId UniqueEffect{900U};
inline constexpr AbilityMutationId UniqueMutation{901U};

inline constexpr LootTableId MixedTable{1U};
inline constexpr LootTableId SwordOnlyTable{2U};
inline constexpr LootTableId StackTable{3U};
inline constexpr LootTableId MagicSwordTable{4U};
inline constexpr LootTableId UniqueTable{5U};
inline constexpr LootTableId KnightOnlyTable{6U};

inline ItemCatalog BuildCatalog() {
  ItemCatalog catalog;

  static_cast<void>(catalog.AddRarity({Common, 1U, 100U, 0U, 0U}));
  static_cast<void>(catalog.AddRarity({Magic, 1U, 100U, 2U, 2U}));
  static_cast<void>(catalog.AddRarity({UniqueRarity, 1U, 100U, 0U, 0U}));

  AffixDefinition attack;
  attack.Id = AttackAffix;
  attack.ExclusiveGroup = OffensiveGroup;
  attack.Kind = AffixKind::Prefix;
  attack.Weight = 5U;
  attack.MinimumItemLevel = 1U;
  attack.MaximumItemLevel = 100U;
  attack.AllowedItemTypes = {WeaponItem};
  attack.Modifiers = {{Stats::CombatAttributes::AttackPower,
                       Stats::ModifierOperation::Additive, 4.0, 8.0}};
  static_cast<void>(catalog.AddAffix(attack));

  AffixDefinition alternate = attack;
  alternate.Id = AlternateAttackAffix;
  alternate.Weight = 1U;
  alternate.Modifiers = {{Stats::CombatAttributes::AttackPower,
                          Stats::ModifierOperation::Additive, 10.0, 12.0}};
  static_cast<void>(catalog.AddAffix(alternate));

  AffixDefinition armor;
  armor.Id = ArmorAffix;
  armor.ExclusiveGroup = DefensiveGroup;
  armor.Kind = AffixKind::Suffix;
  armor.Weight = 4U;
  armor.MinimumItemLevel = 1U;
  armor.MaximumItemLevel = 100U;
  armor.AllowedItemTypes = {WeaponItem, ArmorItem};
  armor.Modifiers = {{Stats::CombatAttributes::Armor,
                      Stats::ModifierOperation::Additive, 2.0, 5.0}};
  static_cast<void>(catalog.AddAffix(armor));

  AffixDefinition wizard = attack;
  wizard.Id = WizardAffix;
  wizard.ExclusiveGroup = ClassGroup;
  wizard.AllowedClasses = {Wizard};
  wizard.Modifiers = {{Stats::CombatAttributes::SpellPower,
                       Stats::ModifierOperation::Additive, 7.0, 9.0}};
  static_cast<void>(catalog.AddAffix(wizard));

  ItemDefinition sword;
  sword.Id = Sword;
  sword.Type = WeaponItem;
  sword.WeaponType = SwordWeapon;
  sword.AllowedEquipmentSlots = {MainHand};
  sword.AllowedRarities = {Common, Magic};
  sword.MinimumItemLevel = 1U;
  sword.MaximumItemLevel = 100U;
  sword.SocketCapacity = 1U;
  sword.BaseModifiers = {{Stats::CombatAttributes::AttackPower,
                          Stats::ModifierOperation::Additive, 3.0}};
  static_cast<void>(catalog.AddItem(sword));

  ItemDefinition helmet;
  helmet.Id = Helmet;
  helmet.Type = ArmorItem;
  helmet.AllowedEquipmentSlots = {Head};
  helmet.AllowedRarities = {Common, Magic};
  helmet.MinimumItemLevel = 1U;
  helmet.MaximumItemLevel = 100U;
  helmet.BaseModifiers = {{Stats::CombatAttributes::Armor,
                           Stats::ModifierOperation::Additive, 5.0}};
  static_cast<void>(catalog.AddItem(helmet));

  ItemDefinition potion;
  potion.Id = Potion;
  potion.Type = ConsumableItem;
  potion.MinimumItemLevel = 1U;
  potion.MaximumItemLevel = 100U;
  potion.MaximumStackSize = 20U;
  static_cast<void>(catalog.AddItem(potion));

  ItemDefinition gem;
  gem.Id = Gem;
  gem.Type = SocketItem;
  gem.MinimumItemLevel = 1U;
  gem.MaximumItemLevel = 100U;
  gem.MaximumStackSize = 20U;
  gem.CanBeSocketed = true;
  gem.BaseModifiers = {{Stats::CombatAttributes::AttackPower,
                        Stats::ModifierOperation::Additive, 2.0}};
  static_cast<void>(catalog.AddItem(gem));

  ItemDefinition knightBlade = sword;
  knightBlade.Id = KnightBlade;
  knightBlade.AllowedClasses = {Knight};
  knightBlade.BaseModifiers = {{Stats::CombatAttributes::AttackPower,
                                Stats::ModifierOperation::Additive, 7.0}};
  static_cast<void>(catalog.AddItem(knightBlade));

  ItemDefinition unique = sword;
  unique.Id = UniqueBlade;
  unique.AllowedRarities = {UniqueRarity};
  unique.FixedRarity = UniqueRarity;
  unique.Unique = true;
  unique.AllowedClasses = {Knight};
  unique.BaseModifiers = {{Stats::CombatAttributes::AttackPower,
                           Stats::ModifierOperation::Additive, 15.0}};
  unique.UniqueEffects = {UniqueEffect};
  unique.AbilityMutations = {UniqueMutation};
  static_cast<void>(catalog.AddItem(unique));

  ItemDefinition ring;
  ring.Id = Ring;
  ring.Type = ArmorItem;
  ring.AllowedEquipmentSlots = {RingLeft, RingRight};
  ring.AllowedRarities = {Common};
  ring.MinimumItemLevel = 1U;
  ring.MaximumItemLevel = 100U;
  ring.BaseModifiers = {{Stats::CombatAttributes::Armor,
                         Stats::ModifierOperation::Additive, 1.0}};
  static_cast<void>(catalog.AddItem(ring));

  return catalog;
}

inline LootCatalog BuildLootCatalog(const ItemCatalog &items) {
  static_cast<void>(items);
  LootCatalog catalog;

  LootTableDefinition mixed;
  mixed.Id = MixedTable;
  mixed.Entries = {{Sword, 5U, 1U, 1U, {}},
                   {Helmet, 2U, 1U, 1U, {}},
                   {Potion, 3U, 1U, 4U, {}}};
  mixed.Rarities = {{Common, 1U}, {Magic, 4U}};
  static_cast<void>(catalog.AddTable(mixed));

  LootTableDefinition swordOnly;
  swordOnly.Id = SwordOnlyTable;
  swordOnly.Entries = {{Sword, 1U, 1U, 1U, {}}};
  swordOnly.Rarities = {{Common, 1U}};
  static_cast<void>(catalog.AddTable(swordOnly));

  LootTableDefinition stack;
  stack.Id = StackTable;
  stack.Entries = {{Potion, 1U, 2U, 5U, {}}};
  static_cast<void>(catalog.AddTable(stack));

  LootTableDefinition magicSword;
  magicSword.Id = MagicSwordTable;
  magicSword.Entries = {{Sword, 1U, 1U, 1U, {}}};
  magicSword.Rarities = {{Magic, 1U}};
  static_cast<void>(catalog.AddTable(magicSword));

  LootTableDefinition unique;
  unique.Id = UniqueTable;
  unique.Entries = {{UniqueBlade, 1U, 1U, 1U, {Knight}}};
  static_cast<void>(catalog.AddTable(unique));

  LootTableDefinition knight;
  knight.Id = KnightOnlyTable;
  knight.Entries = {{KnightBlade, 1U, 1U, 1U, {Knight}}};
  knight.Rarities = {{Common, 1U}};
  static_cast<void>(catalog.AddTable(knight));

  return catalog;
}

inline ItemInstance CommonInstance(const ItemInstanceId instance,
                                   const ItemId item,
                                   const std::uint32_t level = 10U) {
  return ItemInstance{instance, item, level, Common, {}, {}};
}

} // namespace Lostsense::Gameplay::TestData
