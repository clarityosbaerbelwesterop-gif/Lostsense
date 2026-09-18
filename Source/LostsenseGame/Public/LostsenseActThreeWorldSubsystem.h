#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "LostsenseActThreeWorldSubsystem.generated.h"

UCLASS()
class LOSTSENSEGAME_API ULostsenseActThreeWorldSubsystem final
    : public UWorldSubsystem {
  GENERATED_BODY()

public:
  virtual void OnWorldBeginPlay(UWorld &InWorld) override;
};
