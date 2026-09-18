#include "LostsenseEnemyCharacter.h"

#include "LostsenseKnightCharacter.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseStorySubsystem.h"
#include "LostsenseWorldDropActor.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Effects/EffectRuntime.h"
#include "Lostsense/Stats/CombatAttributes.h"

namespace {
struct FArchetypeTuning final {
  double MaximumHealth;
  double AttackPower;
  double Armor;
  double AggroRadius;
  double MeleeRange;
  double BaseDamage;
  double AttackCoefficient;
  float MoveSpeed;
  float WindupSeconds;
  float RecoverySeconds;
  float CooldownSeconds;
};

FArchetypeTuning TuningFor(const ELostsenseEnemyArchetype Archetype,
                           const int32 BossPhase) {
  switch (Archetype) {
  case ELostsenseEnemyArchetype::BellMaddenedCarrion:
    return {48.0, 5.0,    1.0,   1150.0, 145.0, 5.0,
            0.65, 330.0F, 0.28F, 0.48F,  0.85F};
  case ELostsenseEnemyArchetype::CharterDeserter:
    return {68.0, 6.0,    5.0,   1100.0, 175.0, 7.0,
            0.75, 235.0F, 0.52F, 0.72F,  1.15F};
  case ELostsenseEnemyArchetype::EchoMiner:
    return {82.0, 8.0,    5.0,   1050.0, 185.0, 11.0,
            0.90, 185.0F, 0.92F, 1.05F,  1.45F};
  case ELostsenseEnemyArchetype::HaulConstruct:
    return {105.0, 9.0,    9.0,   1300.0, 210.0, 13.0,
            0.95,  205.0F, 0.68F, 1.35F,  1.55F};
  case ELostsenseEnemyArchetype::ForemanKett:
    return {155.0, 11.0,   8.0,   1400.0, 195.0, 12.0,
            0.95,  220.0F, 0.72F, 0.85F,  1.15F};
  case ELostsenseEnemyArchetype::GildedDead:
    return {118.0, 10.0,   15.0,  1180.0, 205.0, 14.0,
            0.90,  155.0F, 1.05F, 1.20F,  1.55F};
  case ELostsenseEnemyArchetype::PressureMutant:
    return {96.0, 12.0,   4.0,   1320.0, 235.0, 10.0,
            1.05, 255.0F, 0.62F, 1.05F,  1.10F};
  case ELostsenseEnemyArchetype::RailMarshal:
    return {172.0, 13.0,   10.0,  1550.0, 230.0, 15.0,
            1.00,  275.0F, 0.48F, 0.70F,  0.92F};
  case ELostsenseEnemyArchetype::Rootbound:
    return {92.0, 9.0,    7.0,   1280.0, 185.0, 11.0,
            0.92, 190.0F, 0.70F, 0.90F,  1.05F};
  case ELostsenseEnemyArchetype::EchoStag:
    return {126.0, 14.0,   5.0,   1750.0, 300.0, 12.0,
            1.08,  320.0F, 0.58F, 0.82F,  1.15F};
  case ELostsenseEnemyArchetype::ThornPenitent:
    return {108.0, 12.0,   9.0,   1450.0, 235.0, 13.0,
            0.98,  245.0F, 0.66F, 0.78F,  0.98F};
  case ELostsenseEnemyArchetype::CourtShade:
    return {132.0, 14.0,   7.0,   1450.0, 215.0, 14.0,
            0.96,  260.0F, 0.58F, 0.74F,  0.92F};
  case ELostsenseEnemyArchetype::BlankKnight:
    return {185.0, 16.0,   18.0,  1500.0, 245.0, 17.0,
            1.02,  205.0F, 0.82F, 0.88F,  1.12F};
  case ELostsenseEnemyArchetype::MotherVeyr:
    if (BossPhase >= 2) {
      return {410.0, 18.0,   12.0,  1850.0, 280.0, 18.0,
              1.05,  300.0F, 0.52F, 0.68F,  0.82F};
    }
    return {410.0, 18.0,   12.0,  1850.0, 250.0, 15.0,
            1.00,  235.0F, 0.76F, 0.88F,  1.08F};
  case ELostsenseEnemyArchetype::Caldris:
    if (BossPhase >= 2) {
      return {560.0, 22.0,   20.0,  1950.0, 300.0, 22.0,
              1.10,  315.0F, 0.48F, 0.62F,  0.78F};
    }
    return {560.0, 22.0,   20.0,  1950.0, 270.0, 18.0,
            1.05,  245.0F, 0.72F, 0.84F,  1.02F};
  case ELostsenseEnemyArchetype::Odran:
    if (BossPhase >= 2) {
      return {280.0, 14.0,   11.0,  1600.0, 215.0, 15.0,
              0.90,  285.0F, 0.58F, 0.72F,  0.92F};
    }
    return {280.0, 14.0,   11.0,  1600.0, 215.0, 12.0,
            0.90,  225.0F, 0.78F, 0.92F,  1.20F};
  }
  return {65.0, 5.0, 3.0, 1100.0, 165.0, 6.0, 0.75, 210.0F, 0.5F, 0.8F, 1.2F};
}
} // namespace

