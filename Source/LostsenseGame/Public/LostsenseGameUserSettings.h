#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "LostsenseGameUserSettings.generated.h"

UCLASS(config = GameUserSettings)
class LOSTSENSEGAME_API ULostsenseGameUserSettings final
    : public UGameUserSettings {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintPure, Category = "Lostsense|Settings")
  int32 GetFrameRatePreset() const { return FrameRatePreset; }

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Settings")
  void SetFrameRatePreset(int32 Preset);

  UFUNCTION(BlueprintPure, Category = "Lostsense|Settings")
  bool GetCameraShakeEnabled() const { return bCameraShakeEnabled; }

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Settings")
  void SetCameraShakeEnabled(bool bEnabled);

  UFUNCTION(BlueprintPure, Category = "Lostsense|Settings")
  bool GetDamageNumbersEnabled() const { return bDamageNumbersEnabled; }

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Settings")
  void SetDamageNumbersEnabled(bool bEnabled);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Settings")
  void ApplyAndPersist();

private:
  UPROPERTY(Config)
  int32 FrameRatePreset = 60;

  UPROPERTY(Config)
  bool bCameraShakeEnabled = true;

  UPROPERTY(Config)
  bool bDamageNumbersEnabled = true;
};
