#include "LostsensePlayerController.h"

#include "LostsenseInteractable.h"
#include "LostsenseKnightCharacter.h"
#include "LostsenseMenuProjection.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseStorySubsystem.h"

#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"

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
  InputComponent->BindKey(EKeys::Up, IE_Pressed, this,
                          &ALostsensePlayerController::MenuSelectPrevious);
  InputComponent->BindKey(EKeys::Down, IE_Pressed, this,
                          &ALostsensePlayerController::MenuSelectNext);
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

void ALostsensePlayerController::ToggleInventoryMenu() {
  const bool bOpen = !bInventoryMenuOpen;
  bInventoryMenuOpen = bOpen;
  bScarAtlasMenuOpen = false;
  MenuSelectionIndex = 0;
  NormalizeMenuSelection();
}

void ALostsensePlayerController::ToggleScarAtlasMenu() {
  const bool bOpen = !bScarAtlasMenuOpen;
  bScarAtlasMenuOpen = bOpen;
  bInventoryMenuOpen = false;
  MenuSelectionIndex = 0;
  NormalizeMenuSelection();
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

void ALostsensePlayerController::MenuConfirm() {
  if (!IsGameplayInputSuppressed()) {
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

  if (bInventoryMenuOpen) {
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
    const bool bEquipped = Runtime->EquipFirstInventoryItem();
    ClientMessage(bEquipped ? TEXT("Equipped the next compatible inventory item")
                            : TEXT("No compatible inventory item could be equipped"));
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
    ClientMessage(Selected.bAllocated
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
  } else if (Selected.NodeId ==
             static_cast<int32>(MeasureBreakerNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active2),
        static_cast<int32>(MeasureBreaker.Value)));
  } else if (Selected.NodeId == static_cast<int32>(MarchSweepNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active3),
        static_cast<int32>(MarchSweep.Value)));
  } else if (Selected.NodeId ==
             static_cast<int32>(AnsweringGuardNode.Value)) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(AbilityLoadoutSlot::Active4),
        static_cast<int32>(AnsweringGuard.Value)));
  }
  ClientMessage(FString::Printf(TEXT("Allocated Scar Atlas node: %s"),
                                *Selected.Name));
}

void ALostsensePlayerController::MenuCancel() {
  if (!IsGameplayInputSuppressed()) {
    return;
  }
  bInventoryMenuOpen = false;
  bScarAtlasMenuOpen = false;
  MenuSelectionIndex = 0;
}

int32 ALostsensePlayerController::CurrentMenuEntryCount() const {
  const UGameInstance *GameInstance = GetGameInstance();
  const ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr) {
    return 0;
  }

  if (bInventoryMenuOpen) {
    TArray<FLostsenseInventoryMenuEntry> Entries;
    return FLostsenseMenuProjection::CaptureInventory(*Runtime, Entries)
               ? Entries.Num()
               : 0;
  }
  if (bScarAtlasMenuOpen) {
    TArray<FLostsenseScarMenuEntry> Entries;
    return FLostsenseMenuProjection::CaptureScarAtlas(*Runtime, Entries)
               ? Entries.Num()
               : 0;
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
  ClientMessage(bSaved ? TEXT("Lostsense slice save written")
                       : TEXT("Lostsense slice save failed"));
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
    ClientMessage(TEXT("Lostsense slice load failed"));
    return;
  }

  FString CharacterRollback;
  FString StoryRollback;
  if (!Runtime->SaveCharacterToText(CharacterRollback) ||
      !Story->SaveStoryToText(StoryRollback)) {
    ClientMessage(TEXT("Lostsense slice load failed"));
    return;
  }

  const bool bCharacterLoaded =
      Runtime->LoadCharacterFromText(CharacterPayload);
  const bool bStoryLoaded =
      bCharacterLoaded && Story->LoadStoryFromText(StoryPayload);
  if (!bStoryLoaded) {
    static_cast<void>(Runtime->LoadCharacterFromText(CharacterRollback));
    static_cast<void>(Story->LoadStoryFromText(StoryRollback));
    ClientMessage(TEXT("Lostsense slice load rejected and rolled back"));
    return;
  }

  ClientMessage(TEXT("Lostsense slice save restored"));
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
    ClientMessage(TEXT("Lostsense development reset failed"));
    return;
  }

  ConsoleCommand(TEXT("RestartLevel"), true);
}

FString ALostsensePlayerController::DevelopmentSavePath() const {
  return FPaths::Combine(FPaths::ProjectSavedDir(),
                         TEXT("LostsenseDevelopmentCharacter.save"));
}

FString ALostsensePlayerController::DevelopmentStorySavePath() const {
  return FPaths::Combine(FPaths::ProjectSavedDir(),
                         TEXT("LostsenseDevelopmentStory.save"));
}
