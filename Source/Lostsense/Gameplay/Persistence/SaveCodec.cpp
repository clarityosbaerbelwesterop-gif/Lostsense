#include "Lostsense/Gameplay/Persistence/SaveCodec.h"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>

namespace Lostsense::Gameplay {
namespace {

constexpr std::size_t MaximumPayloadBytes = 8U * 1024U * 1024U;
constexpr std::size_t MaximumAttributes = 4096U;
constexpr std::size_t MaximumModifiers = 65536U;
constexpr std::size_t MaximumEffects = 65536U;
constexpr std::size_t MaximumAbilities = 65536U;
constexpr std::size_t MaximumCooldownGroups = 65536U;
constexpr std::size_t MaximumSkillNodes = 65536U;
constexpr std::size_t MaximumInventoryRecords = 65536U;
constexpr std::size_t MaximumEquipmentItems = 1024U;
constexpr std::size_t MaximumAffixesPerItem = 128U;
constexpr std::size_t MaximumMagnitudesPerAffix = 128U;
constexpr std::size_t MaximumSocketsPerItem = 128U;

class TokenReader final {
public:
  explicit TokenReader(const std::string_view payload)
      : stream_{std::string{payload}} {}

  [[nodiscard]] bool Read(std::string &token) {
    return static_cast<bool>(stream_ >> token);
  }

  [[nodiscard]] bool Expect(const std::string_view expected) {
    std::string token;
    return Read(token) && token == expected;
  }

