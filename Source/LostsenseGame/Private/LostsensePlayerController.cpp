#include "LostsensePlayerController.h"

#include "LostsenseGameUserSettings.h"
#include "LostsenseInputBridgeLibrary.h"
#include "LostsenseInteractable.h"
#include "LostsenseKnightCharacter.h"
#include "LostsenseMenuProjection.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseStorySubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"
#include "Lostsense/Gameplay/Story/CampaignCatalog.h"

ALostsensePlayerController::ALostsensePlayerController() {
  bShowMouseCursor = false;
  bEnableClickEvents = false;
  bEnableMouseOverEvents = false;
}

void ALostsensePlayerController::BeginPlay() {
  Super::BeginPlay();

  UGameInstance *GameInstance = GetGameInstance();
  if (GameInstance == nullptr) {
    return;
  }

  if (ULostsenseRuntimeSubsystem *Runtime =
          GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()) {
    static_cast<void>(Runtime->SaveCharacterToText(StartupCharacterSnapshot));
  }
  if (ULostsenseStorySubsystem *Story =
          GameInstance->GetSubsystem<ULostsenseStorySubsystem>()) {
    static_cast<void>(Story->SaveStoryToText(StartupStorySnapshot));
    static_cast<void>(
        Story->CompleteBeat(ELostsenseStoryBeat::ReturnedAwakened));
  }

  OpenMenu(ELostsenseMenuPage::Start);
}

void ALostsensePlayerController::SetupInputComponent() {
  Super::SetupInputComponent();
  if (InputComponent == nullptr) {
    return;
  }

  InputComponent->BindKey(EKeys::E, IE_Pressed, this,
                          &ALostsensePlayerController::InputInteract);
  InputComponent->BindKey(EKeys::I, IE_Pressed, this,
                          &ALostsensePlayerController::ToggleInventoryMenu);
  InputComponent->BindKey(EKeys::Tab, IE_Pressed, this,
                          &ALostsensePlayerController::ToggleScarAtlasMenu);
  InputComponent->BindKey(EKeys::J, IE_Pressed, this,
                          &ALostsensePlayerController::ToggleQuestJournal);
  InputComponent->BindKey(EKeys::M, IE_Pressed, this,
                          &ALostsensePlayerController::ToggleWorldMap);
  InputComponent->BindKey(EKeys::P, IE_Pressed, this,
                          &ALostsensePlayerController::TogglePauseMenu);
  InputComponent->BindKey(EKeys::Up, IE_Pressed, this,
                          &ALostsensePlayerController::MenuSelectPrevious);
  InputComponent->BindKey(EKeys::Down, IE_Pressed, this,
                          &ALostsensePlayerController::MenuSelectNext);
  InputComponent->BindKey(EKeys::Left, IE_Pressed, this,
                          &ALostsensePlayerController::MenuAdjustLeft);
  InputComponent->BindKey(EKeys::Right, IE_Pressed, this,
                          &ALostsensePlayerController::MenuAdjustRight);
  InputComponent->BindKey(EKeys::Enter, IE_Pressed, this,
                          &ALostsensePlayerController::MenuConfirm);
  InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
                          &ALostsensePlayerController::MenuCancel);
  InputComponent->BindKey(
      EKeys::F5, IE_Pressed, this,
      &ALostsensePlayerController::SaveDevelopmentCharacter);
  InputComponent->BindKey(
      EKeys::F9, IE_Pressed, this,
      &ALostsensePlayerController::LoadDevelopmentCharacter);
  InputComponent->BindKey(
      EKeys::F10, IE_Pressed, this,
      &ALostsensePlayerController::ResetDevelopmentEncounter);
}

void ALostsensePlayerController::InputInteract() {
  if (IsGameplayInputSuppressed()) {
    MenuConfirm();
    return;
  }
  TryInteract();
}

void ALostsensePlayerController::OpenMenu(const ELostsenseMenuPage Page) {
  ActiveMenuPage = Page;
  MenuSelectionIndex = 0;
  NormalizeMenuSelection();
  ApplyMenuInputMode();
}

