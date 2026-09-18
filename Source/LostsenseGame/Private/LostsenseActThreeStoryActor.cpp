#include "LostsenseActThreeStoryActor.h"

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

ALostsenseActThreeStoryActor::ALostsenseActThreeStoryActor() {
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

void ALostsenseActThreeStoryActor::Configure(
    const ELostsenseActThreeInteraction InInteraction) {
  Interaction = InInteraction;
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActThreeStoryActor::GetInteractionPrompt() const {
  switch (Interaction) {
  case ELostsenseActThreeInteraction::SolenneEntry:
    return FText::FromString(
        TEXT("Enter Crownless Reach with Regent Solenne Mave"));
  case ELostsenseActThreeInteraction::RoyalLedgers:
    return FText::FromString(TEXT("Recover the erased royal ledgers"));
  case ELostsenseActThreeInteraction::GildedCharterEvidence:
    return FText::FromString(
        TEXT("Expose the Gilded Charter descent financing"));
  case ELostsenseActThreeInteraction::PalaceArchive:
    return FText::FromString(
        TEXT("Reconcile the contradictory palace archive"));
  }
  return FText::GetEmpty();
}

bool ALostsenseActThreeStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  if (!bConfigured || IsCompleted() ||
      FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) >
          FMath::Square(300.0F)) {
    return false;
  }
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return Story != nullptr && Story->GetObjectiveState(QuestId()) ==
                                 ELostsenseObjectiveState::Active;
}

bool ALostsenseActThreeStoryActor::Interact(
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

void ALostsenseActThreeStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

int32 ALostsenseActThreeStoryActor::QuestId() const {
  switch (Interaction) {
  case ELostsenseActThreeInteraction::SolenneEntry:
    return 80020;
  case ELostsenseActThreeInteraction::RoyalLedgers:
    return 80021;
  case ELostsenseActThreeInteraction::GildedCharterEvidence:
    return 80022;
  case ELostsenseActThreeInteraction::PalaceArchive:
    return 80023;
  }
  return 0;
}

bool ALostsenseActThreeStoryActor::IsCompleted() const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return Story != nullptr && Story->GetObjectiveState(QuestId()) ==
                                 ELostsenseObjectiveState::Completed;
}

void ALostsenseActThreeStoryActor::SynchronizePresentation() {
  if (!bConfigured) {
    return;
  }
  SetActorScale3D(RestScale);
  if (IsCompleted() &&
      Interaction != ELostsenseActThreeInteraction::SolenneEntry) {
    SetActorScale3D(RestScale * 0.45F);
  }
}
