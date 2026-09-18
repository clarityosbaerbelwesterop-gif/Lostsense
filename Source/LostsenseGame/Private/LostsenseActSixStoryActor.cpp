#include "LostsenseActSixStoryActor.h"

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

ALostsenseActSixStoryActor::ALostsenseActSixStoryActor() {
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

void ALostsenseActSixStoryActor::Configure(const int32 InQuestId) {
  QuestId = InQuestId;
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActSixStoryActor::GetInteractionPrompt() const {
  switch (QuestId) {
  case 80050:
    return FText::FromString(TEXT("Bind the Quiet Assembly anchor line"));
  case 80051:
    return FText::FromString(TEXT("Request bearing from Serit No-Echo"));
  case 80052:
    return FText::FromString(
        TEXT("Recover the rejected contradiction record"));
  case 80053:
    return FText::FromString(
        TEXT("Rotate the Blind Astrarium absence lenses"));
  default:
    return FText::GetEmpty();
  }
}

bool ALostsenseActSixStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return bConfigured && Story != nullptr &&
         Story->GetObjectiveState(QuestId) == ELostsenseObjectiveState::Active &&
         FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) <=
             FMath::Square(320.0F);
}

bool ALostsenseActSixStoryActor::Interact(
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

void ALostsenseActSixStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

void ALostsenseActSixStoryActor::SynchronizePresentation() {
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
