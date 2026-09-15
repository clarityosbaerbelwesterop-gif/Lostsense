#include "LostsenseDevelopmentHUD.h"

#include "LostsenseEnemyCharacter.h"
#include "LostsenseRuntimeSubsystem.h"

#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void ALostsenseDevelopmentHUD::DrawHUD() {
  Super::DrawHUD();

  if (Canvas == nullptr || GetWorld() == nullptr) {
    return;
  }

  UGameInstance *GameInstance = GetWorld()->GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr || !Runtime->IsPortableRuntimeReady()) {
    DrawText(TEXT("LOSTSENSE - portable runtime unavailable"), FLinearColor::Red,
             32.0F, 32.0F, nullptr, 1.0F, false);
    return;
  }

  DrawText(TEXT("LOSTSENSE // DEVELOPMENT COMBAT SLICE"), FLinearColor::White,
           32.0F, 24.0F, nullptr, 1.0F, false);
  DrawMeter(TEXT("VITALITY"), Runtime->GetPlayerHealth(),
            Runtime->GetPlayerMaximumHealth(), 32.0F, 58.0F, 330.0F);
  DrawMeter(TEXT("RESOLVE"), Runtime->GetPlayerResource(),
            Runtime->GetPlayerMaximumResource(), 32.0F, 102.0F, 330.0F);

  DrawText(FString::Printf(TEXT("SCAR POINTS  %d"),
                           Runtime->GetUnspentSkillPoints()),
           FLinearColor::White, 32.0F, 150.0F, nullptr, 0.9F, false);

  DrawText(TEXT("LMB Primary   RMB Heavy   Space Dodge   Shift Guard"),
           FLinearColor(0.82F, 0.82F, 0.82F), 32.0F, 184.0F, nullptr, 0.82F,
           false);
  DrawText(TEXT("Q/E/R/F Skills   K Scar Atlas   T Equip pickup"),
           FLinearColor(0.82F, 0.82F, 0.82F), 32.0F, 208.0F, nullptr, 0.82F,
           false);
  DrawText(TEXT("F5 Save   F9 Load   F10 Reset development encounter"),
           FLinearColor(0.82F, 0.82F, 0.82F), 32.0F, 232.0F, nullptr, 0.82F,
           false);

  ALostsenseEnemyCharacter *Boss = nullptr;
  for (TActorIterator<ALostsenseEnemyCharacter> It(GetWorld()); It; ++It) {
    if (It->IsBoss() && !It->IsDefeated()) {
      Boss = *It;
      break;
    }
  }

  if (Boss != nullptr) {
    const float Width = FMath::Min(620.0F, Canvas->ClipX - 120.0F);
    const float X = (Canvas->ClipX - Width) * 0.5F;
    const float Y = Canvas->ClipY - 96.0F;
    DrawText(FString::Printf(TEXT("ODRAN, THE BELL WITHOUT A TONGUE // PHASE %d"),
                             Boss->GetBossPhase()),
             FLinearColor::White, X, Y - 26.0F, nullptr, 0.9F, false);
    DrawMeter(TEXT("BOSS"), Boss->GetCurrentHealth(), Boss->GetMaximumHealth(),
              X, Y, Width);
  }
}

void ALostsenseDevelopmentHUD::DrawMeter(const FString &Label,
                                         const double Current,
                                         const double Maximum, const float X,
                                         const float Y,
                                         const float Width) const {
  const double SafeMaximum = FMath::Max(1.0, Maximum);
  const float Fraction = static_cast<float>(
      FMath::Clamp(Current / SafeMaximum, 0.0, 1.0));

  DrawRect(FLinearColor(0.06F, 0.06F, 0.06F, 0.88F), X, Y, Width, 22.0F);
  DrawRect(FLinearColor(0.72F, 0.72F, 0.72F, 0.95F), X + 2.0F, Y + 2.0F,
           (Width - 4.0F) * Fraction, 18.0F);
  DrawText(FString::Printf(TEXT("%s  %.0f / %.0f"), *Label, Current, Maximum),
           FLinearColor::White, X + 6.0F, Y + 2.0F, nullptr, 0.72F, false);
}
