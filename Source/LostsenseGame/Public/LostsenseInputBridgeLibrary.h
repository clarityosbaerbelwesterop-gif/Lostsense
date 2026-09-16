#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "LostsenseInputBridgeLibrary.generated.h"

class APlayerController;

UCLASS()
class LOSTSENSEGAME_API ULostsenseInputBridgeLibrary final
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void Move(APlayerController *Controller, FVector2D Axis);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void Look(APlayerController *Controller, FVector2D Axis);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void PrimaryAttack(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void HeavyAttack(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void Dodge(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void GuardPressed(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void GuardReleased(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void ActivateSkill(APlayerController *Controller, int32 SlotNumber);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void Interact(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void QuickEquip(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void ToggleInventory(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void ToggleScarAtlas(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void MenuNavigate(APlayerController *Controller, int32 Direction);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void MenuConfirm(APlayerController *Controller);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Input")
  static void MenuCancel(APlayerController *Controller);
};
