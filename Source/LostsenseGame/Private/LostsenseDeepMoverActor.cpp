#include "LostsenseDeepMoverActor.h"

#include "LostsenseStorySubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALostsenseDeepMoverActor::ALostsenseDeepMoverActor() {
  PrimaryActorTick.bCanEverTick = true;
  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

  static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
      TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (CubeMesh.Succeeded()) {
    Visual->SetStaticMesh(CubeMesh.Object);
  }
}

void ALostsenseDeepMoverActor::Configure(const ELostsenseDeepMoverKind InKind,
                                         const FVector InPrimaryOffset,
                                         const FVector InAlternateOffset) {
  Kind = InKind;
  Origin = GetActorLocation();
  PrimaryOffset = InPrimaryOffset;
  AlternateOffset = InAlternateOffset;
  bConfigured = true;
}

void ALostsenseDeepMoverActor::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  if (!bConfigured) {
    return;
  }

  UGameInstance *GameInstance = GetGameInstance();
  const ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Story == nullptr) {
    return;
  }

  FVector Target = Origin;
  switch (Kind) {
  case ELostsenseDeepMoverKind::LanternRailCart: {
    const auto State = Story->GetDeepMechanismState(
        ELostsenseDeepMechanism::LanternRailSwitch);
    if (State == ELostsenseDeepMechanismState::Primary) {
      Target += PrimaryOffset;
    } else if (State == ELostsenseDeepMechanismState::Alternate) {
      Target += AlternateOffset;
    }
    break;
  }
  case ELostsenseDeepMoverKind::ServiceCage:
    if (Story->GetDeepMechanismState(
            ELostsenseDeepMechanism::ServiceCageBrake) ==
        ELostsenseDeepMechanismState::Restored) {
      Target += PrimaryOffset;
    }
    break;
  case ELostsenseDeepMoverKind::VentilationRotor:
    if (Story->GetDeepMechanismState(
            ELostsenseDeepMechanism::VentilationIntake) ==
        ELostsenseDeepMechanismState::Open) {
      AddActorLocalRotation(FRotator(0.0F, DeltaSeconds * 85.0F, 0.0F));
    }
    return;
  }

  SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), Target,
                                            DeltaSeconds, 260.0F),
                   true);
}
