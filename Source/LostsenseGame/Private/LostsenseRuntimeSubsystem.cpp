#include "LostsenseRuntimeSubsystem.h"

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Events/GameplayEvents.h"
#include "Lostsense/Gameplay/Items/Equipment.h"
#include "Lostsense/Gameplay/Items/Inventory.h"
#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"
#include "Lostsense/Gameplay/Persistence/CharacterPersistence.h"
#include "Lostsense/Gameplay/Persistence/SaveCodec.h"
#include "Lostsense/Stats/CombatAttributes.h"

#include "Math/UnrealMathUtility.h"

#include <limits>
#include <string>

namespace {
ELostsensePresentationEventType
ToPresentationType(const Lostsense::Gameplay::GameplayEventType Type) {
  using EventType = Lostsense::Gameplay::GameplayEventType;
  switch (Type) {
  case EventType::DamageApplied:
    return ELostsensePresentationEventType::DamageApplied;
  case EventType::CombatantDied:
    return ELostsensePresentationEventType::CombatantDied;
  case EventType::EffectApplied:
    return ELostsensePresentationEventType::EffectApplied;
  case EventType::EffectRemoved:
    return ELostsensePresentationEventType::EffectRemoved;
  case EventType::AbilityActivated:
    return ELostsensePresentationEventType::AbilityActivated;
  case EventType::ItemDropped:
    return ELostsensePresentationEventType::ItemDropped;
  case EventType::ItemPickedUp:
    return ELostsensePresentationEventType::ItemPickedUp;
  case EventType::ItemEquipped:
    return ELostsensePresentationEventType::ItemEquipped;
  case EventType::ItemUnequipped:
    return ELostsensePresentationEventType::ItemUnequipped;
  case EventType::SkillAllocated:
    return ELostsensePresentationEventType::SkillAllocated;
  case EventType::SkillRefunded:
    return ELostsensePresentationEventType::SkillRefunded;
  }

  return ELostsensePresentationEventType::DamageApplied;
}

int64 CheckedSignedId(const std::uint64_t Value) {
  return Value <= static_cast<std::uint64_t>(MAX_int64)
             ? static_cast<int64>(Value)
             : MAX_int64;
}

int64 PrimaryContentId(const Lostsense::Gameplay::GameplayEvent &Event) {
  if (Event.Ability.IsValid()) {
    return static_cast<int64>(Event.Ability.Value);
  }
  if (Event.Effect.IsValid()) {
    return static_cast<int64>(Event.Effect.Value);
  }
  if (Event.SkillNode.IsValid()) {
    return static_cast<int64>(Event.SkillNode.Value);
  }
  if (Event.ItemInstance.IsValid()) {
    return CheckedSignedId(Event.ItemInstance.Value);
  }
  if (Event.Item.IsValid()) {
    return static_cast<int64>(Event.Item.Value);
  }
  return 0;
}

bool IsActivationSuccess(
    const Lostsense::Gameplay::AbilityActivationOutcome &Outcome) {
  return Outcome.Result ==
         Lostsense::Gameplay::AbilityActivationResult::Success;
}
} // namespace

struct ULostsenseRuntimeSubsystem::FPortableRuntime {
  Lostsense::Core::DeterministicRandom Random{0x4C4F535453454E53ULL, 7U};
  Lostsense::Combat::Combatant Player{Lostsense::Combat::CombatantId{1U},
                                      Lostsense::Combat::CombatantKind::Player};
  Lostsense::Gameplay::GameplayEventStream Events{4096U};
  Lostsense::Gameplay::ItemCatalog Items{
      Lostsense::Gameplay::FirstSlice::BuildItemCatalog()};
  Lostsense::Gameplay::LootCatalog LootTables{
      Lostsense::Gameplay::FirstSlice::BuildLootCatalog()};
  Lostsense::Gameplay::EffectRuntime Effects{
      Player, Lostsense::Gameplay::FirstSlice::BuildKnightEffects()};
  Lostsense::Gameplay::AbilityRuntime Abilities{
      Player, Effects, Random, Lostsense::Gameplay::FirstSlice::KnightClass,
      Lostsense::Gameplay::FirstSlice::BuildKnightAbilities()};
  Lostsense::Gameplay::AbilityLoadout Loadout{Abilities};
  Lostsense::Gameplay::Inventory InventoryState{Items, 30U};
  Lostsense::Gameplay::EquipmentRuntime Equipment{
      Items, Player, Lostsense::Gameplay::FirstSlice::KnightClass};
  Lostsense::Gameplay::LootRuntime Loot{Items, LootTables, Random};
  Lostsense::Gameplay::SkillTreeRuntime Skills{
      Player, Abilities, Loadout,
      Lostsense::Gameplay::FirstSlice::BuildKnightScarAtlas(), 1U};
  Lostsense::Gameplay::CharacterPersistence Persistence{
      Lostsense::Gameplay::CharacterPersistentId{1U},
      Lostsense::Gameplay::FirstSlice::KnightClass,
      Player,
      Random,
      Effects,
      Abilities,
      Skills,
      Loadout,
      InventoryState,
      Equipment,
      Loot};
  bool Ready{false};

