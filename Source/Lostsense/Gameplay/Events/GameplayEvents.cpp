#include "Lostsense/Gameplay/Events/GameplayEvents.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

[[nodiscard]] bool IsCommittedEffectResult(const EffectApplyResult result) {
  switch (result) {
  case EffectApplyResult::Applied:
  case EffectApplyResult::Replaced:
  case EffectApplyResult::Refreshed:
  case EffectApplyResult::Stacked:
    return true;
  case EffectApplyResult::Immune:
  case EffectApplyResult::AtMaxStacks:
  case EffectApplyResult::UnknownEffect:
  case EffectApplyResult::InvalidSource:
  case EffectApplyResult::InvalidRuntime:
  case EffectApplyResult::InternalFailure:
    return false;
  }
  return false;
}

void PublishIgnoringBackpressure(GameplayEventStream &events,
                                 GameplayEvent event) noexcept {
  static_cast<void>(events.Publish(std::move(event)));
}

} // namespace

GameplayEventStream::GameplayEventStream(const std::size_t capacity) noexcept
    : capacity_{capacity} {
  events_.reserve(capacity_);
}

bool GameplayEventStream::Publish(GameplayEvent event) noexcept {
  if (capacity_ == 0U || events_.size() >= capacity_ ||
      nextSequence_ == 0U || nextSequence_ ==
                                 std::numeric_limits<std::uint64_t>::max()) {
    return false;
  }
  event.Sequence = nextSequence_++;
  events_.push_back(event);
  return true;
}

GameplayEventCheckpoint GameplayEventStream::Checkpoint() const noexcept {
  return {events_.size(), nextSequence_};
}

bool GameplayEventStream::Rollback(
    const GameplayEventCheckpoint checkpoint) noexcept {
  if (checkpoint.EventCount > events_.size() || checkpoint.NextSequence == 0U ||
      checkpoint.NextSequence > nextSequence_) {
    return false;
  }
  events_.resize(checkpoint.EventCount);
  nextSequence_ = checkpoint.NextSequence;
  return true;
}

std::vector<GameplayEvent> GameplayEventStream::Drain() {
  std::vector<GameplayEvent> drained = std::move(events_);
  events_.clear();
  events_.reserve(capacity_);
  return drained;
}

void GameplayEventStream::Clear() noexcept { events_.clear(); }

Combat::DamageApplication GameplayEventAuthority::ApplyDamage(
    const Combat::CombatantId source, Combat::Combatant &target,
    const double amount, GameplayEventStream &events) noexcept {
  const Combat::DamageApplication result = target.ApplyDamage(amount);
  if (!result.WasValid || result.Applied <= 0.0) {
    return result;
  }

  GameplayEvent damage;
  damage.Type = GameplayEventType::DamageApplied;
  damage.Source = source;
  damage.Target = target.Id();
  damage.Value = result.Applied;
  PublishIgnoringBackpressure(events, damage);

  if (result.BecameDead) {
    GameplayEvent death;
    death.Type = GameplayEventType::CombatantDied;
    death.Source = source;
    death.Target = target.Id();
    PublishIgnoringBackpressure(events, death);
  }
  return result;
}

EffectApplyOutcome GameplayEventAuthority::ApplyEffect(
    EffectRuntime &runtime, const EffectId effect,
    const Combat::CombatantId source, GameplayEventStream &events) {
  const EffectApplyOutcome outcome = runtime.Apply(effect, source);
  if (IsCommittedEffectResult(outcome.Result)) {
    GameplayEvent event;
    event.Type = GameplayEventType::EffectApplied;
    event.Source = source;
    event.Target = runtime.OwnerId();
    event.Effect = effect;
    event.EffectInstance = outcome.InstanceId;
    PublishIgnoringBackpressure(events, event);
  }
  return outcome;
}

bool GameplayEventAuthority::RemoveEffect(
    EffectRuntime &runtime, const EffectInstanceId instance,
    GameplayEventStream &events) noexcept {
  const EffectRuntimeState before = runtime.CaptureState();
  const auto found = std::find_if(
      before.ActiveEffects.begin(), before.ActiveEffects.end(),
      [instance](const ActiveEffectState &active) {
        return active.InstanceId == instance;
      });
  if (found == before.ActiveEffects.end() || !runtime.Remove(instance)) {
    return false;
  }
  GameplayEvent event;
  event.Type = GameplayEventType::EffectRemoved;
  event.Target = runtime.OwnerId();
  event.Effect = found->DefinitionId;
  event.EffectInstance = instance;
  PublishIgnoringBackpressure(events, event);
  return true;
}

