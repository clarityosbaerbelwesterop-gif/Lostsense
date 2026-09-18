#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseActFourStoryActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseActFourInteraction : uint8 {
  CharterDepthMarker,
  CathedralVentilationManifold,
  KingsBoreAlignment
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseActFourStoryActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseActFourStoryActor();
  void Configure(ELostsenseActFourInteraction InInteraction);

  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

protected:
  virtual void Tick(float DeltaSeconds) override;

private:
  int32 QuestId() const;
  bool IsCompleted() const;
  void SynchronizePresentation();

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseActFourInteraction Interaction =
      ELostsenseActFourInteraction::CharterDepthMarker;

  bool bConfigured = false;
  FVector RestScale = FVector::OneVector;
};
