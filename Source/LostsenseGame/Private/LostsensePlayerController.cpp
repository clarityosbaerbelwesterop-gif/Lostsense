#include "LostsensePlayerController.h"

#include "LostsenseInteractable.h"
#include "LostsenseKnightCharacter.h"
#include "LostsenseRuntimeSubsystem.h"
#include "LostsenseStorySubsystem.h"

#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
    static_cast<void>(Story->CompleteBeat(ELostsenseStoryBeat::ReturnedAwakened));
  }
}

void ALostsensePlayerController::SetupInputComponent() {
  Super::SetupInputComponent();
  if (InputComponent == nullptr) {
    return;
  }

  InputComponent->BindKey(EKeys::E, IE_Pressed, this,
                          &ALostsensePlayerController::TryInteract);
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

void ALostsensePlayerController::TryInteract() {
  ALostsenseKnightCharacter *Knight = Cast<ALostsenseKnightCharacter>(GetPawn());
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

    ILostsenseInteractable *Interactable = Cast<ILostsenseInteractable>(Candidate);
    if (Interactable == nullptr || !Interactable->CanInteract(*Knight)) {
      continue;
    }

    const double DistanceSquared =
        FVector::DistSquared(Knight->GetActorLocation(),
                             Candidate->GetActorLocation());
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

  const bool bCharacterLoaded = Runtime->LoadCharacterFromText(CharacterPayload);
  const bool bStoryLoaded = bCharacterLoaded && Story->LoadStoryFromText(StoryPayload);
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
