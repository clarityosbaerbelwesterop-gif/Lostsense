#include "LostsenseDeepMechanismActor.h"

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

ELostsenseDeepMechanism
ToAuthorityMechanism(const ELostsenseDeepMechanismKind Kind) {
  switch (Kind) {
  case ELostsenseDeepMechanismKind::LanternRailSwitch:
    return ELostsenseDeepMechanism::LanternRailSwitch;
  case ELostsenseDeepMechanismKind::ServiceCageBrake:
    return ELostsenseDeepMechanism::ServiceCageBrake;
  case ELostsenseDeepMechanismKind::RoyalPressureDoor:
    return ELostsenseDeepMechanism::RoyalPressureDoor;
  case ELostsenseDeepMechanismKind::VentilationIntake:
    return ELostsenseDeepMechanism::VentilationIntake;
  case ELostsenseDeepMechanismKind::ReliefVent:
    return ELostsenseDeepMechanism::ReliefVent;
  case ELostsenseDeepMechanismKind::VaurReturnShortcut:
    return ELostsenseDeepMechanism::VaurReturnShortcut;
  case ELostsenseDeepMechanismKind::PumpCathedralApproachGate:
    return ELostsenseDeepMechanism::RoyalPressureDoor;
  }
  return ELostsenseDeepMechanism::LanternRailSwitch;
}
} // namespace

ALostsenseDeepMechanismActor::ALostsenseDeepMechanismActor() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.1F;
  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

  static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (CubeMesh.Succeeded()) {
    Visual->SetStaticMesh(CubeMesh.Object);
  }
}

void ALostsenseDeepMechanismActor::Configure(
    const ELostsenseDeepMechanismKind InKind) {
  Kind = InKind;
  bConfigured = true;
  ClosedLocation = GetActorLocation();
  RestRotation = GetActorRotation();
  SynchronizePresentation();
}

FText ALostsenseDeepMechanismActor::GetInteractionPrompt() const {
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  switch (Kind) {
  case ELostsenseDeepMechanismKind::LanternRailSwitch:
    if (Story != nullptr && Story->GetDeepMechanismState(
                                ELostsenseDeepMechanism::LanternRailSwitch) ==
                                ELostsenseDeepMechanismState::Alternate) {
      return FText::FromString(TEXT("Route rail to main descent"));
    }
    return FText::FromString(TEXT("Route rail to maintenance gallery"));
  case ELostsenseDeepMechanismKind::ServiceCageBrake:
    return FText::FromString(TEXT("Restore service-cage brake"));
  case ELostsenseDeepMechanismKind::RoyalPressureDoor:
    return FText::FromString(TEXT("Open royal pressure lock"));
  case ELostsenseDeepMechanismKind::VentilationIntake:
    return FText::FromString(TEXT("Open ventilation intake"));
  case ELostsenseDeepMechanismKind::ReliefVent:
    return FText::FromString(TEXT("Open pressure relief vent"));
  case ELostsenseDeepMechanismKind::VaurReturnShortcut:
    return FText::FromString(TEXT("Release Vaur return shortcut"));
  case ELostsenseDeepMechanismKind::PumpCathedralApproachGate:
    return FText::FromString(TEXT("Open Pump Cathedral approach"));
  }
  return FText::GetEmpty();
}

bool ALostsenseDeepMechanismActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  if (!bConfigured ||
      FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) >
          FMath::Square(300.0F)) {
    return false;
  }

  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr) {
    return false;
  }

  if (Kind == ELostsenseDeepMechanismKind::PumpCathedralApproachGate) {
    return Story->HasDeepRouteMilestone(
               ELostsenseDeepRouteMilestone::ActFourClearanceGranted) &&
           !Story->HasDeepRouteMilestone(
               ELostsenseDeepRouteMilestone::PumpCathedralApproachOpened);
  }

  const ELostsenseDeepMechanismState State =
      Story->GetDeepMechanismState(ToAuthorityMechanism(Kind));
  if (State == ELostsenseDeepMechanismState::Locked) {
    return false;
  }
  if (Kind == ELostsenseDeepMechanismKind::LanternRailSwitch) {
    return true;
  }
  if (Kind == ELostsenseDeepMechanismKind::ServiceCageBrake) {
    return State != ELostsenseDeepMechanismState::Restored;
  }
  return State != ELostsenseDeepMechanismState::Open;
}

