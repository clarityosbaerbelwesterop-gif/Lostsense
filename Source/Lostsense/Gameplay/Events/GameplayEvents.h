#pragma once

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"
#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Gameplay/Items/Equipment.h"
#include "Lostsense/Gameplay/Progression/SkillGraph.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Lostsense::Gameplay {

enum class GameplayEventType : std::uint8_t {
  DamageApplied,
  CombatantDied,
  EffectApplied,
  EffectRemoved,
  AbilityActivated,
  ItemDropped,
  ItemPickedUp,
  ItemEquipped,
  ItemUnequipped,
  SkillAllocated,
  SkillRefunded,
};

struct GameplayEvent final {
  std::uint64_t Sequence{0U};
  GameplayEventType Type{GameplayEventType::DamageApplied};
  Combat::CombatantId Source{};
  Combat::CombatantId Target{};
  EffectId Effect{};
  EffectInstanceId EffectInstance{};
  AbilityId Ability{};
  ItemId Item{};
  ItemInstanceId ItemInstance{};
  EquipmentSlotId EquipmentSlot{};
  SkillNodeId SkillNode{};
  double Value{0.0};
  std::uint32_t Quantity{0U};
};

struct GameplayEventCheckpoint final {
  std::size_t EventCount{0U};
  std::uint64_t NextSequence{1U};
  std::uint64_t Epoch{0U};
};

class GameplayEventStream final {
public:
  explicit GameplayEventStream(std::size_t capacity = 4096U) noexcept;

  [[nodiscard]] std::size_t Capacity() const noexcept { return capacity_; }
  [[nodiscard]] std::size_t Size() const noexcept { return events_.size(); }
  [[nodiscard]] bool Empty() const noexcept { return events_.empty(); }
  [[nodiscard]] const std::vector<GameplayEvent> &Events() const noexcept {
    return events_;
  }

  // Presentation/event delivery must never become a second gameplay authority.
  // A full stream therefore reports failure without altering committed
  // gameplay.
  [[nodiscard]] bool Publish(GameplayEvent event) noexcept;
  [[nodiscard]] GameplayEventCheckpoint Checkpoint() const noexcept;
  [[nodiscard]] bool Rollback(GameplayEventCheckpoint checkpoint) noexcept;
  [[nodiscard]] std::vector<GameplayEvent> Drain();
  void Clear() noexcept;

private:
  void AdvanceCheckpointEpoch() noexcept;

  std::size_t capacity_{0U};
  std::uint64_t nextSequence_{1U};
  std::uint64_t checkpointEpoch_{1U};
  std::uint64_t epochStartSequence_{1U};
  std::vector<GameplayEvent> events_{};
};

class GameplayEventAuthority final {
public:
  [[nodiscard]] static Combat::DamageApplication
  ApplyDamage(Combat::CombatantId source, Combat::Combatant &target,
              double amount, GameplayEventStream &events) noexcept;

  [[nodiscard]] static EffectApplyOutcome
  ApplyEffect(EffectRuntime &runtime, EffectId effect,
              Combat::CombatantId source, GameplayEventStream &events);

  [[nodiscard]] static bool RemoveEffect(EffectRuntime &runtime,
                                         EffectInstanceId instance,
                                         GameplayEventStream &events) noexcept;

  [[nodiscard]] static AbilityActivationOutcome
  ActivateAbility(AbilityRuntime &runtime, AbilityId ability,
                  const AbilityTarget &target, Combat::CombatantId actor,
                  GameplayEventStream &events);

  [[nodiscard]] static SkillOperationResult
  AllocateSkill(SkillTreeRuntime &runtime, SkillNodeId node,
                Combat::CombatantId actor, GameplayEventStream &events);

  [[nodiscard]] static SkillOperationResult
  RefundSkill(SkillTreeRuntime &runtime, SkillNodeId node,
              Combat::CombatantId actor, GameplayEventStream &events);

  [[nodiscard]] static InventoryResult
  PickupInstance(Inventory &inventory, ItemInstance instance,
                 Combat::CombatantId actor, GameplayEventStream &events);

  [[nodiscard]] static InventoryResult
  DropInstance(Inventory &inventory, ItemInstanceId instance,
               Combat::CombatantId actor, ItemInstance &dropped,
               GameplayEventStream &events);

  [[nodiscard]] static EquipmentResult
  Equip(EquipmentRuntime &equipment, Inventory &inventory,
        ItemInstanceId instance, EquipmentSlotId slot,
        Combat::CombatantId actor, GameplayEventStream &events);

  [[nodiscard]] static EquipmentResult Unequip(EquipmentRuntime &equipment,
                                               Inventory &inventory,
                                               EquipmentSlotId slot,
                                               Combat::CombatantId actor,
                                               GameplayEventStream &events);
};

} // namespace Lostsense::Gameplay
