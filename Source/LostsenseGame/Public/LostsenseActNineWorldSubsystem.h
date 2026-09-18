#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "LostsenseActNineWorldSubsystem.generated.h"

UCLASS()
class LOSTSENSEGAME_API ULostsenseActNineWorldSubsystem final
    : public UWorldSubsystem {
  GENERATED_BODY()

public:
  virtual void OnWorldBeginPlay(UWorld &InWorld) override;
};
