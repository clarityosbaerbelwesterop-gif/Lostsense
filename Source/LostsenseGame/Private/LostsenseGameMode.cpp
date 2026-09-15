#include "LostsenseGameMode.h"

#include "LostsenseDevelopmentHUD.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseKnightCharacter.h"
#include "LostsenseMechanismActor.h"
#include "LostsenseNpcActor.h"
#include "LostsensePlayerController.h"
#include "LostsenseStoryGateActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
struct FSliceMeshes final {
  UStaticMesh *Cube = nullptr;
  UStaticMesh *Cylinder = nullptr;
};

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

void AttachMarker(UWorld &World, AActor &Owner, UStaticMesh &Mesh,
                  const FVector &Scale) {
  AStaticMeshActor *Marker = World.SpawnActor<AStaticMeshActor>(
      Owner.GetActorLocation(), Owner.GetActorRotation());
  if (Marker == nullptr) {
    return;
  }
  Marker->GetStaticMeshComponent()->SetStaticMesh(&Mesh);
  Marker->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
  Marker->GetStaticMeshComponent()->SetCollisionEnabled(
      ECollisionEnabled::NoCollision);
  Marker->SetActorScale3D(Scale);
  Marker->AttachToActor(&Owner, FAttachmentTransformRules::KeepWorldTransform);
}