  FPortableRuntime() {
    using namespace Lostsense::Gameplay;
    using namespace Lostsense::Gameplay::FirstSlice;

    const bool AttributesReady =
        Player.SetBaseAttribute(Lostsense::Stats::CombatAttributes::AttackPower,
                                10.0) &&
        Player.SetBaseAttribute(Lostsense::Stats::CombatAttributes::Armor, 8.0);

    const bool CoreAbilitiesReady = Abilities.Unlock(KnightPrimaryAttack) &&
                                    Abilities.Unlock(KnightHeavyAttack) &&
                                    Abilities.Unlock(KnightDodge) &&
                                    Abilities.Unlock(KnightGuard);

    const bool CoreLoadoutReady =
        Loadout.Equip(AbilityLoadoutSlot::PrimaryAttack, KnightPrimaryAttack) ==
            LoadoutResult::Success &&
        Loadout.Equip(AbilityLoadoutSlot::SecondaryAttack, KnightHeavyAttack) ==
            LoadoutResult::Success &&
        Loadout.Equip(AbilityLoadoutSlot::Dodge, KnightDodge) ==
            LoadoutResult::Success &&
        Loadout.Equip(AbilityLoadoutSlot::ClassMechanic, KnightGuard) ==
            LoadoutResult::Success;

    Ready = AttributesReady && Items.Validate() && LootTables.Validate(Items) &&
            Effects.IsValid() && Abilities.IsValid() && Loadout.IsValid() &&
            InventoryState.IsValid() && Equipment.IsValid() && Loot.IsValid() &&
            Skills.IsValid() && Persistence.IsValid() && CoreAbilitiesReady &&
            CoreLoadoutReady;
  }
};

ULostsenseRuntimeSubsystem::ULostsenseRuntimeSubsystem() = default;
ULostsenseRuntimeSubsystem::~ULostsenseRuntimeSubsystem() = default;

void ULostsenseRuntimeSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  PortableRuntime = MakeUnique<FPortableRuntime>();
}

void ULostsenseRuntimeSubsystem::Deinitialize() {
  PortableRuntime.Reset();
  Super::Deinitialize();
}

bool ULostsenseRuntimeSubsystem::IsPortableRuntimeReady() const {
  return PortableRuntime.IsValid() && PortableRuntime->Ready;
}

double ULostsenseRuntimeSubsystem::GetPlayerHealth() const {
  return PortableRuntime.IsValid() ? PortableRuntime->Player.Health().Current()
                                   : 0.0;
}

double ULostsenseRuntimeSubsystem::GetPlayerMaximumHealth() const {
  return PortableRuntime.IsValid() ? PortableRuntime->Player.Health().Maximum()
                                   : 0.0;
}

double ULostsenseRuntimeSubsystem::GetPlayerResource() const {
  return PortableRuntime.IsValid()
             ? PortableRuntime->Player.Resource().Current()
             : 0.0;
}

double ULostsenseRuntimeSubsystem::GetPlayerMaximumResource() const {
  return PortableRuntime.IsValid()
             ? PortableRuntime->Player.Resource().Maximum()
             : 0.0;
}

int32 ULostsenseRuntimeSubsystem::GetUnspentSkillPoints() const {
  if (!PortableRuntime.IsValid()) {
    return 0;
  }
  return static_cast<int32>(
      FMath::Min<std::uint32_t>(PortableRuntime->Skills.UnspentPoints(),
                                static_cast<std::uint32_t>(MAX_int32)));
}

bool ULostsenseRuntimeSubsystem::GrantSkillPoints(const int32 Points) {
  return PortableRuntime.IsValid() && Points > 0 &&
         PortableRuntime->Skills.GrantPoints(
             static_cast<std::uint32_t>(Points));
}

