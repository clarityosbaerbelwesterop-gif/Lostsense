#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseActTwoStoryActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseActTwoInteraction : uint8 {
  BlackSapTrace,
  DomaIre,
  InfectedVillager,
  GiltfenRootTunnel,
  ThornChoirThreshold
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseActTwoStoryActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseActTwoStoryActor();
  void Configure(ELostsenseActTwoInteraction InInteraction);

  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

protected:
  virtual void Tick(float DeltaSeconds) override;

private:
  bool IsCompleted() const;
  void SynchronizePresentation();

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseActTwoInteraction Interaction =
      ELostsenseActTwoInteraction::BlackSapTrace;

  bool bConfigured = false;
  FVector RestLocation = FVector::ZeroVector;
};
