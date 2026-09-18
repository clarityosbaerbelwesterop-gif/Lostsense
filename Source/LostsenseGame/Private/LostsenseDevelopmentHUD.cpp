#include "LostsenseDevelopmentHUD.h"

#include "LostsenseEnemyCharacter.h"
#include "LostsenseGameUserSettings.h"
#include "LostsenseMenuProjection.h"
#include "LostsensePlayerController.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseStorySubsystem.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "Lostsense/Gameplay/Story/CampaignCatalog.h"

namespace {
FString ObjectiveText(const int32 ObjectiveId) {
  const auto Quest = Lostsense::Gameplay::CampaignCatalog::FindQuest(
      static_cast<std::uint32_t>(FMath::Max(0, ObjectiveId)));
  return Quest.has_value()
             ? FString::Printf(TEXT("%d // %s // %s"), ObjectiveId,
                               UTF8_TO_TCHAR(Quest->Title.data()),
                               UTF8_TO_TCHAR(Quest->Objective.data()))
             : TEXT("No active campaign objective");
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

FString ObjectiveStateText(const ELostsenseObjectiveState State) {
  switch (State) {
  case ELostsenseObjectiveState::Active:
    return TEXT("ACTIVE");
  case ELostsenseObjectiveState::Completed:
    return TEXT("DONE");
  case ELostsenseObjectiveState::Locked:
    return TEXT("LOCKED");
  }
  return TEXT("LOCKED");
}

void DrawSelection(AHUD &Hud, const bool bSelected, const FString &Text,
                   const float X, const float Y, const float Width) {
  if (bSelected) {
    Hud.DrawRect(FLinearColor(0.18F, 0.18F, 0.20F, 0.98F), X - 10.0F,
                 Y - 5.0F, Width, 30.0F);
  }
  Hud.DrawText(FString::Printf(TEXT("%s%s"),
                               bSelected ? TEXT("> ") : TEXT("  "), *Text),
               bSelected ? FLinearColor::White
                         : FLinearColor(0.74F, 0.74F, 0.77F),
               X, Y, nullptr, 0.78F, false);
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

  const ALostsensePlayerController *Controller =
      Cast<ALostsensePlayerController>(GetOwningPlayerController());
  const ELostsenseMenuPage Page =
      Controller != nullptr ? Controller->GetActiveMenuPage()
                            : ELostsenseMenuPage::None;

  if (Page != ELostsenseMenuPage::Start) {
    DrawText(TEXT("LOSTSENSE // AVARRA"), FLinearColor::White, 32.0F, 24.0F,
             nullptr, 1.0F, false);
    DrawMeter(TEXT("VITALITY"), Runtime->GetPlayerHealth(),
              Runtime->GetPlayerMaximumHealth(), 32.0F, 58.0F, 330.0F);
    DrawMeter(TEXT("RESOLVE"), Runtime->GetPlayerResource(),
              Runtime->GetPlayerMaximumResource(), 32.0F, 102.0F, 330.0F);

    const int32 ObjectiveId =
        Story != nullptr ? Story->GetCurrentObjectiveId() : 0;
    DrawText(ObjectiveText(ObjectiveId), FLinearColor::White, 32.0F, 150.0F,
             nullptr, 0.76F, false);
    DrawText(
        TEXT("E Interact  I Inventory  Tab Scar Atlas  J Quests  M Map  P Pause"),
        FLinearColor(0.78F, 0.78F, 0.80F), 32.0F, 180.0F, nullptr, 0.70F,
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
      DrawText(
          FString::Printf(TEXT("BOSS // PHASE %d"), Boss->GetBossPhase()),
          FLinearColor::White, X, Y - 26.0F, nullptr, 0.9F, false);
      DrawMeter(TEXT("BOSS"), Boss->GetCurrentHealth(),
                Boss->GetMaximumHealth(), X, Y, Width);
    }
  }

  if (Controller == nullptr || Page == ELostsenseMenuPage::None) {
    return;
  }

  if (Page == ELostsenseMenuPage::Start) {
    DrawRect(FLinearColor(0.015F, 0.015F, 0.02F, 1.0F), 0.0F, 0.0F,
             Canvas->ClipX, Canvas->ClipY);
  }

  const float PanelWidth = FMath::Min(760.0F, Canvas->ClipX - 64.0F);
  const float PanelHeight = FMath::Min(700.0F, Canvas->ClipY - 80.0F);
  const float PanelX = (Canvas->ClipX - PanelWidth) * 0.5F;
  const float PanelY = (Canvas->ClipY - PanelHeight) * 0.5F;
  DrawRect(FLinearColor(0.025F, 0.025F, 0.03F, 0.97F), PanelX, PanelY,
           PanelWidth, PanelHeight);
  DrawRect(FLinearColor(0.32F, 0.32F, 0.35F, 0.96F), PanelX, PanelY, 5.0F,
           PanelHeight);

  const int32 Selection = Controller->GetMenuSelectionIndex();
  float RowY = PanelY + 96.0F;
  constexpr float RowHeight = 38.0F;
  constexpr int32 MaximumRows = 13;

  if (Page == ELostsenseMenuPage::Start) {
    DrawText(TEXT("LOSTSENSE"), FLinearColor::White, PanelX + 34.0F,
             PanelY + 22.0F, nullptr, 1.55F, false);
    DrawText(TEXT("A world remembers what people cannot."),
             FLinearColor(0.62F, 0.62F, 0.66F), PanelX + 36.0F, PanelY + 58.0F,
             nullptr, 0.74F, false);
    const FString Entries[] = {TEXT("Continue"), TEXT("New Game"),
                               TEXT("Options"), TEXT("Quit")};
    for (int32 Index = 0; Index < 4; ++Index) {
      DrawSelection(*this, Selection == Index, Entries[Index], PanelX + 44.0F,
                    RowY, PanelWidth - 88.0F);
      RowY += RowHeight;
    }
    DrawText(TEXT("UP/DOWN Navigate   ENTER Confirm"),
             FLinearColor(0.65F, 0.65F, 0.68F), PanelX + 36.0F,
             PanelY + PanelHeight - 40.0F, nullptr, 0.68F, false);
    return;
  }

  if (Page == ELostsenseMenuPage::Pause) {
    DrawText(TEXT("PAUSED"), FLinearColor::White, PanelX + 34.0F,
             PanelY + 24.0F, nullptr, 1.20F, false);
    const FString Entries[] = {
        TEXT("Resume"), TEXT("Save Game"), TEXT("Load Game"),
        TEXT("Quest Journal"), TEXT("World Map"), TEXT("Options"),
        TEXT("Return to Title")};
    for (int32 Index = 0; Index < 7; ++Index) {
      DrawSelection(*this, Selection == Index, Entries[Index], PanelX + 44.0F,
                    RowY, PanelWidth - 88.0F);
      RowY += RowHeight;
    }
    return;
  }

  if (Page == ELostsenseMenuPage::Options) {
    DrawText(TEXT("OPTIONS"), FLinearColor::White, PanelX + 34.0F,
             PanelY + 24.0F, nullptr, 1.20F, false);
    ULostsenseGameUserSettings *Settings =
        GEngine != nullptr
            ? Cast<ULostsenseGameUserSettings>(GEngine->GetGameUserSettings())
            : nullptr;
    float ScaleNormalized = 1.0F;
    int32 ScaleValue = 100;
    int32 ScaleMin = 50;
    int32 ScaleMax = 100;
    if (Settings != nullptr) {
      Settings->GetResolutionScaleInformationEx(ScaleNormalized, ScaleValue,
                                                ScaleMin, ScaleMax);
    }
    const int32 FrameRate =
        Settings != nullptr ? Settings->GetFrameRatePreset() : 60;
    const FString Values[] = {
        FString::Printf(TEXT("VSync: %s"),
                        Settings != nullptr && Settings->IsVSyncEnabled()
                            ? TEXT("On")
                            : TEXT("Off")),
        FString::Printf(TEXT("Frame Rate Limit: %s"),
                        FrameRate == 0
                            ? TEXT("Unlimited")
                            : *FString::Printf(TEXT("%d"), FrameRate)),
        FString::Printf(TEXT("Resolution Scale: %d%%"), ScaleValue),
        FString::Printf(TEXT("Camera Shake: %s"),
                        Settings != nullptr &&
                                Settings->GetCameraShakeEnabled()
                            ? TEXT("On")
                            : TEXT("Off")),
        FString::Printf(TEXT("Damage Numbers: %s"),
                        Settings != nullptr &&
                                Settings->GetDamageNumbersEnabled()
                            ? TEXT("On")
                            : TEXT("Off")),
        TEXT("Apply & Save"), TEXT("Back")};
    for (int32 Index = 0; Index < 7; ++Index) {
      DrawSelection(*this, Selection == Index, Values[Index], PanelX + 44.0F,
                    RowY, PanelWidth - 88.0F);
      RowY += RowHeight;
    }
    DrawText(TEXT("LEFT/RIGHT Change   ENTER Apply/Toggle   ESC Back"),
             FLinearColor(0.65F, 0.65F, 0.68F), PanelX + 36.0F,
             PanelY + PanelHeight - 40.0F, nullptr, 0.68F, false);
    return;
  }

  if (Page == ELostsenseMenuPage::QuestJournal) {
    DrawText(TEXT("QUEST JOURNAL // MAIN CAMPAIGN"), FLinearColor::White,
             PanelX + 34.0F, PanelY + 24.0F, nullptr, 1.05F, false);
    const auto &Quests = Lostsense::Gameplay::CampaignCatalog::Quests();
    const int32 StartIndex =
        FMath::Clamp(Selection - 6, 0,
                     FMath::Max(0, static_cast<int32>(Quests.size()) -
                                       MaximumRows));
    const int32 EndIndex =
        FMath::Min(static_cast<int32>(Quests.size()), StartIndex + MaximumRows);
    for (int32 Index = StartIndex; Index < EndIndex; ++Index) {
      const auto &Quest = Quests[static_cast<std::size_t>(Index)];
      const ELostsenseObjectiveState State =
          Story != nullptr
              ? Story->GetObjectiveState(static_cast<int32>(Quest.Id))
              : ELostsenseObjectiveState::Locked;
      const FString Label = FString::Printf(
          TEXT("[%s] %u // %s"), *ObjectiveStateText(State), Quest.Id,
          UTF8_TO_TCHAR(Quest.Title.data()));
      DrawSelection(*this, Selection == Index, Label, PanelX + 34.0F, RowY,
                    PanelWidth - 68.0F);
      if (Selection == Index) {
        DrawText(UTF8_TO_TCHAR(Quest.Objective.data()),
                 FLinearColor(0.60F, 0.60F, 0.64F), PanelX + 58.0F,
                 RowY + 21.0F, nullptr, 0.58F, false);
      }
      RowY += RowHeight;
    }
    DrawText(TEXT("UP/DOWN Browse canonical Acts I-IX   ESC Close"),
             FLinearColor(0.65F, 0.65F, 0.68F), PanelX + 36.0F,
             PanelY + PanelHeight - 40.0F, nullptr, 0.68F, false);
    return;
  }

  if (Page == ELostsenseMenuPage::WorldMap) {
    DrawText(TEXT("MAP // AVARRA"), FLinearColor::White, PanelX + 34.0F,
             PanelY + 24.0F, nullptr, 1.05F, false);
    const auto &Regions = Lostsense::Gameplay::CampaignCatalog::Regions();
    const int32 StartIndex =
        FMath::Clamp(Selection - 6, 0,
                     FMath::Max(0, static_cast<int32>(Regions.size()) -
                                       MaximumRows));
    const int32 EndIndex = FMath::Min(static_cast<int32>(Regions.size()),
                                     StartIndex + MaximumRows);
    for (int32 Index = StartIndex; Index < EndIndex; ++Index) {
      const auto &Region = Regions[static_cast<std::size_t>(Index)];
      const FString Label = FString::Printf(
          TEXT("%u // %s // %s"), Region.Id, UTF8_TO_TCHAR(Region.Name.data()),
          UTF8_TO_TCHAR(Region.Layer.data()));
      DrawSelection(*this, Selection == Index, Label, PanelX + 34.0F, RowY,
                    PanelWidth - 68.0F);
      RowY += RowHeight;
    }
    DrawText(TEXT("Surface -> Vaur -> Namarith -> Abyss -> Red Archive -> Loom"),
             FLinearColor(0.65F, 0.65F, 0.68F), PanelX + 36.0F,
             PanelY + PanelHeight - 40.0F, nullptr, 0.68F, false);
    return;
  }

  if (Page == ELostsenseMenuPage::Inventory) {
    DrawText(TEXT("INVENTORY // FIELD LOADOUT"), FLinearColor::White,
             PanelX + 34.0F, PanelY + 24.0F, nullptr, 1.05F, false);
    TArray<FLostsenseInventoryMenuEntry> Entries;
    if (FLostsenseMenuProjection::CaptureInventory(*Runtime, Entries)) {
      const int32 StartIndex = FMath::Clamp(
          Selection - 5, 0, FMath::Max(0, Entries.Num() - MaximumRows));
      const int32 EndIndex =
          FMath::Min(Entries.Num(), StartIndex + MaximumRows);
      for (int32 Index = StartIndex; Index < EndIndex; ++Index) {
        DrawSelection(*this, Selection == Index, Entries[Index].Label,
                      PanelX + 34.0F, RowY, PanelWidth - 68.0F);
        RowY += RowHeight;
      }
    }
    return;
  }

  DrawText(TEXT("SCAR ATLAS // KNIGHT 102"), FLinearColor::White,
           PanelX + 34.0F, PanelY + 24.0F, nullptr, 1.05F, false);
  DrawText(FString::Printf(TEXT("Unspent Scar Points: %d"),
                           Runtime->GetUnspentSkillPoints()),
           FLinearColor(0.68F, 0.68F, 0.70F), PanelX + 34.0F, PanelY + 54.0F,
           nullptr, 0.72F, false);

  TArray<FLostsenseScarMenuEntry> Entries;
  if (FLostsenseMenuProjection::CaptureScarAtlas(*Runtime, Entries)) {
    const int32 StartIndex = FMath::Clamp(
        Selection - 5, 0, FMath::Max(0, Entries.Num() - MaximumRows));
    const int32 EndIndex = FMath::Min(Entries.Num(), StartIndex + MaximumRows);
    for (int32 Index = StartIndex; Index < EndIndex; ++Index) {
      const FLostsenseScarMenuEntry &Entry = Entries[Index];
      const FString Label =
          FString::Printf(TEXT("[%s] %s // %s // COST %d"),
                          *ScarStateText(Entry), *Entry.Name, *Entry.Category,
                          Entry.PointCost);
      DrawSelection(*this, Selection == Index, Label, PanelX + 34.0F, RowY,
                    PanelWidth - 68.0F);
      RowY += RowHeight;
    }
  }
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
