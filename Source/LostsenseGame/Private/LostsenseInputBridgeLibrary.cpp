#include "LostsenseInputBridgeLibrary.h"

#include "LostsenseKnightCharacter.h"
#include "LostsensePlayerController.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

namespace {
ALostsenseKnightCharacter *KnightFor(APlayerController *Controller) {
  return Controller != nullptr
             ? Cast<ALostsenseKnightCharacter>(Controller->GetPawn())
             : nullptr;
}

ALostsensePlayerController *LostsenseControllerFor(
    APlayerController *Controller) {
  return Cast<ALostsensePlayerController>(Controller);
}

bool GameplaySuppressed(APlayerController *Controller) {
  const ALostsensePlayerController *LostsenseController =
      LostsenseControllerFor(Controller);
  return LostsenseController != nullptr &&
         LostsenseController->IsGameplayInputSuppressed();
}
} // namespace

void ULostsenseInputBridgeLibrary::Move(APlayerController *Controller,
                                        FVector2D Axis) {
  ALostsenseKnightCharacter *Knight = KnightFor(Controller);
  if (Knight == nullptr || Knight->CameraBoom == nullptr ||
      GameplaySuppressed(Controller)) {
    return;
  }

  Axis.X = FMath::Clamp(Axis.X, -1.0, 1.0);
  Axis.Y = FMath::Clamp(Axis.Y, -1.0, 1.0);
  const FRotator Yaw(0.0, Knight->CameraBoom->GetComponentRotation().Yaw, 0.0);
  Knight->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
  Knight->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ULostsenseInputBridgeLibrary::Look(APlayerController *Controller,
                                        const FVector2D Axis) {
  ALostsenseKnightCharacter *Knight = KnightFor(Controller);
  if (Knight == nullptr || GameplaySuppressed(Controller)) {
    return;
  }
  Knight->LookX(FInputActionValue(static_cast<float>(Axis.X)));
  Knight->LookY(FInputActionValue(static_cast<float>(Axis.Y)));
}

void ULostsenseInputBridgeLibrary::PrimaryAttack(APlayerController *Controller) {
  if (ALostsenseKnightCharacter *Knight = KnightFor(Controller);
      Knight != nullptr && !GameplaySuppressed(Controller)) {
    Knight->PrimaryAttack(FInputActionValue(true));
  }
}

void ULostsenseInputBridgeLibrary::HeavyAttack(APlayerController *Controller) {
  if (ALostsenseKnightCharacter *Knight = KnightFor(Controller);
      Knight != nullptr && !GameplaySuppressed(Controller)) {
    Knight->HeavyAttack(FInputActionValue(true));
  }
}

void ULostsenseInputBridgeLibrary::Dodge(APlayerController *Controller) {
  if (ALostsenseKnightCharacter *Knight = KnightFor(Controller);
      Knight != nullptr && !GameplaySuppressed(Controller)) {
    Knight->Dodge(FInputActionValue(true));
  }
}

void ULostsenseInputBridgeLibrary::GuardPressed(APlayerController *Controller) {
  if (ALostsenseKnightCharacter *Knight = KnightFor(Controller);
      Knight != nullptr && !GameplaySuppressed(Controller)) {
    Knight->GuardStarted(FInputActionValue(true));
  }
}

void ULostsenseInputBridgeLibrary::GuardReleased(APlayerController *Controller) {
  if (ALostsenseKnightCharacter *Knight = KnightFor(Controller)) {
    Knight->GuardCompleted(FInputActionValue(false));
  }
}

void ULostsenseInputBridgeLibrary::ActivateSkill(APlayerController *Controller,
                                                 const int32 SlotNumber) {
  ALostsenseKnightCharacter *Knight = KnightFor(Controller);
  if (Knight == nullptr || GameplaySuppressed(Controller)) {
    return;
  }

  switch (SlotNumber) {
  case 1:
    Knight->ActivateSlot1(FInputActionValue(true));
    break;
  case 2:
    Knight->ActivateSlot2(FInputActionValue(true));
    break;
  case 3:
    Knight->ActivateSlot3(FInputActionValue(true));
    break;
  case 4:
    Knight->ActivateSlot4(FInputActionValue(true));
    break;
  default:
    break;
  }
}

void ULostsenseInputBridgeLibrary::Interact(APlayerController *Controller) {
  if (ALostsensePlayerController *LostsenseController =
          LostsenseControllerFor(Controller)) {
    LostsenseController->InputInteract();
  }
}

void ULostsenseInputBridgeLibrary::QuickEquip(APlayerController *Controller) {
  if (ALostsenseKnightCharacter *Knight = KnightFor(Controller);
      Knight != nullptr && !GameplaySuppressed(Controller)) {
    Knight->EquipFirstPickup(FInputActionValue(true));
  }
}

void ULostsenseInputBridgeLibrary::ToggleInventory(
    APlayerController *Controller) {
  if (ALostsensePlayerController *LostsenseController =
          LostsenseControllerFor(Controller)) {
    GuardReleased(Controller);
    LostsenseController->ToggleInventoryMenu();
  }
}

void ULostsenseInputBridgeLibrary::ToggleScarAtlas(
    APlayerController *Controller) {
  if (ALostsensePlayerController *LostsenseController =
          LostsenseControllerFor(Controller)) {
    GuardReleased(Controller);
    LostsenseController->ToggleScarAtlasMenu();
  }
}

void ULostsenseInputBridgeLibrary::MenuNavigate(APlayerController *Controller,
                                                const int32 Direction) {
  if (ALostsensePlayerController *LostsenseController =
          LostsenseControllerFor(Controller)) {
    if (Direction < 0) {
      LostsenseController->MenuSelectPrevious();
    } else if (Direction > 0) {
      LostsenseController->MenuSelectNext();
    }
  }
}

void ULostsenseInputBridgeLibrary::MenuConfirm(APlayerController *Controller) {
  if (ALostsensePlayerController *LostsenseController =
          LostsenseControllerFor(Controller)) {
    LostsenseController->MenuConfirm();
  }
}

void ULostsenseInputBridgeLibrary::MenuCancel(APlayerController *Controller) {
  if (ALostsensePlayerController *LostsenseController =
          LostsenseControllerFor(Controller)) {
    GuardReleased(Controller);
    LostsenseController->MenuCancel();
  }
}
