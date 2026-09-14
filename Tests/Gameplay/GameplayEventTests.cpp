#include "Lostsense/Gameplay/Events/GameplayEvents.h"
#include "PersistenceTestFixtures.h"
#include "TestHarness.h"

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using namespace Lostsense::Gameplay::PersistenceTestData;
using namespace Lostsense::Gameplay::TestData;
using Tests::TestSuite;

void TestStreamOrderingCapacityAndRollback(TestSuite &suite) {
  GameplayEventStream stream{3U};
  GameplayEvent first;
  first.Type = GameplayEventType::DamageApplied;
  suite.Expect(stream.Publish(first), "first event publishes");
  const GameplayEventCheckpoint checkpoint = stream.Checkpoint();

  GameplayEvent second;
  second.Type = GameplayEventType::AbilityActivated;
  GameplayEvent third;
  third.Type = GameplayEventType::ItemPickedUp;
  suite.Expect(stream.Publish(second) && stream.Publish(third),
               "events publish until bounded capacity");
  suite.Expect(stream.Events().size() == 3U &&
                   stream.Events()[0].Sequence == 1U &&
                   stream.Events()[1].Sequence == 2U &&
                   stream.Events()[2].Sequence == 3U,
               "event sequence is deterministic and monotonic");
  suite.Expect(!stream.Publish(first) && stream.Size() == 3U,
               "overflow is explicit and does not grow history");

  GameplayEventCheckpoint forged = checkpoint;
  forged.NextSequence = 1U;
  suite.Expect(!stream.Rollback(forged),
               "forged checkpoint cannot create duplicate sequence IDs");
  suite.Expect(stream.Rollback(checkpoint),
               "checkpoint rollback removes later events");
  suite.Expect(stream.Size() == 1U, "rollback restores checkpoint size");
  suite.Expect(stream.Publish(second) && stream.Events().back().Sequence == 2U,
               "rollback restores deterministic next sequence");

  const GameplayEventCheckpoint staleAfterDrain = stream.Checkpoint();
  const std::vector<GameplayEvent> drained = stream.Drain();
  suite.Expect(drained.size() == 2U && stream.Empty(),
               "drain transfers committed event batch");
  suite.Expect(!stream.Rollback(staleAfterDrain),
               "checkpoint cannot rewind sequence after delivered drain");
  suite.Expect(stream.Publish(third) && stream.Events().back().Sequence == 3U,
               "sequence remains monotonic across drain");
  stream.Clear();
  suite.Expect(stream.Empty() && stream.Publish(first) &&
                   stream.Events().back().Sequence == 4U,
               "clear does not reuse historical sequence IDs");

  suite.Expect(!stream.Rollback({99U, 1U, 1U}),
               "malformed checkpoint is rejected");
}