void ALostsensePlayerController::ToggleInventoryMenu() {
  OpenMenu(ActiveMenuPage == ELostsenseMenuPage::Inventory
               ? ELostsenseMenuPage::None
               : ELostsenseMenuPage::Inventory);
}

void ALostsensePlayerController::ToggleScarAtlasMenu() {
  OpenMenu(ActiveMenuPage == ELostsenseMenuPage::ScarAtlas
               ? ELostsenseMenuPage::None
               : ELostsenseMenuPage::ScarAtlas);
}

void ALostsensePlayerController::ToggleQuestJournal() {
  OpenMenu(ActiveMenuPage == ELostsenseMenuPage::QuestJournal
               ? ELostsenseMenuPage::None
               : ELostsenseMenuPage::QuestJournal);
}

void ALostsensePlayerController::ToggleWorldMap() {
  OpenMenu(ActiveMenuPage == ELostsenseMenuPage::WorldMap
               ? ELostsenseMenuPage::None
               : ELostsenseMenuPage::WorldMap);
}

void ALostsensePlayerController::TogglePauseMenu() {
  if (ActiveMenuPage == ELostsenseMenuPage::Start) {
    return;
  }
  OpenMenu(ActiveMenuPage == ELostsenseMenuPage::Pause
               ? ELostsenseMenuPage::None
               : ELostsenseMenuPage::Pause);
}

void ALostsensePlayerController::MenuSelectPrevious() {
  if (!IsGameplayInputSuppressed()) {
    return;
  }
  --MenuSelectionIndex;
  NormalizeMenuSelection();
}

void ALostsensePlayerController::MenuSelectNext() {
  if (!IsGameplayInputSuppressed()) {
    return;
  }
  ++MenuSelectionIndex;
  NormalizeMenuSelection();
}

void ALostsensePlayerController::MenuAdjustLeft() {
  if (ActiveMenuPage == ELostsenseMenuPage::Options) {
    AdjustOption(-1);
  }
}

void ALostsensePlayerController::MenuAdjustRight() {
  if (ActiveMenuPage == ELostsenseMenuPage::Options) {
    AdjustOption(1);
  }
}

void ALostsensePlayerController::AdjustOption(const int32 Direction) {
  if (GEngine == nullptr || Direction == 0) {
    return;
  }
  ULostsenseGameUserSettings *Settings =
      Cast<ULostsenseGameUserSettings>(GEngine->GetGameUserSettings());
  if (Settings == nullptr) {
    return;
  }

  switch (MenuSelectionIndex) {
  case 0:
    Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled());
    break;
  case 1: {
    constexpr int32 Presets[] = {30, 60, 120, 0};
    int32 Index = 0;
    for (int32 Candidate = 0; Candidate < 4; ++Candidate) {
      if (Presets[Candidate] == Settings->GetFrameRatePreset()) {
        Index = Candidate;
        break;
      }
    }
    Index = (Index + Direction + 4) % 4;
    Settings->SetFrameRatePreset(Presets[Index]);
    break;
  }
  case 2: {
    float Current = 1.0F;
    int32 CurrentValue = 100;
    int32 MinimumValue = 50;
    int32 MaximumValue = 100;
    Settings->GetResolutionScaleInformationEx(Current, CurrentValue,
                                              MinimumValue, MaximumValue);
    const float Next = FMath::Clamp(
        Current + static_cast<float>(Direction) * 0.1F, 0.5F, 1.0F);
    Settings->SetResolutionScaleNormalized(Next);
    break;
  }
  case 3:
    Settings->SetCameraShakeEnabled(!Settings->GetCameraShakeEnabled());
    break;
  case 4:
    Settings->SetDamageNumbersEnabled(!Settings->GetDamageNumbersEnabled());
    break;
  default:
    break;
  }
}

