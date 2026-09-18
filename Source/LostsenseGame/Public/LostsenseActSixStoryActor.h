#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseActSixStoryActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOSTSENSEGAME_API ALostsenseActSixStoryActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseActSixStoryActor();
  void Configure(int32 InQuestId);
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
  int32 QuestId = 80050;

  bool bConfigured = false;
  FVector RestScale = FVector::OneVector;
};