void TestCommittedOperationsPublishOnlyAfterSuccess(TestSuite &suite) {
  RuntimeFixture fixture;
  GameplayEventStream events{32U};

  const std::size_t initial = events.Size();
  const Combat::DamageApplication invalid = GameplayEventAuthority::ApplyDamage(
      fixture.Owner.Id(), fixture.Owner, -1.0, events);
  suite.Expect(!invalid.WasValid && events.Size() == initial,
               "failed damage operation emits no event");

  suite.Expect(GameplayEventAuthority::AllocateSkill(
                   fixture.Skills, AbilityNode, fixture.Owner.Id(), events) ==
                   SkillOperationResult::Success,
               "successful skill allocation commits");
  suite.Expect(events.Events().back().Type == GameplayEventType::SkillAllocated,
               "skill allocation publishes typed event");
  const std::size_t afterAllocation = events.Size();
  suite.Expect(GameplayEventAuthority::AllocateSkill(
                   fixture.Skills, AbilityNode, fixture.Owner.Id(), events) ==
                       SkillOperationResult::AlreadyAllocated &&
                   events.Size() == afterAllocation,
               "failed allocation emits no committed event");

  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active1,
                                     KnightAbility) == LoadoutResult::Success,
               "unlocked ability equips for event test");
  AbilityTarget noTarget{};
  const auto activation = GameplayEventAuthority::ActivateAbility(
      fixture.Abilities, KnightAbility, noTarget, fixture.Owner.Id(), events);
  suite.Expect(activation.Result == AbilityActivationResult::Success &&
                   events.Events().back().Type ==
                       GameplayEventType::AbilityActivated,
               "successful ability activation publishes after commit");
  const std::size_t afterActivation = events.Size();
  suite.Expect(GameplayEventAuthority::ActivateAbility(
                   fixture.Abilities, KnightAbility, noTarget,
                   fixture.Owner.Id(), events)
                           .Result == AbilityActivationResult::OnCooldown &&
                   events.Size() == afterActivation,
               "failed cooldown activation leaves stream unchanged");

  const EffectApplyOutcome applied = GameplayEventAuthority::ApplyEffect(
      fixture.Effects, PersistentBuff, fixture.Owner.Id(), events);
  suite.Expect(applied.Result == EffectApplyResult::Applied &&
                   events.Events().back().Type ==
                       GameplayEventType::EffectApplied,
               "effect apply publishes committed effect event");
  suite.Expect(
      GameplayEventAuthority::RemoveEffect(fixture.Effects, applied.InstanceId,
                                           events) &&
          events.Events().back().Type == GameplayEventType::EffectRemoved &&
          events.Events().back().Effect == PersistentBuff,
      "effect removal derives authoritative effect identity");
  const std::size_t afterRemoval = events.Size();
  suite.Expect(!GameplayEventAuthority::RemoveEffect(
                   fixture.Effects, applied.InstanceId, events) &&
                   events.Size() == afterRemoval,
               "failed effect removal emits nothing");
}

void TestAbilityDamagePublishesCommittedCombatEvents(TestSuite &suite) {
  constexpr AbilityId Strike{799U};
  constexpr ClassId EventKnight{798U};

  Combat::Combatant owner{Combat::CombatantId{3100U},
                          Combat::CombatantKind::Player};
  Combat::Combatant target{Combat::CombatantId{3200U},
                           Combat::CombatantKind::Enemy};
  EffectRuntime ownerEffects{owner, {}};
  EffectRuntime targetEffects{target, {}};
  Core::DeterministicRandom random{12U, 5U};

  AbilityDefinition strike;
  strike.Id = Strike;
  strike.RequiredClass = EventKnight;
  strike.TargetRule = AbilityTargetRule::Hostile;
  strike.DealsDamage = true;
  strike.Damage
      .BaseDamage[static_cast<std::size_t>(Combat::DamageType::Physical)] =
      500.0;
  strike.Damage.CanCritical = false;
  strike.Damage.CanBlock = false;

  AbilityRuntime abilities{owner, ownerEffects, random, EventKnight, {strike}};
  suite.Expect(abilities.IsValid() && abilities.Unlock(Strike),
               "damaging event-test ability initializes and unlocks");

  GameplayEventStream events{8U};
  AbilityTarget hostile{&target, &targetEffects, TargetRelation::Hostile};
  const AbilityActivationOutcome outcome =
      GameplayEventAuthority::ActivateAbility(abilities, Strike, hostile,
                                              owner.Id(), events);

  suite.Expect(outcome.Result == AbilityActivationResult::Success &&
                   outcome.DamageResolved &&
                   outcome.Damage.AppliedToHealth.BecameDead,
               "damaging ability commits lethal authoritative damage");
  suite.Expect(
      events.Size() == 3U,
      "lethal ability publishes activation damage and death exactly once");
  if (events.Size() == 3U) {
    suite.Expect(
        events.Events()[0].Type == GameplayEventType::AbilityActivated &&
            events.Events()[1].Type == GameplayEventType::DamageApplied &&
            events.Events()[2].Type == GameplayEventType::CombatantDied,
        "ability combat event ordering is deterministic");
    suite.Expect(events.Events()[1].Ability == Strike &&
                     events.Events()[1].Source == owner.Id() &&
                     events.Events()[1].Target == target.Id() &&
                     events.Events()[1].Value > 0.0 &&
                     events.Events()[2].Ability == Strike,
                 "ability damage and death events preserve causal identity");
  }
}

