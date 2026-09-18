#include "LostsenseActSixWorldSubsystem.h"

#include "LostsenseActSixStoryActor.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
void Block(UWorld &World, UStaticMesh &Mesh, const FVector &Location,
           const FVector &Scale,
           const FRotator &Rotation = FRotator::ZeroRotator) {
  AStaticMeshActor *Actor =
      World.SpawnActor<AStaticMeshActor>(Location, Rotation);
  if (Actor != nullptr) {
    Actor->GetStaticMeshComponent()->SetStaticMesh(&Mesh);
    Actor->GetStaticMeshComponent()->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics);
    Actor->SetActorScale3D(Scale);
  }
}

void Story(UWorld &World, const FVector &Location, const int32 QuestId) {
  const FTransform Transform(FRotator::ZeroRotator, Location, FVector(0.8F));
  ALostsenseActSixStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActSixStoryActor>(
          ALostsenseActSixStoryActor::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor != nullptr) {
    Actor->Configure(QuestId);
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
    if (Archetype == ELostsenseEnemyArchetype::Kharos) {
      Actor->ConfigureKharosBoss(Id, Level);
    } else {
      Actor->ConfigureEnemyArchetype(Id, Archetype, Level);
    }
    UGameplayStatics::FinishSpawningActor(Actor, Transform);
  }
}
} // namespace

void ULostsenseActSixWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  // Hollow Meridian begins at a Namarith breach. Critical traversal remains
  // authored even though the surrounding fault shelves imply unstable distance.
  const FVector Breach(82500.0F, 0.0F, -15000.0F);
  for (int32 Shelf = 0; Shelf < 7; ++Shelf) {
    const float Y = Shelf % 2 == 0 ? 0.0F : 700.0F;
    Block(InWorld, *Cube,
          Breach + FVector(static_cast<float>(Shelf) * 760.0F, Y,
                           -static_cast<float>(Shelf) * 180.0F),
          FVector(4.2F, 2.7F, 0.32F));
  }
  Story(InWorld, Breach + FVector(1100.0F, 0.0F, 120.0F), 80050);

  // Quiet House of Bearings: communal witness objects are readable anchors,
  // not hostile props. Serit's interaction advances the negotiated entry.
  const FVector QuietHouse(88000.0F, 500.0F, -16600.0F);
  Block(InWorld, *Cube, QuietHouse, FVector(8.0F, 7.0F, 0.45F));
  for (int32 Anchor = 0; Anchor < 6; ++Anchor) {
    const float Angle = static_cast<float>(Anchor) * 60.0F;
    Block(InWorld, *Cube,
          QuietHouse +
              FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * 900.0F,
                      FMath::Sin(FMath::DegreesToRadians(Angle)) * 900.0F,
                      500.0F),
          FVector(0.35F, 0.35F, 4.5F));
  }
  Story(InWorld, QuietHouse + FVector(0.0F, 0.0F, 120.0F), 80051);

  const FVector ContradictionShelf(92500.0F, -500.0F, -17600.0F);
  Block(InWorld, *Cube, ContradictionShelf, FVector(7.0F, 5.0F, 0.4F));
  Story(InWorld, ContradictionShelf + FVector(-700.0F, 0.0F, 120.0F), 80052);
  Enemy(InWorld, ContradictionShelf + FVector(500.0F, 650.0F, 120.0F), 6501U,
        ELostsenseEnemyArchetype::NullMantle, 21U);
  Enemy(InWorld, ContradictionShelf + FVector(1200.0F, -500.0F, 120.0F),
        6502U, ELostsenseEnemyArchetype::ThoughtEater, 21U);

  // Blind Astrarium (20028): offset lens towers frame deliberately absent
  // corridors. The interaction represents reorientation of the absence lenses.
  const FVector Astrarium(97000.0F, 0.0F, -19000.0F);
  Block(InWorld, *Cube, Astrarium, FVector(12.0F, 10.0F, 0.5F));
  for (int32 Lens = 0; Lens < 8; ++Lens) {
    const float Angle = static_cast<float>(Lens) * 45.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1300.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1300.0F,
                         720.0F);
    Block(InWorld, *Cube, Astrarium + Offset, FVector(0.5F, 0.5F, 6.5F),
          FRotator(0.0F, Angle, 0.0F));
  }
  Story(InWorld, Astrarium + FVector(-1200.0F, 0.0F, 140.0F), 80053);
  Enemy(InWorld, Astrarium + FVector(600.0F, 900.0F, 120.0F), 6601U,
        ELostsenseEnemyArchetype::ThoughtEater, 22U);

  // Kharos' three source-level phases map Local Map -> Broken Horizon ->
  // No Centre. Radial gaps provide visible missing-space geometry; the combat
  // implementation keeps fixed telegraph timing rather than invisible damage.
  const FVector KharosArena(102500.0F, 0.0F, -20200.0F);
  Block(InWorld, *Cube, KharosArena, FVector(11.0F, 11.0F, 0.55F));
  for (int32 Line = 0; Line < 10; ++Line) {
    const float Angle = static_cast<float>(Line) * 36.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1000.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1000.0F,
                         80.0F);
    Block(InWorld, *Cube, KharosArena + Offset,
          FVector(1.8F, 0.12F, 0.08F), FRotator(0.0F, Angle, 0.0F));
  }
  Enemy(InWorld, KharosArena + FVector(0.0F, 0.0F, 160.0F), 30041U,
        ELostsenseEnemyArchetype::Kharos, 23U);

  // Choirless Expanse is visible beyond the encounter as a silent basin with
  // architecture missing expected centres; Act VII retains campaign authority.
  const FVector Choirless(108000.0F, 0.0F, -21600.0F);
  for (int32 Plaza = 0; Plaza < 5; ++Plaza) {
    Block(InWorld, *Cube,
          Choirless + FVector(static_cast<float>(Plaza) * 850.0F,
                              Plaza % 2 == 0 ? -900.0F : 900.0F, 0.0F),
          FVector(3.5F, 3.5F, 0.3F));
  }
}