AbilityActivationOutcome GameplayEventAuthority::ActivateAbility(
    AbilityRuntime &runtime, const AbilityId ability,
    const AbilityTarget &target, const Combat::CombatantId actor,
    GameplayEventStream &events) {
  AbilityActivationOutcome outcome = runtime.Activate(ability, target);
  if (outcome.Result == AbilityActivationResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::AbilityActivated;
    event.Source = actor;
    event.Target = target.Combatant == nullptr ? Combat::CombatantId{}
                                                : target.Combatant->Id();
    event.Ability = ability;
    PublishIgnoringBackpressure(events, event);
  }
  return outcome;
}

SkillOperationResult GameplayEventAuthority::AllocateSkill(
    SkillTreeRuntime &runtime, const SkillNodeId node,
    const Combat::CombatantId actor, GameplayEventStream &events) {
  const SkillOperationResult result = runtime.Allocate(node);
  if (result == SkillOperationResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::SkillAllocated;
    event.Source = actor;
    event.SkillNode = node;
    PublishIgnoringBackpressure(events, event);
  }
  return result;
}

SkillOperationResult GameplayEventAuthority::RefundSkill(
    SkillTreeRuntime &runtime, const SkillNodeId node,
    const Combat::CombatantId actor, GameplayEventStream &events) {
  const SkillOperationResult result = runtime.Refund(node);
  if (result == SkillOperationResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::SkillRefunded;
    event.Source = actor;
    event.SkillNode = node;
    PublishIgnoringBackpressure(events, event);
  }
  return result;
}

InventoryResult GameplayEventAuthority::PickupInstance(
    Inventory &inventory, ItemInstance instance,
    const Combat::CombatantId actor, GameplayEventStream &events) {
  const ItemId item = instance.DefinitionId;
  const ItemInstanceId id = instance.InstanceId;
  const InventoryResult result = inventory.AddInstance(std::move(instance));
  if (result == InventoryResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::ItemPickedUp;
    event.Target = actor;
    event.Item = item;
    event.ItemInstance = id;
    event.Quantity = 1U;
    PublishIgnoringBackpressure(events, event);
  }
  return result;
}

InventoryResult GameplayEventAuthority::DropInstance(
    Inventory &inventory, const ItemInstanceId instance,
    const Combat::CombatantId actor, ItemInstance &dropped,
    GameplayEventStream &events) {
  const InventoryResult result = inventory.TakeInstance(instance, dropped);
  if (result == InventoryResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::ItemDropped;
    event.Source = actor;
    event.Item = dropped.DefinitionId;
    event.ItemInstance = dropped.InstanceId;
    event.Quantity = 1U;
    PublishIgnoringBackpressure(events, event);
  }
  return result;
}

EquipmentResult GameplayEventAuthority::Equip(
    EquipmentRuntime &equipment, Inventory &inventory,
    const ItemInstanceId instance, const EquipmentSlotId slot,
    const Combat::CombatantId actor, GameplayEventStream &events) {
  const ItemInstance *candidate = inventory.FindInstance(instance);
  const ItemId item = candidate == nullptr ? ItemId{} : candidate->DefinitionId;
  const EquipmentResult result =
      equipment.EquipFromInventory(inventory, instance, slot);
  if (result == EquipmentResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::ItemEquipped;
    event.Source = actor;
    event.Item = item;
    event.ItemInstance = instance;
    event.EquipmentSlot = slot;
    PublishIgnoringBackpressure(events, event);
  }
  return result;
}

EquipmentResult GameplayEventAuthority::Unequip(
    EquipmentRuntime &equipment, Inventory &inventory,
    const EquipmentSlotId slot, const Combat::CombatantId actor,
    GameplayEventStream &events) {
  const ItemInstance *equipped = equipment.EquippedAt(slot);
  const ItemId item = equipped == nullptr ? ItemId{} : equipped->DefinitionId;
  const ItemInstanceId instance =
      equipped == nullptr ? ItemInstanceId{} : equipped->InstanceId;
  const EquipmentResult result = equipment.UnequipToInventory(inventory, slot);
  if (result == EquipmentResult::Success) {
    GameplayEvent event;
    event.Type = GameplayEventType::ItemUnequipped;
    event.Source = actor;
    event.Item = item;
    event.ItemInstance = instance;
    event.EquipmentSlot = slot;
    PublishIgnoringBackpressure(events, event);
  }
  return result;
}

} // namespace Lostsense::Gameplay