void ALostsensePlayerController::MenuConfirm() {
  if (!IsGameplayInputSuppressed()) {
    return;
  }

  if (ActiveMenuPage == ELostsenseMenuPage::Start) {
    switch (MenuSelectionIndex) {
    case 0:
      LoadDevelopmentCharacter();
      OpenMenu(ELostsenseMenuPage::None);
      return;
    case 1:
      ResetDevelopmentEncounter();
      return;
    case 2:
      bOptionsOpenedFromStart = true;
      OpenMenu(ELostsenseMenuPage::Options);
      return;
    case 3:
      ConsoleCommand(TEXT("quit"), true);
      return;
    default:
      return;
    }
  }

  if (ActiveMenuPage == ELostsenseMenuPage::Pause) {
    switch (MenuSelectionIndex) {
    case 0:
      OpenMenu(ELostsenseMenuPage::None);
      return;
    case 1:
      SaveDevelopmentCharacter();
      return;
    case 2:
      LoadDevelopmentCharacter();
      return;
    case 3:
      OpenMenu(ELostsenseMenuPage::QuestJournal);
      return;
    case 4:
      OpenMenu(ELostsenseMenuPage::WorldMap);
      return;
    case 5:
      bOptionsOpenedFromStart = false;
      OpenMenu(ELostsenseMenuPage::Options);
      return;
    case 6:
      OpenMenu(ELostsenseMenuPage::Start);
      return;
    default:
      return;
    }
  }

  if (ActiveMenuPage == ELostsenseMenuPage::Options) {
    if (MenuSelectionIndex == 5 && GEngine != nullptr) {
      if (ULostsenseGameUserSettings *Settings =
              Cast<ULostsenseGameUserSettings>(
                  GEngine->GetGameUserSettings())) {
        Settings->ApplyAndPersist();
      }
      return;
    }
    if (MenuSelectionIndex == 6) {
      OpenMenu(bOptionsOpenedFromStart ? ELostsenseMenuPage::Start
                                       : ELostsenseMenuPage::Pause);
      return;
    }
    AdjustOption(1);
    return;
  }

  if (ActiveMenuPage == ELostsenseMenuPage::QuestJournal ||
      ActiveMenuPage == ELostsenseMenuPage::WorldMap) {
    return;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr) {
    return;
  }

  if (ActiveMenuPage == ELostsenseMenuPage::Inventory) {
    TArray<FLostsenseInventoryMenuEntry> Entries;
    if (!FLostsenseMenuProjection::CaptureInventory(*Runtime, Entries) ||
        Entries.IsEmpty()) {
      return;
    }
    NormalizeMenuSelection();
    const FLostsenseInventoryMenuEntry &Selected = Entries[MenuSelectionIndex];
    if (!Selected.bEquippable) {
      ClientMessage(Selected.bEquipped
                        ? TEXT("That item is already equipped")
                        : TEXT("Selected entry is not equippable"));
      return;
    }
    const bool bEquipped = Runtime->EquipInventoryItem(Selected.InstanceId);
    ClientMessage(bEquipped
                      ? TEXT("Equipped selected inventory item")
                      : TEXT("Selected inventory item could not be equipped"));
    NormalizeMenuSelection();
    return;
  }

  TArray<FLostsenseScarMenuEntry> Entries;
  if (!FLostsenseMenuProjection::CaptureScarAtlas(*Runtime, Entries) ||
      Entries.IsEmpty()) {
    return;
  }
  NormalizeMenuSelection();
  const FLostsenseScarMenuEntry &Selected = Entries[MenuSelectionIndex];
  if (!Selected.bAvailable) {
    ClientMessage(
        Selected.bAllocated
            ? TEXT("Scar node already allocated")
            : (Selected.bBlocked
                   ? TEXT("Scar node blocked by an exclusive oath")
                   : TEXT("Scar node is locked or needs more points")));
    return;
  }

  if (!Runtime->AllocateSkillNode(Selected.NodeId)) {
    ClientMessage(TEXT("Scar allocation was rejected by gameplay authority"));
    return;
  }

  using namespace Lostsense::Gameplay;
  using namespace Lostsense::Gameplay::FirstSlice;
  if (Selected.NodeId == static_cast<int32>(BellstepNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active1),
        static_cast<int32>(Bellstep.Value)));
  } else if (Selected.NodeId == static_cast<int32>(MeasureBreakerNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active2),
        static_cast<int32>(MeasureBreaker.Value)));
  } else if (Selected.NodeId == static_cast<int32>(MarchSweepNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active3),
        static_cast<int32>(MarchSweep.Value)));
  } else if (Selected.NodeId == static_cast<int32>(AnsweringGuardNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active4),
        static_cast<int32>(AnsweringGuard.Value)));
  }
  ClientMessage(
      FString::Printf(TEXT("Allocated Scar Atlas node: %s"), *Selected.Name));
}