struct ALostsenseEnemyCharacter::FPortableEnemy {
  Lostsense::Combat::Combatant Combatant;
  Lostsense::Gameplay::EffectRuntime Effects;

  FPortableEnemy(const uint64 Id, const ELostsenseEnemyArchetype Archetype)
      : Combatant{Lostsense::Combat::CombatantId{Id},
                  (Archetype == ELostsenseEnemyArchetype::Odran ||
                   Archetype == ELostsenseEnemyArchetype::MotherVeyr ||
                   Archetype == ELostsenseEnemyArchetype::Caldris)
                      ? Lostsense::Combat::CombatantKind::Boss
                      : ((Archetype == ELostsenseEnemyArchetype::ForemanKett ||
                          Archetype == ELostsenseEnemyArchetype::RailMarshal)
                             ? Lostsense::Combat::CombatantKind::Elite
                             : Lostsense::Combat::CombatantKind::Enemy)},
        Effects{Combatant, {}} {
    const FArchetypeTuning Tuning = TuningFor(Archetype, 1);
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::MaxHealth, Tuning.MaximumHealth));
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::AttackPower, Tuning.AttackPower));
    static_cast<void>(Combatant.SetBaseAttribute(
        Lostsense::Stats::CombatAttributes::Armor, Tuning.Armor));
    static_cast<void>(Combatant.Heal(Tuning.MaximumHealth));
  }
};

ALostsenseEnemyCharacter::ALostsenseEnemyCharacter() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.0F;
  GetCharacterMovement()->MaxWalkSpeed = 235.0F;
  GetCharacterMovement()->bOrientRotationToMovement = true;
}

ALostsenseEnemyCharacter::~ALostsenseEnemyCharacter() = default;

void ALostsenseEnemyCharacter::ConfigureEnemy(const uint64 InCombatantId,
                                              const bool bInElite,
                                              const uint32 InItemLevel) {
  ConfigureEnemyArchetype(InCombatantId,
                          bInElite ? ELostsenseEnemyArchetype::ForemanKett
                                   : ELostsenseEnemyArchetype::CharterDeserter,
                          InItemLevel);
}

void ALostsenseEnemyCharacter::ConfigureEnemyArchetype(
    const uint64 InCombatantId, const ELostsenseEnemyArchetype InArchetype,
    const uint32 InItemLevel) {
  PendingCombatantId = InCombatantId;
  Archetype = InArchetype;
  bElite = InArchetype == ELostsenseEnemyArchetype::ForemanKett ||
           InArchetype == ELostsenseEnemyArchetype::RailMarshal ||
           InArchetype == ELostsenseEnemyArchetype::BlankKnight;
  bBoss = InArchetype == ELostsenseEnemyArchetype::Odran ||
          InArchetype == ELostsenseEnemyArchetype::MotherVeyr ||
          InArchetype == ELostsenseEnemyArchetype::Caldris;
  ItemLevel = FMath::Max(1U, InItemLevel);
}

void ALostsenseEnemyCharacter::ConfigureOdranBoss(const uint64 InCombatantId,
                                                  const uint32 InItemLevel) {
  ConfigureEnemyArchetype(InCombatantId, ELostsenseEnemyArchetype::Odran,
                          InItemLevel);
}

void ALostsenseEnemyCharacter::ConfigureMotherVeyrBoss(
    const uint64 InCombatantId, const uint32 InItemLevel) {
  ConfigureEnemyArchetype(InCombatantId, ELostsenseEnemyArchetype::MotherVeyr,
                          InItemLevel);
}

