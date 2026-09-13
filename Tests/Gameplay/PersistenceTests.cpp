#include "PersistenceTestFixtures.h"
#include "Lostsense/Gameplay/Persistence/SaveCodec.h"
#include "TestHarness.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <string>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using namespace Lostsense::Gameplay::PersistenceTestData;
using namespace Lostsense::Gameplay::TestData;
using Tests::TestSuite;

bool SerializeState(const CharacterSaveState &state, std::string &payload) {
  payload.clear();
  return SaveCodec::Serialize(state, payload);
}

std::string VersionOneFixtureFrom(std::string versionTwo) {
  const std::string versionTwoHeader = "LOSTSENSE_SAVE 2\n";
  const std::string versionOneHeader = "LOSTSENSE_SAVE 1\n";
  if (versionTwo.rfind(versionTwoHeader, 0U) == 0U) {
    versionTwo.replace(0U, versionTwoHeader.size(), versionOneHeader);
  }
  const std::size_t loot = versionTwo.find("LOOT ");
  if (loot != std::string::npos) {
    const std::size_t end = versionTwo.find('\n', loot);
    versionTwo.erase(loot, end - loot + 1U);
  }
  return versionTwo;
}

Stats::AttributeModifier *FindModifier(CharacterSaveState &state,
                                       const Stats::ModifierSource source) {
  const auto found = std::find_if(
      state.Combatant.Attributes.Modifiers.begin(),
      state.Combatant.Attributes.Modifiers.end(),
      [source](const Stats::AttributeModifier &modifier) {
        return modifier.Source == source;
      });
  return found == state.Combatant.Attributes.Modifiers.end() ? nullptr
                                                              : &*found;
}

void TestCaptureSerializeDeserializeAndRestore(TestSuite &suite) {
  RuntimeFixture fixture;
  suite.Expect(fixture.PopulatePersistentState(),
               "representative persistent runtime state populates");
  suite.Expect(fixture.Persistence.IsValid(),
               "aggregate persistence runtime is valid");

  const CharacterSaveState captured = fixture.Persistence.CaptureState();
  suite.Expect(fixture.Persistence.ValidateState(captured),
               "captured aggregate validates cross-system invariants");
  suite.Expect(captured.SchemaVersion == CurrentSaveSchemaVersion &&
                   captured.Character == Character &&
                   captured.CharacterClass == Knight &&
                   captured.SkillTreeDefinition == PersistenceTree,
               "save envelope preserves stable identity and schema");

  std::string firstPayload;
  std::string secondPayload;
  suite.Expect(SerializeState(captured, firstPayload) &&
                   SerializeState(captured, secondPayload) &&
                   firstPayload == secondPayload,
               "serialization is deterministic for identical state");

  CharacterSaveState decoded;
  const SaveDecodeResult decode = SaveCodec::Deserialize(firstPayload, decoded);
  suite.Expect(decode.Status == SaveDecodeStatus::Success &&
                   decode.SourceVersion == CurrentSaveSchemaVersion,
               "current schema deserializes successfully");
  std::string decodedPayload;
  suite.Expect(SerializeState(decoded, decodedPayload) &&
                   decodedPayload == firstPayload,
               "deserialize/serialize round-trip is byte deterministic");

  static_cast<void>(fixture.Owner.ApplyDamage(25.0));
  static_cast<void>(fixture.Random.NextUInt32());
  static_cast<void>(fixture.Loadout.Unequip(AbilityLoadoutSlot::Active1));
  static_cast<void>(fixture.Effects.Remove(
      fixture.Effects.CaptureState().ActiveEffects.front().InstanceId));
  static_cast<void>(fixture.Equipment.UnequipToInventory(
      fixture.InventoryStateAuthority, MainHand));
  static_cast<void>(fixture.InventoryStateAuthority.RemoveStack(Potion, 3U));
  static_cast<void>(fixture.Loot.RestoreState({5000U}));

  suite.Expect(fixture.Persistence.RestoreState(decoded) ==
                   CharacterRestoreResult::Success,
               "aggregate restore succeeds after broad live-state mutation");
  std::string restoredPayload;
  suite.Expect(SerializeState(fixture.Persistence.CaptureState(),
                              restoredPayload) &&
                   restoredPayload == firstPayload,
               "full restore recreates exact serialized aggregate state");
}

