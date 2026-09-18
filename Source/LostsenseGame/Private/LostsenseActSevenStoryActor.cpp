#include "LostsenseActSevenStoryActor.h"

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

ALostsenseActSevenStoryActor::ALostsenseActSevenStoryActor() {
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

void ALostsenseActSevenStoryActor::Configure(
    const int32 InQuestId,
    const ELostsenseActSevenArchiveChoice InArchiveChoice) {
  QuestId = InQuestId;
  ArchiveChoice = InArchiveChoice;
  RestScale = GetActorScale3D();
  bConfigured = true;
  SynchronizePresentation();
}

FText ALostsenseActSevenStoryActor::GetInteractionPrompt() const {
  if (QuestId == 80062) {
    return ArchiveChoice ==
                   ELostsenseActSevenArchiveChoice::BellgraveCivilianArchive
               ? FText::FromString(
                     TEXT("Evacuate the Bellgrave civilian archive"))
               : FText::FromString(
                     TEXT("Evacuate the Crownless civilian archive"));
  }
  switch (QuestId) {
  case 80060:
    return FText::FromString(TEXT("Witness the synchronized noon bells"));
  case 80061:
    return FText::FromString(TEXT("Confront Oren Sile with the mentor ledger"));
  case 80063:
    return FText::FromString(TEXT("Rewrite the Gallows relay verdict"));
  default:
    return FText::GetEmpty();
  }
}

bool ALostsenseActSevenStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return bConfigured && Story != nullptr &&
         Story->GetObjectiveState(QuestId) == ELostsenseObjectiveState::Active &&
         FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) <=
             FMath::Square(320.0F);
}

bool ALostsenseActSevenStoryActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  const bool Completed =
      Story != nullptr &&
      (QuestId == 80062
           ? Story->ResolveActSevenArchiveChoice(ArchiveChoice)
           : Story->CompleteCampaignQuest(QuestId));
  if (Completed) {
    SynchronizePresentation();
  }
  return Completed;
}

void ALostsenseActSevenStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

void ALostsenseActSevenStoryActor::SynchronizePresentation() {
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
