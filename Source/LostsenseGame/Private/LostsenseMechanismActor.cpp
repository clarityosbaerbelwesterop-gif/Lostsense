#include "LostsenseMechanismActor.h"

#include "LostsenseKnightCharacter.h"
#include "LostsenseStorySubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALostsenseMechanismActor::ALostsenseMechanismActor() {
  PrimaryActorTick.bCanEverTick = false;

  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

  static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (CubeMesh.Succeeded()) {
    Visual->SetStaticMesh(CubeMesh.Object);
  }
}

void ALostsenseMechanismActor::Configure(
    const ELostsenseMechanismKind InKind, const bool bInInitiallyRaised) {
  Kind = InKind;
  bRaised = bInInitiallyRaised;
  bConfigured = true;
  RaisedLocation = GetActorLocation();
  ApplyCounterweightState();
}

FText ALostsenseMechanismActor::GetInteractionPrompt() const {
  switch (Kind) {
  case ELostsenseMechanismKind::NinthDescentPlate:
    return FText::FromString(TEXT("Examine Ninth Descent plate"));
  case ELostsenseMechanismKind::Counterweight:
    return FText::FromString(
        bRaised ? TEXT("Release counterweight") : TEXT("Restore counterweight"));
  case ELostsenseMechanismKind::NinthDescentRecord:
    return FText::FromString(TEXT("Recover Ninth Descent record"));
  case ELostsenseMechanismKind::BellCoreRewardLift:
    return FText::FromString(TEXT("Raise Bell-Core reward lift"));
  }
  return FText::GetEmpty();
}

bool ALostsenseMechanismActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  if (!bConfigured || bConsumed) {
    return false;
  }
  if (FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) >
      FMath::Square(280.0F)) {
    return false;
  }

  if (Kind != ELostsenseMechanismKind::BellCoreRewardLift) {
    return true;
  }

  UGameInstance *GameInstance = GetGameInstance();
  const ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  return Story != nullptr && Story->HasBeat(ELostsenseStoryBeat::OdranDefeated) &&
         Story->HasBeat(ELostsenseStoryBeat::NinthDescentRecordRecovered);
}

bool ALostsenseMechanismActor::Interact(ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }

  if (Kind == ELostsenseMechanismKind::Counterweight) {
    bRaised = !bRaised;
    ApplyCounterweightState();
    return true;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Story == nullptr) {
    return false;
  }

  bool bAdvanced = false;
  switch (Kind) {
  case ELostsenseMechanismKind::NinthDescentPlate:
    bAdvanced = Story->CompleteBeat(ELostsenseStoryBeat::FoundNinthDescentPlate);
    break;
  case ELostsenseMechanismKind::NinthDescentRecord:
    bAdvanced =
        Story->CompleteBeat(ELostsenseStoryBeat::NinthDescentRecordRecovered);
    break;
  case ELostsenseMechanismKind::BellCoreRewardLift:
    bAdvanced = Story->CompleteBeat(ELostsenseStoryBeat::BellgraveChanged);
    break;
  case ELostsenseMechanismKind::Counterweight:
    break;
  }

  if (bAdvanced) {
    bConsumed = true;
  }
  return bAdvanced;
}

bool ALostsenseMechanismActor::IsRaised() const { return bRaised; }

void ALostsenseMechanismActor::ApplyCounterweightState() {
  if (Kind != ELostsenseMechanismKind::Counterweight || !bConfigured) {
    return;
  }
  SetActorLocation(RaisedLocation +
                   FVector(0.0F, 0.0F, bRaised ? 0.0F : -520.0F));
}
