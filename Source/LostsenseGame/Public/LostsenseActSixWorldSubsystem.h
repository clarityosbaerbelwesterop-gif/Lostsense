#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "LostsenseActSixWorldSubsystem.generated.h"

UCLASS()
class LOSTSENSEGAME_API ULostsenseActSixWorldSubsystem final
    : public UWorldSubsystem {
  GENERATED_BODY()

public:
  virtual void OnWorldBeginPlay(UWorld &InWorld) override;
};
