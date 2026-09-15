#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseMechanismActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseMechanismKind : uint8 {
  NinthDescentPlate,
  Counterweight,
  NinthDescentRecord,
  BellCoreRewardLift
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseMechanismActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseMechanismActor();

  void Configure(ELostsenseMechanismKind InKind, bool bInInitiallyRaised = true);

  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Mechanism")
  bool IsRaised() const;

private:
  void ApplyCounterweightState();

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseMechanismKind Kind = ELostsenseMechanismKind::Counterweight;

  bool bConfigured = false;
  bool bRaised = true;
  bool bConsumed = false;
  FVector RaisedLocation = FVector::ZeroVector;
};
