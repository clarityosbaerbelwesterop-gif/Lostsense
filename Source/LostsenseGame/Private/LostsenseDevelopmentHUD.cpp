#include "LostsenseDevelopmentHUD.h"

#include "LostsenseEnemyCharacter.h"
#include "LostsenseMenuProjection.h"
#include "LostsensePlayerController.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseStorySubsystem.h"

#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace {
FString ObjectiveText(const int32 ObjectiveId) {
  switch (ObjectiveId) {
  case 80001:
    return TEXT("Find Mara Venn at the bellsmith");
  case 80002:
    return TEXT("Meet Hadrun and prove your measure in the guard yard");
  case 80003:
    return TEXT("Follow Ravelwood into Weeping Cut; find Ninth Descent marks");
  case 80004:
    return TEXT("Descend into the Upper Vaur Goldworks");
  case 80005:
    return TEXT("Enter Coinless Shaft and follow the silent rail gallery");
  case 80006:
    return TEXT(
        "Recover the Ninth Descent record and return Bellgrave changed");
  default:
    return TEXT("Orient yourself beneath Bellgrave's stabilization bell");
  }
}

FString ScarStateText(const FLostsenseScarMenuEntry &Entry) {
  if (Entry.bAllocated) {
    return TEXT("ALLOCATED");
  }
  if (Entry.bBlocked) {
    return TEXT("OATH LOCKED");
  }
  if (Entry.bAvailable) {
    return TEXT("AVAILABLE");
  }
  return TEXT("LOCKED");
}
} // namespace

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
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Runtime == nullptr || !Runtime->IsPortableRuntimeReady()) {
    DrawText(TEXT("LOSTSENSE - portable runtime unavailable"),
             FLinearColor::Red, 32.0F, 32.0F, nullptr, 1.0F, false);
    return;
  }

  ConsumePresentationEvents();

  DrawText(TEXT("LOSTSENSE // BELLGRAVE DESCENT SLICE"), FLinearColor::White,
           32.0F, 24.0F, nullptr, 1.0F, false);
  DrawMeter(TEXT("VITALITY"), Runtime->GetPlayerHealth(),
            Runtime->GetPlayerMaximumHealth(), 32.0F, 58.0F, 330.0F);
  DrawMeter(TEXT("RESOLVE"), Runtime->GetPlayerResource(),
            Runtime->GetPlayerMaximumResource(), 32.0F, 102.0F, 330.0F);

  DrawText(FString::Printf(TEXT("SCAR POINTS  %d"),
                           Runtime->GetUnspentSkillPoints()),
           FLinearColor::White, 32.0F, 150.0F, nullptr, 0.9F, false);

  const int32 ObjectiveId =
      Story != nullptr ? Story->GetCurrentObjectiveId() : 0;
  DrawText(FString::Printf(TEXT("OBJECTIVE  %s"), *ObjectiveText(ObjectiveId)),
           FLinearColor::White, 32.0F, 182.0F, nullptr, 0.82F, false);
  DrawText(
      TEXT("E Interact   LMB Primary   RMB Heavy   Space Dodge   Shift Guard"),
      FLinearColor(0.82F, 0.82F, 0.82F), 32.0F, 212.0F, nullptr, 0.78F, false);
  DrawText(TEXT("Q/2/R/F Skills   I Inventory   Tab Scar Atlas   T Quick Equip"),
           FLinearColor(0.82F, 0.82F, 0.82F), 32.0F, 236.0F, nullptr, 0.78F,
           false);
  DrawText(TEXT("F5 Save   F9 Load   F10 Reset slice"),
           FLinearColor(0.72F, 0.72F, 0.72F), 32.0F, 260.0F, nullptr, 0.72F,
           false);

  float EventY = 302.0F;
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
    DrawText(
        FString::Printf(TEXT("ODRAN, THE BELL WITHOUT A TONGUE // PHASE %d"),
                        Boss->GetBossPhase()),
        FLinearColor::White, X, Y - 26.0F, nullptr, 0.9F, false);
    DrawMeter(TEXT("BOSS"), Boss->GetCurrentHealth(), Boss->GetMaximumHealth(),
              X, Y, Width);
  }

  const ALostsensePlayerController *Controller =
      Cast<ALostsensePlayerController>(GetOwningPlayerController());
  if (Controller == nullptr || !Controller->IsGameplayInputSuppressed()) {
    return;
  }

  const float PanelWidth = FMath::Min(610.0F, Canvas->ClipX - 64.0F);
  const float PanelHeight = FMath::Min(650.0F, Canvas->ClipY - 96.0F);
  const float PanelX = FMath::Max(32.0F, Canvas->ClipX - PanelWidth - 32.0F);
  const float PanelY = 48.0F;
  DrawRect(FLinearColor(0.025F, 0.025F, 0.03F, 0.96F), PanelX, PanelY,
           PanelWidth, PanelHeight);
  DrawRect(FLinearColor(0.28F, 0.28F, 0.30F, 0.96F), PanelX, PanelY, 5.0F,
           PanelHeight);

  const int32 Selection = Controller->GetMenuSelectionIndex();
  float RowY = PanelY + 82.0F;
  constexpr float RowHeight = 38.0F;
  constexpr int32 MaximumRows = 12;

  if (Controller->IsInventoryMenuOpen()) {
    DrawText(TEXT("INVENTORY // FIELD LOADOUT"), FLinearColor::White,
             PanelX + 24.0F, PanelY + 20.0F, nullptr, 1.05F, false);
    DrawText(TEXT("Recovered gear, equipped state and materials"),
             FLinearColor(0.68F, 0.68F, 0.70F), PanelX + 24.0F,
             PanelY + 48.0F, nullptr, 0.72F, false);

    TArray<FLostsenseInventoryMenuEntry> Entries;
    if (FLostsenseMenuProjection::CaptureInventory(*Runtime, Entries)) {
      const int32 StartIndex = FMath::Clamp(
          Selection - 5, 0, FMath::Max(0, Entries.Num() - MaximumRows));
      const int32 EndIndex = FMath::Min(Entries.Num(), StartIndex + MaximumRows);
      for (int32 Index = StartIndex; Index < EndIndex; ++Index) {
        const bool bSelected = Index == Selection;
        if (bSelected) {
          DrawRect(FLinearColor(0.18F, 0.18F, 0.20F, 0.98F), PanelX + 16.0F,
                   RowY - 5.0F, PanelWidth - 32.0F, RowHeight - 3.0F);
        }
        DrawText(FString::Printf(TEXT("%s%s"), bSelected ? TEXT("> ") : TEXT("  "),
                                 *Entries[Index].Label),
                 bSelected ? FLinearColor::White
                           : FLinearColor(0.76F, 0.76F, 0.78F),
                 PanelX + 26.0F, RowY, nullptr, 0.72F, false);
        RowY += RowHeight;
      }
    }
    DrawText(TEXT("UP/DOWN Browse   ENTER/E Equip compatible gear   ESC Close"),
             FLinearColor(0.72F, 0.72F, 0.74F), PanelX + 24.0F,
             PanelY + PanelHeight - 34.0F, nullptr, 0.68F, false);
    return;
  }

  DrawText(TEXT("SCAR ATLAS // KNIGHT 102"), FLinearColor::White,
           PanelX + 24.0F, PanelY + 20.0F, nullptr, 1.05F, false);
  DrawText(FString::Printf(TEXT("Unspent Scar Points: %d"),
                           Runtime->GetUnspentSkillPoints()),
           FLinearColor(0.68F, 0.68F, 0.70F), PanelX + 24.0F,
           PanelY + 48.0F, nullptr, 0.72F, false);

  TArray<FLostsenseScarMenuEntry> Entries;
  if (FLostsenseMenuProjection::CaptureScarAtlas(*Runtime, Entries)) {
    const int32 StartIndex = FMath::Clamp(
        Selection - 5, 0, FMath::Max(0, Entries.Num() - MaximumRows));
    const int32 EndIndex = FMath::Min(Entries.Num(), StartIndex + MaximumRows);
    for (int32 Index = StartIndex; Index < EndIndex; ++Index) {
      const FLostsenseScarMenuEntry &Entry = Entries[Index];
      const bool bSelected = Index == Selection;
      if (bSelected) {
        DrawRect(FLinearColor(0.18F, 0.18F, 0.20F, 0.98F), PanelX + 16.0F,
                 RowY - 5.0F, PanelWidth - 32.0F, RowHeight - 3.0F);
      }
      DrawText(
          FString::Printf(TEXT("%s[%s] %s // %s // COST %d"),
                          bSelected ? TEXT("> ") : TEXT("  "),
                          *ScarStateText(Entry), *Entry.Name, *Entry.Category,
                          Entry.PointCost),
          bSelected ? FLinearColor::White
                    : FLinearColor(0.76F, 0.76F, 0.78F),
          PanelX + 26.0F, RowY, nullptr, 0.68F, false);
      DrawText(Entry.Detail, FLinearColor(0.56F, 0.56F, 0.60F),
               PanelX + 46.0F, RowY + 17.0F, nullptr, 0.60F, false);
      RowY += RowHeight;
    }
  }
  DrawText(TEXT("UP/DOWN Navigate   ENTER/E Allocate   ESC Close"),
           FLinearColor(0.72F, 0.72F, 0.74F), PanelX + 24.0F,
           PanelY + PanelHeight - 34.0F, nullptr, 0.68F, false);
}