void TestTemporaryModifiersAreNotPersisted(TestSuite &suite) {
  RuntimeFixture fixture;
  suite.Expect(fixture.PopulatePersistentState(), "fixture populates");
  const Stats::ModifierId temporaryId{12345U};
  suite.Expect(fixture.Owner.AddAttributeModifier(
                   {temporaryId, Stats::CombatAttributes::MaxHealth,
                    Stats::ModifierOperation::Additive,
                    Stats::ModifierSource::Temporary, 50.0}),
               "temporary max-health modifier applies to live combatant");
  suite.ExpectNear(fixture.Owner.Health().Maximum(), 150.0,
                   "temporary modifier affects live derived pool");

  const CharacterSaveState captured = fixture.Persistence.CaptureState();
  suite.Expect(std::none_of(captured.Combatant.Attributes.Modifiers.begin(),
                            captured.Combatant.Attributes.Modifiers.end(),
                            [](const Stats::AttributeModifier &modifier) {
                              return modifier.Source ==
                                     Stats::ModifierSource::Temporary;
                            }),
               "capture excludes transient AttributeSet modifiers");
  suite.ExpectNear(captured.Combatant.Health.Maximum, 100.0,
                   "capture recomputes durable health maximum without transient modifier");
  suite.Expect(fixture.Persistence.ValidateState(captured),
               "filtered durable capture remains valid");
}

void TestCrossStateValidation(TestSuite &suite) {
  RuntimeFixture fixture;
  suite.Expect(fixture.PopulatePersistentState(), "fixture populates");
  const CharacterSaveState good = fixture.Persistence.CaptureState();

  CharacterSaveState duplicate = good;
  duplicate.Inventory.Instances.push_back(duplicate.Equipment.Items.front().Item);
  suite.Expect(!fixture.Persistence.ValidateState(duplicate),
               "inventory/equipment duplicate ownership is rejected");

  CharacterSaveState allocatorCollision = good;
  allocatorCollision.Loot.NextInstanceValue = 100U;
  suite.Expect(!fixture.Persistence.ValidateState(allocatorCollision),
               "loot allocator cannot collide with persistent instance IDs");

  CharacterSaveState unknownLoadout = good;
  unknownLoadout.Loadout.Slots[static_cast<std::size_t>(
      AbilityLoadoutSlot::Active1)] = AbilityId{99999U};
  suite.Expect(!fixture.Persistence.ValidateState(unknownLoadout),
               "unknown loadout ability is rejected");

  CharacterSaveState lockedLoadout = good;
  lockedLoadout.Abilities.Abilities.front().Unlocked = false;
  suite.Expect(!fixture.Persistence.ValidateState(lockedLoadout),
               "locked loadout ability and skill-unlock inconsistency reject");

  for (const Stats::ModifierSource source :
       {Stats::ModifierSource::Equipment, Stats::ModifierSource::SkillTree,
        Stats::ModifierSource::StatusEffect}) {
    CharacterSaveState corrupted = good;
    Stats::AttributeModifier *modifier = FindModifier(corrupted, source);
    suite.Expect(modifier != nullptr,
                 "fixture exposes each derived modifier namespace");
    if (modifier != nullptr) {
      modifier->Magnitude += 1.0;
      std::string before;
      suite.Expect(SerializeState(fixture.Persistence.CaptureState(), before),
                   "pre-failure aggregate serializes");
      suite.Expect(fixture.Persistence.RestoreState(corrupted) !=
                       CharacterRestoreResult::Success,
                   "incorrect derived modifier rejects aggregate restore");
      std::string after;
      suite.Expect(SerializeState(fixture.Persistence.CaptureState(), after) &&
                       before == after,
                   "failed derived-modifier restore rolls entire runtime back");
    }
  }
}

void TestTransactionalAggregateFailure(TestSuite &suite) {
  RuntimeFixture fixture;
  suite.Expect(fixture.PopulatePersistentState(), "fixture populates");
  std::string before;
  suite.Expect(SerializeState(fixture.Persistence.CaptureState(), before),
               "live state serializes before corruption attempt");

  CharacterSaveState corrupted = fixture.Persistence.CaptureState();
  corrupted.Inventory.Stacks.push_back({ItemId{999999U}, 1U});
  suite.Expect(fixture.Persistence.RestoreState(corrupted) ==
                   CharacterRestoreResult::SubsystemRestoreFailed,
               "subsystem-level corruption fails aggregate restore");
  std::string after;
  suite.Expect(SerializeState(fixture.Persistence.CaptureState(), after) &&
                   after == before,
               "aggregate rollback restores combat RNG effects abilities skills loadout inventory equipment and loot");
}

