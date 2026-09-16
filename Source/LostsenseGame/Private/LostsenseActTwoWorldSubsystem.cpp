#include "LostsenseActTwoWorldSubsystem.h"

#include "LostsenseActTwoStoryActor.h"
#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace {
AStaticMeshActor *Block(UWorld &World, UStaticMesh &Mesh, const FVector &Location,
                        const FVector &Scale,
                        const FRotator &Rotation = FRotator::ZeroRotator) {
  AStaticMeshActor *Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
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
                const ELostsenseActTwoInteraction Interaction) {
  const FTransform Transform(FRotator::ZeroRotator, Location, Scale);
  ALostsenseActTwoStoryActor *Actor =
      World.SpawnActorDeferred<ALostsenseActTwoStoryActor>(
          ALostsenseActTwoStoryActor::StaticClass(), Transform, nullptr, nullptr,
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
} // namespace

void ULostsenseActTwoWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
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

  // Deeper Ravelwood rises away from the Act I edge instead of replacing it.
  // Staggered root shelves create a readable northward route to Morrowstep.
  const FVector Ravelwood(9300.0F, 5200.0F, 180.0F);
  for (int32 Step = 0; Step < 8; ++Step) {
    const float X = Ravelwood.X + static_cast<float>(Step) * 650.0F;
    const float Y = Ravelwood.Y + static_cast<float>(Step) * 420.0F;
    const float Z = Ravelwood.Z + static_cast<float>(Step % 3) * 95.0F;
    Block(InWorld, *Cube, FVector(X, Y, Z), FVector(3.8F, 2.4F, 0.28F),
          FRotator(0.0F, static_cast<float>((Step % 2) * 12 - 6), 0.0F));
    Block(InWorld, *Cube, FVector(X + 150.0F, Y - 700.0F, Z + 500.0F),
          FVector(0.45F, 0.45F, 5.5F), FRotator(0.0F, 18.0F, 20.0F));
  }

  StoryActor(InWorld, Ravelwood + FVector(900.0F, 250.0F, 180.0F),
             FVector(0.55F, 0.55F, 0.22F),
             ELostsenseActTwoInteraction::BlackSapTrace);

  // Morrowstep: a small root-stilt social anchor, with Doma and an infected
  // villager deliberately separated from the combat lane so the authored
  // nonlethal resolution cannot be mistaken for an enemy kill objective.
  const FVector Morrowstep(14100.0F, 8500.0F, 520.0F);
  for (int32 Platform = 0; Platform < 5; ++Platform) {
    const FVector Center =
        Morrowstep + FVector(static_cast<float>(Platform) * 620.0F,
                            static_cast<float>((Platform % 2) * 700 - 350),
                            static_cast<float>(Platform % 3) * 120.0F);
    Block(InWorld, *Cube, Center, FVector(3.4F, 2.6F, 0.24F));
    Block(InWorld, *Cube, Center + FVector(0.0F, 0.0F, -520.0F),
          FVector(0.35F, 0.35F, 5.2F));
  }
  StoryActor(InWorld, Morrowstep + FVector(550.0F, -250.0F, 180.0F),
             FVector(0.6F, 0.6F, 1.8F), ELostsenseActTwoInteraction::DomaIre);
  StoryActor(InWorld, Morrowstep + FVector(1350.0F, 500.0F, 220.0F),
             FVector(0.65F, 0.65F, 1.7F),
             ELostsenseActTwoInteraction::InfectedVillager);

  Enemy(InWorld, Ravelwood + FVector(2300.0F, -400.0F, 220.0F), 2101U,
        ELostsenseEnemyArchetype::BellMaddenedCarrion, 8U);
  Enemy(InWorld, Ravelwood + FVector(3200.0F, 600.0F, 260.0F), 2102U,
        ELostsenseEnemyArchetype::EchoMiner, 8U);

  // The remembered road is a physical root tunnel: a sealed root mass moves
  // only after the nonlethal villager beat, then the route bends toward Giltfen.
  const FVector RootTunnel(17600.0F, 9600.0F, 250.0F);
  for (int32 Segment = 0; Segment < 6; ++Segment) {
    Block(InWorld, *Cube,
          RootTunnel + FVector(static_cast<float>(Segment) * 650.0F,
                               static_cast<float>(Segment) * 180.0F,
                               -static_cast<float>(Segment) * 55.0F),
          FVector(3.6F, 2.0F, 0.25F));
  }
  StoryActor(InWorld, RootTunnel + FVector(450.0F, 0.0F, 420.0F),
             FVector(0.65F, 3.2F, 4.5F),
             ELostsenseActTwoInteraction::GiltfenRootTunnel);

  // Thorn Choir is only approached in this block. The abbey threshold is
  // authored and discoverable, but Mother Veyr's final encounter is reserved
  // for the subsequent boss-production pass.
  const FVector Abbey(21800.0F, 10800.0F, -120.0F);
  Block(InWorld, *Cube, Abbey, FVector(9.0F, 6.0F, 0.45F));
  Block(InWorld, *Cube, Abbey + FVector(1400.0F, 0.0F, 800.0F),
        FVector(0.7F, 5.5F, 8.0F));
  Block(InWorld, *Cube, Abbey + FVector(2500.0F, 0.0F, 1150.0F),
        FVector(4.5F, 0.6F, 0.55F));
  StoryActor(InWorld, Abbey + FVector(900.0F, 0.0F, 180.0F),
             FVector(0.6F, 4.5F, 4.8F),
             ELostsenseActTwoInteraction::ThornChoirThreshold);
}
