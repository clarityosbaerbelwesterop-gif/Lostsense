#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "LostsenseDeepWorldSubsystem.generated.h"

UCLASS()
class LOSTSENSEGAME_API ULostsenseDeepWorldSubsystem final
    : public UWorldSubsystem {
  GENERATED_BODY()

public:
  virtual void OnWorldBeginPlay(UWorld &InWorld) override;
};