  [[nodiscard]] bool Finished() {
    std::string token;
    return !(stream_ >> token);
  }

private:
  std::istringstream stream_;
};

template <typename T>
[[nodiscard]] bool ParseUnsigned(const std::string &token, T &value) {
  static_assert(std::is_unsigned_v<T>);
  if (token.empty() || token.front() == '-') {
    return false;
  }
  std::uint64_t parsed = 0U;
  const char *begin = token.data();
  const char *end = token.data() + token.size();
  const auto result = std::from_chars(begin, end, parsed, 10);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
    return false;
  }
  value = static_cast<T>(parsed);
  return true;
}

template <typename T>
[[nodiscard]] bool ReadUnsigned(TokenReader &reader, T &value) {
  std::string token;
  return reader.Read(token) && ParseUnsigned(token, value);
}

[[nodiscard]] bool ReadBool(TokenReader &reader, bool &value) {
  std::uint8_t raw = 0U;
  if (!ReadUnsigned(reader, raw) || raw > 1U) {
    return false;
  }
  value = raw == 1U;
  return true;
}

[[nodiscard]] std::string EncodeDouble(const double value) {
  const std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
  std::ostringstream output;
  output << std::hex << std::setfill('0') << std::setw(16) << bits;
  return output.str();
}

[[nodiscard]] bool ReadDouble(TokenReader &reader, double &value) {
  std::string token;
  if (!reader.Read(token) || token.size() != 16U) {
    return false;
  }
  std::uint64_t bits = 0U;
  const char *begin = token.data();
  const char *end = token.data() + token.size();
  const auto result = std::from_chars(begin, end, bits, 16);
  if (result.ec != std::errc{} || result.ptr != end) {
    return false;
  }
  value = std::bit_cast<double>(bits);
  return std::isfinite(value);
}

[[nodiscard]] bool ReadCount(TokenReader &reader, std::size_t &count,
                             const std::size_t maximum) {
  std::uint64_t raw = 0U;
  if (!ReadUnsigned(reader, raw) || raw > maximum) {
    return false;
  }
  count = static_cast<std::size_t>(raw);
  return true;
}

void WriteItemInstance(std::ostringstream &output,
                       const ItemInstance &instance) {
  output << "ITEM " << instance.InstanceId.Value << ' '
         << instance.DefinitionId.Value << ' ' << instance.ItemLevel << ' '
         << instance.Rarity.Value << ' ' << instance.Affixes.size() << ' '
         << instance.SocketedItems.size() << '\n';
  for (const RolledAffix &affix : instance.Affixes) {
    output << "AFFIX " << affix.Id.Value << ' ' << affix.Magnitudes.size();
    for (const double magnitude : affix.Magnitudes) {
      output << ' ' << EncodeDouble(magnitude);
    }
    output << '\n';
  }
  for (const ItemId socket : instance.SocketedItems) {
    output << "SOCKET " << socket.Value << '\n';
  }
}

[[nodiscard]] bool ReadItemInstance(TokenReader &reader,
                                    ItemInstance &instance) {
  std::size_t affixCount = 0U;
  std::size_t socketCount = 0U;
  if (!reader.Expect("ITEM") ||
      !ReadUnsigned(reader, instance.InstanceId.Value) ||
      !ReadUnsigned(reader, instance.DefinitionId.Value) ||
      !ReadUnsigned(reader, instance.ItemLevel) ||
      !ReadUnsigned(reader, instance.Rarity.Value) ||
      !ReadCount(reader, affixCount, MaximumAffixesPerItem) ||
      !ReadCount(reader, socketCount, MaximumSocketsPerItem)) {
    return false;
  }

  instance.Affixes.clear();
  instance.Affixes.reserve(affixCount);
  for (std::size_t index = 0; index < affixCount; ++index) {
    RolledAffix affix;
    std::size_t magnitudeCount = 0U;
    if (!reader.Expect("AFFIX") || !ReadUnsigned(reader, affix.Id.Value) ||
        !ReadCount(reader, magnitudeCount, MaximumMagnitudesPerAffix)) {
      return false;
    }
    affix.Magnitudes.resize(magnitudeCount);
    for (double &magnitude : affix.Magnitudes) {
      if (!ReadDouble(reader, magnitude)) {
        return false;
      }
    }
    instance.Affixes.push_back(std::move(affix));
  }

  instance.SocketedItems.clear();
  instance.SocketedItems.reserve(socketCount);
  for (std::size_t index = 0; index < socketCount; ++index) {
    ItemId socket;
    if (!reader.Expect("SOCKET") || !ReadUnsigned(reader, socket.Value)) {
      return false;
    }
    instance.SocketedItems.push_back(socket);
  }
  return true;
}

[[nodiscard]] bool ParseBody(TokenReader &reader, const std::uint32_t version,
                             CharacterSaveState &state) {
  if (!reader.Expect("IDENTITY") ||
      !ReadUnsigned(reader, state.Character.Value) ||
      !ReadUnsigned(reader, state.CharacterClass.Value) ||
      !ReadUnsigned(reader, state.SkillTreeDefinition.Value) ||
      !reader.Expect("RNG") || !ReadUnsigned(reader, state.Random.State) ||
      !ReadUnsigned(reader, state.Random.Increment)) {
    return false;
  }

  if (version >= 2U) {
    if (!reader.Expect("LOOT") ||
        !ReadUnsigned(reader, state.Loot.NextInstanceValue)) {
      return false;
    }
  } else {
    state.Loot = {};
  }

  std::uint8_t combatKind = 0U;
  if (!reader.Expect("COMBATANT") ||
      !ReadUnsigned(reader, state.Combatant.Id.Value) ||
      !ReadUnsigned(reader, combatKind)) {
    return false;
  }
  state.Combatant.Kind = static_cast<Combat::CombatantKind>(combatKind);

  std::size_t baseCount = 0U;
  if (!reader.Expect("ATTR_BASES") ||
      !ReadCount(reader, baseCount, MaximumAttributes)) {
    return false;
  }
  state.Combatant.Attributes.BaseValues.clear();
  state.Combatant.Attributes.BaseValues.reserve(baseCount);
  for (std::size_t index = 0; index < baseCount; ++index) {
    Stats::AttributeBaseState base;
    if (!reader.Expect("ATTR_BASE") || !ReadUnsigned(reader, base.Id.Value) ||
        !ReadDouble(reader, base.Value)) {
      return false;
    }
    state.Combatant.Attributes.BaseValues.push_back(base);
  }

  std::size_t modifierCount = 0U;
  if (!reader.Expect("ATTR_MODIFIERS") ||
      !ReadCount(reader, modifierCount, MaximumModifiers)) {
    return false;
  }
  state.Combatant.Attributes.Modifiers.clear();
  state.Combatant.Attributes.Modifiers.reserve(modifierCount);
  for (std::size_t index = 0; index < modifierCount; ++index) {
    Stats::AttributeModifier modifier;
    std::uint8_t operation = 0U;
    std::uint8_t source = 0U;
    if (!reader.Expect("ATTR_MOD") ||
        !ReadUnsigned(reader, modifier.Id.Value) ||
        !ReadUnsigned(reader, modifier.Attribute.Value) ||
        !ReadUnsigned(reader, operation) || !ReadUnsigned(reader, source) ||
        !ReadDouble(reader, modifier.Magnitude)) {
      return false;
    }
    modifier.Operation = static_cast<Stats::ModifierOperation>(operation);
    modifier.Source = static_cast<Stats::ModifierSource>(source);
    state.Combatant.Attributes.Modifiers.push_back(modifier);
  }

  if (!reader.Expect("HEALTH") ||
      !ReadDouble(reader, state.Combatant.Health.Maximum) ||
      !ReadDouble(reader, state.Combatant.Health.Current) ||
      !ReadBool(reader, state.Combatant.Health.Dead) ||
      !reader.Expect("RESOURCE") ||
      !ReadDouble(reader, state.Combatant.Resource.Maximum) ||
      !ReadDouble(reader, state.Combatant.Resource.Current)) {
    return false;
  }

  std::size_t effectCount = 0U;
  if (!reader.Expect("EFFECTS") ||
      !ReadUnsigned(reader, state.Effects.NextInstanceValue) ||
      !ReadCount(reader, effectCount, MaximumEffects)) {
    return false;
  }
  state.Effects.ActiveEffects.clear();
  state.Effects.ActiveEffects.reserve(effectCount);
  for (std::size_t index = 0; index < effectCount; ++index) {
    ActiveEffectState effect;
    if (!reader.Expect("EFFECT") ||
        !ReadUnsigned(reader, effect.InstanceId.Value) ||
        !ReadUnsigned(reader, effect.DefinitionId.Value) ||
        !ReadUnsigned(reader, effect.SourceId.Value) ||
        !ReadUnsigned(reader, effect.Stacks) ||
        !ReadDouble(reader, effect.RemainingSeconds) ||
        !ReadDouble(reader, effect.TimeUntilNextTick)) {
      return false;
    }
    state.Effects.ActiveEffects.push_back(effect);
  }

  std::size_t abilityCount = 0U;
  if (!reader.Expect("ABILITIES") ||
      !ReadCount(reader, abilityCount, MaximumAbilities)) {
    return false;
  }
  state.Abilities.Abilities.clear();
  state.Abilities.Abilities.reserve(abilityCount);
  for (std::size_t index = 0; index < abilityCount; ++index) {
    AbilityRuntimeEntryState ability;
    if (!reader.Expect("ABILITY") || !ReadUnsigned(reader, ability.Id.Value) ||
        !ReadBool(reader, ability.Unlocked) ||
        !ReadUnsigned(reader, ability.Charges) ||
        !ReadDouble(reader, ability.CooldownRemaining) ||
        !ReadDouble(reader, ability.RechargeRemaining)) {
      return false;
    }
    state.Abilities.Abilities.push_back(ability);
  }

  std::size_t cooldownCount = 0U;
  if (!reader.Expect("COOLDOWNS") ||
      !ReadCount(reader, cooldownCount, MaximumCooldownGroups)) {
    return false;
  }
  state.Abilities.CooldownGroups.clear();
  state.Abilities.CooldownGroups.reserve(cooldownCount);
  for (std::size_t index = 0; index < cooldownCount; ++index) {
    CooldownGroupState cooldown;
    if (!reader.Expect("COOLDOWN") ||
        !ReadUnsigned(reader, cooldown.Id.Value) ||
        !ReadDouble(reader, cooldown.Remaining)) {
      return false;
    }
    state.Abilities.CooldownGroups.push_back(cooldown);
  }

  std::size_t allocatedCount = 0U;
  if (!reader.Expect("SKILL") ||
      !ReadUnsigned(reader, state.SkillTree.TotalPoints) ||
      !ReadUnsigned(reader, state.SkillTree.UnspentPoints) ||
      !ReadCount(reader, allocatedCount, MaximumSkillNodes)) {
    return false;
  }
  state.SkillTree.AllocatedNodes.clear();
  state.SkillTree.AllocatedNodes.reserve(allocatedCount);
  for (std::size_t index = 0; index < allocatedCount; ++index) {
    SkillNodeId node;
    if (!reader.Expect("SKILL_NODE") || !ReadUnsigned(reader, node.Value)) {
      return false;
    }
    state.SkillTree.AllocatedNodes.push_back(node);
  }

  std::size_t slotCount = 0U;
  if (!reader.Expect("LOADOUT") ||
      !ReadCount(reader, slotCount, AbilityLoadoutSlotCount) ||
      slotCount != AbilityLoadoutSlotCount) {
    return false;
  }
  for (AbilityId &slot : state.Loadout.Slots) {
    if (!ReadUnsigned(reader, slot.Value)) {
      return false;
    }
  }

  std::size_t stackCount = 0U;
  std::size_t instanceCount = 0U;
  if (!reader.Expect("INVENTORY") ||
      !ReadUnsigned(reader, state.Inventory.Capacity) ||
      !ReadCount(reader, stackCount, MaximumInventoryRecords) ||
      !ReadCount(reader, instanceCount, MaximumInventoryRecords)) {
    return false;
  }
  state.Inventory.Stacks.clear();
  state.Inventory.Stacks.reserve(stackCount);
  for (std::size_t index = 0; index < stackCount; ++index) {
    ItemStackState stack;
    if (!reader.Expect("STACK") || !ReadUnsigned(reader, stack.Item.Value) ||
        !ReadUnsigned(reader, stack.Quantity)) {
      return false;
    }
    state.Inventory.Stacks.push_back(stack);
  }
  state.Inventory.Instances.clear();
  state.Inventory.Instances.reserve(instanceCount);
  for (std::size_t index = 0; index < instanceCount; ++index) {
    ItemInstance item;
    if (!ReadItemInstance(reader, item)) {
      return false;
    }
    state.Inventory.Instances.push_back(std::move(item));
  }

  std::size_t equipmentCount = 0U;
  if (!reader.Expect("EQUIPMENT") ||
      !ReadCount(reader, equipmentCount, MaximumEquipmentItems)) {
    return false;
  }
  state.Equipment.Items.clear();
  state.Equipment.Items.reserve(equipmentCount);
  for (std::size_t index = 0; index < equipmentCount; ++index) {
    EquippedItemState equipped;
    if (!reader.Expect("EQUIPPED") ||
        !ReadUnsigned(reader, equipped.Slot.Value) ||
        !ReadItemInstance(reader, equipped.Item)) {
      return false;
    }
    state.Equipment.Items.push_back(std::move(equipped));
  }

  return reader.Expect("END") && reader.Finished();
}

[[nodiscard]] bool MigrateVersionOne(CharacterSaveState &state) {
  std::set<ItemInstanceId> instances;
  std::uint64_t maximum = 0U;
  const auto accept = [&](const ItemInstance &item) {
    if (!item.InstanceId.IsValid() || !item.DefinitionId.IsValid() ||
        !item.Rarity.IsValid() || item.ItemLevel == 0U ||
        !instances.insert(item.InstanceId).second) {
      return false;
    }
    maximum = std::max(maximum, item.InstanceId.Value);
    return true;
  };
  for (const ItemInstance &item : state.Inventory.Instances) {
    if (!accept(item)) {
      return false;
    }
  }
  for (const EquippedItemState &item : state.Equipment.Items) {
    if (!item.Slot.IsValid() || !accept(item.Item)) {
      return false;
    }
  }
  if (maximum == std::numeric_limits<std::uint64_t>::max()) {
    return false;
  }
  state.Loot.NextInstanceValue = maximum + 1U;
  if (state.Loot.NextInstanceValue == 0U) {
    return false;
  }
  state.SchemaVersion = CurrentSaveSchemaVersion;
  return true;
}

} // namespace

