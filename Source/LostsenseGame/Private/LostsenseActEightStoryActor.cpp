#include "LostsenseActEightStoryActor.h"

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

ALostsenseActEightStoryActor::ALostsenseActEightStoryActor() {
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

void ALostsenseActEightStoryActor::Configure(const int32 InQuestId) {
  QuestId = InQuestId;
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActEightStoryActor::GetInteractionPrompt() const {
  switch (QuestId) {
  case 80070:
    return FText::FromString(TEXT("Enter the Red Archive burn layer"));
  case 80071:
    return FText::FromString(TEXT("Negotiate the Ember Compact"));
  case 80072:
    return FText::FromString(TEXT("Disable the senseprint furnace quota"));
  case 80073:
    return FText::FromString(TEXT("Open the Foundry Cathedral intake throat"));
  case 80075:
    return FText::FromString(TEXT("Open the door that remembers the Returned"));
  default:
    return FText::GetEmpty();
  }
}

bool ALostsenseActEightStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return bConfigured && Story != nullptr &&
         Story->GetObjectiveState(QuestId) == ELostsenseObjectiveState::Active &&
         FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) <=
             FMath::Square(320.0F);
}

bool ALostsenseActEightStoryActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  const bool Completed =
      Story != nullptr && Story->CompleteCampaignQuest(QuestId);
  if (Completed) {
    SynchronizePresentation();
  }
  return Completed;
}

void ALostsenseActEightStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

void ALostsenseActEightStoryActor::SynchronizePresentation() {
  if (!bConfigured) {
    return;
  }
  SetActorScale3D(RestScale);
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story != nullptr &&
      Story->GetObjectiveState(QuestId) == ELostsenseObjectiveState::Completed) {
    SetActorScale3D(RestScale * 0.45F);
  }
}
