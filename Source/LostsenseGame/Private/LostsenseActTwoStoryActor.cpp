#include "LostsenseActTwoStoryActor.h"

#include "LostsenseKnightCharacter.h"
#include "LostsenseStorySubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace {
ULostsenseStorySubsystem *StoryFor(const AActor &Actor) {
  UGameInstance *GameInstance = Actor.GetGameInstance();
  return GameInstance != nullptr
             ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
             : nullptr;
}

ELostsenseActTwoStoryBeat
BeatFor(const ELostsenseActTwoInteraction Interaction) {
  switch (Interaction) {
  case ELostsenseActTwoInteraction::BlackSapTrace:
    return ELostsenseActTwoStoryBeat::BlackSapTrailFound;
  case ELostsenseActTwoInteraction::DomaIre:
    return ELostsenseActTwoStoryBeat::DomaIreMet;
  case ELostsenseActTwoInteraction::InfectedVillager:
    return ELostsenseActTwoStoryBeat::InfectedVillagerStabilized;
  case ELostsenseActTwoInteraction::GiltfenRootTunnel:
    return ELostsenseActTwoStoryBeat::GiltfenRootTunnelOpened;
  case ELostsenseActTwoInteraction::ThornChoirThreshold:
    return ELostsenseActTwoStoryBeat::ThornChoirDiscovered;
  case ELostsenseActTwoInteraction::PreserveWitnessRoot:
  case ELostsenseActTwoInteraction::BurnWitnessRoot:
    return ELostsenseActTwoStoryBeat::WitnessRootResolved;
  }
  return ELostsenseActTwoStoryBeat::BlackSapTrailFound;
}
} // namespace

ALostsenseActTwoStoryActor::ALostsenseActTwoStoryActor() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.15F;
  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (CubeMesh.Succeeded()) {
    Visual->SetStaticMesh(CubeMesh.Object);
  }
}

void ALostsenseActTwoStoryActor::Configure(
    const ELostsenseActTwoInteraction InInteraction) {
  Interaction = InInteraction;
  RestLocation = GetActorLocation();
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActTwoStoryActor::GetInteractionPrompt() const {
  switch (Interaction) {
  case ELostsenseActTwoInteraction::BlackSapTrace:
    return FText::FromString(TEXT("Inspect black sap memory trail"));
  case ELostsenseActTwoInteraction::DomaIre:
    return FText::FromString(TEXT("Speak with Doma Ire"));
  case ELostsenseActTwoInteraction::InfectedVillager:
    return FText::FromString(TEXT("Stabilize senseprint without killing host"));
  case ELostsenseActTwoInteraction::GiltfenRootTunnel:
    return FText::FromString(TEXT("Open remembered road through roots"));
  case ELostsenseActTwoInteraction::ThornChoirThreshold:
    return FText::FromString(TEXT("Enter the Thorn Choir sanctum"));
  case ELostsenseActTwoInteraction::PreserveWitnessRoot:
    return FText::FromString(TEXT("Preserve the abbey witness root"));
  case ELostsenseActTwoInteraction::BurnWitnessRoot:
    return FText::FromString(TEXT("Burn the abbey witness root"));
  }
  return FText::GetEmpty();
}

bool ALostsenseActTwoStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  if (!bConfigured || IsCompleted() ||
      FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) >
          FMath::Square(300.0F)) {
    return false;
  }
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr) {
    return false;
  }
  switch (Interaction) {
  case ELostsenseActTwoInteraction::BlackSapTrace:
    return Story->GetObjectiveState(80010) == ELostsenseObjectiveState::Active;
  case ELostsenseActTwoInteraction::DomaIre:
    return Story->HasActTwoBeat(ELostsenseActTwoStoryBeat::BlackSapTrailFound);
  case ELostsenseActTwoInteraction::InfectedVillager:
    return Story->HasActTwoBeat(ELostsenseActTwoStoryBeat::DomaIreMet);
  case ELostsenseActTwoInteraction::GiltfenRootTunnel:
    return Story->HasActTwoBeat(
        ELostsenseActTwoStoryBeat::InfectedVillagerStabilized);
  case ELostsenseActTwoInteraction::ThornChoirThreshold:
    return Story->HasActTwoBeat(
        ELostsenseActTwoStoryBeat::GiltfenRootTunnelOpened);
  case ELostsenseActTwoInteraction::PreserveWitnessRoot:
  case ELostsenseActTwoInteraction::BurnWitnessRoot:
    return Story->HasActTwoBeat(
               ELostsenseActTwoStoryBeat::MotherVeyrDefeated) &&
           !Story->HasActTwoBeat(
               ELostsenseActTwoStoryBeat::WitnessRootResolved);
  }
  return false;
}

bool ALostsenseActTwoStoryActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr) {
    return false;
  }
  bool Completed = false;
  if (Interaction == ELostsenseActTwoInteraction::PreserveWitnessRoot) {
    Completed =
        Story->ResolveWitnessRoot(ELostsenseWitnessRootDecision::Preserve);
  } else if (Interaction == ELostsenseActTwoInteraction::BurnWitnessRoot) {
    Completed = Story->ResolveWitnessRoot(ELostsenseWitnessRootDecision::Burn);
  } else {
    Completed = Story->CompleteActTwoBeat(BeatFor(Interaction));
  }
  if (!Completed) {
    return false;
  }
  SynchronizePresentation();
  return true;
}

void ALostsenseActTwoStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

bool ALostsenseActTwoStoryActor::IsCompleted() const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return Story != nullptr && Story->HasActTwoBeat(BeatFor(Interaction));
}

void ALostsenseActTwoStoryActor::SynchronizePresentation() {
  if (!bConfigured) {
    return;
  }
  SetActorLocation(RestLocation);
  SetActorScale3D(RestScale);
  if (!IsCompleted()) {
    return;
  }
  if (Interaction == ELostsenseActTwoInteraction::GiltfenRootTunnel ||
      Interaction == ELostsenseActTwoInteraction::ThornChoirThreshold) {
    SetActorLocation(RestLocation + FVector(0.0F, 0.0F, 700.0F));
  } else if (Interaction == ELostsenseActTwoInteraction::BlackSapTrace ||
             Interaction == ELostsenseActTwoInteraction::InfectedVillager ||
             Interaction == ELostsenseActTwoInteraction::PreserveWitnessRoot ||
             Interaction == ELostsenseActTwoInteraction::BurnWitnessRoot) {
    SetActorScale3D(RestScale * 0.65F);
  }
}