void SpawnNpc(UWorld &World, UStaticMesh *Cylinder,
              const ELostsenseNpcIdentity Identity, const FVector &Location) {
  const FTransform Transform(FRotator::ZeroRotator, Location);
  ALostsenseNpcActor *Npc = World.SpawnActorDeferred<ALostsenseNpcActor>(
      ALostsenseNpcActor::StaticClass(), Transform, nullptr, nullptr,
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
  if (Npc == nullptr) {
    return;
  }
  Npc->Configure(Identity);
  UGameplayStatics::FinishSpawningActor(Npc, Transform);
  if (Cylinder != nullptr) {
    AttachMarker(World, *Npc, *Cylinder, FVector(0.48F, 0.48F, 0.95F));
  }
}

void SpawnEnemy(UWorld &World, UStaticMesh *Cylinder, const FVector &Location,
                const uint64 CombatantId,
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
  if (Cylinder != nullptr) {
    const bool bLarge = Archetype == ELostsenseEnemyArchetype::ForemanKett ||
                        Archetype == ELostsenseEnemyArchetype::HaulConstruct;
    AttachMarker(World, *Enemy, *Cylinder,
                 bLarge ? FVector(0.72F, 0.72F, 1.15F)
                        : FVector(0.55F, 0.55F, 0.95F));
  }
}

void SpawnStoryGate(UWorld &World, const FVector &Location,
                    const ELostsenseStoryBeat Beat,
                    const ELostsenseStoryBeat RequiredBeat) {
  const FTransform Transform(FRotator::ZeroRotator, Location);
  ALostsenseStoryGateActor *Gate =
      World.SpawnActorDeferred<ALostsenseStoryGateActor>(
          ALostsenseStoryGateActor::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Gate == nullptr) {
    return;
  }
  Gate->Configure(Beat, RequiredBeat, true);
  UGameplayStatics::FinishSpawningActor(Gate, Transform);
}

void SpawnMechanism(UWorld &World, const FVector &Location,
                    const FVector &Scale, const ELostsenseMechanismKind Kind,
                    const bool bInitiallyRaised = true) {
  const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
  ALostsenseMechanismActor *Mechanism =
      World.SpawnActorDeferred<ALostsenseMechanismActor>(
          ALostsenseMechanismActor::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Mechanism == nullptr) {
    return;
  }
  Mechanism->Configure(Kind, bInitiallyRaised);
  UGameplayStatics::FinishSpawningActor(Mechanism, Transform);
}

void SpawnBellgrave(UWorld &World, const FSliceMeshes &Meshes) {
  if (Meshes.Cube == nullptr) {
    return;
  }
  UStaticMesh &Cube = *Meshes.Cube;

  SpawnBlock(World, Cube, FVector(0.0F, 0.0F, -55.0F),
             FVector(22.0F, 18.0F, 1.0F));

  const FVector BelfryPiers[] = {
      FVector(-450.0F, -450.0F, 300.0F), FVector(-450.0F, 450.0F, 300.0F),
      FVector(450.0F, -450.0F, 300.0F), FVector(450.0F, 450.0F, 300.0F)};
  for (const FVector &Pier : BelfryPiers) {
    SpawnBlock(World, Cube, Pier, FVector(0.7F, 0.7F, 4.0F));
  }
  SpawnBlock(World, Cube, FVector(0.0F, 0.0F, 690.0F),
             FVector(5.5F, 0.7F, 0.45F));
  SpawnBlock(World, Cube, FVector(0.0F, 0.0F, 690.0F),
             FVector(0.7F, 5.5F, 0.45F));

  SpawnBlock(World, Cube, FVector(-1250.0F, -650.0F, 120.0F),
             FVector(3.0F, 2.4F, 1.8F));
  SpawnBlock(World, Cube, FVector(1150.0F, -700.0F, 60.0F),
             FVector(4.2F, 3.4F, 0.35F));
  SpawnBlock(World, Cube, FVector(-1250.0F, 700.0F, 80.0F),
             FVector(3.0F, 2.0F, 1.2F));
  SpawnBlock(World, Cube, FVector(1250.0F, 700.0F, 170.0F),
             FVector(2.7F, 2.7F, 2.4F));
  SpawnBlock(World, Cube, FVector(1250.0F, 700.0F, -240.0F),
             FVector(1.8F, 1.8F, 2.5F));

  for (int32 Index = 0; Index < 4; ++Index) {
    const float X = -1900.0F + static_cast<float>(Index) * 520.0F;
    SpawnBlock(World, Cube, FVector(X, 1500.0F, 95.0F),
               FVector(2.0F, 1.5F, 1.5F));
    SpawnBlock(World, Cube, FVector(X + 220.0F, -1550.0F, 95.0F),
               FVector(1.7F, 1.4F, 1.5F));
  }

  for (int32 Index = 0; Index < 6; ++Index) {
    SpawnBlock(World, Cube,
               FVector(2200.0F + static_cast<float>(Index) * 520.0F, 0.0F,
                       -15.0F + static_cast<float>(Index) * 25.0F),
               FVector(3.0F, 2.2F, 0.35F));
  }

  SpawnNpc(World, Meshes.Cylinder, ELostsenseNpcIdentity::MaraVenn,
           FVector(-850.0F, -650.0F, 120.0F));
  SpawnNpc(World, Meshes.Cylinder, ELostsenseNpcIdentity::HadrunPike,
           FVector(900.0F, -700.0F, 110.0F));
  SpawnNpc(World, Meshes.Cylinder, ELostsenseNpcIdentity::TamsinCoil,
           FVector(-950.0F, 700.0F, 110.0F));
  SpawnEnemy(World, Meshes.Cylinder, FVector(1050.0F, -250.0F, 110.0F), 1000U,
             ELostsenseEnemyArchetype::CharterDeserter, 2U);
  SpawnStoryGate(World, FVector(4950.0F, 0.0F, 180.0F),
                 ELostsenseStoryBeat::EnteredRavelwood,
                 ELostsenseStoryBeat::BellgraveDepartureAllowed);
}

void SpawnRavelwoodAndWeepingCut(UWorld &World, const FSliceMeshes &Meshes) {
  if (Meshes.Cube == nullptr) {
    return;
  }
  UStaticMesh &Cube = *Meshes.Cube;

  SpawnBlock(World, Cube, FVector(5700.0F, 0.0F, 90.0F),
             FVector(9.0F, 8.0F, 0.6F));
  SpawnBlock(World, Cube, FVector(6500.0F, 900.0F, 210.0F),
             FVector(5.0F, 2.2F, 0.45F), FRotator(0.0F, 18.0F, 0.0F));
  SpawnBlock(World, Cube, FVector(6550.0F, -900.0F, 70.0F),
             FVector(5.0F, 2.0F, 0.45F), FRotator(0.0F, -20.0F, 0.0F));
  for (int32 Index = 0; Index < 8; ++Index) {
    const float Y = -1400.0F + static_cast<float>(Index) * 400.0F;
    SpawnBlock(
        World, Cube,
        FVector(5600.0F + static_cast<float>(Index % 3) * 500.0F, Y, 420.0F),
        FVector(0.45F, 0.45F, 5.0F),
        FRotator(0.0F, static_cast<float>(Index) * 17.0F, 0.0F));
  }

  SpawnEnemy(World, Meshes.Cylinder, FVector(5600.0F, -450.0F, 180.0F), 1101U,
             ELostsenseEnemyArchetype::BellMaddenedCarrion, 3U);
  SpawnEnemy(World, Meshes.Cylinder, FVector(6300.0F, 420.0F, 210.0F), 1102U,
             ELostsenseEnemyArchetype::CharterDeserter, 3U);

  for (int32 Index = 0; Index < 7; ++Index) {
    SpawnBlock(World, Cube,
               FVector(7600.0F + static_cast<float>(Index) * 420.0F,
                       (Index % 2 == 0) ? -260.0F : 260.0F,
                       40.0F - static_cast<float>(Index) * 120.0F),
               FVector(2.8F, 2.3F, 0.4F));
  }
  SpawnBlock(World, Cube, FVector(8500.0F, 1000.0F, -120.0F),
             FVector(5.0F, 1.4F, 0.35F), FRotator(0.0F, 28.0F, 0.0F));
  SpawnEnemy(World, Meshes.Cylinder, FVector(8500.0F, 0.0F, -220.0F), 1201U,
             ELostsenseEnemyArchetype::EchoMiner, 4U);
  SpawnMechanism(World, FVector(9050.0F, 180.0F, -560.0F),
                 FVector(1.2F, 1.2F, 0.18F),
                 ELostsenseMechanismKind::NinthDescentPlate);
  SpawnStoryGate(World, FVector(10000.0F, 0.0F, -620.0F),
                 ELostsenseStoryBeat::EnteredUpperVaur,
                 ELostsenseStoryBeat::FoundNinthDescentPlate);
}

void SpawnUpperVaurAndCoinless(UWorld &World, const FSliceMeshes &Meshes) {
  if (Meshes.Cube == nullptr) {
    return;
  }
  UStaticMesh &Cube = *Meshes.Cube;

  const float BaseX = 10800.0F;
  for (int32 Tier = 0; Tier < 4; ++Tier) {
    const float Z = -700.0F - static_cast<float>(Tier) * 420.0F;
    SpawnBlock(World, Cube, FVector(BaseX, 0.0F, Z),
               FVector(8.0F, 3.0F, 0.35F));
    SpawnBlock(World, Cube, FVector(BaseX + 700.0F, 850.0F, Z - 150.0F),
               FVector(4.5F, 1.2F, 0.25F), FRotator(0.0F, 22.0F, 0.0F));
    SpawnBlock(World, Cube, FVector(BaseX - 700.0F, -850.0F, Z - 280.0F),
               FVector(4.5F, 1.2F, 0.25F), FRotator(0.0F, -22.0F, 0.0F));
  }
  for (int32 Index = 0; Index < 6; ++Index) {
    SpawnBlock(World, Cube,
               FVector(BaseX - 1800.0F + static_cast<float>(Index) * 720.0F,
                       (Index % 2 == 0) ? 1250.0F : -1250.0F, -1300.0F),
               FVector(0.45F, 0.45F, 9.0F));
  }

  SpawnEnemy(World, Meshes.Cylinder, FVector(10400.0F, -250.0F, -650.0F), 1301U,
             ELostsenseEnemyArchetype::EchoMiner, 5U);
  SpawnEnemy(World, Meshes.Cylinder, FVector(11100.0F, 350.0F, -1100.0F), 1302U,
             ELostsenseEnemyArchetype::HaulConstruct, 5U);

  const float ShaftX = 14500.0F;
  SpawnStoryGate(World, FVector(ShaftX - 250.0F, 0.0F, -2100.0F),
                 ELostsenseStoryBeat::EnteredCoinlessShaft,
                 ELostsenseStoryBeat::EnteredUpperVaur);
  for (int32 Room = 0; Room < 7; ++Room) {
    const float X = ShaftX + static_cast<float>(Room) * 950.0F;
    const float Z = -2200.0F - static_cast<float>(Room) * 180.0F;
    SpawnBlock(World, Cube, FVector(X, 0.0F, Z), FVector(5.2F, 4.2F, 0.45F));
    SpawnBlock(World, Cube, FVector(X, 430.0F, Z + 360.0F),
               FVector(5.2F, 0.35F, 3.2F));
    SpawnBlock(World, Cube, FVector(X, -430.0F, Z + 360.0F),
               FVector(5.2F, 0.35F, 3.2F));
  }
  SpawnBlock(World, Cube, FVector(ShaftX + 2850.0F, 1050.0F, -2850.0F),
             FVector(5.0F, 1.0F, 0.3F));
  SpawnBlock(World, Cube, FVector(ShaftX + 3600.0F, 1150.0F, -3000.0F),
             FVector(3.0F, 1.0F, 0.3F));

  SpawnMechanism(World, FVector(ShaftX + 2550.0F, -650.0F, -2350.0F),
                 FVector(1.1F, 1.1F, 3.0F),
                 ELostsenseMechanismKind::Counterweight, true);
  SpawnMechanism(World, FVector(ShaftX + 3050.0F, 650.0F, -2450.0F),
                 FVector(1.1F, 1.1F, 3.0F),
                 ELostsenseMechanismKind::Counterweight, false);
  SpawnEnemy(World, Meshes.Cylinder,
             FVector(ShaftX + 3300.0F, 300.0F, -2700.0F), 1402U,
             ELostsenseEnemyArchetype::HaulConstruct, 7U);
  SpawnEnemy(World, Meshes.Cylinder, FVector(ShaftX + 3600.0F, 0.0F, -2700.0F),
             1401U, ELostsenseEnemyArchetype::ForemanKett, 7U);

  const FVector ArenaCenter(ShaftX + 6650.0F, 0.0F, -3400.0F);
  SpawnBlock(World, Cube, ArenaCenter + FVector(0.0F, 0.0F, -60.0F),
             FVector(9.0F, 9.0F, 0.55F));
  const FVector PlateOffsets[] = {
      FVector(650.0F, 0.0F, 20.0F), FVector(-650.0F, 0.0F, 20.0F),
      FVector(0.0F, 650.0F, 20.0F), FVector(0.0F, -650.0F, 20.0F)};
  for (const FVector &Offset : PlateOffsets) {
    SpawnBlock(World, Cube, ArenaCenter + Offset, FVector(1.4F, 1.4F, 0.12F));
  }
  SpawnBlock(World, Cube, ArenaCenter + FVector(0.0F, 0.0F, 900.0F),
             FVector(6.0F, 1.2F, 1.8F));
  SpawnBlock(World, Cube, ArenaCenter + FVector(900.0F, 0.0F, 500.0F),
             FVector(0.25F, 0.25F, 6.0F));
  SpawnBlock(World, Cube, ArenaCenter + FVector(-900.0F, 0.0F, 500.0F),
             FVector(0.25F, 0.25F, 6.0F));
  SpawnMechanism(World, ArenaCenter + FVector(900.0F, 0.0F, 500.0F),
                 FVector(0.9F, 0.9F, 3.0F),
                 ELostsenseMechanismKind::Counterweight, true);
  SpawnMechanism(World, ArenaCenter + FVector(-900.0F, 0.0F, 500.0F),
                 FVector(0.9F, 0.9F, 3.0F),
                 ELostsenseMechanismKind::Counterweight, true);
  SpawnStoryGate(World, ArenaCenter + FVector(-1100.0F, 0.0F, 100.0F),
                 ELostsenseStoryBeat::OdranEncounterStarted,
                 ELostsenseStoryBeat::EnteredCoinlessShaft);
  SpawnMechanism(World, ArenaCenter + FVector(300.0F, 420.0F, 90.0F),
                 FVector(0.8F, 0.8F, 0.25F),
                 ELostsenseMechanismKind::NinthDescentRecord);
  SpawnMechanism(World, ArenaCenter + FVector(1350.0F, 0.0F, 40.0F),
                 FVector(2.0F, 2.0F, 0.35F),
                 ELostsenseMechanismKind::BellCoreRewardLift);

  const FTransform BossTransform(FRotator::ZeroRotator,
                                 ArenaCenter + FVector(0.0F, 0.0F, 120.0F));
  ALostsenseEnemyCharacter *Odran =
      World.SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), BossTransform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Odran != nullptr) {
    Odran->ConfigureOdranBoss(30001U, 8U);
    UGameplayStatics::FinishSpawningActor(Odran, BossTransform);
    if (Meshes.Cylinder != nullptr) {
      AttachMarker(World, *Odran, *Meshes.Cylinder,
                   FVector(1.15F, 1.15F, 1.65F));
    }
  }
}
} // namespace

