#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LostsenseDevelopmentHUD.generated.h"

UCLASS()
class LOSTSENSEGAME_API ALostsenseDevelopmentHUD final : public AHUD {
  GENERATED_BODY()

public:
  virtual void DrawHUD() override;

private:
  void DrawMeter(const FString &Label, double Current, double Maximum, float X,
                 float Y, float Width);
};