void ALostsenseEnemyCharacter::ConfigureCaldrisBoss(
    const uint64 InCombatantId, const uint32 InItemLevel) {
  ConfigureEnemyArchetype(InCombatantId, ELostsenseEnemyArchetype::Caldris,
                          InItemLevel);
}

void ALostsenseEnemyCharacter::BeginPlay() {
  Super::BeginPlay();
  PortableEnemy = MakeUnique<FPortableEnemy>(PendingCombatantId, Archetype);
  GetCharacterMovement()->MaxWalkSpeed =
      TuningFor(Archetype, BossPhase).MoveSpeed;
  EnterState(ELostsenseEnemyAiState::Idle);
}

void ALostsenseEnemyCharacter::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  if (PortableEnemy == nullptr) {
    return;
  }

  static_cast<void>(
      PortableEnemy->Effects.AdvanceTime(static_cast<double>(DeltaSeconds)));

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr) {
    return;
  }

  if (PortableEnemy->Combatant.Health().IsDead()) {
    HandleDefeat(*Runtime);
    return;
  }
  if (!IsEncounterUnlocked()) {
    EnterState(ELostsenseEnemyAiState::Idle);
    return;
  }

  AttackCooldownRemaining =
      FMath::Max(0.0F, AttackCooldownRemaining - DeltaSeconds);
  BossPulseCooldownRemaining =
      FMath::Max(0.0F, BossPulseCooldownRemaining - DeltaSeconds);
  CommandCooldownRemaining =
      FMath::Max(0.0F, CommandCooldownRemaining - DeltaSeconds);
  StateTimeRemaining = FMath::Max(0.0F, StateTimeRemaining - DeltaSeconds);

  if (bBoss && !bBossTransitionCommitted && BossPhase == 1 &&
      GetMaximumHealth() > 0.0 &&
      GetCurrentHealth() / GetMaximumHealth() <= 0.65) {
    bBossTransitionCommitted = true;
    BossPhase = 2;
    BossPulseCooldownRemaining = 1.4F;
    GetCharacterMovement()->MaxWalkSpeed = TuningFor(Archetype, 2).MoveSpeed;
    EnterState(ELostsenseEnemyAiState::Recover, 1.15F);
    return;
  }

  ACharacter *Player = UGameplayStatics::GetPlayerCharacter(this, 0);
  if (Player == nullptr) {
    return;
  }
  TickLivingAi(DeltaSeconds, *Runtime, *Player);
}

void ALostsenseEnemyCharacter::EnterState(const ELostsenseEnemyAiState NewState,
                                          const float Duration) {
  AiState = NewState;
  StateTimeRemaining = FMath::Max(0.0F, Duration);
  if (NewState != ELostsenseEnemyAiState::Windup) {
    bAttackCommitted = false;
  }
}

void ALostsenseEnemyCharacter::TickLivingAi(const float DeltaSeconds,
                                            ULostsenseRuntimeSubsystem &Runtime,
                                            ACharacter &Player) {
  const FArchetypeTuning Tuning = TuningFor(Archetype, BossPhase);
  const FVector Delta = Player.GetActorLocation() - GetActorLocation();
  const double Distance = Delta.Size2D();

  if (AiState == ELostsenseEnemyAiState::Dead) {
    return;
  }
  if (AiState == ELostsenseEnemyAiState::Recover ||
      AiState == ELostsenseEnemyAiState::Stagger) {
    if (StateTimeRemaining <= 0.0F) {
      EnterState(ELostsenseEnemyAiState::Alert);
    }
    return;
  }
  if (AiState == ELostsenseEnemyAiState::Windup) {
    SetActorRotation(Delta.GetSafeNormal2D().Rotation());
    if (StateTimeRemaining <= 0.0F && !bAttackCommitted) {
      bAttackCommitted = true;
      EnterState(ELostsenseEnemyAiState::Attack, 0.05F);
      ResolveCommittedAttack(Runtime, Distance);
      EnterState(ELostsenseEnemyAiState::Recover, Tuning.RecoverySeconds);
      AttackCooldownRemaining = Tuning.CooldownSeconds;
    }
    return;
  }

  if (Distance > Tuning.AggroRadius) {
    EnterState(ELostsenseEnemyAiState::Idle);
    return;
  }
  if (AiState == ELostsenseEnemyAiState::Idle) {
    EnterState(ELostsenseEnemyAiState::Alert, 0.18F);
    return;
  }
  if (AiState == ELostsenseEnemyAiState::Alert && StateTimeRemaining > 0.0F) {
    return;
  }

  if (Archetype == ELostsenseEnemyArchetype::ForemanKett &&
      CommandCooldownRemaining <= 0.0F) {
    CommandNearbyConstructs();
    CommandCooldownRemaining = 5.0F;
    EnterState(ELostsenseEnemyAiState::Recover, 0.55F);
    return;
  }

  if (Distance > Tuning.MeleeRange) {
    EnterState(ELostsenseEnemyAiState::Approach);
    const FVector Direction = Delta.GetSafeNormal2D();
    float MoveSpeed = Tuning.MoveSpeed;
    if (Archetype == ELostsenseEnemyArchetype::HaulConstruct &&
        Distance < 700.0 && AttackCooldownRemaining <= 0.0F) {
      MoveSpeed *= 1.65F;
    }
    AddActorWorldOffset(Direction * MoveSpeed * DeltaSeconds, true);
    SetActorRotation(Direction.Rotation());
    return;
  }

  if (AttackCooldownRemaining <= 0.0F) {
    BeginAttack(Distance);
  }
}

