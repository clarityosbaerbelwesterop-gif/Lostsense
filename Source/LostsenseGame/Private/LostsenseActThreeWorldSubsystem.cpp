#include "LostsenseActThreeWorldSubsystem.h"

#include "LostsenseActThreeStoryActor.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
AStaticMeshActor *Block(UWorld &World, UStaticMesh &Mesh,
                        const FVector &Location, const FVector &Scale,
                        const FRotator &Rotation = FRotator::ZeroRotator) {
  AStaticMeshActor *Actor =
      World.SpawnActor<AStaticMeshActor>(Location, Rotation);
  if (Actor == nullptr) {
    return nullptr;
  }
  Actor->GetStaticMeshComponent()->SetStaticMesh(&Mesh);
  Actor->GetStaticMeshComponent()->SetCollisionEnabled(
      ECollisionEnabled::QueryAndPhysics);
  Actor->SetActorScale3D(Scale);
  return Actor;
}

void StoryActor(UWorld &World, const FVector &Location, const FVector &Scale,
                const ELostsenseActThreeInteraction Interaction) {
  const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
  ALostsenseActThreeStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActThreeStoryActor>(
          ALostsenseActThreeStoryActor::StaticClass(), Transform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor == nullptr) {
    return;
  }
  Actor->Configure(Interaction);
  UGameplayStatics::FinishSpawningActor(Actor, Transform);
}

void Enemy(UWorld &World, const FVector &Location, const uint64 Id,
           const ELostsenseEnemyArchetype Archetype, const uint32 Level) {
  const FTransform Transform(FRotator::ZeroRotator, Location);
  ALostsenseEnemyCharacter *Actor =
      World.SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
  if (Actor == nullptr) {
    return;
  }
  Actor->ConfigureEnemyArchetype(Id, Archetype, Level);
  UGameplayStatics::FinishSpawningActor(Actor, Transform);
}
} // namespace

void ULostsenseActThreeWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
  Super::OnWorldBeginPlay(InWorld);
  if (!InWorld.IsGameWorld() ||
      Cast<ALostsenseGameMode>(InWorld.GetAuthGameMode()) == nullptr) {
    return;
  }

  UStaticMesh *Cube =
      LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (Cube == nullptr) {
    return;
  }

  // The capital is a continuous surface route beyond Giltfen: repaired black
  // stone gives way to monumental civic plazas with deliberately blank crests.
  const FVector Causeway(31800.0F, 12600.0F, 120.0F);
  for (int32 Span = 0; Span < 8; ++Span) {
    Block(InWorld, *Cube,
          Causeway + FVector(static_cast<float>(Span) * 680.0F,
                             static_cast<float>(Span % 2) * 120.0F, 0.0F),
          FVector(4.0F, 2.3F, 0.3F));
  }

  const FVector CivicPlaza(37500.0F, 13800.0F, 180.0F);
  Block(InWorld, *Cube, CivicPlaza, FVector(13.0F, 10.0F, 0.45F));
  for (int32 Monument = 0; Monument < 6; ++Monument) {
    const float X =
        CivicPlaza.X - 1800.0F + static_cast<float>(Monument) * 720.0F;
    const float Y = Monument % 2 == 0 ? 850.0F : -850.0F;
    Block(InWorld, *Cube, FVector(X, CivicPlaza.Y + Y, 720.0F),
          FVector(0.7F, 0.7F, 6.0F));
  }
  StoryActor(InWorld, CivicPlaza + FVector(-1700.0F, 0.0F, 170.0F),
             FVector(0.55F, 0.55F, 1.8F),
             ELostsenseActThreeInteraction::SolenneEntry);

  Enemy(InWorld, CivicPlaza + FVector(700.0F, 650.0F, 160.0F), 3201U,
        ELostsenseEnemyArchetype::CourtShade, 12U);
  Enemy(InWorld, CivicPlaza + FVector(1300.0F, -600.0F, 160.0F), 3202U,
        ELostsenseEnemyArchetype::CourtShade, 12U);

  const FVector LedgerCourt(41000.0F, 15000.0F, 120.0F);
  Block(InWorld, *Cube, LedgerCourt, FVector(8.0F, 7.0F, 0.4F));
  for (int32 Shelf = 0; Shelf < 7; ++Shelf) {
    Block(InWorld, *Cube,
          LedgerCourt + FVector(-1500.0F + static_cast<float>(Shelf) * 500.0F,
                                Shelf % 2 == 0 ? 520.0F : -520.0F, 430.0F),
          FVector(1.8F, 0.35F, 3.5F));
  }
  StoryActor(InWorld, LedgerCourt + FVector(900.0F, 0.0F, 180.0F),
             FVector(1.2F, 0.8F, 0.35F),
             ELostsenseActThreeInteraction::RoyalLedgers);
  Enemy(InWorld, LedgerCourt + FVector(-900.0F, 0.0F, 160.0F), 3210U,
        ELostsenseEnemyArchetype::BlankKnight, 13U);

  const FVector CharterHall(43800.0F, 16000.0F, 80.0F);
  Block(InWorld, *Cube, CharterHall, FVector(7.0F, 5.0F, 0.4F));
  Block(InWorld, *Cube, CharterHall + FVector(0.0F, 520.0F, 520.0F),
        FVector(7.0F, 0.35F, 5.0F));
  Block(InWorld, *Cube, CharterHall + FVector(0.0F, -520.0F, 520.0F),
        FVector(7.0F, 0.35F, 5.0F));
  StoryActor(InWorld, CharterHall + FVector(700.0F, 0.0F, 180.0F),
             FVector(1.0F, 0.7F, 0.3F),
             ELostsenseActThreeInteraction::GildedCharterEvidence);
  Enemy(InWorld, CharterHall + FVector(-850.0F, 250.0F, 160.0F), 3220U,
        ELostsenseEnemyArchetype::CourtShade, 13U);

  // Palace of Empty Names: name-plate aisles descend into contradictory
  // archives before the Regent-Engine chamber.
  const FVector Palace(47000.0F, 17000.0F, -80.0F);
  for (int32 Wing = 0; Wing < 6; ++Wing) {
    const float Z = Palace.Z - static_cast<float>(Wing) * 180.0F;
    Block(InWorld, *Cube,
          Palace + FVector(static_cast<float>(Wing) * 720.0F,
                           Wing % 2 == 0 ? 380.0F : -380.0F, Z),
          FVector(4.4F, 4.0F, 0.42F));
    Block(InWorld, *Cube,
          Palace + FVector(static_cast<float>(Wing) * 720.0F, 0.0F, Z + 650.0F),
          FVector(0.45F, 5.0F, 6.5F));
  }
  StoryActor(InWorld, Palace + FVector(2500.0F, 0.0F, -500.0F),
             FVector(1.4F, 1.0F, 0.4F),
             ELostsenseActThreeInteraction::PalaceArchive);
  Enemy(InWorld, Palace + FVector(1400.0F, 650.0F, -260.0F), 3230U,
        ELostsenseEnemyArchetype::BlankKnight, 14U);
  Enemy(InWorld, Palace + FVector(2200.0F, -650.0F, -440.0F), 3231U,
        ELostsenseEnemyArchetype::CourtShade, 14U);

  const FVector EngineChamber(52500.0F, 17000.0F, -1500.0F);
  Block(InWorld, *Cube, EngineChamber, FVector(11.0F, 10.0F, 0.55F));
  for (int32 Piston = 0; Piston < 8; ++Piston) {
    const float Angle = static_cast<float>(Piston) * 45.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 950.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 950.0F,
                         600.0F);
    Block(InWorld, *Cube, EngineChamber + Offset, FVector(0.5F, 0.5F, 6.0F),
          FRotator(0.0F, Angle, 0.0F));
  }

  const FTransform BossTransform(FRotator::ZeroRotator,
                                 EngineChamber + FVector(0.0F, 0.0F, 160.0F));
  ALostsenseEnemyCharacter *Caldris =
      InWorld.SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), BossTransform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Caldris != nullptr) {
    Caldris->ConfigureCaldrisBoss(30017U, 15U);
    UGameplayStatics::FinishSpawningActor(Caldris, BossTransform);
  }
}
