#include "Lostsense/Gameplay/Content/FirstSliceContent.h"

#include "Lostsense/Stats/CombatAttributes.h"

namespace Lostsense::Gameplay::FirstSlice {
namespace {

constexpr AffixExclusiveGroupId OffensiveAffixGroup{12001U};
constexpr AffixExclusiveGroupId DefensiveAffixGroup{12002U};
constexpr AffixExclusiveGroupId GuardAffixGroup{12003U};
constexpr SkillExclusiveGroupId OathChoiceGroup{13001U};

[[nodiscard]] AbilityDefinition
MakeAttack(const AbilityId id, const AbilityLoadoutSlot slot,
           const double resourceCost, const double cooldownSeconds,
           const double basePhysical, const double attackPowerCoefficient) {
  AbilityDefinition ability;
  ability.Id = id;
  ability.ResourceCost = resourceCost;
  ability.CooldownSeconds = cooldownSeconds;
  ability.TargetRule = AbilityTargetRule::Hostile;
  ability.RequiredClass = KnightClass;
  ability.AllowedLoadoutSlots = AbilityLoadoutSlotBit(slot);
  ability.Damage
      .BaseDamage[static_cast<std::size_t>(Combat::DamageType::Physical)] =
      basePhysical;
  ability.Damage.AttackPowerCoefficients[static_cast<std::size_t>(
      Combat::DamageType::Physical)] = attackPowerCoefficient;
  ability.DealsDamage = true;
  return ability;
}

[[nodiscard]] ItemDefinition
MakeKnightWeapon(const ItemId id, const WeaponTypeId weaponType,
                 const std::vector<EquipmentSlotId> &slots,
                 const double attackPower) {
  ItemDefinition item;
  item.Id = id;
  item.Type = WeaponItem;
  item.WeaponType = weaponType;
  item.AllowedEquipmentSlots = slots;
  item.AllowedRarities = {CommonRarity, RareRarity, LegendaryRarity};
  item.MinimumItemLevel = 1U;
  item.MaximumItemLevel = 100U;
  item.AllowedClasses = {KnightClass};
  item.BaseModifiers = {{Stats::CombatAttributes::AttackPower,
                         Stats::ModifierOperation::Additive, attackPower}};
  return item;
}

} // namespace

std::vector<EffectDefinition> BuildKnightEffects() {
  EffectDefinition bellstep;
  bellstep.Id = BellstepArmor;
  bellstep.StackGroup = EffectStackGroupId{50120U};
  bellstep.Stacking = EffectStackingPolicy::RefreshDuration;
  bellstep.MaxStacks = 1U;
  bellstep.DurationSeconds = 0.6;
  bellstep.Beneficial = true;
  bellstep.AttributeModifiers = {{Stats::CombatAttributes::Armor,
                                  Stats::ModifierOperation::Additive, 8.0}};

  EffectDefinition guard;
  guard.Id = GuardStance;
  guard.StackGroup = EffectStackGroupId{50121U};
  guard.Stacking = EffectStackingPolicy::RefreshDuration;
  guard.MaxStacks = 1U;
  guard.DurationSeconds = 0.25;
  guard.Beneficial = true;
  guard.AttributeModifiers = {{Stats::CombatAttributes::BlockChance,
                               Stats::ModifierOperation::Additive, 0.30},
                              {Stats::CombatAttributes::BlockMitigation,
                               Stats::ModifierOperation::Additive, 0.15}};

  EffectDefinition answering;
  answering.Id = AnsweringGuardWindow;
  answering.StackGroup = EffectStackGroupId{50123U};
  answering.Stacking = EffectStackingPolicy::RefreshDuration;
  answering.MaxStacks = 1U;
  answering.DurationSeconds = 2.0;
  answering.Beneficial = true;
  answering.AttributeModifiers = {{Stats::CombatAttributes::Armor,
                                   Stats::ModifierOperation::Additive, 12.0},
                                  {Stats::CombatAttributes::BlockChance,
                                   Stats::ModifierOperation::Additive, 0.20}};

  return {bellstep, guard, answering};
}

std::vector<AbilityDefinition> BuildKnightAbilities() {
  AbilityDefinition primary =
      MakeAttack(KnightPrimaryAttack, AbilityLoadoutSlot::PrimaryAttack, 0.0,
                 0.25, 8.0, 0.90);
  AbilityDefinition heavy =
      MakeAttack(KnightHeavyAttack, AbilityLoadoutSlot::SecondaryAttack, 8.0,
                 0.75, 15.0, 1.35);

  AbilityDefinition dodge;
  dodge.Id = KnightDodge;
  dodge.ResourceCost = 5.0;
  dodge.CooldownSeconds = 0.65;
  dodge.TargetRule = AbilityTargetRule::None;
  dodge.RequiredClass = KnightClass;
  dodge.AllowedLoadoutSlots = AbilityLoadoutSlotBit(AbilityLoadoutSlot::Dodge);

  AbilityDefinition guard;
  guard.Id = KnightGuard;
  guard.TargetRule = AbilityTargetRule::None;
  guard.RequiredClass = KnightClass;
  guard.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::ClassMechanic);
  guard.EffectsOnSelf = {GuardStance};

