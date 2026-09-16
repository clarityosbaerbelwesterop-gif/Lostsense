#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseDeepMechanismActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseDeepMechanismKind : uint8 {
  LanternRailSwitch,
  ServiceCageBrake,
  RoyalPressureDoor,
  VentilationIntake,
  ReliefVent,
  VaurReturnShortcut,
  PumpCathedralApproachGate
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseDeepMechanismActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseDeepMechanismActor();

  void Configure(ELostsenseDeepMechanismKind InKind);

  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

  UFUNCTION(BlueprintPure, Category = "Lostsense|DeepRoute")
  ELostsenseDeepMechanismKind GetMechanismKind() const { return Kind; }

private:
  void SynchronizePresentation();

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseDeepMechanismKind Kind =
      ELostsenseDeepMechanismKind::LanternRailSwitch;

  bool bConfigured = false;
  FVector ClosedLocation = FVector::ZeroVector;
  FRotator RestRotation = FRotator::ZeroRotator;
};