bool ULostsenseRuntimeSubsystem::AllocateSkillNode(const int32 SkillNodeId) {
  if (!PortableRuntime.IsValid() || SkillNodeId <= 0) {
    return false;
  }

  const Lostsense::Gameplay::SkillOperationResult Result =
      Lostsense::Gameplay::GameplayEventAuthority::AllocateSkill(
          PortableRuntime->Skills,
          Lostsense::Gameplay::SkillNodeId{
              static_cast<std::uint32_t>(SkillNodeId)},
          PortableRuntime->Player.Id(), PortableRuntime->Events);
  return Result == Lostsense::Gameplay::SkillOperationResult::Success;
}

bool ULostsenseRuntimeSubsystem::EquipAbilityInSlot(const int32 SlotIndex,
                                                    const int32 AbilityId) {
  if (!PortableRuntime.IsValid() || SlotIndex < 0 ||
      SlotIndex >=
          static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Count) ||
      AbilityId <= 0) {
    return false;
  }

  const auto Slot =
      static_cast<Lostsense::Gameplay::AbilityLoadoutSlot>(SlotIndex);
  return PortableRuntime->Loadout.Equip(
             Slot, Lostsense::Gameplay::AbilityId{static_cast<std::uint32_t>(
                       AbilityId)}) ==
         Lostsense::Gameplay::LoadoutResult::Success;
}

bool ULostsenseRuntimeSubsystem::EquipFirstInventoryItem() {
  if (!PortableRuntime.IsValid()) {
    return false;
  }

  const Lostsense::Gameplay::InventoryState State =
      PortableRuntime->InventoryState.CaptureState();
  for (const Lostsense::Gameplay::ItemInstance &Instance : State.Instances) {
    const Lostsense::Gameplay::ItemDefinition *Definition =
        PortableRuntime->Items.FindItem(Instance.DefinitionId);
    if (Definition == nullptr || Definition->AllowedEquipmentSlots.empty()) {
      continue;
    }

    const Lostsense::Gameplay::EquipmentSlotId Slot =
        Definition->AllowedEquipmentSlots.front();
    const Lostsense::Gameplay::EquipmentResult Result =
        Lostsense::Gameplay::GameplayEventAuthority::Equip(
            PortableRuntime->Equipment, PortableRuntime->InventoryState,
            Instance.InstanceId, Slot, PortableRuntime->Player.Id(),
            PortableRuntime->Events);
    if (Result == Lostsense::Gameplay::EquipmentResult::Success) {
      return true;
    }
  }
  return false;
}

bool ULostsenseRuntimeSubsystem::SaveCharacterToText(
    FString &OutPayload) const {
  if (!PortableRuntime.IsValid() || !PortableRuntime->Ready) {
    return false;
  }

  std::string Payload;
  if (!Lostsense::Gameplay::SaveCodec::Serialize(
          PortableRuntime->Persistence.CaptureState(), Payload)) {
    return false;
  }

  OutPayload = UTF8_TO_TCHAR(Payload.c_str());
  return true;
}

bool ULostsenseRuntimeSubsystem::LoadCharacterFromText(const FString &Payload) {
  if (!PortableRuntime.IsValid()) {
    return false;
  }

  const std::string Utf8Payload(TCHAR_TO_UTF8(*Payload));
  Lostsense::Gameplay::CharacterSaveState State;
  const Lostsense::Gameplay::SaveDecodeResult Decode =
      Lostsense::Gameplay::SaveCodec::Deserialize(Utf8Payload, State);
  if (Decode.Status != Lostsense::Gameplay::SaveDecodeStatus::Success) {
    return false;
  }

  return PortableRuntime->Persistence.RestoreState(State) ==
         Lostsense::Gameplay::CharacterRestoreResult::Success;
}

void ULostsenseRuntimeSubsystem::AdvancePlayerTime(const float DeltaSeconds) {
  if (!PortableRuntime.IsValid() || DeltaSeconds <= 0.0F) {
    return;
  }

  const double Seconds = static_cast<double>(DeltaSeconds);
  static_cast<void>(PortableRuntime->Effects.AdvanceTime(Seconds));
  static_cast<void>(PortableRuntime->Abilities.AdvanceTime(Seconds));
}

bool ULostsenseRuntimeSubsystem::ActivatePlayerAbility(const uint32 AbilityId) {
  if (!PortableRuntime.IsValid()) {
    return false;
  }

  Lostsense::Gameplay::AbilityTarget Target;
  const auto Outcome =
      Lostsense::Gameplay::GameplayEventAuthority::ActivateAbility(
          PortableRuntime->Abilities, Lostsense::Gameplay::AbilityId{AbilityId},
          Target, PortableRuntime->Player.Id(), PortableRuntime->Events);
  return IsActivationSuccess(Outcome);
}

