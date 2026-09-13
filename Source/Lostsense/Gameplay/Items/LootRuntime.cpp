#include "Lostsense/Gameplay/Items/LootRuntime.h"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

constexpr std::uint8_t MaximumLootRolls = 64U;

[[nodiscard]] bool ContainsClass(const std::vector<ClassId> &classes,
                                 const ClassId value) {
  return std::find(classes.begin(), classes.end(), value) != classes.end();
}

template <typename T>
[[nodiscard]] bool HasUniqueValidIds(const std::vector<T> &ids) {
  std::set<T> unique;
  for (const T id : ids) {
    if (!id.IsValid() || !unique.insert(id).second) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
SelectWeightedIndex(Core::DeterministicRandom &random,
                    const std::vector<std::uint64_t> &weights,
                    std::size_t &selected) noexcept {
  if (weights.empty()) {
    return false;
  }
  std::uint64_t total = 0U;
  for (const std::uint64_t weight : weights) {
    if (weight == 0U ||
        weight > std::numeric_limits<std::uint64_t>::max() - total) {
      return false;
    }
    total += weight;
  }
  if (total == 0U) {
    return false;
  }

  const std::uint64_t threshold = (0ULL - total) % total;
  std::uint64_t sample = 0U;
  do {
    sample = random.NextUInt64();
  } while (sample < threshold);
  const std::uint64_t roll = sample % total;

  std::uint64_t cursor = 0U;
  for (std::size_t index = 0; index < weights.size(); ++index) {
    cursor += weights[index];
    if (roll < cursor) {
      selected = index;
      return true;
    }
  }
  return false;
}

[[nodiscard]] double RollMagnitude(Core::DeterministicRandom &random,
                                   const double minimum,
                                   const double maximum) noexcept {
  if (minimum == maximum) {
    return minimum;
  }
  return minimum + (maximum - minimum) * random.NextUnit();
}

} // namespace

bool LootCatalog::AddTable(LootTableDefinition definition) {
  if (!IsTableShapeValid(definition) || tables_.contains(definition.Id)) {
    return false;
  }
  return tables_.emplace(definition.Id, std::move(definition)).second;
}

bool LootCatalog::Validate(const ItemCatalog &items) const noexcept {
  if (!items.Validate() || tables_.empty()) {
    return false;
  }
  for (const auto &[id, table] : tables_) {
    static_cast<void>(id);
    bool needsRolledRarity = false;
    for (const LootTableEntryDefinition &entry : table.Entries) {
      const ItemDefinition *item = items.FindItem(entry.Item);
      if (item == nullptr) {
        return false;
      }
      if (item->MaximumStackSize == 1U) {
        if (entry.MinimumQuantity != 1U || entry.MaximumQuantity != 1U) {
          return false;
        }
        needsRolledRarity = needsRolledRarity || !item->FixedRarity.IsValid();
      }
    }

    std::set<RarityId> rarityIds;
    for (const LootRarityWeight &rarity : table.Rarities) {
      if (items.FindRarity(rarity.Rarity) == nullptr || rarity.Weight == 0U ||
          !rarityIds.insert(rarity.Rarity).second) {
        return false;
      }
    }
    if (needsRolledRarity && table.Rarities.empty()) {
      return false;
    }
  }
  return true;
}

const LootTableDefinition *
LootCatalog::FindTable(const LootTableId id) const noexcept {
  const auto it = tables_.find(id);
  return it == tables_.end() ? nullptr : &it->second;
}

bool LootCatalog::IsTableShapeValid(
    const LootTableDefinition &definition) noexcept {
  if (!definition.Id.IsValid() || definition.Rolls == 0U ||
      definition.Rolls > MaximumLootRolls || definition.Entries.empty()) {
    return false;
  }
  std::set<ItemId> entryIds;
  for (const LootTableEntryDefinition &entry : definition.Entries) {
    if (!entry.Item.IsValid() || entry.Weight == 0U ||
        entry.MinimumQuantity == 0U ||
        entry.MinimumQuantity > entry.MaximumQuantity ||
        !HasUniqueValidIds(entry.AllowedClasses) ||
        !entryIds.insert(entry.Item).second) {
      return false;
    }
  }
  std::set<RarityId> rarityIds;
  for (const LootRarityWeight &rarity : definition.Rarities) {
    if (!rarity.Rarity.IsValid() || rarity.Weight == 0U ||
        !rarityIds.insert(rarity.Rarity).second) {
      return false;
    }
  }
  return true;
}

LootRuntime::LootRuntime(const ItemCatalog &items, const LootCatalog &tables,
                         Core::DeterministicRandom &random) noexcept
    : items_{items}, tables_{tables}, random_{random},
      valid_{items.Validate() && tables.Validate(items)} {}

LootGenerationOutcome LootRuntime::Generate(const LootTableId tableId,
                                            const LootContext &context) {
  if (!valid_) {
    return {LootGenerationResult::InvalidRuntime, {}};
  }
  const LootTableDefinition *table = tables_.FindTable(tableId);
  if (table == nullptr) {
    return {LootGenerationResult::UnknownTable, {}};
  }
  if (context.ItemLevel == 0U) {
    return {LootGenerationResult::InvalidContext, {}};
  }

  const Core::RandomState randomState = random_.CaptureState();
  const LootRuntimeState runtimeState = CaptureState();
  const auto fail = [&](const LootGenerationResult result) {
    static_cast<void>(random_.RestoreState(randomState));
    static_cast<void>(RestoreState(runtimeState));
    return LootGenerationOutcome{result, {}};
  };

  LootGenerationOutcome outcome;
  outcome.Result = LootGenerationResult::Success;
  outcome.Drops.reserve(table->Rolls);
  std::set<ItemId> generatedUniques;

  for (std::uint8_t rollIndex = 0U; rollIndex < table->Rolls; ++rollIndex) {
    static_cast<void>(rollIndex);
    std::vector<const LootTableEntryDefinition *> candidates;
    std::vector<std::uint64_t> weights;
    for (const LootTableEntryDefinition &entry : table->Entries) {
      const ItemDefinition &item = *items_.FindItem(entry.Item);
      if (context.ItemLevel < item.MinimumItemLevel ||
          context.ItemLevel > item.MaximumItemLevel ||
          (context.PlayerClass.IsValid() && !entry.AllowedClasses.empty() &&
           !ContainsClass(entry.AllowedClasses, context.PlayerClass)) ||
          !items_.IsItemAllowedForClass(item, context.PlayerClass) ||
          (item.Unique && generatedUniques.contains(item.Id))) {
        continue;
      }
      candidates.push_back(&entry);
      weights.push_back(entry.Weight);
    }
    std::size_t selectedIndex = 0U;
    if (!SelectWeightedIndex(random_, weights, selectedIndex)) {
      return fail(LootGenerationResult::NoEligibleEntry);
    }

    const LootTableEntryDefinition &entry = *candidates[selectedIndex];
    const ItemDefinition &item = *items_.FindItem(entry.Item);
    GeneratedLootEntry generated;
    generated.Item = item.Id;

    if (item.MaximumStackSize > 1U) {
      generated.Quantity = entry.MinimumQuantity;
      if (entry.MaximumQuantity > entry.MinimumQuantity) {
        const std::uint64_t span =
            static_cast<std::uint64_t>(entry.MaximumQuantity) -
            entry.MinimumQuantity + 1U;
        if (span > std::numeric_limits<std::uint32_t>::max()) {
          return fail(LootGenerationResult::InternalFailure);
        }
        generated.Quantity +=
            random_.NextBounded(static_cast<std::uint32_t>(span));
      }
      outcome.Drops.push_back(std::move(generated));
      continue;
    }

    ItemInstance instance;
    instance.InstanceId = AllocateInstanceId();
    if (!instance.InstanceId.IsValid()) {
      return fail(LootGenerationResult::InstanceIdExhausted);
    }
    instance.DefinitionId = item.Id;
    instance.ItemLevel = context.ItemLevel;
    if (!RollRarity(*table, item, context.ItemLevel, instance.Rarity)) {
      return fail(LootGenerationResult::NoEligibleRarity);
    }
    const RarityDefinition &rarity = *items_.FindRarity(instance.Rarity);
    if (!RollAffixes(item, rarity, context, instance.Affixes)) {
      return fail(LootGenerationResult::NoEligibleAffix);
    }
    if (!items_.ValidateInstance(instance, context.PlayerClass)) {
      return fail(LootGenerationResult::InternalFailure);
    }

    generated.Quantity = 1U;
    generated.Instance = std::move(instance);
    outcome.Drops.push_back(std::move(generated));
    if (item.Unique) {
      generatedUniques.insert(item.Id);
    }
  }
  return outcome;
}

LootRuntimeState LootRuntime::CaptureState() const noexcept {
  return LootRuntimeState{nextInstanceValue_};
}

bool LootRuntime::RestoreState(const LootRuntimeState &state) noexcept {
  if (state.NextInstanceValue == 0U ||
      state.NextInstanceValue > MaximumPersistentItemInstanceId + 1ULL) {
    return false;
  }
  nextInstanceValue_ = state.NextInstanceValue;
  return true;
}

bool LootRuntime::RollRarity(const LootTableDefinition &table,
                             const ItemDefinition &item,
                             const std::uint32_t itemLevel, RarityId &rarity) {
  if (item.FixedRarity.IsValid()) {
    const RarityDefinition *fixed = items_.FindRarity(item.FixedRarity);
    if (fixed == nullptr || itemLevel < fixed->MinimumItemLevel ||
        itemLevel > fixed->MaximumItemLevel) {
      return false;
    }
    rarity = item.FixedRarity;
    return true;
  }

  std::vector<RarityId> candidates;
  std::vector<std::uint64_t> weights;
  for (const LootRarityWeight &weighted : table.Rarities) {
    const RarityDefinition &definition = *items_.FindRarity(weighted.Rarity);
    if (!items_.IsRarityAllowed(item, weighted.Rarity) ||
        itemLevel < definition.MinimumItemLevel ||
        itemLevel > definition.MaximumItemLevel) {
      continue;
    }
    candidates.push_back(weighted.Rarity);
    weights.push_back(weighted.Weight);
  }
  std::size_t selected = 0U;
  if (!SelectWeightedIndex(random_, weights, selected)) {
    return false;
  }
  rarity = candidates[selected];
  return true;
}

bool LootRuntime::RollAffixes(const ItemDefinition &item,
                              const RarityDefinition &rarity,
                              const LootContext &context,
                              std::vector<RolledAffix> &affixes) {
  std::uint32_t targetCount = rarity.MinimumAffixes;
  if (rarity.MaximumAffixes > rarity.MinimumAffixes) {
    const std::uint32_t span =
        static_cast<std::uint32_t>(rarity.MaximumAffixes -
                                   rarity.MinimumAffixes) +
        1U;
    targetCount += random_.NextBounded(span);
  }
  if (targetCount == 0U) {
    affixes.clear();
    return true;
  }

  std::vector<const AffixDefinition *> eligible;
  items_.CollectEligibleAffixes(item, context.ItemLevel, context.PlayerClass,
                                eligible);
  std::set<AffixExclusiveGroupId> availableGroups;
  std::size_t ungrouped = 0U;
  for (const AffixDefinition *affix : eligible) {
    if (affix->ExclusiveGroup.IsValid()) {
      availableGroups.insert(affix->ExclusiveGroup);
    } else {
      ++ungrouped;
    }
  }
  if (static_cast<std::size_t>(targetCount) >
      ungrouped + availableGroups.size()) {
    return false;
  }

  std::set<AffixId> selectedIds;
  std::set<AffixExclusiveGroupId> selectedGroups;
  std::vector<RolledAffix> rolledAffixes;
  rolledAffixes.reserve(targetCount);

  for (std::uint32_t index = 0U; index < targetCount; ++index) {
    static_cast<void>(index);
    std::vector<const AffixDefinition *> candidates;
    std::vector<std::uint64_t> weights;
    for (const AffixDefinition *affix : eligible) {
      if (selectedIds.contains(affix->Id) ||
          (affix->ExclusiveGroup.IsValid() &&
           selectedGroups.contains(affix->ExclusiveGroup))) {
        continue;
      }
      candidates.push_back(affix);
      weights.push_back(affix->Weight);
    }
    std::size_t selected = 0U;
    if (!SelectWeightedIndex(random_, weights, selected)) {
      return false;
    }
    const AffixDefinition &affix = *candidates[selected];
    RolledAffix rolled;
    rolled.Id = affix.Id;
    rolled.Magnitudes.reserve(affix.Modifiers.size());
    for (const AffixModifierRollDefinition &modifier : affix.Modifiers) {
      rolled.Magnitudes.push_back(RollMagnitude(
          random_, modifier.MinimumMagnitude, modifier.MaximumMagnitude));
    }
    selectedIds.insert(affix.Id);
    if (affix.ExclusiveGroup.IsValid()) {
      selectedGroups.insert(affix.ExclusiveGroup);
    }
    rolledAffixes.push_back(std::move(rolled));
  }

  affixes = std::move(rolledAffixes);
  return true;
}

ItemInstanceId LootRuntime::AllocateInstanceId() noexcept {
  if (nextInstanceValue_ == 0U ||
      nextInstanceValue_ > MaximumPersistentItemInstanceId) {
    return {};
  }
  const ItemInstanceId id{nextInstanceValue_};
  ++nextInstanceValue_;
  return id;
}

} // namespace Lostsense::Gameplay
