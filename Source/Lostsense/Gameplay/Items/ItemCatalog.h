#pragma once

#include "Lostsense/Gameplay/GameplayTypes.h"
#include "Lostsense/Stats/AttributeSet.h"

#include <cstdint>
#include <map>
#include <vector>

namespace Lostsense::Gameplay {

inline constexpr std::uint64_t MaximumPersistentItemInstanceId =
    (1ULL << 45U) - 1ULL;

struct ItemAttributeModifier final {
  Stats::AttributeId Attribute{};
  Stats::ModifierOperation Operation{Stats::ModifierOperation::Additive};
  double Magnitude{0.0};
};

struct AffixModifierRollDefinition final {
  Stats::AttributeId Attribute{};
  Stats::ModifierOperation Operation{Stats::ModifierOperation::Additive};
  double MinimumMagnitude{0.0};
  double MaximumMagnitude{0.0};
};

enum class AffixKind : std::uint8_t { Prefix, Suffix };

struct RarityDefinition final {
  RarityId Id{};
  std::uint32_t MinimumItemLevel{1U};
  std::uint32_t MaximumItemLevel{1U};
  std::uint8_t MinimumAffixes{0U};
  std::uint8_t MaximumAffixes{0U};
};

struct AffixDefinition final {
  AffixId Id{};
  AffixExclusiveGroupId ExclusiveGroup{};
  AffixKind Kind{AffixKind::Prefix};
  std::uint32_t Weight{1U};
  std::uint32_t MinimumItemLevel{1U};
  std::uint32_t MaximumItemLevel{1U};
  std::vector<ItemTypeId> AllowedItemTypes{};
  std::vector<WeaponTypeId> AllowedWeaponTypes{};
  std::vector<EquipmentSlotId> AllowedEquipmentSlots{};
  std::vector<ClassId> AllowedClasses{};
  std::vector<AffixModifierRollDefinition> Modifiers{};
};

struct ItemDefinition final {
  ItemId Id{};
  ItemTypeId Type{};
  WeaponTypeId WeaponType{};
  std::vector<EquipmentSlotId> AllowedEquipmentSlots{};
  std::vector<RarityId> AllowedRarities{};
  RarityId FixedRarity{};
  std::uint32_t MinimumItemLevel{1U};
  std::uint32_t MaximumItemLevel{1U};
  std::uint32_t MaximumStackSize{1U};
  std::uint8_t SocketCapacity{0U};
  bool Unique{false};
  bool CanBeSocketed{false};
  std::vector<ClassId> AllowedClasses{};
  std::vector<ItemAttributeModifier> BaseModifiers{};
  std::vector<GameplayTagId> Tags{};
  std::vector<EffectId> UniqueEffects{};
  std::vector<AbilityMutationId> AbilityMutations{};
};

struct RolledAffix final {
  AffixId Id{};
  std::vector<double> Magnitudes{};
};

struct ItemInstance final {
  ItemInstanceId InstanceId{};
  ItemId DefinitionId{};
  std::uint32_t ItemLevel{1U};
  RarityId Rarity{};
  std::vector<RolledAffix> Affixes{};
  std::vector<ItemId> SocketedItems{};
};

struct ResolvedItemModifier final {
  Stats::AttributeId Attribute{};
  Stats::ModifierOperation Operation{Stats::ModifierOperation::Additive};
  double Magnitude{0.0};
};

class ItemCatalog final {
public:
  [[nodiscard]] bool AddRarity(RarityDefinition definition);
  [[nodiscard]] bool AddAffix(AffixDefinition definition);
  [[nodiscard]] bool AddItem(ItemDefinition definition);
  [[nodiscard]] bool Validate() const noexcept;

  [[nodiscard]] const RarityDefinition *FindRarity(RarityId id) const noexcept;
  [[nodiscard]] const AffixDefinition *FindAffix(AffixId id) const noexcept;
  [[nodiscard]] const ItemDefinition *FindItem(ItemId id) const noexcept;

  [[nodiscard]] bool IsItemAllowedForClass(const ItemDefinition &definition,
                                           ClassId ownerClass) const noexcept;
  [[nodiscard]] bool IsRarityAllowed(const ItemDefinition &item,
                                     RarityId rarity) const noexcept;
  [[nodiscard]] bool IsAffixEligible(const AffixDefinition &affix,
                                     const ItemDefinition &item,
                                     std::uint32_t itemLevel,
                                     ClassId ownerClass) const noexcept;
  [[nodiscard]] bool ValidateInstance(const ItemInstance &instance,
                                      ClassId ownerClass = {}) const noexcept;
  [[nodiscard]] bool
  ResolveModifiers(const ItemInstance &instance,
                   std::vector<ResolvedItemModifier> &modifiers) const;
  void
  CollectEligibleAffixes(const ItemDefinition &item, std::uint32_t itemLevel,
                         ClassId ownerClass,
                         std::vector<const AffixDefinition *> &affixes) const;

private:
  [[nodiscard]] static bool
  IsRarityShapeValid(const RarityDefinition &definition) noexcept;
  [[nodiscard]] static bool
  IsAffixShapeValid(const AffixDefinition &definition) noexcept;
  [[nodiscard]] static bool
  IsItemShapeValid(const ItemDefinition &definition) noexcept;

  std::map<RarityId, RarityDefinition> rarities_{};
  std::map<AffixId, AffixDefinition> affixes_{};
  std::map<ItemId, ItemDefinition> items_{};
};

} // namespace Lostsense::Gameplay