void ALostsenseDevelopmentHUD::DrawMeter(const FString &Label,
                                         const double Current,
                                         const double Maximum, const float X,
                                         const float Y, const float Width) {
  const double SafeMaximum = FMath::Max(1.0, Maximum);
  const float Fraction =
      static_cast<float>(FMath::Clamp(Current / SafeMaximum, 0.0, 1.0));

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
    return FString::Printf(TEXT("#%lld combatant %lld defeated"),
                           Event.Sequence, Event.TargetId);
  case ELostsensePresentationEventType::AbilityActivated:
    return FString::Printf(TEXT("#%lld ability %lld activated"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::ItemDropped:
    return FString::Printf(TEXT("#%lld item %lld dropped x%d"), Event.Sequence,
                           Event.ContentId, Event.Quantity);
  case ELostsensePresentationEventType::ItemPickedUp:
    return FString::Printf(TEXT("#%lld item %lld picked up x%d"),
                           Event.Sequence, Event.ContentId, Event.Quantity);
  case ELostsensePresentationEventType::ItemEquipped:
    return FString::Printf(TEXT("#%lld item %lld equipped"), Event.Sequence,
                           Event.ContentId);
  case ELostsensePresentationEventType::SkillAllocated:
    return FString::Printf(TEXT("#%lld scar node %lld allocated"),
                           Event.Sequence, Event.ContentId);
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
    return FString::Printf(TEXT("#%lld scar node %lld refunded"),
                           Event.Sequence, Event.ContentId);
  }

  return FString::Printf(TEXT("#%lld gameplay event"), Event.Sequence);
}
