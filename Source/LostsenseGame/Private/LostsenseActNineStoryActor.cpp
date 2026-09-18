#include "LostsenseActNineStoryActor.h"

#include "LostsenseKnightCharacter.h"

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

ALostsenseActNineStoryActor::ALostsenseActNineStoryActor() {
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

void ALostsenseActNineStoryActor::Configure(
    const int32 InQuestId, const ELostsenseCampaignEnding InEnding) {
  QuestId = InQuestId;
  Ending = InEnding;
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActNineStoryActor::GetInteractionPrompt() const {
  if (QuestId == 80084) {
    switch (Ending) {
    case ELostsenseCampaignEnding::Sever:
      return FText::FromString(TEXT("Sever the Loom's consensus authority"));
    case ELostsenseCampaignEnding::Bind:
      return FText::FromString(TEXT("Bind the Returned as living mediator"));
    case ELostsenseCampaignEnding::Scatter:
      return FText::FromString(TEXT("Scatter the Loom into bounded anchors"));
    default:
      return FText::GetEmpty();
    }
  }
  switch (QuestId) {
  case 80080:
    return FText::FromString(
        TEXT("Identify the stable authored reconstruction"));
  case 80081:
    return FText::FromString(TEXT("Free the Ninth Descent senseprints"));
  case 80082:
    return FText::FromString(TEXT("Shut down the consensus spines"));
  default:
    return FText::GetEmpty();
  }
}

bool ALostsenseActNineStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return bConfigured && Story != nullptr &&
         Story->GetObjectiveState(QuestId) ==
             ELostsenseObjectiveState::Active &&
         FVector::DistSquared(GetActorLocation(),
                              Interactor.GetActorLocation()) <=
             FMath::Square(320.0F);
}

bool ALostsenseActNineStoryActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  const bool Completed =
      Story != nullptr &&
      (QuestId == 80084 ? Story->ResolveCampaignEnding(Ending)
                        : Story->CompleteCampaignQuest(QuestId));
  if (Completed) {
    SynchronizePresentation();
  }
  return Completed;
}

void ALostsenseActNineStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

void ALostsenseActNineStoryActor::SynchronizePresentation() {
  if (!bConfigured) {
    return;
  }
  SetActorScale3D(RestScale);
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story != nullptr && Story->GetObjectiveState(QuestId) ==
                              ELostsenseObjectiveState::Completed) {
    SetActorScale3D(RestScale * 0.45F);
  }
}
