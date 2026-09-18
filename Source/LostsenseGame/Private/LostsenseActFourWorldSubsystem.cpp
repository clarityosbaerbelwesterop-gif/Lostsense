#include "LostsenseActFourWorldSubsystem.h"

#include "LostsenseActFourStoryActor.h"
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
                const ELostsenseActFourInteraction Interaction) {
  const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
  ALostsenseActFourStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActFourStoryActor>(
          ALostsenseActFourStoryActor::StaticClass(), Transform, nullptr,
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

void Boss(UWorld &World, const FVector &Location, const uint64 Id,
          const ELostsenseEnemyArchetype Archetype, const uint32 Level) {
  const FTransform Transform(FRotator::ZeroRotator, Location);
  ALostsenseEnemyCharacter *Actor =
      World.SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor == nullptr) {
    return;
  }
  if (Archetype == ELostsenseEnemyArchetype::BishopPiston) {
    Actor->ConfigureBishopPistonBoss(Id, Level);
  } else {
    Actor->ConfigureGildedLungBoss(Id, Level);
  }
  UGameplayStatics::FinishSpawningActor(Actor, Transform);
}
} // namespace

void ULostsenseActFourWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  // Continues directly beyond the PR10 Pump Cathedral gate.
  const FVector CharterLine(38800.0F, 0.0F, -7000.0F);
  for (int32 Span = 0; Span < 5; ++Span) {
    Block(InWorld, *Cube,
          CharterLine + FVector(static_cast<float>(Span) * 680.0F, 0.0F,
                                -static_cast<float>(Span) * 130.0F),
          FVector(4.0F, 3.2F, 0.35F));
  }
  StoryActor(InWorld, CharterLine + FVector(900.0F, 0.0F, 180.0F),
             FVector(0.8F, 2.4F, 1.4F),
             ELostsenseActFourInteraction::CharterDepthMarker);

  // Pump Cathedral: giant paired pump aisles, pressure-lock lanes and a
  // readable ventilation manifold before Bishop Piston.
  const FVector Cathedral(42500.0F, 0.0F, -7850.0F);
  Block(InWorld, *Cube, Cathedral, FVector(14.0F, 10.0F, 0.5F));
  for (int32 Pump = 0; Pump < 8; ++Pump) {
    const float X = Cathedral.X - 2400.0F + static_cast<float>(Pump) * 700.0F;
    const float Y = Pump % 2 == 0 ? 1050.0F : -1050.0F;
    Block(InWorld, *Cube, FVector(X, Y, Cathedral.Z + 700.0F),
          FVector(0.75F, 0.75F, 7.0F));
  }
  StoryActor(InWorld, Cathedral + FVector(-1300.0F, 0.0F, 180.0F),
             FVector(1.5F, 1.0F, 0.5F),
             ELostsenseActFourInteraction::CathedralVentilationManifold);
  Enemy(InWorld, Cathedral + FVector(-200.0F, 800.0F, 160.0F), 4301U,
        ELostsenseEnemyArchetype::GildedDead, 16U);
  Enemy(InWorld, Cathedral + FVector(600.0F, -800.0F, 160.0F), 4302U,
        ELostsenseEnemyArchetype::PressureMutant, 16U);

  const FVector PistonArena(47200.0F, 0.0F, -8200.0F);
  Block(InWorld, *Cube, PistonArena, FVector(10.0F, 9.0F, 0.55F));
  for (int32 Vent = 0; Vent < 6; ++Vent) {
    const float Angle = static_cast<float>(Vent) * 60.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 900.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 900.0F,
                         180.0F);
    Block(InWorld, *Cube, PistonArena + Offset, FVector(1.2F, 1.2F, 0.3F));
  }
  Boss(InWorld, PistonArena + FVector(0.0F, 0.0F, 160.0F), 30024U,
       ELostsenseEnemyArchetype::BishopPiston, 17U);

  // King's Bore descends in rotating-looking offset galleries. The explicit
  // alignment interaction commits quest 80033 before the final boss unlocks.
  const FVector Bore(51000.0F, 0.0F, -9000.0F);
  for (int32 Ring = 0; Ring < 7; ++Ring) {
    const float Angle = static_cast<float>(Ring) * 28.0F;
    const FVector Centre =
        Bore + FVector(static_cast<float>(Ring) * 720.0F,
                       FMath::Sin(FMath::DegreesToRadians(Angle)) * 520.0F,
                       -static_cast<float>(Ring) * 240.0F);
    Block(InWorld, *Cube, Centre, FVector(4.3F, 3.2F, 0.38F),
          FRotator(0.0F, Angle, 0.0F));
    if (Ring == 2 || Ring == 4) {
      Enemy(InWorld, Centre + FVector(0.0F, 420.0F, 160.0F),
            4400U + static_cast<uint64>(Ring),
            ELostsenseEnemyArchetype::GildedDead, 17U);
    }
  }
  StoryActor(InWorld, Bore + FVector(3100.0F, 0.0F, -850.0F),
             FVector(1.4F, 1.4F, 0.45F),
             ELostsenseActFourInteraction::KingsBoreAlignment);

  // The Gilded Lung is an environmental megastructure: ribs and valve islands
  // provide readable safe positions while its three health phases model
  // Assisted Breathing -> Ruptured Bellows -> Last Breath.
  const FVector Lung(57000.0F, 0.0F, -10800.0F);
  Block(InWorld, *Cube, Lung, FVector(13.0F, 11.0F, 0.6F));
  for (int32 Rib = 0; Rib < 10; ++Rib) {
    const float Angle = static_cast<float>(Rib) * 36.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1100.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1100.0F,
                         650.0F);
    Block(InWorld, *Cube, Lung + Offset, FVector(0.45F, 0.45F, 6.5F),
          FRotator(0.0F, Angle, 0.0F));
  }
  Boss(InWorld, Lung + FVector(0.0F, 0.0F, 220.0F), 30026U,
       ELostsenseEnemyArchetype::GildedLung, 18U);

  // The first intact Namarith street is visible immediately beyond the lung,
  // making the Act IV consequence physical before Act V takes authority.
  const FVector NamarithBreach(60600.0F, 0.0F, -11600.0F);
  for (int32 Street = 0; Street < 5; ++Street) {
    Block(InWorld, *Cube,
          NamarithBreach +
              FVector(static_cast<float>(Street) * 620.0F, 0.0F, 0.0F),
          FVector(3.8F, 2.8F, 0.32F));
  }
}