ALostsenseGameMode::ALostsenseGameMode() {
  DefaultPawnClass = ALostsenseKnightCharacter::StaticClass();
  PlayerControllerClass = ALostsensePlayerController::StaticClass();
  HUDClass = ALostsenseDevelopmentHUD::StaticClass();
}

void ALostsenseGameMode::StartPlay() {
  Super::StartPlay();
  SpawnVerticalSliceWorld();
  EnsurePlayableKnight();
}

void ALostsenseGameMode::EnsurePlayableKnight() {
  UWorld *World = GetWorld();
  APlayerController *Controller =
      World != nullptr ? World->GetFirstPlayerController() : nullptr;
  if (World == nullptr || Controller == nullptr) {
    return;
  }

  ALostsenseKnightCharacter *Knight =
      Cast<ALostsenseKnightCharacter>(Controller->GetPawn());
  if (Knight == nullptr) {
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Knight = World->SpawnActor<ALostsenseKnightCharacter>(
        ALostsenseKnightCharacter::StaticClass(), FVector(0.0F, 0.0F, 120.0F),
        FRotator::ZeroRotator, Parameters);
    if (Knight != nullptr) {
      Controller->Possess(Knight);
    }
  }

  if (Knight == nullptr) {
    return;
  }

  UStaticMesh *Cylinder = LoadObject<UStaticMesh>(
      nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  if (Cylinder != nullptr) {
    AttachMarker(*World, *Knight, *Cylinder, FVector(0.52F, 0.52F, 1.05F));
  }
}

void ALostsenseGameMode::SpawnVerticalSliceWorld() {
  UWorld *World = GetWorld();
  if (World == nullptr) {
    return;
  }

  FSliceMeshes Meshes;
  Meshes.Cube =
      LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  Meshes.Cylinder = LoadObject<UStaticMesh>(
      nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  if (Meshes.Cube == nullptr) {
    return;
  }

  SpawnBellgrave(*World, Meshes);
  SpawnRavelwoodAndWeepingCut(*World, Meshes);
  SpawnUpperVaurAndCoinless(*World, Meshes);
}
