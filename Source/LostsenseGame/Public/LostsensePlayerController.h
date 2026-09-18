#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "LostsensePlayerController.generated.h"

UENUM(BlueprintType)
enum class ELostsenseMenuPage : uint8 {
  None,
  Start,
  Pause,
  Inventory,
  ScarAtlas,
  QuestJournal,
  WorldMap,
  Options
};

UCLASS()
class LOSTSENSEGAME_API ALostsensePlayerController final
    : public APlayerController {
  GENERATED_BODY()

public:
  ALostsensePlayerController();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  void InputInteract();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void ToggleInventoryMenu();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void ToggleScarAtlasMenu();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void ToggleQuestJournal();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void ToggleWorldMap();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void TogglePauseMenu();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuSelectPrevious();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuSelectNext();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuAdjustLeft();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuAdjustRight();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuConfirm();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuCancel();

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  ELostsenseMenuPage GetActiveMenuPage() const { return ActiveMenuPage; }

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  bool IsInventoryMenuOpen() const {
    return ActiveMenuPage == ELostsenseMenuPage::Inventory;
  }

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  bool IsScarAtlasMenuOpen() const {
    return ActiveMenuPage == ELostsenseMenuPage::ScarAtlas;
  }

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  bool IsGameplayInputSuppressed() const {
    return ActiveMenuPage != ELostsenseMenuPage::None;
  }

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  int32 GetMenuSelectionIndex() const { return MenuSelectionIndex; }

protected:
  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;

private:
  void TryInteract();
  void SaveDevelopmentCharacter();
  void LoadDevelopmentCharacter();
  void ResetDevelopmentEncounter();
  void OpenMenu(ELostsenseMenuPage Page);
  void AdjustOption(int32 Direction);
  int32 CurrentMenuEntryCount() const;
  void NormalizeMenuSelection();
  void ApplyMenuInputMode();
  FString DevelopmentSavePath() const;
  FString DevelopmentStorySavePath() const;

  FString StartupCharacterSnapshot;
  FString StartupStorySnapshot;
  ELostsenseMenuPage ActiveMenuPage = ELostsenseMenuPage::Start;
  int32 MenuSelectionIndex = 0;
  bool bOptionsOpenedFromStart = false;
};
