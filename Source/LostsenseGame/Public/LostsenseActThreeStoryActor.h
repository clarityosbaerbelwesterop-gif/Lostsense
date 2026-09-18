#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseActThreeStoryActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseActThreeInteraction : uint8 {
  SolenneEntry,
  RoyalLedgers,
  GildedCharterEvidence,
  PalaceArchive
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseActThreeStoryActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseActThreeStoryActor();
  void Configure(ELostsenseActThreeInteraction InInteraction);

  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

private:
  int32 QuestId() const;
  bool IsCompleted() const;
  void SynchronizePresentation();

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseActThreeInteraction Interaction =
      ELostsenseActThreeInteraction::SolenneEntry;

  bool bConfigured = false;
  FVector RestScale = FVector::OneVector;
};
