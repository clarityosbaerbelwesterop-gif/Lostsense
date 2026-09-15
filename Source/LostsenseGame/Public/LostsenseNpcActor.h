#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseInteractable.h"

#include "LostsenseNpcActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseNpcIdentity : uint8 {
  MaraVenn,
  HadrunPike,
  TamsinCoil
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseNpcActor final : public AActor,
                                                   public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseNpcActor();

  void Configure(ELostsenseNpcIdentity identity);

  virtual FText GetInteractionPrompt() const override;
  virtual bool CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

  UFUNCTION(BlueprintPure, Category = "Lostsense|NPC")
  ELostsenseNpcIdentity GetIdentity() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|NPC")
  FText GetDisplayName() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|NPC")
  FText GetCurrentLine() const;

private:
  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseNpcIdentity Identity = ELostsenseNpcIdentity::MaraVenn;

  UPROPERTY(VisibleInstanceOnly)
  int32 DialogueIndex = 0;

  bool bConfigured = false;
};