void ALostsensePlayerController::MenuCancel() {
  if (ActiveMenuPage == ELostsenseMenuPage::None) {
    OpenMenu(ELostsenseMenuPage::Pause);
    return;
  }
  if (ActiveMenuPage == ELostsenseMenuPage::Start) {
    return;
  }
  if (ActiveMenuPage == ELostsenseMenuPage::Options) {
    OpenMenu(bOptionsOpenedFromStart ? ELostsenseMenuPage::Start
                                     : ELostsenseMenuPage::Pause);
    return;
  }
  OpenMenu(ELostsenseMenuPage::None);
}

int32 ALostsensePlayerController::CurrentMenuEntryCount() const {
  UGameInstance *GameInstance = GetGameInstance();
  const ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;

  switch (ActiveMenuPage) {
  case ELostsenseMenuPage::Start:
    return 4;
  case ELostsenseMenuPage::Pause:
    return 7;
  case ELostsenseMenuPage::Options:
    return 7;
  case ELostsenseMenuPage::QuestJournal:
    return static_cast<int32>(
        Lostsense::Gameplay::CampaignCatalog::Quests().size());
  case ELostsenseMenuPage::WorldMap:
    return static_cast<int32>(
        Lostsense::Gameplay::CampaignCatalog::Regions().size());
  case ELostsenseMenuPage::Inventory: {
    TArray<FLostsenseInventoryMenuEntry> Entries;
    return Runtime != nullptr &&
                   FLostsenseMenuProjection::CaptureInventory(*Runtime, Entries)
               ? Entries.Num()
               : 0;
  }
  case ELostsenseMenuPage::ScarAtlas: {
    TArray<FLostsenseScarMenuEntry> Entries;
    return Runtime != nullptr &&
                   FLostsenseMenuProjection::CaptureScarAtlas(*Runtime, Entries)
               ? Entries.Num()
               : 0;
  }
  case ELostsenseMenuPage::None:
    return 0;
  }
  return 0;
}

void ALostsensePlayerController::NormalizeMenuSelection() {
  const int32 Count = CurrentMenuEntryCount();
  if (Count <= 0) {
    MenuSelectionIndex = 0;
    return;
  }
  MenuSelectionIndex = (MenuSelectionIndex % Count + Count) % Count;
}

void ALostsensePlayerController::ApplyMenuInputMode() {
  APawn *ControlledPawn = GetPawn();
  if (ControlledPawn == nullptr) {
    return;
  }

  if (IsGameplayInputSuppressed()) {
    ULostsenseInputBridgeLibrary::GuardReleased(this);
    ControlledPawn->DisableInput(this);
    return;
  }
  ControlledPawn->EnableInput(this);
}