  AbilityDefinition bellstep =
      MakeAttack(Bellstep, AbilityLoadoutSlot::Active1, 10.0, 3.0, 6.0, 0.70);
  bellstep.EffectsOnSelf = {BellstepArmor};

  AbilityDefinition breaker = MakeAttack(
      MeasureBreaker, AbilityLoadoutSlot::Active2, 20.0, 5.0, 18.0, 1.55);
  breaker.Prerequisites = {Bellstep};

  AbilityDefinition sweep = MakeAttack(MarchSweep, AbilityLoadoutSlot::Active3,
                                       14.0, 4.0, 10.0, 0.95);
  sweep.Prerequisites = {Bellstep};
  sweep.MaximumTargets = 8U;

  AbilityDefinition answering;
  answering.Id = AnsweringGuard;
  answering.ResourceCost = 12.0;
  answering.CooldownSeconds = 7.0;
  answering.TargetRule = AbilityTargetRule::None;
  answering.RequiredClass = KnightClass;
  answering.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::Active4);
  answering.Prerequisites = {Bellstep};
  answering.EffectsOnSelf = {AnsweringGuardWindow};

  return {primary, heavy, dodge, guard, bellstep, breaker, sweep, answering};
}

SkillTreeDefinition BuildKnightScarAtlas() {
  SkillTreeDefinition tree;
  tree.Id = KnightScarAtlas;
  tree.SupportedClasses = {KnightClass};
  tree.ExclusiveGroups = {{OathChoiceGroup, 1U}};

  SkillNodeDefinition root;
  root.Id = FirstMeasure;
  root.Category = SkillNodeCategory::Minor;
  root.RequiredClass = KnightClass;
  root.AttributeModifiers = {{Stats::CombatAttributes::AttackPower,
                              Stats::ModifierOperation::Additive, 1.0}};

  SkillNodeDefinition bellstep;
  bellstep.Id = BellstepNode;
  bellstep.Category = SkillNodeCategory::Major;
  bellstep.RequiredClass = KnightClass;
  bellstep.Prerequisites = {FirstMeasure};
  bellstep.UnlockAbility = Bellstep;

  SkillNodeDefinition resolve;
  resolve.Id = ResolveWell;
  resolve.Category = SkillNodeCategory::Minor;
  resolve.RequiredClass = KnightClass;
  resolve.Prerequisites = {FirstMeasure};
  resolve.AttributeModifiers = {{Stats::CombatAttributes::MaxResource,
                                 Stats::ModifierOperation::Additive, 10.0}};

  SkillNodeDefinition breaker;
  breaker.Id = MeasureBreakerNode;
  breaker.Category = SkillNodeCategory::Major;
  breaker.RequiredClass = KnightClass;
  breaker.Prerequisites = {BellstepNode};
  breaker.UnlockAbility = MeasureBreaker;

  SkillNodeDefinition guardDensity;
  guardDensity.Id = GuardDensity;
  guardDensity.Category = SkillNodeCategory::Minor;
  guardDensity.RequiredClass = KnightClass;
  guardDensity.Prerequisites = {ResolveWell};
  guardDensity.AttributeModifiers = {{Stats::CombatAttributes::Armor,
                                      Stats::ModifierOperation::Additive, 4.0}};

  SkillNodeDefinition sweep;
  sweep.Id = MarchSweepNode;
  sweep.Category = SkillNodeCategory::Major;
  sweep.RequiredClass = KnightClass;
  sweep.Prerequisites = {BellstepNode};
  sweep.UnlockAbility = MarchSweep;

  SkillNodeDefinition answering;
  answering.Id = AnsweringGuardNode;
  answering.Category = SkillNodeCategory::ClassMechanic;
  answering.RequiredClass = KnightClass;
  answering.Prerequisites = {GuardDensity};
  answering.UnlockAbility = AnsweringGuard;

  SkillNodeDefinition stand;
  stand.Id = StandWhereItFallsNode;
  stand.Category = SkillNodeCategory::Keystone;
  stand.RequiredClass = KnightClass;
  stand.Prerequisites = {AnsweringGuardNode};
  stand.ExclusiveGroup = OathChoiceGroup;
  stand.AbilityMutations = {StandWhereItFalls};
  stand.AttributeModifiers = {{Stats::CombatAttributes::Armor,
                               Stats::ModifierOperation::Additive, 5.0}};

  SkillNodeDefinition hollow;
  hollow.Id = HollowMeasureNode;
  hollow.Category = SkillNodeCategory::Keystone;
  hollow.RequiredClass = KnightClass;
  hollow.Prerequisites = {AnsweringGuardNode};
  hollow.ExclusiveGroup = OathChoiceGroup;
  hollow.AbilityMutations = {HollowMeasure};
  hollow.AttributeModifiers = {{Stats::CombatAttributes::DamageBonusPercent,
                                Stats::ModifierOperation::Additive, 8.0}};

  SkillNodeDefinition heavy;
  heavy.Id = HeavyMeasure;
  heavy.Category = SkillNodeCategory::Minor;
  heavy.RequiredClass = KnightClass;
  heavy.Prerequisites = {MeasureBreakerNode};
  heavy.AttributeModifiers = {{Stats::CombatAttributes::AttackPower,
                               Stats::ModifierOperation::Additive, 3.0}};

  SkillNodeDefinition vitality;
  vitality.Id = BellwardVitality;
  vitality.Category = SkillNodeCategory::Minor;
  vitality.RequiredClass = KnightClass;
  vitality.Prerequisites = {GuardDensity};
  vitality.AttributeModifiers = {{Stats::CombatAttributes::MaxHealth,
                                  Stats::ModifierOperation::Additive, 10.0}};

  SkillNodeDefinition clapper;
  clapper.Id = ClappersReturnNode;
  clapper.Category = SkillNodeCategory::Transformation;
  clapper.RequiredClass = KnightClass;
  clapper.Prerequisites = {AnsweringGuardNode};
  clapper.AbilityMutations = {ClappersReturn};

  tree.Nodes = {root,      bellstep, resolve, breaker, guardDensity, sweep,
                answering, stand,    hollow,  heavy,   vitality,     clapper};
  return tree;
}

