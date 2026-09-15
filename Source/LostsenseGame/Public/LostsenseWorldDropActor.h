#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lostsense/Gameplay/Items/LootRuntime.h"
#include "LostsenseWorldDropActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LOSTSENSEGAME_API ALostsenseWorldDropActor final : public AActor {
  GENERATED_BODY()

public:
  ALostsenseWorldDropActor();

  void InitializeDrop(const Lostsense::Gameplay::GeneratedLootEntry &InDrop);

protected:
  virtual void BeginPlay() override;

private:
  UFUNCTION()
  void OnPickupOverlap(UPrimitiveComponent *OverlappedComponent,
                       AActor *OtherActor, UPrimitiveComponent *OtherComponent,
                       int32 OtherBodyIndex, bool bFromSweep,
                       const FHitResult &SweepResult);

  UPROPERTY(VisibleAnywhere, Category = "Lostsense|Loot")
  TObjectPtr<USphereComponent> PickupSphere;

  UPROPERTY(VisibleAnywhere, Category = "Lostsense|Loot")
  TObjectPtr<UStaticMeshComponent> Visual;

  TUniquePtr<Lostsense::Gameplay::GeneratedLootEntry> Drop;
};
