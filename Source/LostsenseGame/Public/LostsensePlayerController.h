#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "LostsensePlayerController.generated.h"

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
  void MenuSelectPrevious();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuSelectNext();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuConfirm();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Menu")
  void MenuCancel();

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  bool IsInventoryMenuOpen() const { return bInventoryMenuOpen; }

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  bool IsScarAtlasMenuOpen() const { return bScarAtlasMenuOpen; }

  UFUNCTION(BlueprintPure, Category = "Lostsense|Menu")
  bool IsGameplayInputSuppressed() const {
    return bInventoryMenuOpen || bScarAtlasMenuOpen;
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
  int32 CurrentMenuEntryCount() const;
  void NormalizeMenuSelection();
  void ApplyMenuInputMode();
  FString DevelopmentSavePath() const;
  FString DevelopmentStorySavePath() const;

  FString StartupCharacterSnapshot;
  FString StartupStorySnapshot;
  bool bInventoryMenuOpen = false;
  bool bScarAtlasMenuOpen = false;
  int32 MenuSelectionIndex = 0;
};