void TestVersionOneMigrationAndParserHardening(TestSuite &suite) {
  RuntimeFixture fixture;
  suite.Expect(fixture.PopulatePersistentState(), "fixture populates");
  std::string v2;
  suite.Expect(SerializeState(fixture.Persistence.CaptureState(), v2),
               "current fixture serializes for historical fixture generation");
  const std::string v1 = VersionOneFixtureFrom(v2);

  CharacterSaveState migrated;
  const SaveDecodeResult result = SaveCodec::Deserialize(v1, migrated);
  suite.Expect(result.Status == SaveDecodeStatus::Success &&
                   result.SourceVersion == 1U &&
                   migrated.SchemaVersion == CurrentSaveSchemaVersion,
               "V1 fixture migrates explicitly to V2");
  suite.Expect(migrated.Loot.NextInstanceValue == 201U,
               "V1 migration reconstructs allocator from highest persistent instance ID");
  suite.Expect(fixture.Persistence.ValidateState(migrated),
               "migrated V1 aggregate validates against current runtime");

  std::string invalidId = v1;
  const std::size_t item = invalidId.find("ITEM 200 ");
  suite.Expect(item != std::string::npos, "historical fixture contains inventory instance");
  if (item != std::string::npos) {
    invalidId.replace(item, std::string{"ITEM 200"}.size(), "ITEM 0");
    CharacterSaveState rejected;
    suite.Expect(SaveCodec::Deserialize(invalidId, rejected).Status ==
                     SaveDecodeStatus::MigrationFailed,
                 "V1 migration rejects invalid persistent instance ID");
  }

  std::string overflow = v1;
  const std::size_t maxItem = overflow.find("ITEM 200 ");
  if (maxItem != std::string::npos) {
    overflow.replace(maxItem + 5U, 3U,
                     "18446744073709551615");
    CharacterSaveState rejected;
    suite.Expect(SaveCodec::Deserialize(overflow, rejected).Status ==
                     SaveDecodeStatus::MigrationFailed,
                 "V1 migration rejects allocator overflow");
  }

  CharacterSaveState ignored;
  suite.Expect(SaveCodec::Deserialize("LOSTSENSE_SAVE 99\nEND\n", ignored)
                       .Status == SaveDecodeStatus::UnsupportedVersion,
               "future schema is rejected explicitly");
  suite.Expect(SaveCodec::Deserialize(v2 + "TRAILING\n", ignored).Status ==
                   SaveDecodeStatus::Malformed,
               "trailing payload records are rejected");

  std::string malformedFloat = v2;
  const std::size_t health = malformedFloat.find("HEALTH ");
  if (health != std::string::npos) {
    const std::size_t number = health + 7U;
    malformedFloat.replace(number, 16U, "zzzzzzzzzzzzzzzz");
    suite.Expect(SaveCodec::Deserialize(malformedFloat, ignored).Status ==
                     SaveDecodeStatus::Malformed,
                 "malformed exact-double encoding is rejected");
  }

  const std::string tooLarge((8U * 1024U * 1024U) + 1U, 'x');
  suite.Expect(SaveCodec::Deserialize(tooLarge, ignored).Status ==
                   SaveDecodeStatus::PayloadTooLarge,
               "parser rejects payload beyond hard byte limit");
}

void TestExactDoubleRoundTrip(TestSuite &suite) {
  RuntimeFixture fixture;
  suite.Expect(fixture.PopulatePersistentState(), "fixture populates");
  CharacterSaveState state = fixture.Persistence.CaptureState();
  const double exact = std::bit_cast<double>(std::uint64_t{0x3fd5555555555555ULL});
  state.Combatant.Attributes.BaseValues.front().Value = exact;
  std::string payload;
  suite.Expect(SerializeState(state, payload), "exact-double state serializes");
  CharacterSaveState decoded;
  suite.Expect(SaveCodec::Deserialize(payload, decoded).Status ==
                   SaveDecodeStatus::Success,
               "exact-double payload decodes");
  suite.Expect(std::bit_cast<std::uint64_t>(
                   decoded.Combatant.Attributes.BaseValues.front().Value) ==
                   std::bit_cast<std::uint64_t>(exact),
               "double bit pattern survives text round-trip exactly");
}

} // namespace

int main() {
  TestSuite suite{"gameplay.persistence"};
  TestCaptureSerializeDeserializeAndRestore(suite);
  TestTemporaryModifiersAreNotPersisted(suite);
  TestCrossStateValidation(suite);
  TestTransactionalAggregateFailure(suite);
  TestVersionOneMigrationAndParserHardening(suite);
  TestExactDoubleRoundTrip(suite);
  return suite.Finish();
}
