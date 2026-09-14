#include "LostsenseGameMode.h"

#include "LostsenseEnemyCharacter.h"
#include "LostsenseKnightCharacter.h"
#include "LostsensePlayerController.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ALostsenseGameMode::ALostsenseGameMode() {
  DefaultPawnClass = ALostsenseKnightCharacter::StaticClass();
  PlayerControllerClass = ALostsensePlayerController::StaticClass();
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
  if (World == nullptr || Controller == nullptr ||
      Controller->GetPawn() != nullptr) {
    return;
  }

  FActorSpawnParameters Parameters;
  Parameters.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
  ALostsenseKnightCharacter *Knight =
      World->SpawnActor<ALostsenseKnightCharacter>(
          ALostsenseKnightCharacter::StaticClass(), FVector(0.0F, 0.0F, 120.0F),
          FRotator::ZeroRotator, Parameters);
  if (Knight != nullptr) {
    Controller->Possess(Knight);
  }
}

void ALostsenseGameMode::SpawnDevelopmentArena() {
  UWorld *World = GetWorld();
  if (World == nullptr) {
    return;
  }

  UStaticMesh *Cube =
      LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
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
    }
  }
}
