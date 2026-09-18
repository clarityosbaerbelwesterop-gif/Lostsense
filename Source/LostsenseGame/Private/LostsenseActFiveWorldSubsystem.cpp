#include "LostsenseActFiveWorldSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LostsenseActFiveStoryActor.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameMode.h"
namespace {
void Block(UWorld &W, UStaticMesh &M, const FVector &L, const FVector &S) {
  AStaticMeshActor *A =
      W.SpawnActor<AStaticMeshActor>(L, FRotator::ZeroRotator);
  if (A) {
    A->GetStaticMeshComponent()->SetStaticMesh(&M);
    A->SetActorScale3D(S);
  }
}
void Story(UWorld &W, const FVector &L, int32 Q) {
  const FTransform T(FRotator::ZeroRotator, L, FVector(0.8F));
  auto *A = W.SpawnActorDeferred<ALostsenseActFiveStoryActor>(
      ALostsenseActFiveStoryActor::StaticClass(), T, nullptr, nullptr,
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (A) {
    A->Configure(Q);
    UGameplayStatics::FinishSpawningActor(A, T);
  }
}
void Enemy(UWorld &W, const FVector &L, uint64 Id, ELostsenseEnemyArchetype K,
           uint32 Lv) {
  const FTransform T(FRotator::ZeroRotator, L);
  auto *A = W.SpawnActorDeferred<ALostsenseEnemyCharacter>(
      ALostsenseEnemyCharacter::StaticClass(), T, nullptr, nullptr,
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (A) {
    A->ConfigureEnemyArchetype(Id, K, Lv);
    UGameplayStatics::FinishSpawningActor(A, T);
  }
}
} // namespace
void ULostsenseActFiveWorldSubsystem::OnWorldBeginPlay(UWorld &W) {
  Super::OnWorldBeginPlay(W);
  if (!W.IsGameWorld() ||
      Cast<ALostsenseGameMode>(W.GetAuthGameMode()) == nullptr)
    return;
  UStaticMesh *Cube =
      LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (!Cube)
    return;
  const FVector Street(62000.0F, 0.0F, -11600.0F);
  for (int32 I = 0; I < 8; ++I) {
    Block(W, *Cube, Street + FVector(I * 700.0F, 0.0F, 0.0F),
          FVector(4.2F, 3.0F, 0.35F));
  }
  Story(W, Street + FVector(900.0F, 0.0F, 180.0F), 80040);
  const FVector Anchors(67000.0F, 800.0F, -11800.0F);
  for (int32 I = 0; I < 5; ++I)
    Block(W, *Cube,
          Anchors + FVector(I * 650.0F, (I % 2 ? 700.0F : -700.0F), 500.0F),
          FVector(0.6F, 0.6F, 5.0F));
  Story(W, Anchors + FVector(800.0F, 0.0F, 100.0F), 80041);
  const FVector Loom(71000.0F, 0.0F, -12400.0F);
  for (int32 I = 0; I < 6; ++I)
    Block(W, *Cube, Loom + FVector(I * 720.0F, 0.0F, -I * 150.0F),
          FVector(4.3F, 4.0F, 0.4F));
  Story(W, Loom + FVector(900.0F, 0.0F, 120.0F), 80042);
  Enemy(W, Loom + FVector(1600.0F, 600.0F, 100.0F), 5401U,
        ELostsenseEnemyArchetype::CourtShade, 19U);
  const FVector Reconstruction(75200.0F, 0.0F, -13600.0F);
  Block(W, *Cube, Reconstruction, FVector(9.0F, 8.0F, 0.5F));
  Story(W, Reconstruction + FVector(-800.0F, 0.0F, 160.0F), 80043);
  for (int32 I = 0; I < 6; ++I)
    Block(W, *Cube,
          Reconstruction + FVector(-1500.0F + I * 600.0F,
                                   (I % 2 ? 850.0F : -850.0F), 650.0F),
          FVector(0.5F, 0.5F, 6.0F));
  const FVector Ysil(79000.0F, 0.0F, -14200.0F);
  Block(W, *Cube, Ysil, FVector(11.0F, 10.0F, 0.55F));
  Enemy(W, Ysil + FVector(0.0F, 0.0F, 180.0F), 30034U,
        ELostsenseEnemyArchetype::KeeperYsil, 20U);
}
