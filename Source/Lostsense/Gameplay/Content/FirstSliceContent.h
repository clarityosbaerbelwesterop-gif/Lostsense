#pragma once

#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"
#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Gameplay/Items/LootRuntime.h"
#include "Lostsense/Gameplay/Progression/SkillGraph.h"

#include <vector>

namespace Lostsense::Gameplay::FirstSlice {

inline constexpr ClassId KnightClass{102U};
inline constexpr SkillTreeId KnightScarAtlas{102U};

inline constexpr AbilityId KnightPrimaryAttack{40110U};
inline constexpr AbilityId KnightHeavyAttack{40111U};
inline constexpr AbilityId KnightDodge{40112U};
inline constexpr AbilityId KnightGuard{40113U};
inline constexpr AbilityId Bellstep{40120U};
inline constexpr AbilityId MeasureBreaker{40121U};
inline constexpr AbilityId MarchSweep{40122U};
inline constexpr AbilityId AnsweringGuard{40123U};

inline constexpr EffectId BellstepArmor{50120U};
inline constexpr EffectId GuardStance{50121U};
inline constexpr EffectId AnsweringGuardWindow{50123U};

inline constexpr AbilityMutationId StandWhereItFalls{60120U};
inline constexpr AbilityMutationId HollowMeasure{60121U};
inline constexpr AbilityMutationId ClappersReturn{60128U};

inline constexpr SkillNodeId FirstMeasure{41001U};
inline constexpr SkillNodeId BellstepNode{41002U};
inline constexpr SkillNodeId ResolveWell{41003U};
inline constexpr SkillNodeId MeasureBreakerNode{41004U};
inline constexpr SkillNodeId GuardDensity{41005U};
inline constexpr SkillNodeId MarchSweepNode{41006U};
inline constexpr SkillNodeId AnsweringGuardNode{41007U};
inline constexpr SkillNodeId StandWhereItFallsNode{41008U};
inline constexpr SkillNodeId HollowMeasureNode{41009U};
inline constexpr SkillNodeId HeavyMeasure{41010U};
inline constexpr SkillNodeId BellwardVitality{41011U};
inline constexpr SkillNodeId ClappersReturnNode{41012U};

inline constexpr ItemTypeId WeaponItem{201U};
inline constexpr ItemTypeId ArmorItem{202U};
inline constexpr ItemTypeId MaterialItem{203U};
inline constexpr WeaponTypeId LongswordWeapon{301U};
inline constexpr WeaponTypeId SabreWeapon{302U};
inline constexpr WeaponTypeId SpearWeapon{303U};
inline constexpr WeaponTypeId ShieldWeapon{304U};
inline constexpr WeaponTypeId BellMaulWeapon{305U};

inline constexpr EquipmentSlotId MainHand{1U};
inline constexpr EquipmentSlotId OffHand{2U};
inline constexpr EquipmentSlotId Chest{4U};

inline constexpr RarityId CommonRarity{201U};
inline constexpr RarityId RareRarity{202U};
inline constexpr RarityId LegendaryRarity{203U};
inline constexpr RarityId BossUniqueRarity{204U};

inline constexpr AffixId MeasuredEdge{11001U};
inline constexpr AffixId BellwardPlating{11002U};
inline constexpr AffixId ResoluteGrip{11003U};
inline constexpr AffixId StalwartFrame{11004U};

inline constexpr ItemId GravesongLongsword{10021U};
inline constexpr ItemId BellguardSabre{10022U};
inline constexpr ItemId MarchSpear{10026U};
inline constexpr ItemId TonguelessShield{10027U};
inline constexpr ItemId OdransClapper{10028U};
inline constexpr ItemId BellwardenCuirass{10201U};
inline constexpr ItemId VaurGoldThread{10901U};

inline constexpr LootTableId VaurEnemyLoot{7001U};
inline constexpr LootTableId VaurEliteLoot{7002U};
inline constexpr LootTableId OdranBossLoot{7003U};

[[nodiscard]] std::vector<EffectDefinition> BuildKnightEffects();
[[nodiscard]] std::vector<AbilityDefinition> BuildKnightAbilities();
[[nodiscard]] SkillTreeDefinition BuildKnightScarAtlas();
[[nodiscard]] ItemCatalog BuildItemCatalog();
[[nodiscard]] LootCatalog BuildLootCatalog();

} // namespace Lostsense::Gameplay::FirstSlice
