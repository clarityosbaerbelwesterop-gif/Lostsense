#include "LostsenseEnemyCharacter.h"

#include "LostsenseKnightCharacter.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseWorldDropActor.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Stats/CombatAttributes.h"

struct ALostsenseEnemyCharacter::FPortableEnemy {
  Lostsense::Combat::Combatant Combatant;
  Lostsense::Gameplay::EffectRuntime Effects;

  FPortableEnemy(const uint64 Id, const bool bIsElite)
      : Combatant{Lostsense::Combat::CombatantId{Id},
                  bIsElite ? Lostsense::Combat::CombatantKind::Elite
                           : Lostsense::Combat::CombatantKind::Enemy},
        Effects{Combatant, {}} {
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::MaxHealth,
        bIsElite ? 100.0 : 65.0));
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::AttackPower, bIsElite ? 9.0 : 5.0));
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::Armor, bIsElite ? 7.0 : 3.0));
  }
};

ALostsenseEnemyCharacter::ALostsenseEnemyCharacter() {
  PrimaryActorTick.bCanEverTick = true;
  GetCharacterMovement()->MaxWalkSpeed = 260.0F;
  GetCharacterMovement()->bOrientRotationToMovement = true;
}

ALostsenseEnemyCharacter::~ALostsenseEnemyCharacter() = default;

void ALostsenseEnemyCharacter::ConfigureEnemy(const uint64 InCombatantId,
                                              const bool bInElite,
                                              const uint32 InItemLevel) {
  PendingCombatantId = InCombatantId;
  bElite = bInElite;
  ItemLevel = FMath::Max(1U, InItemLevel);
}

void ALostsenseEnemyCharacter::BeginPlay() {
  Super::BeginPlay();
  PortableEnemy = MakeUnique<FPortableEnemy>(PendingCombatantId, bElite);
}

void ALostsenseEnemyCharacter::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);

  if (PortableEnemy == nullptr) {
    return;
  }

  static_cast<void>(
      PortableEnemy->Effects.AdvanceTime(static_cast<double>(DeltaSeconds)));
  if (PortableEnemy->Combatant.Health().IsDead()) {
    UGameInstance *GameInstance = GetGameInstance();
    ULostsenseRuntimeSubsystem *Runtime =
        GameInstance != nullptr
            ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
            : nullptr;
    if (Runtime != nullptr) {
      HandleDefeat(*Runtime);
    }
    return;
  }

  AttackCooldownRemaining =
      FMath::Max(0.0F, AttackCooldownRemaining - DeltaSeconds);
  ACharacter *Player = UGameplayStatics::GetPlayerCharacter(this, 0);
  if (Player == nullptr) {
    return;
  }

  const FVector Delta = Player->GetActorLocation() - GetActorLocation();
  const double Distance = Delta.Size2D();
  if (Distance > 1100.0) {
    return;
  }

  if (Distance > 165.0) {
    const FVector Direction = Delta.GetSafeNormal2D();
    AddActorWorldOffset(Direction * 210.0F * DeltaSeconds, true);
    SetActorRotation(Direction.Rotation());
    return;
  }

  if (AttackCooldownRemaining > 0.0F) {
    return;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr) {
    return;
  }

  Lostsense::Combat::DamageSpec Damage;
  Damage.BaseDamage[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Physical)] = bElite ? 10.0 : 6.0;
  Damage.AttackPowerCoefficients[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Physical)] = 0.75;
  static_cast<void>(
      Runtime->ResolveEnemyBasicAttack(PortableEnemy->Combatant, Damage));
  AttackCooldownRemaining = bElite ? 1.25F : 1.65F;
}

bool ALostsenseEnemyCharacter::ReceivePlayerAbility(
    ULostsenseRuntimeSubsystem &Runtime, const uint32 AbilityId) {
  if (PortableEnemy == nullptr || PortableEnemy->Combatant.Health().IsDead()) {
    return false;
  }

  const bool Activated = Runtime.ActivatePlayerAbilityAgainst(
      AbilityId, PortableEnemy->Combatant, PortableEnemy->Effects);
  if (Activated && PortableEnemy->Combatant.Health().IsDead()) {
    HandleDefeat(Runtime);
  }
  return Activated;
}

bool ALostsenseEnemyCharacter::ReceivePlayerLoadoutSlot(
    ULostsenseRuntimeSubsystem &Runtime, const int32 SlotIndex) {
  if (PortableEnemy == nullptr || PortableEnemy->Combatant.Health().IsDead()) {
    return false;
  }

  const bool Activated = Runtime.ActivatePlayerLoadoutSlot(
      SlotIndex, &PortableEnemy->Combatant, &PortableEnemy->Effects);
  if (Activated && PortableEnemy->Combatant.Health().IsDead()) {
    HandleDefeat(Runtime);
  }
  return Activated;
}

bool ALostsenseEnemyCharacter::IsDefeated() const {
  return PortableEnemy != nullptr && PortableEnemy->Combatant.Health().IsDead();
}

void ALostsenseEnemyCharacter::HandleDefeat(
    ULostsenseRuntimeSubsystem &Runtime) {
  if (bDefeatHandled || PortableEnemy == nullptr) {
    return;
  }
  bDefeatHandled = true;

  static_cast<void>(Runtime.GrantSkillPoints(1));

  const uint32 Table =
      bElite ? Lostsense::Gameplay::FirstSlice::VaurEliteLoot.Value
             : Lostsense::Gameplay::FirstSlice::VaurEnemyLoot.Value;
  const TArray<Lostsense::Gameplay::GeneratedLootEntry> Drops =
      Runtime.GenerateLoot(Table, ItemLevel,
                           PortableEnemy->Combatant.Id().Value);

  UWorld *World = GetWorld();
  if (World != nullptr) {
    for (int32 Index = 0; Index < Drops.Num(); ++Index) {
      const FVector Offset(0.0F, static_cast<float>(Index) * 55.0F, 35.0F);
      const FTransform Transform(FRotator::ZeroRotator,
                                 GetActorLocation() + Offset);
      ALostsenseWorldDropActor *DropActor =
          World->SpawnActorDeferred<ALostsenseWorldDropActor>(
              ALostsenseWorldDropActor::StaticClass(), Transform, this, nullptr,
              ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
      if (DropActor != nullptr) {
        DropActor->InitializeDrop(Drops[Index]);
        UGameplayStatics::FinishSpawningActor(DropActor, Transform);
      }
    }
  }

  GetCharacterMovement()->DisableMovement();
  SetActorEnableCollision(false);
  SetLifeSpan(4.0F);
}
