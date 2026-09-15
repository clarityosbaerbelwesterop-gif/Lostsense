#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostsenseStorySubsystem.h"

#include "LostsenseStoryGateActor.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

UCLASS()
class LOSTSENSEGAME_API ALostsenseStoryGateActor final : public AActor {
  GENERATED_BODY()

public:
  ALostsenseStoryGateActor();

  void Configure(ELostsenseStoryBeat InBeat,
                 ELostsenseStoryBeat InRequiredBeat,
                 bool bInRequiresBeat);

protected:
  virtual void BeginPlay() override;

private:
  UFUNCTION()
  void OnGateOverlap(UPrimitiveComponent *OverlappedComponent,
                     AActor *OtherActor, UPrimitiveComponent *OtherComponent,
                     int32 OtherBodyIndex, bool bFromSweep,
                     const FHitResult &SweepResult);

  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UBoxComponent> Trigger;

  ELostsenseStoryBeat Beat = ELostsenseStoryBeat::EnteredRavelwood;
  ELostsenseStoryBeat RequiredBeat =
      ELostsenseStoryBeat::BellgraveDepartureAllowed;
  bool bRequiresBeat = true;
  bool bCommitted = false;
};