void ALostsenseEnemyCharacter::BeginAttack(const double DistanceToPlayer) {
  const FArchetypeTuning Tuning = TuningFor(Archetype, BossPhase);
  if (DistanceToPlayer > Tuning.MeleeRange) {
    return;
  }
  ++AttackPatternIndex;
  EnterState(ELostsenseEnemyAiState::Windup, Tuning.WindupSeconds);
}

void ALostsenseEnemyCharacter::ResolveCommittedAttack(
    ULostsenseRuntimeSubsystem &Runtime, const double DistanceToPlayer) {
  const FArchetypeTuning Tuning = TuningFor(Archetype, BossPhase);
  if (DistanceToPlayer > Tuning.MeleeRange + 45.0 || PortableEnemy == nullptr ||
      PortableEnemy->Combatant.Health().IsDead()) {
    return;
  }

  Lostsense::Combat::DamageSpec Damage;
  Damage.BaseDamage[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Physical)] = Tuning.BaseDamage;
  Damage.AttackPowerCoefficients[static_cast<std::size_t>(
      Lostsense::Combat::DamageType::Physical)] = Tuning.AttackCoefficient;

  if (Archetype == ELostsenseEnemyArchetype::PressureMutant &&
      AttackPatternIndex % 2 == 0) {
    Damage.BaseDamage[static_cast<std::size_t>(
        Lostsense::Combat::DamageType::Physical)] = 5.0;
    Damage.BaseDamage[static_cast<std::size_t>(
        Lostsense::Combat::DamageType::Arcane)] = 9.0;
    Damage.AttackPowerCoefficients[static_cast<std::size_t>(
        Lostsense::Combat::DamageType::Arcane)] = 0.45;
  }

  if (bBoss && BossPhase >= 2 && BossPulseCooldownRemaining <= 0.0F &&
      AttackPatternIndex % 3 == 0) {
    Damage.BaseDamage.fill(0.0);
    Damage.BaseDamage[static_cast<std::size_t>(
        Lostsense::Combat::DamageType::Arcane)] = 11.0;
    Damage.AttackPowerCoefficients.fill(0.0);
    Damage.AttackPowerCoefficients[static_cast<std::size_t>(
        Lostsense::Combat::DamageType::Arcane)] = 0.55;
    Damage.CanBlock = false;
    BossPulseCooldownRemaining = 3.25F;
  }

  static_cast<void>(
      Runtime.ResolveEnemyBasicAttack(PortableEnemy->Combatant, Damage));
}

void ALostsenseEnemyCharacter::CommandNearbyConstructs() {
  UWorld *World = GetWorld();
  if (World == nullptr) {
    return;
  }
  for (TActorIterator<ALostsenseEnemyCharacter> It(World); It; ++It) {
    ALostsenseEnemyCharacter *Other = *It;
    if (Other == nullptr || Other == this || Other->IsDefeated() ||
        Other->Archetype != ELostsenseEnemyArchetype::HaulConstruct ||
        FVector::DistSquared(GetActorLocation(), Other->GetActorLocation()) >
            FMath::Square(900.0)) {
      continue;
    }
    Other->AttackCooldownRemaining = 0.0F;
    if (Other->AiState == ELostsenseEnemyAiState::Idle) {
      Other->EnterState(ELostsenseEnemyAiState::Alert, 0.1F);
    }
  }
}

