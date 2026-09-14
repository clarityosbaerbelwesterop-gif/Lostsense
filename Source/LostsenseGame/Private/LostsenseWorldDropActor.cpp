#include "LostsenseWorldDropActor.h"

#include "LostsenseKnightCharacter.h"
#include "LostsenseRuntimeSubsystem.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALostsenseWorldDropActor::ALostsenseWorldDropActor() {
  PrimaryActorTick.bCanEverTick = false;

  PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
  SetRootComponent(PickupSphere);
  PickupSphere->InitSphereRadius(75.0F);
  PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
  PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  Visual->SetupAttachment(PickupSphere);
  Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Visual->SetRelativeScale3D(FVector(0.22F));

  static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
      TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  if (SphereMesh.Succeeded()) {
    Visual->SetStaticMesh(SphereMesh.Object);
  }
}

void ALostsenseWorldDropActor::InitializeDrop(
    const Lostsense::Gameplay::GeneratedLootEntry &InDrop) {
  Drop = MakeUnique<Lostsense::Gameplay::GeneratedLootEntry>(InDrop);
}

void ALostsenseWorldDropActor::BeginPlay() {
  Super::BeginPlay();
  PickupSphere->OnComponentBeginOverlap.AddDynamic(
      this, &ALostsenseWorldDropActor::OnPickupOverlap);
}

void ALostsenseWorldDropActor::OnPickupOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComponent, const int32 OtherBodyIndex,
    const bool bFromSweep, const FHitResult &SweepResult) {
  static_cast<void>(OverlappedComponent);
  static_cast<void>(OtherComponent);
  static_cast<void>(OtherBodyIndex);
  static_cast<void>(bFromSweep);
  static_cast<void>(SweepResult);

  if (Drop == nullptr ||
      Cast<ALostsenseKnightCharacter>(OtherActor) == nullptr) {
    return;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime != nullptr && Runtime->PickupGeneratedLoot(*Drop)) {
    Destroy();
  }
}
