#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lostsense/Gameplay/Items/LootRuntime.h"
#include "LostsenseInteractable.h"

#include "LostsenseWorldDropActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LOSTSENSEGAME_API ALostsenseWorldDropActor final
    : public AActor,
      public ILostsenseInteractable {
  GENERATED_BODY()

public:
  ALostsenseWorldDropActor();

  void InitializeDrop(const Lostsense::Gameplay::GeneratedLootEntry &InDrop);

  virtual FText GetInteractionPrompt() const override;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const override;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) override;

private:
  UPROPERTY(VisibleAnywhere, Category = "Lostsense|Loot")
  TObjectPtr<USphereComponent> PickupSphere;

  UPROPERTY(VisibleAnywhere, Category = "Lostsense|Loot")
  TObjectPtr<UStaticMeshComponent> Visual;

  TUniquePtr<Lostsense::Gameplay::GeneratedLootEntry> Drop;
};