void ALostsensePlayerController::TryInteract() {
  ALostsenseKnightCharacter *Knight =
      Cast<ALostsenseKnightCharacter>(GetPawn());
  UWorld *World = GetWorld();
  if (Knight == nullptr || World == nullptr) {
    return;
  }

  AActor *BestActor = nullptr;
  ILostsenseInteractable *BestInteractable = nullptr;
  double BestDistanceSquared = FMath::Square(260.0);
  for (TActorIterator<AActor> It(World); It; ++It) {
    AActor *Candidate = *It;
    if (Candidate == nullptr || Candidate == Knight ||
        !Candidate->GetClass()->ImplementsInterface(
            ULostsenseInteractable::StaticClass())) {
      continue;
    }

    ILostsenseInteractable *Interactable =
        Cast<ILostsenseInteractable>(Candidate);
    if (Interactable == nullptr || !Interactable->CanInteract(*Knight)) {
      continue;
    }

    const double DistanceSquared = FVector::DistSquared(
        Knight->GetActorLocation(), Candidate->GetActorLocation());
    if (DistanceSquared < BestDistanceSquared) {
      BestDistanceSquared = DistanceSquared;
      BestActor = Candidate;
      BestInteractable = Interactable;
    }
  }

  if (BestActor != nullptr && BestInteractable != nullptr &&
      BestInteractable->Interact(*Knight)) {
    ClientMessage(BestInteractable->GetInteractionPrompt());
  }
}

void ALostsensePlayerController::SaveDevelopmentCharacter() {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;

  FString CharacterPayload;
  FString StoryPayload;
  const bool bCaptured = Runtime != nullptr && Story != nullptr &&
                         Runtime->SaveCharacterToText(CharacterPayload) &&
                         Story->SaveStoryToText(StoryPayload);
  const bool bSaved =
      bCaptured &&
      FFileHelper::SaveStringToFile(CharacterPayload, *DevelopmentSavePath()) &&
      FFileHelper::SaveStringToFile(StoryPayload, *DevelopmentStorySavePath());
  ClientMessage(bSaved ? TEXT("Lostsense save written")
                       : TEXT("Lostsense save failed"));
}

void ALostsensePlayerController::LoadDevelopmentCharacter() {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;

  FString CharacterPayload;
  FString StoryPayload;
  const bool bRead =
      Runtime != nullptr && Story != nullptr &&
      FFileHelper::LoadFileToString(CharacterPayload, *DevelopmentSavePath()) &&
      FFileHelper::LoadFileToString(StoryPayload, *DevelopmentStorySavePath());
  if (!bRead) {
    ClientMessage(TEXT("No compatible Lostsense save found"));
    return;
  }

  FString CharacterRollback;
  FString StoryRollback;
  if (!Runtime->SaveCharacterToText(CharacterRollback) ||
      !Story->SaveStoryToText(StoryRollback)) {
    ClientMessage(TEXT("Lostsense load failed"));
    return;
  }

  const bool bCharacterLoaded =
      Runtime->LoadCharacterFromText(CharacterPayload);
  const bool bStoryLoaded =
      bCharacterLoaded && Story->LoadStoryFromText(StoryPayload);
  if (!bStoryLoaded) {
    static_cast<void>(Runtime->LoadCharacterFromText(CharacterRollback));
    static_cast<void>(Story->LoadStoryFromText(StoryRollback));
    ClientMessage(TEXT("Lostsense save rejected and rolled back"));
    return;
  }

  ClientMessage(TEXT("Lostsense save restored"));
}

void ALostsensePlayerController::ResetDevelopmentEncounter() {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Runtime == nullptr || Story == nullptr ||
      StartupCharacterSnapshot.IsEmpty() || StartupStorySnapshot.IsEmpty() ||
      !Runtime->LoadCharacterFromText(StartupCharacterSnapshot) ||
      !Story->LoadStoryFromText(StartupStorySnapshot)) {
    ClientMessage(TEXT("Lostsense new game reset failed"));
    return;
  }

  ConsoleCommand(TEXT("RestartLevel"), true);
}

FString ALostsensePlayerController::DevelopmentSavePath() const {
  return FPaths::Combine(FPaths::ProjectSavedDir(),
                         TEXT("LostsenseCharacter.save"));
}

FString ALostsensePlayerController::DevelopmentStorySavePath() const {
  return FPaths::Combine(FPaths::ProjectSavedDir(),
                         TEXT("LostsenseStory.save"));
}
