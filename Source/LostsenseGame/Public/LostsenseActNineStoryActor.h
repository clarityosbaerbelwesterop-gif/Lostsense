#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"
#include "LostsenseStorySubsystem.h"

#include "LostsenseActNineStoryActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOSTSENSEGAME_API ALostsenseActNineStoryActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseActNineStoryActor();
  void Configure(int32 InQuestId,
                 ELostsenseCampaignEnding InEnding =
                     ELostsenseCampaignEnding::None);
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
  int32 QuestId = 80080;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseCampaignEnding Ending = ELostsenseCampaignEnding::None;

  bool bConfigured = false;
  FVector RestScale = FVector::OneVector;
};