ItemCatalog BuildItemCatalog() {
  ItemCatalog catalog;
  static_cast<void>(catalog.AddRarity({CommonRarity, 1U, 100U, 0U, 0U}));
  static_cast<void>(catalog.AddRarity({RareRarity, 1U, 100U, 1U, 1U}));
  static_cast<void>(catalog.AddRarity({LegendaryRarity, 1U, 100U, 2U, 2U}));
  static_cast<void>(catalog.AddRarity({BossUniqueRarity, 1U, 100U, 0U, 0U}));

  AffixDefinition edge;
  edge.Id = MeasuredEdge;
  edge.ExclusiveGroup = OffensiveAffixGroup;
  edge.Kind = AffixKind::Prefix;
  edge.Weight = 8U;
  edge.MinimumItemLevel = 1U;
  edge.MaximumItemLevel = 100U;
  edge.AllowedItemTypes = {WeaponItem};
  edge.AllowedClasses = {KnightClass};
  edge.Modifiers = {{Stats::CombatAttributes::AttackPower,
                     Stats::ModifierOperation::Additive, 2.0, 5.0}};
  static_cast<void>(catalog.AddAffix(edge));

  AffixDefinition plating;
  plating.Id = BellwardPlating;
  plating.ExclusiveGroup = DefensiveAffixGroup;
  plating.Kind = AffixKind::Suffix;
  plating.Weight = 7U;
  plating.MinimumItemLevel = 1U;
  plating.MaximumItemLevel = 100U;
  plating.AllowedItemTypes = {WeaponItem, ArmorItem};
  plating.AllowedClasses = {KnightClass};
  plating.Modifiers = {{Stats::CombatAttributes::Armor,
                        Stats::ModifierOperation::Additive, 3.0, 7.0}};
  static_cast<void>(catalog.AddAffix(plating));

  AffixDefinition grip;
  grip.Id = ResoluteGrip;
  grip.ExclusiveGroup = GuardAffixGroup;
  grip.Kind = AffixKind::Suffix;
  grip.Weight = 5U;
  grip.MinimumItemLevel = 1U;
  grip.MaximumItemLevel = 100U;
  grip.AllowedItemTypes = {WeaponItem};
  grip.AllowedClasses = {KnightClass};
  grip.Modifiers = {{Stats::CombatAttributes::BlockChance,
                     Stats::ModifierOperation::Additive, 0.01, 0.04}};
  static_cast<void>(catalog.AddAffix(grip));

  AffixDefinition frame;
  frame.Id = StalwartFrame;
  frame.ExclusiveGroup = GuardAffixGroup;
  frame.Kind = AffixKind::Prefix;
  frame.Weight = 5U;
  frame.MinimumItemLevel = 1U;
  frame.MaximumItemLevel = 100U;
  frame.AllowedItemTypes = {ArmorItem};
  frame.AllowedClasses = {KnightClass};
  frame.Modifiers = {{Stats::CombatAttributes::MaxHealth,
                      Stats::ModifierOperation::Additive, 5.0, 12.0}};
  static_cast<void>(catalog.AddAffix(frame));

  ItemDefinition gravesong =
      MakeKnightWeapon(GravesongLongsword, LongswordWeapon, {MainHand}, 8.0);
  static_cast<void>(catalog.AddItem(gravesong));

  ItemDefinition sabre =
      MakeKnightWeapon(BellguardSabre, SabreWeapon, {MainHand}, 6.0);
  sabre.BaseModifiers.push_back({Stats::CombatAttributes::BlockChance,
                                 Stats::ModifierOperation::Additive, 0.03});
  static_cast<void>(catalog.AddItem(sabre));

  ItemDefinition spear =
      MakeKnightWeapon(MarchSpear, SpearWeapon, {MainHand}, 10.0);
  static_cast<void>(catalog.AddItem(spear));

  ItemDefinition shield =
      MakeKnightWeapon(TonguelessShield, ShieldWeapon, {OffHand}, 2.0);
  shield.BaseModifiers.push_back({Stats::CombatAttributes::Armor,
                                  Stats::ModifierOperation::Additive, 12.0});
  shield.BaseModifiers.push_back({Stats::CombatAttributes::BlockChance,
                                  Stats::ModifierOperation::Additive, 0.10});
  shield.BaseModifiers.push_back({Stats::CombatAttributes::BlockMitigation,
                                  Stats::ModifierOperation::Additive, 0.10});
  static_cast<void>(catalog.AddItem(shield));

  ItemDefinition clapper =
      MakeKnightWeapon(OdransClapper, BellMaulWeapon, {MainHand}, 18.0);
  clapper.AllowedRarities.clear();
  clapper.FixedRarity = BossUniqueRarity;
  clapper.Unique = true;
  clapper.BaseModifiers.push_back({Stats::CombatAttributes::ArmorPenetration,
                                   Stats::ModifierOperation::Additive, 6.0});
  clapper.AbilityMutations = {ClappersReturn};
  static_cast<void>(catalog.AddItem(clapper));

  ItemDefinition cuirass;
  cuirass.Id = BellwardenCuirass;
  cuirass.Type = ArmorItem;
  cuirass.AllowedEquipmentSlots = {Chest};
  cuirass.AllowedRarities = {CommonRarity, RareRarity, LegendaryRarity};
  cuirass.MinimumItemLevel = 1U;
  cuirass.MaximumItemLevel = 100U;
  cuirass.AllowedClasses = {KnightClass};
  cuirass.BaseModifiers = {{Stats::CombatAttributes::Armor,
                            Stats::ModifierOperation::Additive, 18.0}};
  static_cast<void>(catalog.AddItem(cuirass));

  ItemDefinition thread;
  thread.Id = VaurGoldThread;
  thread.Type = MaterialItem;
  thread.MinimumItemLevel = 1U;
  thread.MaximumItemLevel = 100U;
  thread.MaximumStackSize = 50U;
  static_cast<void>(catalog.AddItem(thread));

  return catalog;
}

