#include "LostsenseGameMode.h"

#include "LostsenseDevelopmentHUD.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseKnightCharacter.h"
#include "LostsensePlayerController.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
void AttachDevelopmentMarker(UWorld &World, AActor &Owner, UStaticMesh &Mesh,
                             const FVector &Scale) {
  AStaticMeshActor *Marker = World.SpawnActor<AStaticMeshActor>(
      Owner.GetActorLocation(), Owner.GetActorRotation());
  if (Marker == nullptr) {
    return;
  }

  Marker->GetStaticMeshComponent()->SetStaticMesh(&Mesh);
  Marker->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Marker->SetActorScale3D(Scale);
  Marker->AttachToActor(&Owner, FAttachmentTransformRules::KeepWorldTransform);
}
} // namespace

ALostsenseGameMode::ALostsenseGameMode() {
  DefaultPawnClass = ALostsenseKnightCharacter::StaticClass();
  PlayerControllerClass = ALostsensePlayerController::StaticClass();
  HUDClass = ALostsenseDevelopmentHUD::StaticClass();
}

void ALostsenseGameMode::StartPlay() {
  Super::StartPlay();
  SpawnDevelopmentArena();
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
    AttachDevelopmentMarker(*World, *Knight, *Cylinder,
                            FVector(0.52F, 0.52F, 1.05F));
  }
}

void ALostsenseGameMode::SpawnDevelopmentArena() {
  UWorld *World = GetWorld();
  if (World == nullptr) {
    return;
  }

  UStaticMesh *Cube =
      LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  UStaticMesh *Cylinder = LoadObject<UStaticMesh>(
      nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

  if (Cube != nullptr) {
    AStaticMeshActor *Floor = World->SpawnActor<AStaticMeshActor>(
        FVector(0.0F, 0.0F, -55.0F), FRotator::ZeroRotator);
    if (Floor != nullptr) {
      Floor->GetStaticMeshComponent()->SetStaticMesh(Cube);
      Floor->SetActorScale3D(FVector(24.0F, 24.0F, 1.0F));
      Floor->GetStaticMeshComponent()->SetCollisionEnabled(
          ECollisionEnabled::QueryAndPhysics);
    }
  }

  struct FEnemySpawn final {
    FVector Location;
    uint64 CombatantId;
    bool bElite;
  };

  const FEnemySpawn Spawns[] = {
      {FVector(520.0F, 0.0F, 100.0F), 1001U, false},
      {FVector(650.0F, 260.0F, 100.0F), 1002U, false},
      {FVector(760.0F, -260.0F, 100.0F), 1003U, true},
  };

  for (const FEnemySpawn &Spawn : Spawns) {
    const FTransform Transform(FRotator::ZeroRotator, Spawn.Location);
    ALostsenseEnemyCharacter *Enemy =
        World->SpawnActorDeferred<ALostsenseEnemyCharacter>(
            ALostsenseEnemyCharacter::StaticClass(), Transform, nullptr,
            nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Enemy != nullptr) {
      Enemy->ConfigureEnemy(Spawn.CombatantId, Spawn.bElite,
                            Spawn.bElite ? 5U : 3U);
      UGameplayStatics::FinishSpawningActor(Enemy, Transform);
      if (Cylinder != nullptr) {
        AttachDevelopmentMarker(*World, *Enemy, *Cylinder,
                                Spawn.bElite ? FVector(0.72F, 0.72F, 1.15F)
                                             : FVector(0.55F, 0.55F, 0.95F));
      }
    }
  }

  const FTransform BossTransform(FRotator::ZeroRotator,
                                 FVector(1180.0F, 0.0F, 110.0F));
  ALostsenseEnemyCharacter *Odran =
      World->SpawnActorDeferred<ALostsenseEnemyCharacter>(
          ALostsenseEnemyCharacter::StaticClass(), BossTransform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Odran != nullptr) {
    Odran->ConfigureOdranBoss(1099U, 8U);
    UGameplayStatics::FinishSpawningActor(Odran, BossTransform);
    if (Cylinder != nullptr) {
      AttachDevelopmentMarker(*World, *Odran, *Cylinder,
                              FVector(1.15F, 1.15F, 1.65F));
    }
  }
}