void TestInventoryEquipmentAndDeathEventOrdering(TestSuite &suite) {
  RuntimeFixture fixture;
  GameplayEventStream events{32U};

  suite.Expect(GameplayEventAuthority::PickupInstance(
                   fixture.InventoryStateAuthority,
                   CommonInstance(ItemInstanceId{250U}, Helmet),
                   fixture.Owner.Id(), events) == InventoryResult::Success &&
                   events.Events().back().Type ==
                       GameplayEventType::ItemPickedUp,
               "pickup publishes only after inventory ownership commits");
  ItemInstance dropped;
  suite.Expect(
      GameplayEventAuthority::DropInstance(
          fixture.InventoryStateAuthority, ItemInstanceId{250U},
          fixture.Owner.Id(), dropped, events) == InventoryResult::Success &&
          dropped.InstanceId == ItemInstanceId{250U} &&
          events.Events().back().Type == GameplayEventType::ItemDropped,
      "drop publishes after ownership leaves inventory");
  const std::size_t afterDrop = events.Size();
  suite.Expect(GameplayEventAuthority::DropInstance(
                   fixture.InventoryStateAuthority, ItemInstanceId{250U},
                   fixture.Owner.Id(), dropped,
                   events) != InventoryResult::Success &&
                   events.Size() == afterDrop,
               "failed duplicate drop emits no committed event");

  suite.Expect(fixture.InventoryStateAuthority.AddInstance(CommonInstance(
                   ItemInstanceId{300U}, Sword)) == InventoryResult::Success,
               "equipment event item enters inventory");
  suite.Expect(GameplayEventAuthority::Equip(
                   fixture.Equipment, fixture.InventoryStateAuthority,
                   ItemInstanceId{300U}, MainHand, fixture.Owner.Id(),
                   events) == EquipmentResult::Success,
               "equip wrapper commits through EquipmentRuntime");
  suite.Expect(events.Events().back().Type == GameplayEventType::ItemEquipped,
               "equip publishes typed event after success");
  const std::size_t afterEquip = events.Size();
  suite.Expect(GameplayEventAuthority::Equip(
                   fixture.Equipment, fixture.InventoryStateAuthority,
                   ItemInstanceId{999U}, MainHand, fixture.Owner.Id(),
                   events) != EquipmentResult::Success &&
                   events.Size() == afterEquip,
               "failed equip emits no event");

  suite.Expect(
      GameplayEventAuthority::Unequip(
          fixture.Equipment, fixture.InventoryStateAuthority, MainHand,
          fixture.Owner.Id(), events) == EquipmentResult::Success &&
          events.Events().back().Type == GameplayEventType::ItemUnequipped,
      "unequip publishes after inventory/equipment transaction commits");

  Combat::Combatant enemy{Combat::CombatantId{2000U},
                          Combat::CombatantKind::Enemy};
  const std::size_t beforeDeath = events.Size();
  const auto damage = GameplayEventAuthority::ApplyDamage(fixture.Owner.Id(),
                                                          enemy, 500.0, events);
  suite.Expect(damage.BecameDead && events.Size() == beforeDeath + 2U,
               "lethal damage emits damage and death events");
  suite.Expect(events.Events()[beforeDeath].Type ==
                       GameplayEventType::DamageApplied &&
                   events.Events()[beforeDeath + 1U].Type ==
                       GameplayEventType::CombatantDied,
               "lethal event ordering is deterministic");
}

} // namespace

int main() {
  TestSuite suite{"gameplay.events"};
  TestStreamOrderingCapacityAndRollback(suite);
  TestCommittedOperationsPublishOnlyAfterSuccess(suite);
  TestAbilityDamagePublishesCommittedCombatEvents(suite);
  TestInventoryEquipmentAndDeathEventOrdering(suite);
  return suite.Finish();
}
