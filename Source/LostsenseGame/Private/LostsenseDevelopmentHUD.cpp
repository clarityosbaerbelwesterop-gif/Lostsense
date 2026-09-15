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

  ConsumePresentationEvents();

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

  float EventY = 278.0F;
  DrawText(TEXT("AUTHORITATIVE EVENT FEED"), FLinearColor::White, 32.0F, EventY,
           nullptr, 0.72F, false);
  EventY += 22.0F;
  for (const FString &Line : RecentEvents) {
    DrawText(Line, FLinearColor(0.72F, 0.72F, 0.72F), 32.0F, EventY, nullptr,
             0.68F, false);
    EventY += 19.0F;
  }

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
                                         const float Y, const float Width) {
  const double SafeMaximum = FMath::Max(1.0, Maximum);
  const float Fraction = static_cast<float>(
      FMath::Clamp(Current / SafeMaximum, 0.0, 1.0));

  DrawRect(FLinearColor(0.06F, 0.06F, 0.06F, 0.88F), X, Y, Width, 22.0F);
  DrawRect(FLinearColor(0.72F, 0.72F, 0.72F, 0.95F), X + 2.0F, Y + 2.0F,
           (Width - 4.0F) * Fraction, 18.0F);
  DrawText(FString::Printf(TEXT("%s  %.0f / %.0f"), *Label, Current, Maximum),
           FLinearColor::White, X + 6.0F, Y + 2.0F, nullptr, 0.72F, false);
}

void ALostsenseDevelopmentHUD::ConsumePresentationEvents() {
  UGameInstance *GameInstance = GetWorld()->GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr) {
    return;
  }

  const TArray<FLostsensePresentationEvent> Events =
      Runtime->DrainPresentationEvents();
  for (const FLostsensePresentationEvent &Event : Events) {
    RecentEvents.Add(FormatPresentationEvent(Event));
  }

  constexpr int32 MaximumVisibleEvents = 6;
  if (RecentEvents.Num() > MaximumVisibleEvents) {
    RecentEvents.RemoveAt(0, RecentEvents.Num() - MaximumVisibleEvents, false);
  }
}

FString ALostsenseDevelopmentHUD::FormatPresentationEvent(
    const FLostsensePresentationEvent &Event) const {
  switch (Event.Type) {
  case ELostsensePresentationEventType::DamageApplied:
    return FString::Printf(TEXT("#%lld damage %.1f  %lld -> %lld"),
                           Event.Sequence, Event.Value, Event.SourceId,
                           Event.TargetId);
  case ELostsensePresentationEventType::CombatantDied:
    return FString::Printf(TEXT("#%lld combatant %lld defeated"), Event.Sequence,
                           Event.TargetId);
  case ELostsensePresentationEventType::AbilityActivated:
    return FString::Printf(TEXT("#%lld ability %lld activated"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::ItemDropped:
    return FString::Printf(TEXT("#%lld item %lld dropped x%d"), Event.Sequence,
                           Event.ContentId, Event.Quantity);
  case ELostsensePresentationEventType::ItemPickedUp:
    return FString::Printf(TEXT("#%lld item %lld picked up x%d"), Event.Sequence,
                           Event.ContentId, Event.Quantity);
  case ELostsensePresentationEventType::ItemEquipped:
    return FString::Printf(TEXT("#%lld item %lld equipped"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::SkillAllocated:
    return FString::Printf(TEXT("#%lld scar node %lld allocated"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::EffectApplied:
    return FString::Printf(TEXT("#%lld effect %lld applied"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::EffectRemoved:
    return FString::Printf(TEXT("#%lld effect %lld removed"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::ItemUnequipped:
    return FString::Printf(TEXT("#%lld item %lld unequipped"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::SkillRefunded:
    return FString::Printf(TEXT("#%lld scar node %lld refunded"), Event.Sequence,
                           Event.ContentId);
  }

  return FString::Printf(TEXT("#%lld gameplay event"), Event.Sequence);
}