bool ALostsenseEnemyCharacter::ReceivePlayerAbility(
    ULostsenseRuntimeSubsystem &Runtime, const uint32 AbilityId) {
  if (PortableEnemy == nullptr || PortableEnemy->Combatant.Health().IsDead() ||
      !IsEncounterUnlocked()) {
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
  if (PortableEnemy == nullptr || PortableEnemy->Combatant.Health().IsDead() ||
      !IsEncounterUnlocked()) {
    return false;
  }
  const bool Activated = Runtime.ActivatePlayerLoadoutSlot(
      SlotIndex, &PortableEnemy->Combatant, &PortableEnemy->Effects);
  if (Activated && PortableEnemy->Combatant.Health().IsDead()) {
    HandleDefeat(Runtime);
  }
  return Activated;
}

Lostsense::Gameplay::AbilityTarget
ALostsenseEnemyCharacter::BuildAbilityTarget() {
  Lostsense::Gameplay::AbilityTarget Target;
  if (PortableEnemy == nullptr || PortableEnemy->Combatant.Health().IsDead()) {
    return Target;
  }
  Target.Combatant = &PortableEnemy->Combatant;
  Target.Effects = &PortableEnemy->Effects;
  Target.Relation = Lostsense::Gameplay::TargetRelation::Hostile;
  return Target;
}

void ALostsenseEnemyCharacter::FinalizePlayerAbility(
    ULostsenseRuntimeSubsystem &Runtime) {
  if (PortableEnemy != nullptr && PortableEnemy->Combatant.Health().IsDead()) {
    HandleDefeat(Runtime);
  }
}

uint64 ALostsenseEnemyCharacter::GetCombatantId() const {
  return PortableEnemy != nullptr ? PortableEnemy->Combatant.Id().Value
                                  : PendingCombatantId;
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

ELostsenseEnemyArchetype ALostsenseEnemyCharacter::GetArchetype() const {
  return Archetype;
}

ELostsenseEnemyAiState ALostsenseEnemyCharacter::GetAiState() const {
  return AiState;
}

void ALostsenseEnemyCharacter::HandleDefeat(
    ULostsenseRuntimeSubsystem &Runtime) {
  if (bDefeatHandled || PortableEnemy == nullptr) {
    return;
  }
  bDefeatHandled = true;
  EnterState(ELostsenseEnemyAiState::Dead);

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

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Story != nullptr) {
    if (Archetype == ELostsenseEnemyArchetype::Odran) {
      static_cast<void>(
          Story->CompleteBeat(ELostsenseStoryBeat::OdranDefeated));
    } else if (Archetype == ELostsenseEnemyArchetype::MotherVeyr) {
      static_cast<void>(Story->CompleteActTwoBeat(
          ELostsenseActTwoStoryBeat::MotherVeyrDefeated));
    } else if (Archetype == ELostsenseEnemyArchetype::Caldris) {
      static_cast<void>(Story->CompleteCampaignQuest(80024));
    } else if (PortableEnemy->Combatant.Id().Value == 1000U) {
      static_cast<void>(
          Story->CompleteBeat(ELostsenseStoryBeat::BellgraveDepartureAllowed));
    }
  }

  GetCharacterMovement()->DisableMovement();
  SetActorEnableCollision(false);
  SetLifeSpan(bBoss ? 8.0F : 4.0F);
}


bool ALostsenseEnemyCharacter::IsEncounterUnlocked() const {
  if (Archetype != ELostsenseEnemyArchetype::MotherVeyr &&
      Archetype != ELostsenseEnemyArchetype::Caldris) {
    return true;
  }
  const UGameInstance *GameInstance = GetGameInstance();
  const ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Story == nullptr) {
    return false;
  }
  if (Archetype == ELostsenseEnemyArchetype::MotherVeyr) {
    return Story->HasActTwoBeat(
        ELostsenseActTwoStoryBeat::ThornChoirDiscovered);
  }
  return Story->GetObjectiveState(80024) == ELostsenseObjectiveState::Active;
}
