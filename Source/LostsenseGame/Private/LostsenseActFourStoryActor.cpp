#include "LostsenseActFourStoryActor.h"

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
} // namespace

ALostsenseActFourStoryActor::ALostsenseActFourStoryActor() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.2F;
  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (CubeMesh.Succeeded()) {
    Visual->SetStaticMesh(CubeMesh.Object);
  }
}

void ALostsenseActFourStoryActor::Configure(
    const ELostsenseActFourInteraction InInteraction) {
  Interaction = InInteraction;
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActFourStoryActor::GetInteractionPrompt() const {
  switch (Interaction) {
  case ELostsenseActFourInteraction::CharterDepthMarker:
    return FText::FromString(TEXT("Cross below the Charter depth line"));
  case ELostsenseActFourInteraction::CathedralVentilationManifold:
    return FText::FromString(
        TEXT("Restart the Pump Cathedral ventilation manifold"));
  case ELostsenseActFourInteraction::KingsBoreAlignment:
    return FText::FromString(
        TEXT("Align the King's Bore galleries with the Ninth Descent route"));
  }
  return FText::GetEmpty();
}

bool ALostsenseActFourStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  if (!bConfigured || IsCompleted() ||
      FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) >
          FMath::Square(320.0F)) {
    return false;
  }
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr ||
      Story->GetObjectiveState(QuestId()) != ELostsenseObjectiveState::Active) {
    return false;
  }
  if (Interaction == ELostsenseActFourInteraction::CharterDepthMarker) {
    return Story->HasDeepRouteMilestone(
        ELostsenseDeepRouteMilestone::PumpCathedralApproachOpened);
  }
  return true;
}

bool ALostsenseActFourStoryActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr || !Story->CompleteCampaignQuest(QuestId())) {
    return false;
  }
  SynchronizePresentation();
  return true;
}

void ALostsenseActFourStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

int32 ALostsenseActFourStoryActor::QuestId() const {
  switch (Interaction) {
  case ELostsenseActFourInteraction::CharterDepthMarker:
    return 80030;
  case ELostsenseActFourInteraction::CathedralVentilationManifold:
    return 80031;
  case ELostsenseActFourInteraction::KingsBoreAlignment:
    return 80033;
  }
  return 0;
}

bool ALostsenseActFourStoryActor::IsCompleted() const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return Story != nullptr &&
         Story->GetObjectiveState(QuestId()) ==
             ELostsenseObjectiveState::Completed;
}

void ALostsenseActFourStoryActor::SynchronizePresentation() {
  if (!bConfigured) {
    return;
  }
  SetActorScale3D(RestScale);
  if (IsCompleted()) {
    SetActorScale3D(RestScale * 0.45F);
  }
}
