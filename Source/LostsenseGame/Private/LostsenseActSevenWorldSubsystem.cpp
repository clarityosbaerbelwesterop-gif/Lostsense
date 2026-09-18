#include "LostsenseActSevenWorldSubsystem.h"

#include "LostsenseActSevenStoryActor.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
void Block(UWorld &World, UStaticMesh &Mesh, const FVector &Location,
           const FVector &Scale) {
  AStaticMeshActor *Actor =
      World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
  if (Actor != nullptr) {
    Actor->GetStaticMeshComponent()->SetStaticMesh(&Mesh);
    Actor->GetStaticMeshComponent()->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics);
    Actor->SetActorScale3D(Scale);
  }
}

void Story(UWorld &World, const FVector &Location, const int32 QuestId,
           const ELostsenseActSevenArchiveChoice Choice =
               ELostsenseActSevenArchiveChoice::None) {
  const FTransform Transform(FRotator::ZeroRotator, Location, FVector(0.8F));
  ALostsenseActSevenStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActSevenStoryActor>(
          ALostsenseActSevenStoryActor::StaticClass(), Transform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor != nullptr) {
    Actor->Configure(QuestId, Choice);
    UGameplayStatics::FinishSpawningActor(Actor, Transform);
  }
}

void Enemy(UWorld &World, const FVector &Location, const uint64 Id,
           const ELostsenseEnemyArchetype Archetype, const uint32 Level) {
  const FTransform Transform(FRotator::ZeroRotator, Location);
  ALostsenseEnemyCharacter *Actor =
      World.SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor != nullptr) {
    if (Archetype == ELostsenseEnemyArchetype::SaelRhyne) {
      Actor->ConfigureSaelRhyneBoss(Id, Level);
    } else {
      Actor->ConfigureEnemyArchetype(Id, Archetype, Level);
    }
    UGameplayStatics::FinishSpawningActor(Actor, Transform);
  }
}
} // namespace

void ULostsenseActSevenWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  // The campaign returns to changed surface geography. These authored stations
  // sit beside the existing Crownless route rather than replacing prior acts.
  const FVector NoonRelay(38500.0F, 20500.0F, 180.0F);
  Block(InWorld, *Cube, NoonRelay, FVector(8.0F, 6.0F, 0.4F));
  Story(InWorld, NoonRelay + FVector(-900.0F, 0.0F, 120.0F), 80060);

  const FVector MentorLedger(42000.0F, 21500.0F, 160.0F);
  Block(InWorld, *Cube, MentorLedger, FVector(7.0F, 5.0F, 0.4F));
  Story(InWorld, MentorLedger + FVector(600.0F, 0.0F, 120.0F), 80061);

  // A Mercy Measured in Names is an explicit mutually exclusive player
  // choice. Both archive anchors exist; choosing either commits quest 80062
  // and the other becomes non-interactable through persistent story state.
  const FVector ArchiveFork(45000.0F, 22500.0F, 120.0F);
  Block(InWorld, *Cube, ArchiveFork, FVector(9.0F, 7.0F, 0.35F));
  Story(InWorld, ArchiveFork + FVector(-900.0F, 700.0F, 120.0F), 80062,
        ELostsenseActSevenArchiveChoice::BellgraveCivilianArchive);
  Story(InWorld, ArchiveFork + FVector(-900.0F, -700.0F, 120.0F), 80062,
        ELostsenseActSevenArchiveChoice::CrownlessCivilianArchive);

  // Rewritten Gallows Library (20012): hanging shelf lanes and verdict relays
  // form the assault route into Sael Rhyne's suppression arena.
  const FVector Gallows(50000.0F, 23500.0F, -300.0F);
  for (int32 Shelf = 0; Shelf < 8; ++Shelf) {
    Block(InWorld, *Cube,
          Gallows + FVector(static_cast<float>(Shelf) * 620.0F,
                            Shelf % 2 == 0 ? 650.0F : -650.0F,
                            -static_cast<float>(Shelf) * 90.0F),
          FVector(3.8F, 1.0F, 0.32F));
  }
  Story(InWorld, Gallows + FVector(2600.0F, 0.0F, -200.0F), 80063);
  Enemy(InWorld, Gallows + FVector(1200.0F, 500.0F, -100.0F), 7601U,
        ELostsenseEnemyArchetype::PaperDead, 24U);
  Enemy(InWorld, Gallows + FVector(1900.0F, -500.0F, -180.0F), 7602U,
        ELostsenseEnemyArchetype::PaperDead, 24U);

  // Censor -> Partial Silence -> Private Archive. Seal markers remain spatially
  // local and visible so movement/basic attack/escape are never all removed.
  const FVector SaelArena(55800.0F, 23500.0F, -1100.0F);
  Block(InWorld, *Cube, SaelArena, FVector(11.0F, 10.0F, 0.5F));
  for (int32 Seal = 0; Seal < 8; ++Seal) {
    const float Angle = static_cast<float>(Seal) * 45.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1050.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1050.0F,
                         100.0F);
    Block(InWorld, *Cube, SaelArena + Offset, FVector(1.4F, 1.4F, 0.08F));
  }
  Enemy(InWorld, SaelArena + FVector(0.0F, 0.0F, 160.0F), 30047U,
        ELostsenseEnemyArchetype::SaelRhyne, 25U);
}
