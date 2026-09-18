#include "LostsenseActFiveStoryActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "LostsenseKnightCharacter.h"
#include "LostsenseStorySubsystem.h"
#include "UObject/ConstructorHelpers.h"
namespace {
ULostsenseStorySubsystem *StoryFor(const AActor &Actor) {
  UGameInstance *GI = Actor.GetGameInstance();
  return GI != nullptr ? GI->GetSubsystem<ULostsenseStorySubsystem>() : nullptr;
}
} // namespace
ALostsenseActFiveStoryActor::ALostsenseActFiveStoryActor() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.2F;
  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (Mesh.Succeeded())
    Visual->SetStaticMesh(Mesh.Object);
}
void ALostsenseActFiveStoryActor::Configure(const int32 InQuestId) {
  QuestId = InQuestId;
  RestScale = GetActorScale3D();
  bConfigured = true;
  Sync();
}
FText ALostsenseActFiveStoryActor::GetInteractionPrompt() const {
  switch (QuestId) {
  case 80040:
    return FText::FromString(
        TEXT("Map the first Namarith district with Nera Quill"));
  case 80041:
    return FText::FromString(TEXT("Operate the civic consensus anchors"));
  case 80042:
    return FText::FromString(TEXT("Recover the First Witness protocol"));
  case 80043:
    return FText::FromString(TEXT("Open the Returned reconstruction chamber"));
  default:
    return FText::GetEmpty();
  }
}
bool ALostsenseActFiveStoryActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  return bConfigured && Story != nullptr &&
         Story->GetObjectiveState(QuestId) ==
             ELostsenseObjectiveState::Active &&
         FVector::DistSquared(GetActorLocation(),
                              Interactor.GetActorLocation()) <=
             FMath::Square(320.0F);
}
bool ALostsenseActFiveStoryActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor))
    return false;
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  const bool Done = Story != nullptr && Story->CompleteCampaignQuest(QuestId);
  if (Done)
    Sync();
  return Done;
}
void ALostsenseActFiveStoryActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  Sync();
}
void ALostsenseActFiveStoryActor::Sync() {
  if (!bConfigured)
    return;
  SetActorScale3D(RestScale);
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story != nullptr &&
      Story->GetObjectiveState(QuestId) == ELostsenseObjectiveState::Completed)
    SetActorScale3D(RestScale * 0.45F);
}
