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
  PickupSphere->SetSphereRadius(125.0F);
  PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
  PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  Visual->SetupAttachment(PickupSphere);
  Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Visual->SetRelativeScale3D(FVector(0.22F, 0.22F, 0.22F));

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

FText ALostsenseWorldDropActor::GetInteractionPrompt() const {
  return FText::FromString(TEXT("Pick up loot"));
}

bool ALostsenseWorldDropActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  return Drop != nullptr && !Interactor.IsActorBeingDestroyed() &&
         FVector::DistSquared(GetActorLocation(),
                              Interactor.GetActorLocation()) <=
             FMath::Square(220.0F);
}

bool ALostsenseWorldDropActor::Interact(ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr || !Runtime->PickupGeneratedLoot(*Drop)) {
    return false;
  }

  Drop.Reset();
  Destroy();
  return true;
}