bool ULostsenseRuntimeSubsystem::ActivatePlayerAbilityAgainst(
    const uint32 AbilityId, Lostsense::Combat::Combatant &Target,
    Lostsense::Gameplay::EffectRuntime &TargetEffects) {
  if (!PortableRuntime.IsValid()) {
    return false;
  }

  Lostsense::Gameplay::AbilityTarget AbilityTarget;
  AbilityTarget.Combatant = &Target;
  AbilityTarget.Effects = &TargetEffects;
  AbilityTarget.Relation = Lostsense::Gameplay::TargetRelation::Hostile;

  const auto Outcome =
      Lostsense::Gameplay::GameplayEventAuthority::ActivateAbility(
          PortableRuntime->Abilities, Lostsense::Gameplay::AbilityId{AbilityId},
          AbilityTarget, PortableRuntime->Player.Id(), PortableRuntime->Events);
  return IsActivationSuccess(Outcome);
}

bool ULostsenseRuntimeSubsystem::ActivatePlayerLoadoutSlot(
    const int32 SlotIndex, Lostsense::Combat::Combatant *Target,
    Lostsense::Gameplay::EffectRuntime *TargetEffects) {
  using namespace Lostsense::Gameplay;

  if (!PortableRuntime.IsValid() || SlotIndex < 0 ||
      SlotIndex >= static_cast<int32>(AbilityLoadoutSlot::Count)) {
    return false;
  }

  const AbilityLoadoutSlot Slot = static_cast<AbilityLoadoutSlot>(SlotIndex);
  const AbilityId Ability = PortableRuntime->Loadout.AbilityAt(Slot);
  const AbilityDefinition *Definition =
      PortableRuntime->Abilities.FindDefinition(Ability);
  if (!Ability.IsValid() || Definition == nullptr) {
    return false;
  }

  AbilityTarget AbilityTarget;
  switch (Definition->TargetRule) {
  case AbilityTargetRule::None:
    break;
  case AbilityTargetRule::Self:
    AbilityTarget.Combatant = &PortableRuntime->Player;
    AbilityTarget.Effects = &PortableRuntime->Effects;
    AbilityTarget.Relation = TargetRelation::Self;
    break;
  case AbilityTargetRule::Hostile:
    if (Target == nullptr || TargetEffects == nullptr) {
      return false;
    }
    AbilityTarget.Combatant = Target;
    AbilityTarget.Effects = TargetEffects;
    AbilityTarget.Relation = TargetRelation::Hostile;
    break;
  case AbilityTargetRule::Friendly:
    if (Target == nullptr || TargetEffects == nullptr) {
      return false;
    }
    AbilityTarget.Combatant = Target;
    AbilityTarget.Effects = TargetEffects;
    AbilityTarget.Relation = TargetRelation::Friendly;
    break;
  }

  const AbilityActivationOutcome Outcome =
      GameplayEventAuthority::ActivateAbility(
          PortableRuntime->Abilities, Ability, AbilityTarget,
          PortableRuntime->Player.Id(), PortableRuntime->Events);
  return IsActivationSuccess(Outcome);
}

bool ULostsenseRuntimeSubsystem::ResolveEnemyBasicAttack(
    Lostsense::Combat::Combatant &Enemy,
    const Lostsense::Combat::DamageSpec &Damage) {
  if (!PortableRuntime.IsValid() || Enemy.Health().IsDead() ||
      PortableRuntime->Player.Health().IsDead()) {
    return false;
  }

  const Lostsense::Combat::CombatResolution Resolution = Enemy.ResolveAttack(
      PortableRuntime->Player, Damage, PortableRuntime->Random);
  if (!Resolution.AppliedToHealth.WasValid) {
    return false;
  }

  Lostsense::Gameplay::GameplayEvent DamageEvent;
  DamageEvent.Type = Lostsense::Gameplay::GameplayEventType::DamageApplied;
  DamageEvent.Source = Enemy.Id();
  DamageEvent.Target = PortableRuntime->Player.Id();
  DamageEvent.Value = Resolution.AppliedToHealth.Applied;
  static_cast<void>(PortableRuntime->Events.Publish(DamageEvent));

  if (Resolution.AppliedToHealth.BecameDead) {
    Lostsense::Gameplay::GameplayEvent DeathEvent;
    DeathEvent.Type = Lostsense::Gameplay::GameplayEventType::CombatantDied;
    DeathEvent.Source = Enemy.Id();
    DeathEvent.Target = PortableRuntime->Player.Id();
    static_cast<void>(PortableRuntime->Events.Publish(DeathEvent));
  }
  return true;
}