LootCatalog BuildLootCatalog() {
  LootCatalog catalog;

  LootTableDefinition enemy;
  enemy.Id = VaurEnemyLoot;
  enemy.Rolls = 1U;
  enemy.Entries = {{GravesongLongsword, 28U, 1U, 1U, {KnightClass}},
                   {BellguardSabre, 18U, 1U, 1U, {KnightClass}},
                   {MarchSpear, 18U, 1U, 1U, {KnightClass}},
                   {BellwardenCuirass, 18U, 1U, 1U, {KnightClass}},
                   {VaurGoldThread, 18U, 1U, 3U, {}}};
  enemy.Rarities = {
      {CommonRarity, 65U}, {RareRarity, 30U}, {LegendaryRarity, 5U}};
  static_cast<void>(catalog.AddTable(enemy));

  LootTableDefinition elite;
  elite.Id = VaurEliteLoot;
  elite.Rolls = 2U;
  elite.Entries = {{GravesongLongsword, 20U, 1U, 1U, {KnightClass}},
                   {BellguardSabre, 20U, 1U, 1U, {KnightClass}},
                   {MarchSpear, 20U, 1U, 1U, {KnightClass}},
                   {TonguelessShield, 20U, 1U, 1U, {KnightClass}},
                   {BellwardenCuirass, 20U, 1U, 1U, {KnightClass}}};
  elite.Rarities = {
      {CommonRarity, 25U}, {RareRarity, 55U}, {LegendaryRarity, 20U}};
  static_cast<void>(catalog.AddTable(elite));

  LootTableDefinition odran;
  odran.Id = OdranBossLoot;
  odran.Rolls = 1U;
  odran.Entries = {{OdransClapper, 1U, 1U, 1U, {KnightClass}}};
  static_cast<void>(catalog.AddTable(odran));

  return catalog;
}

} // namespace Lostsense::Gameplay::FirstSlice
