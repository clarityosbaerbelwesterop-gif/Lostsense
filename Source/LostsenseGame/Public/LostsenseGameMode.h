#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "LostsenseGameMode.generated.h"

UCLASS()
class LOSTSENSEGAME_API ALostsenseGameMode final : public AGameModeBase {
  GENERATED_BODY()

public:
  ALostsenseGameMode();
  virtual void StartPlay() override;

private:
  void EnsurePlayableKnight();
  void SpawnVerticalSliceWorld();
};