bool SaveCodec::Serialize(const CharacterSaveState &state,
                          std::string &payload) {
  if (state.SchemaVersion != CurrentSaveSchemaVersion) {
    return false;
  }

  std::ostringstream output;
  output << "LOSTSENSE_SAVE " << CurrentSaveSchemaVersion << '\n';
  output << "IDENTITY " << state.Character.Value << ' '
         << state.CharacterClass.Value << ' '
         << state.SkillTreeDefinition.Value << '\n';
  output << "RNG " << state.Random.State << ' ' << state.Random.Increment
         << '\n';
  output << "LOOT " << state.Loot.NextInstanceValue << '\n';
  output << "COMBATANT " << state.Combatant.Id.Value << ' '
         << static_cast<unsigned>(state.Combatant.Kind) << '\n';

  output << "ATTR_BASES " << state.Combatant.Attributes.BaseValues.size()
         << '\n';
  for (const Stats::AttributeBaseState &base :
       state.Combatant.Attributes.BaseValues) {
    output << "ATTR_BASE " << base.Id.Value << ' ' << EncodeDouble(base.Value)
           << '\n';
  }

  output << "ATTR_MODIFIERS " << state.Combatant.Attributes.Modifiers.size()
         << '\n';
  for (const Stats::AttributeModifier &modifier :
       state.Combatant.Attributes.Modifiers) {
    output << "ATTR_MOD " << modifier.Id.Value << ' '
           << modifier.Attribute.Value << ' '
           << static_cast<unsigned>(modifier.Operation) << ' '
           << static_cast<unsigned>(modifier.Source) << ' '
           << EncodeDouble(modifier.Magnitude) << '\n';
  }

  output << "HEALTH " << EncodeDouble(state.Combatant.Health.Maximum) << ' '
         << EncodeDouble(state.Combatant.Health.Current) << ' '
         << (state.Combatant.Health.Dead ? 1U : 0U) << '\n';
  output << "RESOURCE " << EncodeDouble(state.Combatant.Resource.Maximum)
         << ' ' << EncodeDouble(state.Combatant.Resource.Current) << '\n';

  output << "EFFECTS " << state.Effects.NextInstanceValue << ' '
         << state.Effects.ActiveEffects.size() << '\n';
  for (const ActiveEffectState &effect : state.Effects.ActiveEffects) {
    output << "EFFECT " << effect.InstanceId.Value << ' '
           << effect.DefinitionId.Value << ' ' << effect.SourceId.Value << ' '
           << effect.Stacks << ' ' << EncodeDouble(effect.RemainingSeconds)
           << ' ' << EncodeDouble(effect.TimeUntilNextTick) << '\n';
  }

  output << "ABILITIES " << state.Abilities.Abilities.size() << '\n';
  for (const AbilityRuntimeEntryState &ability : state.Abilities.Abilities) {
    output << "ABILITY " << ability.Id.Value << ' '
           << (ability.Unlocked ? 1U : 0U) << ' ' << ability.Charges << ' '
           << EncodeDouble(ability.CooldownRemaining) << ' '
           << EncodeDouble(ability.RechargeRemaining) << '\n';
  }
  output << "COOLDOWNS " << state.Abilities.CooldownGroups.size() << '\n';
  for (const CooldownGroupState &group : state.Abilities.CooldownGroups) {
    output << "COOLDOWN " << group.Id.Value << ' '
           << EncodeDouble(group.Remaining) << '\n';
  }

  output << "SKILL " << state.SkillTree.TotalPoints << ' '
         << state.SkillTree.UnspentPoints << ' '
         << state.SkillTree.AllocatedNodes.size() << '\n';
  for (const SkillNodeId node : state.SkillTree.AllocatedNodes) {
    output << "SKILL_NODE " << node.Value << '\n';
  }

  output << "LOADOUT " << AbilityLoadoutSlotCount;
  for (const AbilityId slot : state.Loadout.Slots) {
    output << ' ' << slot.Value;
  }
  output << '\n';

  output << "INVENTORY " << state.Inventory.Capacity << ' '
         << state.Inventory.Stacks.size() << ' '
         << state.Inventory.Instances.size() << '\n';
  for (const ItemStackState &stack : state.Inventory.Stacks) {
    output << "STACK " << stack.Item.Value << ' ' << stack.Quantity << '\n';
  }
  for (const ItemInstance &instance : state.Inventory.Instances) {
    WriteItemInstance(output, instance);
  }

  output << "EQUIPMENT " << state.Equipment.Items.size() << '\n';
  for (const EquippedItemState &entry : state.Equipment.Items) {
    output << "EQUIPPED " << entry.Slot.Value << '\n';
    WriteItemInstance(output, entry.Item);
  }
  output << "END\n";

  std::string encoded = output.str();
  if (encoded.size() > MaximumPayloadBytes) {
    return false;
  }
  payload = std::move(encoded);
  return true;
}

