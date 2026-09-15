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

  FPortableEnemy(const uint64 Id, const bool bIsElite, const bool bIsBoss)
      : Combatant{Lostsense::Combat::CombatantId{Id},
                  bIsBoss ? Lostsense::Combat::CombatantKind::Boss
                          : (bIsElite ? Lostsense::Combat::CombatantKind::Elite
                                     : Lostsense::Combat::CombatantKind::Enemy)},
        Effects{Combatant, {}} {
    const double MaximumHealth = bIsBoss ? 280.0 : (bIsElite ? 100.0 : 65.0);
    const double AttackPower = bIsBoss ? 14.0 : (bIsElite ? 9.0 : 5.0);
    const double Armor = bIsBoss ? 11.0 : (bIsElite ? 7.0 : 3.0);

    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::MaxHealth, MaximumHealth));
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::AttackPower, AttackPower));
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::Armor, Armor));
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
  bBoss = false;
  ItemLevel = FMath::Max(1U, InItemLevel);
}

void ALostsenseEnemyCharacter::ConfigureOdranBoss(const uint64 InCombatantId,
                                                  const uint32 InItemLevel) {
  PendingCombatantId = InCombatantId;
  bElite = false;
  bBoss = true;
  ItemLevel = FMath::Max(1U, InItemLevel);
}

void ALostsenseEnemyCharacter::BeginPlay() {
  Super::BeginPlay();
  PortableEnemy =
      MakeUnique<FPortableEnemy>(PendingCombatantId, bElite, bBoss);
  GetCharacterMovement()->MaxWalkSpeed = bBoss ? 225.0F : 260.0F;
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
  BossPulseCooldownRemaining =
      FMath::Max(0.0F, BossPulseCooldownRemaining - DeltaSeconds);

  if (bBoss && BossPhase == 1 && GetMaximumHealth() > 0.0 &&
      GetCurrentHealth() / GetMaximumHealth() <= 0.65) {
    BossPhase = 2;
    BossPulseCooldownRemaining = 0.35F;
    GetCharacterMovement()->MaxWalkSpeed = 285.0F;
  }

  ACharacter *Player = UGameplayStatics::GetPlayerCharacter(this, 0);
  if (Player == nullptr) {
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

  const FVector Delta = Player->GetActorLocation() - GetActorLocation();
  const double Distance = Delta.Size2D();
  const double AggroRadius = bBoss ? 1550.0 : 1100.0;
  if (Distance > AggroRadius) {
    return;
  }

  if (bBoss && BossPhase >= 2 && TryResolveBossPulse(*Runtime, Distance)) {
    return;
  }

  const double MeleeRange = bBoss ? 205.0 : 165.0;
  if (Distance > MeleeRange) {
    const FVector Direction = Delta.GetSafeNormal2D();
    const float MoveSpeed = bBoss && BossPhase >= 2 ? 275.0F : 210.0F;
    AddActorWorldOffset(Direction * MoveSpeed * DeltaSeconds, true);
    SetActorRotation(Direction.Rotation());
    return;
  }

  if (AttackCooldownRemaining > 0.0F) {
    return;
  }

  Lostsense::Combat::DamageSpec Damage;
  Damage.BaseDamage[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Physical)] =
      bBoss ? (BossPhase >= 2 ? 15.0 : 12.0) : (bElite ? 10.0 : 6.0);
  Damage.AttackPowerCoefficients[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Physical)] = bBoss ? 0.90 : 0.75;
  static_cast<void>(
      Runtime->ResolveEnemyBasicAttack(PortableEnemy->Combatant, Damage));
  AttackCooldownRemaining =
      bBoss ? (BossPhase >= 2 ? 0.95F : 1.20F) : (bElite ? 1.25F : 1.65F);
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

bool ALostsenseEnemyCharacter::IsBoss() const { return bBoss; }

double ALostsenseEnemyCharacter::GetCurrentHealth() const {
  return PortableEnemy != nullptr ? PortableEnemy->Combatant.Health().Current()
                                  : 0.0;
}

double ALostsenseEnemyCharacter::GetMaximumHealth() const {
  return PortableEnemy != nullptr ? PortableEnemy->Combatant.Health().Maximum()
                                  : 0.0;
}

int32 ALostsenseEnemyCharacter::GetBossPhase() const {
  return bBoss ? BossPhase : 0;
}

bool ALostsenseEnemyCharacter::TryResolveBossPulse(
    ULostsenseRuntimeSubsystem &Runtime, const double DistanceToPlayer) {
  if (!bBoss || BossPhase < 2 || BossPulseCooldownRemaining > 0.0F ||
      DistanceToPlayer > 560.0) {
    return false;
  }

  Lostsense::Combat::DamageSpec Pulse;
  Pulse.BaseDamage[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Arcane)] = 11.0;
  Pulse.AttackPowerCoefficients[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Arcane)] = 0.55;
  Pulse.CanBlock = false;
  static_cast<void>(
      Runtime.ResolveEnemyBasicAttack(PortableEnemy->Combatant, Pulse));
  BossPulseCooldownRemaining = 3.25F;
  AttackCooldownRemaining = FMath::Max(AttackCooldownRemaining, 0.65F);
  return true;
}

void ALostsenseEnemyCharacter::HandleDefeat(
    ULostsenseRuntimeSubsystem &Runtime) {
  if (bDefeatHandled || PortableEnemy == nullptr) {
    return;
  }
  bDefeatHandled = true;

  static_cast<void>(Runtime.GrantSkillPoints(bBoss ? 3 : 1));

  const uint32 Table =
      bBoss ? Lostsense::Gameplay::FirstSlice::OdranBossLoot.Value
            : (bElite ? Lostsense::Gameplay::FirstSlice::VaurEliteLoot.Value
                      : Lostsense::Gameplay::FirstSlice::VaurEnemyLoot.Value);
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
  SetLifeSpan(bBoss ? 8.0F : 4.0F);
}
