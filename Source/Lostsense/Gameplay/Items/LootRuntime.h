#pragma once

#include "Lostsense/Core/DeterministicRandom.h"
#include "Lostsense/Gameplay/Items/ItemCatalog.h"

#include <cstdint>
#include <map>
#include <vector>

namespace Lostsense::Gameplay {

struct LootRarityWeight final {
  RarityId Rarity{};
  std::uint32_t Weight{0U};
};

struct LootTableEntryDefinition final {
  ItemId Item{};
  std::uint32_t Weight{0U};
  std::uint32_t MinimumQuantity{1U};
  std::uint32_t MaximumQuantity{1U};
  std::vector<ClassId> AllowedClasses{};
};

struct LootTableDefinition final {
  LootTableId Id{};
  std::uint8_t Rolls{1U};
  std::vector<LootTableEntryDefinition> Entries{};
  std::vector<LootRarityWeight> Rarities{};
};

struct LootContext final {
  std::uint32_t ItemLevel{1U};
  ClassId PlayerClass{};
};

struct GeneratedLootEntry final {
  ItemId Item{};
  std::uint32_t Quantity{0U};
  ItemInstance Instance{};

  [[nodiscard]] bool IsItemInstance() const noexcept {
    return Instance.InstanceId.IsValid();
  }
};

struct LootRuntimeState final {
  std::uint64_t NextInstanceValue{1U};
};

enum class LootGenerationResult : std::uint8_t {
  Success,
  InvalidRuntime,
  UnknownTable,
  InvalidContext,
  NoEligibleEntry,
  NoEligibleRarity,
  NoEligibleAffix,
  InstanceIdExhausted,
  InternalFailure,
};

struct LootGenerationOutcome final {
  LootGenerationResult Result{LootGenerationResult::InternalFailure};
  std::vector<GeneratedLootEntry> Drops{};
};

class LootCatalog final {
public:
  [[nodiscard]] bool AddTable(LootTableDefinition definition);
  [[nodiscard]] bool Validate(const ItemCatalog &items) const noexcept;
  [[nodiscard]] const LootTableDefinition *
  FindTable(LootTableId id) const noexcept;

private:
  [[nodiscard]] static bool
  IsTableShapeValid(const LootTableDefinition &definition) noexcept;

  std::map<LootTableId, LootTableDefinition> tables_{};
};

class LootRuntime final {
public:
  LootRuntime(const ItemCatalog &items, const LootCatalog &tables,
              Core::DeterministicRandom &random) noexcept;

  [[nodiscard]] bool IsValid() const noexcept { return valid_; }
  [[nodiscard]] LootGenerationOutcome Generate(LootTableId table,
                                               const LootContext &context);
  [[nodiscard]] LootRuntimeState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(const LootRuntimeState &state) noexcept;

private:
  [[nodiscard]] bool RollRarity(const LootTableDefinition &table,
                                const ItemDefinition &item,
                                std::uint32_t itemLevel, RarityId &rarity);
  [[nodiscard]] bool RollAffixes(const ItemDefinition &item,
                                 const RarityDefinition &rarity,
                                 const LootContext &context,
                                 std::vector<RolledAffix> &affixes);
  [[nodiscard]] ItemInstanceId AllocateInstanceId() noexcept;

  const ItemCatalog &items_;
  const LootCatalog &tables_;
  Core::DeterministicRandom &random_;
  std::uint64_t nextInstanceValue_{1U};
  bool valid_{false};
};

} // namespace Lostsense::Gameplay
