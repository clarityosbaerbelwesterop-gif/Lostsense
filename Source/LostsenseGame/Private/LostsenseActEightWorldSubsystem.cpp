#include "LostsenseActEightWorldSubsystem.h"

#include "LostsenseActEightStoryActor.h"
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
  ALostsenseActEightStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActEightStoryActor>(
          ALostsenseActEightStoryActor::StaticClass(), Transform, nullptr,
          nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
    if (Archetype == ELostsenseEnemyArchetype::IlyrVael) {
      Actor->ConfigureIlyrVaelBoss(Id, Level);
    } else {
      Actor->ConfigureEnemyArchetype(Id, Archetype, Level);
    }
    UGameplayStatics::FinishSpawningActor(Actor, Transform);
  }
}
} // namespace

void ULostsenseActEightWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  // Ashwake Wastes: cooled slag causeways through memory-ash intake fields.
  const FVector Ashwake(112000.0F, 0.0F, -22500.0F);
  for (int32 Span = 0; Span < 7; ++Span) {
    Block(InWorld, *Cube,
          Ashwake + FVector(static_cast<float>(Span) * 780.0F,
                            Span % 2 == 0 ? -500.0F : 500.0F,
                            -static_cast<float>(Span) * 80.0F),
          FVector(4.4F, 2.6F, 0.3F));
  }
  Story(InWorld, Ashwake + FVector(900.0F, 0.0F, 100.0F), 80070);
  Enemy(InWorld, Ashwake + FVector(2200.0F, 600.0F, 100.0F), 8701U,
        ELostsenseEnemyArchetype::Ashbound, 26U);

  // Ember Court is a social/trade enclave: the campaign explicitly negotiates
  // with Ossa Vark rather than treating the Red Archive as uniformly hostile.
  const FVector EmberCourt(118000.0F, 1200.0F, -23200.0F);
  Block(InWorld, *Cube, EmberCourt, FVector(9.0F, 7.0F, 0.45F));
  Story(InWorld, EmberCourt + FVector(0.0F, 0.0F, 120.0F), 80071);

  // Vask quota yard: furnace towers and pressure pipes feed the old destruction
  // quotas. The authored control commits quest 80072 after the player reaches
  // it.
  const FVector Vask(123000.0F, 0.0F, -24000.0F);
  Block(InWorld, *Cube, Vask, FVector(12.0F, 9.0F, 0.5F));
  for (int32 Furnace = 0; Furnace < 8; ++Furnace) {
    Block(InWorld, *Cube,
          Vask + FVector(-2100.0F + static_cast<float>(Furnace) * 600.0F,
                         Furnace % 2 == 0 ? 900.0F : -900.0F, 750.0F),
          FVector(0.65F, 0.65F, 7.0F));
  }
  Story(InWorld, Vask + FVector(-1200.0F, 0.0F, 120.0F), 80072);
  Enemy(InWorld, Vask + FVector(700.0F, 650.0F, 120.0F), 8721U,
        ELostsenseEnemyArchetype::Ashbound, 27U);

  // Foundry Cathedral (20034): pressure/furnace conduits form readable external
  // targets around the intake throat before Ilyr's arena.
  const FVector Cathedral(129000.0F, 0.0F, -25000.0F);
  Block(InWorld, *Cube, Cathedral, FVector(14.0F, 11.0F, 0.55F));
  for (int32 Conduit = 0; Conduit < 10; ++Conduit) {
    const float Angle = static_cast<float>(Conduit) * 36.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1450.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1450.0F,
                         650.0F);
    Block(InWorld, *Cube, Cathedral + Offset, FVector(0.45F, 0.45F, 6.0F),
          FRotator(0.0F, Angle, 0.0F));
  }
  Story(InWorld, Cathedral + FVector(-1500.0F, 0.0F, 120.0F), 80073);

  // Ilyr: The Necessary Man -> Second Silence -> Mercy Engine. Phase-three
  // conduit geometry remains external/readable rather than a generic demon
  // form.
  const FVector IlyrArena(135000.0F, 0.0F, -26000.0F);
  Block(InWorld, *Cube, IlyrArena, FVector(11.0F, 10.0F, 0.5F));
  for (int32 Conduit = 0; Conduit < 6; ++Conduit) {
    const float Angle = static_cast<float>(Conduit) * 60.0F;
    const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 1000.0F,
                         FMath::Sin(FMath::DegreesToRadians(Angle)) * 1000.0F,
                         350.0F);
    Block(InWorld, *Cube, IlyrArena + Offset, FVector(0.35F, 0.35F, 3.2F));
  }
  Enemy(InWorld, IlyrArena + FVector(0.0F, 0.0F, 160.0F), 30055U,
        ELostsenseEnemyArchetype::IlyrVael, 28U);

  // The post-Ilyr door is a separate campaign beat. It only becomes active
  // after 80074 completes and opens the authored descent toward the Last
  // Archive.
  const FVector LastArchiveDoor(139000.0F, 0.0F, -27000.0F);
  Block(InWorld, *Cube, LastArchiveDoor, FVector(2.0F, 5.0F, 5.5F));
  Story(InWorld, LastArchiveDoor + FVector(-450.0F, 0.0F, 120.0F), 80075);
}
