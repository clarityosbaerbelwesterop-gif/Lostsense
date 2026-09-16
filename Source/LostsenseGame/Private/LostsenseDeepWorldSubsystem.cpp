#include "LostsenseDeepWorldSubsystem.h"

#include "LostsenseDeepMechanismActor.h"
#include "LostsenseDeepMoverActor.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
AStaticMeshActor *SpawnBlock(UWorld &World, UStaticMesh &Mesh,
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

void SpawnEnemy(UWorld &World, const FVector &Location, const uint64 CombatantId,
                const ELostsenseEnemyArchetype Archetype,
                const uint32 ItemLevel) {
  const FTransform Transform(FRotator::ZeroRotator, Location);
  ALostsenseEnemyCharacter *Enemy =
      World.SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
  if (Enemy == nullptr) {
    return;
  }
  Enemy->ConfigureEnemyArchetype(CombatantId, Archetype, ItemLevel);
  UGameplayStatics::FinishSpawningActor(Enemy, Transform);
}

void SpawnMechanism(UWorld &World, const FVector &Location,
                    const FVector &Scale,
                    const ELostsenseDeepMechanismKind Kind) {
  const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
  ALostsenseDeepMechanismActor *Actor =
      World.SpawnActorDeferred<ALostsenseDeepMechanismActor>(
          ALostsenseDeepMechanismActor::StaticClass(), Transform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor == nullptr) {
    return;
  }
  Actor->Configure(Kind);
  UGameplayStatics::FinishSpawningActor(Actor, Transform);
}

void SpawnMover(UWorld &World, const FVector &Location, const FVector &Scale,
                const ELostsenseDeepMoverKind Kind,
                const FVector &PrimaryOffset,
                const FVector &AlternateOffset = FVector::ZeroVector) {
  const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
  ALostsenseDeepMoverActor *Actor =
      World.SpawnActorDeferred<ALostsenseDeepMoverActor>(
          ALostsenseDeepMoverActor::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor == nullptr) {
    return;
  }
  Actor->Configure(Kind, PrimaryOffset, AlternateOffset);
  UGameplayStatics::FinishSpawningActor(Actor, Transform);
}

void SpawnLanternRail(UWorld &World, UStaticMesh &Cube) {
  const FVector Entry(22500.0F, 0.0F, -3650.0F);

  // The timber/iron Upper Vaur language narrows into an abandoned shift
  // exchange. The rail itself creates two readable routes rather than a room
  // selector: main descent and a raised maintenance gallery.
  for (int32 Segment = 0; Segment < 8; ++Segment) {
    const float X = Entry.X + static_cast<float>(Segment) * 620.0F;
    const float Z = Entry.Z - static_cast<float>(Segment) * 85.0F;
    SpawnBlock(World, Cube, FVector(X, 0.0F, Z),
               FVector(3.5F, 2.8F, 0.32F));
    SpawnBlock(World, Cube, FVector(X, 1150.0F, Z + 360.0F),
               FVector(3.2F, 0.9F, 0.24F));
    if (Segment % 2 == 0) {
      SpawnBlock(World, Cube, FVector(X, -900.0F, Z + 430.0F),
                 FVector(0.35F, 0.35F, 4.5F));
    }
  }

  SpawnBlock(World, Cube, Entry + FVector(1350.0F, 850.0F, 260.0F),
             FVector(6.0F, 1.0F, 0.25F), FRotator(0.0F, 12.0F, 0.0F));
  SpawnBlock(World, Cube, Entry + FVector(2600.0F, 850.0F, 80.0F),
             FVector(6.0F, 1.0F, 0.25F), FRotator(0.0F, -12.0F, 0.0F));

  SpawnMechanism(World, Entry + FVector(500.0F, -350.0F, 120.0F),
                 FVector(0.35F, 0.35F, 1.2F),
                 ELostsenseDeepMechanismKind::LanternRailSwitch);
  SpawnMover(World, Entry + FVector(900.0F, 0.0F, 130.0F),
             FVector(1.8F, 1.2F, 0.55F),
             ELostsenseDeepMoverKind::LanternRailCart,
             FVector(1800.0F, 0.0F, -240.0F),
             FVector(1200.0F, 850.0F, 160.0F));

  SpawnEnemy(World, Entry + FVector(1250.0F, -350.0F, 120.0F), 1501U,
             ELostsenseEnemyArchetype::EchoMiner, 8U);
  SpawnEnemy(World, Entry + FVector(2150.0F, 250.0F, -80.0F), 1502U,
             ELostsenseEnemyArchetype::HaulConstruct, 8U);
  SpawnEnemy(World, Entry + FVector(3000.0F, 850.0F, 150.0F), 1503U,
             ELostsenseEnemyArchetype::EchoMiner, 9U);

  // Brake service chamber. Once restored, the cage physically descends to the
  // Royal threshold and the authoritative route state unlocks the pressure door.
  const FVector BrakeRoom = Entry + FVector(4300.0F, 0.0F, -620.0F);
  SpawnBlock(World, Cube, BrakeRoom, FVector(5.0F, 4.0F, 0.4F));
  SpawnBlock(World, Cube, BrakeRoom + FVector(0.0F, 0.0F, 650.0F),
             FVector(0.45F, 4.0F, 6.0F));
  SpawnMechanism(World, BrakeRoom + FVector(-250.0F, -300.0F, 150.0F),
                 FVector(0.3F, 0.3F, 1.1F),
                 ELostsenseDeepMechanismKind::ServiceCageBrake);
  SpawnMover(World, BrakeRoom + FVector(650.0F, 0.0F, 180.0F),
             FVector(2.2F, 2.2F, 0.28F), ELostsenseDeepMoverKind::ServiceCage,
             FVector(0.0F, 0.0F, -1250.0F));
  SpawnEnemy(World, BrakeRoom + FVector(450.0F, 500.0F, 120.0F), 1510U,
             ELostsenseEnemyArchetype::ForemanKett, 9U);
}

void SpawnRoyalThreshold(UWorld &World, UStaticMesh &Cube) {
  const FVector Threshold(27800.0F, 0.0F, -5450.0F);

  // Royal masonry cuts across the practical Charter tunnels: broad stone ribs,
  // oversized pressure infrastructure and a vertical ventilation nave.
  SpawnBlock(World, Cube, Threshold, FVector(8.0F, 5.5F, 0.5F));
  for (int32 Rib = 0; Rib < 5; ++Rib) {
    const float X = Threshold.X + static_cast<float>(Rib) * 900.0F;
    SpawnBlock(World, Cube, FVector(X, 900.0F, Threshold.Z + 620.0F),
               FVector(0.55F, 0.55F, 7.0F));
    SpawnBlock(World, Cube, FVector(X, -900.0F, Threshold.Z + 620.0F),
               FVector(0.55F, 0.55F, 7.0F));
    SpawnBlock(World, Cube, FVector(X, 0.0F, Threshold.Z + 1250.0F),
               FVector(4.8F, 0.55F, 0.5F));
  }

  SpawnMechanism(World, Threshold + FVector(650.0F, 0.0F, 480.0F),
                 FVector(0.55F, 4.5F, 5.0F),
                 ELostsenseDeepMechanismKind::RoyalPressureDoor);

  const FVector Nave = Threshold + FVector(3000.0F, 0.0F, -420.0F);
  SpawnBlock(World, Cube, Nave, FVector(10.0F, 6.0F, 0.45F));
  SpawnBlock(World, Cube, Nave + FVector(0.0F, 0.0F, 1550.0F),
             FVector(10.0F, 0.6F, 0.45F));
  SpawnMechanism(World, Nave + FVector(-900.0F, -900.0F, 180.0F),
                 FVector(0.8F, 0.8F, 0.4F),
                 ELostsenseDeepMechanismKind::VentilationIntake);
  SpawnMechanism(World, Nave + FVector(900.0F, 900.0F, 180.0F),
                 FVector(0.8F, 0.8F, 0.4F),
                 ELostsenseDeepMechanismKind::ReliefVent);
  SpawnMover(World, Nave + FVector(0.0F, 0.0F, 1150.0F),
             FVector(3.2F, 0.35F, 3.2F),
             ELostsenseDeepMoverKind::VentilationRotor, FVector::ZeroVector);

  // Safe relief route and dangerous direct pressure lane remain spatially
  // distinct so later real-engine tuning can change hazard pressure without
  // changing authored topology.
  for (int32 Step = 0; Step < 5; ++Step) {
    SpawnBlock(World, Cube,
               Nave + FVector(static_cast<float>(Step) * 620.0F,
                              1350.0F, -120.0F - static_cast<float>(Step) * 80.0F),
               FVector(3.4F, 1.0F, 0.25F));
    SpawnBlock(World, Cube,
               Nave + FVector(static_cast<float>(Step) * 620.0F,
                              -350.0F, -260.0F - static_cast<float>(Step) * 95.0F),
               FVector(3.4F, 1.8F, 0.25F));
  }

  SpawnEnemy(World, Nave + FVector(400.0F, -450.0F, 120.0F), 1601U,
             ELostsenseEnemyArchetype::HaulConstruct, 10U);
  SpawnEnemy(World, Nave + FVector(1300.0F, 500.0F, 80.0F), 1602U,
             ELostsenseEnemyArchetype::EchoMiner, 10U);
  SpawnEnemy(World, Nave + FVector(2100.0F, -250.0F, -80.0F), 1603U,
             ELostsenseEnemyArchetype::ForemanKett, 11U);

  SpawnMechanism(World, Nave + FVector(2500.0F, 1500.0F, 120.0F),
                 FVector(0.55F, 2.2F, 3.2F),
                 ELostsenseDeepMechanismKind::VaurReturnShortcut);

  const FVector PumpApproach = Nave + FVector(3900.0F, 0.0F, -850.0F);
  SpawnBlock(World, Cube, PumpApproach, FVector(9.0F, 6.0F, 0.5F));
  SpawnBlock(World, Cube, PumpApproach + FVector(1200.0F, 0.0F, 750.0F),
             FVector(2.8F, 2.8F, 8.0F));
  SpawnBlock(World, Cube, PumpApproach + FVector(2300.0F, 0.0F, 520.0F),
             FVector(3.6F, 3.6F, 6.0F));
  SpawnMechanism(World, PumpApproach + FVector(2850.0F, 0.0F, 480.0F),
                 FVector(0.6F, 5.0F, 5.5F),
                 ELostsenseDeepMechanismKind::PumpCathedralApproachGate);
}
} // namespace

void ULostsenseDeepWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  SpawnLanternRail(InWorld, *Cube);
  SpawnRoyalThreshold(InWorld, *Cube);
}
