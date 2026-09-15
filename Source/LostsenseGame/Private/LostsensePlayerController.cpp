#include "LostsensePlayerController.h"

#include "LostsenseRuntimeSubsystem.h"

#include "Engine/GameInstance.h"
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

  if (UGameInstance *GameInstance = GetGameInstance()) {
    if (ULostsenseRuntimeSubsystem *Runtime =
            GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()) {
      static_cast<void>(Runtime->SaveCharacterToText(StartupCharacterSnapshot));
    }
  }
}

void ALostsensePlayerController::SetupInputComponent() {
  Super::SetupInputComponent();
  if (InputComponent == nullptr) {
    return;
  }

  InputComponent->BindKey(EKeys::F5, IE_Pressed, this,
                          &ALostsensePlayerController::SaveDevelopmentCharacter);
  InputComponent->BindKey(EKeys::F9, IE_Pressed, this,
                          &ALostsensePlayerController::LoadDevelopmentCharacter);
  InputComponent->BindKey(EKeys::F10, IE_Pressed, this,
                          &ALostsensePlayerController::ResetDevelopmentEncounter);
}

void ALostsensePlayerController::SaveDevelopmentCharacter() {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  FString Payload;
  const bool bSaved = Runtime != nullptr &&
                      Runtime->SaveCharacterToText(Payload) &&
                      FFileHelper::SaveStringToFile(Payload,
                                                    *DevelopmentSavePath());
  ClientMessage(bSaved ? TEXT("Lostsense development save written")
                       : TEXT("Lostsense development save failed"));
}

void ALostsensePlayerController::LoadDevelopmentCharacter() {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  FString Payload;
  const bool bLoaded = Runtime != nullptr &&
                       FFileHelper::LoadFileToString(Payload,
                                                     *DevelopmentSavePath()) &&
                       Runtime->LoadCharacterFromText(Payload);
  ClientMessage(bLoaded ? TEXT("Lostsense development save restored")
                        : TEXT("Lostsense development load failed"));
}

void ALostsensePlayerController::ResetDevelopmentEncounter() {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  if (Runtime == nullptr || StartupCharacterSnapshot.IsEmpty() ||
      !Runtime->LoadCharacterFromText(StartupCharacterSnapshot)) {
    ClientMessage(TEXT("Lostsense development reset failed"));
    return;
  }

  ConsoleCommand(TEXT("RestartLevel"), true);
}

FString ALostsensePlayerController::DevelopmentSavePath() const {
  return FPaths::Combine(FPaths::ProjectSavedDir(),
                         TEXT("LostsenseDevelopmentCharacter.save"));
}