bool ALostsenseDeepMechanismActor::Interact(
    ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }
  ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr) {
    return false;
  }

  bool bChanged = false;
  switch (Kind) {
  case ELostsenseDeepMechanismKind::LanternRailSwitch: {
    if (!Story->HasDeepRouteMilestone(
            ELostsenseDeepRouteMilestone::LanternRailDiscovered) &&
        !Story->CompleteDeepRouteMilestone(
            ELostsenseDeepRouteMilestone::LanternRailDiscovered)) {
      return false;
    }
    const auto Current = Story->GetDeepMechanismState(
        ELostsenseDeepMechanism::LanternRailSwitch);
    const auto Next = Current == ELostsenseDeepMechanismState::Alternate
                          ? ELostsenseDeepMechanismState::Primary
                          : ELostsenseDeepMechanismState::Alternate;
    bChanged = Story->SetDeepMechanismState(
        ELostsenseDeepMechanism::LanternRailSwitch, Next);
    break;
  }
  case ELostsenseDeepMechanismKind::ServiceCageBrake:
    bChanged =
        Story->SetDeepMechanismState(ELostsenseDeepMechanism::ServiceCageBrake,
                                     ELostsenseDeepMechanismState::Restored) &&
        Story->CompleteDeepRouteMilestone(
            ELostsenseDeepRouteMilestone::ServiceBrakeRestored);
    break;
  case ELostsenseDeepMechanismKind::RoyalPressureDoor:
    bChanged =
        Story->SetDeepMechanismState(ELostsenseDeepMechanism::RoyalPressureDoor,
                                     ELostsenseDeepMechanismState::Open) &&
        Story->CompleteDeepRouteMilestone(
            ELostsenseDeepRouteMilestone::RoyalThresholdOpened);
    break;
  case ELostsenseDeepMechanismKind::VentilationIntake:
    bChanged =
        Story->SetDeepMechanismState(ELostsenseDeepMechanism::VentilationIntake,
                                     ELostsenseDeepMechanismState::Open);
    break;
  case ELostsenseDeepMechanismKind::ReliefVent:
    bChanged = Story->SetDeepMechanismState(ELostsenseDeepMechanism::ReliefVent,
                                            ELostsenseDeepMechanismState::Open);
    break;
  case ELostsenseDeepMechanismKind::VaurReturnShortcut:
    bChanged = Story->SetDeepMechanismState(
        ELostsenseDeepMechanism::VaurReturnShortcut,
        ELostsenseDeepMechanismState::Open);
    break;
  case ELostsenseDeepMechanismKind::PumpCathedralApproachGate:
    bChanged = Story->CompleteDeepRouteMilestone(
        ELostsenseDeepRouteMilestone::PumpCathedralApproachOpened);
    break;
  }

  const bool bVentilationOpen =
      Story->GetDeepMechanismState(
          ELostsenseDeepMechanism::VentilationIntake) ==
          ELostsenseDeepMechanismState::Open &&
      Story->GetDeepMechanismState(ELostsenseDeepMechanism::ReliefVent) ==
          ELostsenseDeepMechanismState::Open;
  if (bChanged && bVentilationOpen &&
      !Story->HasDeepRouteMilestone(
          ELostsenseDeepRouteMilestone::VentilationNaveStabilized)) {
    bChanged = Story->CompleteDeepRouteMilestone(
        ELostsenseDeepRouteMilestone::VentilationNaveStabilized);
  }
  if (bChanged &&
      Story->HasDeepRouteMilestone(
          ELostsenseDeepRouteMilestone::VentilationNaveStabilized) &&
      Story->GetObjectiveState(80030) == ELostsenseObjectiveState::Active &&
      !Story->HasDeepRouteMilestone(
          ELostsenseDeepRouteMilestone::ActFourClearanceGranted)) {
    bChanged = Story->CompleteDeepRouteMilestone(
        ELostsenseDeepRouteMilestone::ActFourClearanceGranted);
  }

  if (bChanged) {
    SynchronizePresentation();
  }
  return bChanged;
}

void ALostsenseDeepMechanismActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  SynchronizePresentation();
}

void ALostsenseDeepMechanismActor::SynchronizePresentation() {
  if (!bConfigured) {
    return;
  }
  const ULostsenseStorySubsystem *Story = StoryFor(*this);
  if (Story == nullptr) {
    return;
  }

  SetActorLocation(ClosedLocation);
  SetActorRotation(RestRotation);

  if (Kind == ELostsenseDeepMechanismKind::PumpCathedralApproachGate) {
    if (Story->HasDeepRouteMilestone(
            ELostsenseDeepRouteMilestone::PumpCathedralApproachOpened)) {
      SetActorLocation(ClosedLocation + FVector(0.0F, 0.0F, 650.0F));
    }
    return;
  }

  const auto State = Story->GetDeepMechanismState(ToAuthorityMechanism(Kind));
  switch (Kind) {
  case ELostsenseDeepMechanismKind::LanternRailSwitch:
    if (State == ELostsenseDeepMechanismState::Alternate) {
      SetActorRotation(RestRotation + FRotator(0.0F, 35.0F, 0.0F));
    } else if (State == ELostsenseDeepMechanismState::Primary) {
      SetActorRotation(RestRotation + FRotator(0.0F, -35.0F, 0.0F));
    }
    break;
  case ELostsenseDeepMechanismKind::ServiceCageBrake:
    if (State == ELostsenseDeepMechanismState::Restored) {
      SetActorRotation(RestRotation + FRotator(-55.0F, 0.0F, 0.0F));
    }
    break;
  case ELostsenseDeepMechanismKind::RoyalPressureDoor:
  case ELostsenseDeepMechanismKind::VaurReturnShortcut:
    if (State == ELostsenseDeepMechanismState::Open) {
      SetActorLocation(ClosedLocation + FVector(0.0F, 0.0F, 650.0F));
    }
    break;
  case ELostsenseDeepMechanismKind::VentilationIntake:
  case ELostsenseDeepMechanismKind::ReliefVent:
    if (State == ELostsenseDeepMechanismState::Open) {
      SetActorRotation(RestRotation + FRotator(0.0F, 90.0F, 0.0F));
    }
    break;
  case ELostsenseDeepMechanismKind::PumpCathedralApproachGate:
    break;
  }
}
