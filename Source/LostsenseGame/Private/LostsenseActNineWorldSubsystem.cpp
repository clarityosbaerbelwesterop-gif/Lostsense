#include "LostsenseActNineWorldSubsystem.h"

#include "LostsenseActNineStoryActor.h"
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
void Story(
    UWorld &World, const FVector &Location, const int32 QuestId,
    const ELostsenseCampaignEnding Ending = ELostsenseCampaignEnding::None) {
  const FTransform Transform(FRotator::ZeroRotator, Location, FVector(0.8F));
  ALostsenseActNineStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActNineStoryActor>(
          ALostsenseActNineStoryActor::StaticClass(), Transform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (Actor != nullptr) {
    Actor->Configure(QuestId, Ending);
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
    if (Archetype == ELostsenseEnemyArchetype::AsterNull) {
      Actor->ConfigureAsterNullBoss(Id, Level);
    } else {
      Actor->ConfigureEnemyArchetype(Id, Archetype, Level);
    }
    UGameplayStatics::FinishSpawningActor(Actor, Transform);
  }
}
} // namespace

void ULostsenseActNineWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  // Zero Gallery / Sensewell (20036) begins with curated campaign-space
  // reconstructions. Repeated structural tells make the stable route readable.
  const FVector Gallery(144000.0F, 0.0F, -28200.0F);
  for (int32 Version = 0; Version < 7; ++Version) {
    Block(InWorld, *Cube,
          Gallery + FVector(static_cast<float>(Version) * 760.0F,
                            Version % 2 == 0 ? -650.0F : 650.0F,
                            -static_cast<float>(Version) * 100.0F),
          FVector(4.2F, 3.0F, 0.34F));
  }
  Story(InWorld, Gallery + FVector(900.0F, 0.0F, 100.0F), 80080);
  Enemy(InWorld, Gallery + FVector(2200.0F, 600.0F, 80.0F), 9801U,
        ELostsenseEnemyArchetype::ReconstructedElite, 29U);

  const FVector DescentRecords(150000.0F, 0.0F, -29200.0F);
  Block(InWorld, *Cube, DescentRecords, FVector(9.0F, 7.0F, 0.45F));
  Story(InWorld, DescentRecords + FVector(-700.0F, 0.0F, 100.0F), 80081);

  // Three consensus spines surround the route to Sensewell. Their source
  // composition makes the canonical count explicit while quest authority stays
  // deterministic and ordered.
  const FVector Spines(155000.0F, 0.0F, -30200.0F);
  Block(InWorld, *Cube, Spines, FVector(10.0F, 8.0F, 0.45F));
  for (int32 Spine = 0; Spine < 3; ++Spine) {
    Block(InWorld, *Cube,
          Spines + FVector(-900.0F + static_cast<float>(Spine) * 900.0F, 0.0F,
                           850.0F),
          FVector(0.55F, 0.55F, 8.0F));
  }
  Story(InWorld, Spines + FVector(0.0F, -900.0F, 100.0F), 80082);

  // Aster remains human-scale. Arena rings stand in for explicit Anchor,
  // Sequence and Reflection rule telegraphs before Witness of Many.
  const FVector Sensewell(161000.0F, 0.0F, -31500.0F);
  Block(InWorld, *Cube, Sensewell, FVector(13.0F, 12.0F, 0.55F));
  for (int32 Rule = 0; Rule < 12; ++Rule) {
    const float Angle = static_cast<float>(Rule) * 30.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1250.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1250.0F,
                         100.0F);
    Block(InWorld, *Cube, Sensewell + Offset, FVector(1.5F, 0.12F, 0.08F),
          FRotator(0.0F, Angle, 0.0F));
  }
  Enemy(InWorld, Sensewell + FVector(0.0F, 0.0F, 160.0F), 30060U,
        ELostsenseEnemyArchetype::AsterNull, 30U);

  // The ending is only reachable after Aster's death. Three distinct anchors
  // preserve player agency and persist exactly one Sever / Bind / Scatter
  // state.
  const FVector EndingDais(165000.0F, 0.0F, -32000.0F);
  Block(InWorld, *Cube, EndingDais, FVector(7.0F, 7.0F, 0.45F));
  Story(InWorld, EndingDais + FVector(-800.0F, 0.0F, 120.0F), 80084,
        ELostsenseCampaignEnding::Sever);
  Story(InWorld, EndingDais + FVector(0.0F, 800.0F, 120.0F), 80084,
        ELostsenseCampaignEnding::Bind);
  Story(InWorld, EndingDais + FVector(800.0F, 0.0F, 120.0F), 80084,
        ELostsenseCampaignEnding::Scatter);
}
