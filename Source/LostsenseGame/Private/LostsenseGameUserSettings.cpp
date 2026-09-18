#include "LostsenseGameUserSettings.h"

void ULostsenseGameUserSettings::SetFrameRatePreset(const int32 Preset) {
  if (Preset <= 30) {
    FrameRatePreset = 30;
  } else if (Preset <= 60) {
    FrameRatePreset = 60;
  } else if (Preset <= 120) {
    FrameRatePreset = 120;
  } else {
    FrameRatePreset = 0;
  }
  SetFrameRateLimit(FrameRatePreset > 0 ? static_cast<float>(FrameRatePreset)
                                        : 0.0F);
}

void ULostsenseGameUserSettings::SetCameraShakeEnabled(const bool bEnabled) {
  bCameraShakeEnabled = bEnabled;
}

void ULostsenseGameUserSettings::SetDamageNumbersEnabled(const bool bEnabled) {
  bDamageNumbersEnabled = bEnabled;
}

void ULostsenseGameUserSettings::ApplyAndPersist() {
  ApplySettings(false);
  SaveSettings();
}