SaveDecodeResult SaveCodec::Deserialize(const std::string_view payload,
                                        CharacterSaveState &state) {
  if (payload.size() > MaximumPayloadBytes) {
    return {SaveDecodeStatus::PayloadTooLarge, 0U};
  }

  TokenReader reader{payload};
  std::uint32_t sourceVersion = 0U;
  if (!reader.Expect("LOSTSENSE_SAVE") ||
      !ReadUnsigned(reader, sourceVersion)) {
    return {SaveDecodeStatus::Malformed, 0U};
  }
  if (sourceVersion < OldestSupportedSaveSchemaVersion ||
      sourceVersion > CurrentSaveSchemaVersion) {
    return {SaveDecodeStatus::UnsupportedVersion, sourceVersion};
  }

  CharacterSaveState decoded;
  decoded.SchemaVersion = sourceVersion;
  if (!ParseBody(reader, sourceVersion, decoded)) {
    return {SaveDecodeStatus::Malformed, sourceVersion};
  }
  if (sourceVersion == 1U && !MigrateVersionOne(decoded)) {
    return {SaveDecodeStatus::MigrationFailed, sourceVersion};
  }
  decoded.SchemaVersion = CurrentSaveSchemaVersion;
  state = std::move(decoded);
  return {SaveDecodeStatus::Success, sourceVersion};
}

} // namespace Lostsense::Gameplay