TArray<Lostsense::Gameplay::GeneratedLootEntry>
ULostsenseRuntimeSubsystem::GenerateLoot(const uint32 LootTableId,
                                         const uint32 ItemLevel,
                                         const uint64 SourceCombatantId) {
  TArray<Lostsense::Gameplay::GeneratedLootEntry> Result;
  if (!PortableRuntime.IsValid() || ItemLevel == 0U) {
    return Result;
  }

  Lostsense::Gameplay::LootContext Context;
  Context.ItemLevel = ItemLevel;
  Context.PlayerClass = Lostsense::Gameplay::FirstSlice::KnightClass;
  Lostsense::Gameplay::LootGenerationOutcome Outcome =
      PortableRuntime->Loot.Generate(
          Lostsense::Gameplay::LootTableId{LootTableId}, Context);
  if (Outcome.Result != Lostsense::Gameplay::LootGenerationResult::Success) {
    return Result;
  }

  Result.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
      Outcome.Drops.size(), static_cast<std::size_t>(MAX_int32))));
  for (Lostsense::Gameplay::GeneratedLootEntry &Drop : Outcome.Drops) {
    Lostsense::Gameplay::GameplayEvent Event;
    Event.Type = Lostsense::Gameplay::GameplayEventType::ItemDropped;
    Event.Source = Lostsense::Combat::CombatantId{SourceCombatantId};
    Event.Item = Drop.Item;
    Event.ItemInstance = Drop.Instance.InstanceId;
    Event.Quantity = Drop.Quantity;
    static_cast<void>(PortableRuntime->Events.Publish(Event));
    Result.Add(MoveTemp(Drop));
  }
  return Result;
}

bool ULostsenseRuntimeSubsystem::PickupGeneratedLoot(
    const Lostsense::Gameplay::GeneratedLootEntry &Loot) {
  if (!PortableRuntime.IsValid()) {
    return false;
  }

  if (Loot.IsItemInstance()) {
    const Lostsense::Gameplay::InventoryResult Result =
        Lostsense::Gameplay::GameplayEventAuthority::PickupInstance(
            PortableRuntime->InventoryState, Loot.Instance,
            PortableRuntime->Player.Id(), PortableRuntime->Events);
    return Result == Lostsense::Gameplay::InventoryResult::Success;
  }

  const Lostsense::Gameplay::InventoryResult Result =
      PortableRuntime->InventoryState.AddStack(Loot.Item, Loot.Quantity);
  if (Result != Lostsense::Gameplay::InventoryResult::Success) {
    return false;
  }

  Lostsense::Gameplay::GameplayEvent Event;
  Event.Type = Lostsense::Gameplay::GameplayEventType::ItemPickedUp;
  Event.Source = PortableRuntime->Player.Id();
  Event.Item = Loot.Item;
  Event.Quantity = Loot.Quantity;
  static_cast<void>(PortableRuntime->Events.Publish(Event));
  return true;
}

TArray<FLostsensePresentationEvent>
ULostsenseRuntimeSubsystem::DrainPresentationEvents() {
  TArray<FLostsensePresentationEvent> Result;
  if (!PortableRuntime.IsValid()) {
    return Result;
  }

  const std::vector<Lostsense::Gameplay::GameplayEvent> Events =
      PortableRuntime->Events.Drain();
  Result.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
      Events.size(), static_cast<std::size_t>(MAX_int32))));

  for (const Lostsense::Gameplay::GameplayEvent &Event : Events) {
    FLostsensePresentationEvent Projection;
    Projection.Sequence = CheckedSignedId(Event.Sequence);
    Projection.Type = ToPresentationType(Event.Type);
    Projection.SourceId = CheckedSignedId(Event.Source.Value);
    Projection.TargetId = CheckedSignedId(Event.Target.Value);
    Projection.ContentId = PrimaryContentId(Event);
    Projection.Value = Event.Value;
    Projection.Quantity = static_cast<int32>(FMath::Min<std::uint32_t>(
        Event.Quantity, static_cast<std::uint32_t>(MAX_int32)));
    Result.Add(Projection);
  }
  return Result;
}
