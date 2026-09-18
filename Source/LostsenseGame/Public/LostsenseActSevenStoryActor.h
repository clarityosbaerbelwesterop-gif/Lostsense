#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"
#include "LostsenseStorySubsystem.h"

#include "LostsenseActSevenStoryActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOSTSENSEGAME_API ALostsenseActSevenStoryActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseActSevenStoryActor();
  void Configure(int32 InQuestId,
                 ELostsenseActSevenArchiveChoice InArchiveChoice =
                     ELostsenseActSevenArchiveChoice::None);
  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

protected:
  virtual void Tick(float DeltaSeconds) override;

private:
  void SynchronizePresentation();

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  int32 QuestId = 80060;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseActSevenArchiveChoice ArchiveChoice =
      ELostsenseActSevenArchiveChoice::None;

  bool bConfigured = false;
  FVector RestScale = FVector::OneVector;
};
