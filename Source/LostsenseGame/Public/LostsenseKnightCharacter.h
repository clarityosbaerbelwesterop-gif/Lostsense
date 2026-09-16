#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LostsenseKnightCharacter.generated.h"

class ALostsenseEnemyCharacter;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ULostsenseInputBridgeLibrary;
class USpringArmComponent;
struct FInputActionValue;

UCLASS()
class LOSTSENSEGAME_API ALostsenseKnightCharacter final : public ACharacter {
  GENERATED_BODY()

public:
  ALostsenseKnightCharacter();

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaSeconds) override;
  virtual void
  SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;

private:
  friend class ULostsenseInputBridgeLibrary;

  void MoveForward(const FInputActionValue &Value);
  void MoveBackward(const FInputActionValue &Value);
  void MoveLeft(const FInputActionValue &Value);
  void MoveRight(const FInputActionValue &Value);
  void LookX(const FInputActionValue &Value);
  void LookY(const FInputActionValue &Value);
  void PrimaryAttack(const FInputActionValue &Value);
  void HeavyAttack(const FInputActionValue &Value);
  void Dodge(const FInputActionValue &Value);
  void GuardStarted(const FInputActionValue &Value);
  void GuardCompleted(const FInputActionValue &Value);
  void ActivateSlot1(const FInputActionValue &Value);
  void ActivateSlot2(const FInputActionValue &Value);
  void ActivateSlot3(const FInputActionValue &Value);
  void ActivateSlot4(const FInputActionValue &Value);
  void AdvanceStarterSkill(const FInputActionValue &Value);
  void EquipFirstPickup(const FInputActionValue &Value);

  bool ActivateLoadoutSlot(int32 SlotIndex);
  bool ActivateLoadoutSlotAgainstNearestEnemy(int32 SlotIndex);
  ALostsenseEnemyCharacter *FindNearestLivingEnemy(float Radius) const;

  UPROPERTY(VisibleAnywhere, Category = "Lostsense|Camera")
  TObjectPtr<USpringArmComponent> CameraBoom;

  UPROPERTY(VisibleAnywhere, Category = "Lostsense|Camera")
  TObjectPtr<UCameraComponent> FollowCamera;

  UPROPERTY(Transient)
  TObjectPtr<UInputMappingContext> GameplayMappingContext;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> MoveForwardAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> MoveBackwardAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> MoveLeftAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> MoveRightAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> LookXAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> LookYAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> PrimaryAttackAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> HeavyAttackAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> DodgeAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> GuardAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> ActiveSlot1Action;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> ActiveSlot2Action;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> ActiveSlot3Action;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> ActiveSlot4Action;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> AdvanceSkillAction;

  UPROPERTY(Transient)
  TObjectPtr<UInputAction> EquipPickupAction;

  bool bGuardHeld = false;
  float GuardRefreshRemaining = 0.0F;
};
