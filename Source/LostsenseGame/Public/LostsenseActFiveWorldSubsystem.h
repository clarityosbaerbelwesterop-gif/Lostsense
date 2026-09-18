#pragma once
#include "CoreMinimal.h"
#include "LostsenseActFiveWorldSubsystem.generated.h"
#include "Subsystems/WorldSubsystem.h"
UCLASS()
class LOSTSENSEGAME_API ULostsenseActFiveWorldSubsystem final
    : public UWorldSubsystem {
  GENERATED_BODY()
public:
  virtual void OnWorldBeginPlay(UWorld &InWorld) override;
};
