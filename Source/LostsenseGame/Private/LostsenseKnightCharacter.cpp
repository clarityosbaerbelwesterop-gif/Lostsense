#include "LostsenseKnightCharacter.h"

#include "LostsenseEnemyCharacter.h"
#include "LostsenseRuntimeSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

#include "Lostsense/Gameplay/Content/FirstSliceContent.h"

#include <algorithm>
#include <vector>

namespace {
UInputAction *CreateDigitalAction(AActor &Owner, const TCHAR *Name) {
  UInputAction *Action = Owner.CreateDefaultSubobject<UInputAction>(Name);
  Action->ValueType = EInputActionValueType::Boolean;
  return Action;
}

UInputAction *CreateAxisAction(AActor &Owner, const TCHAR *Name) {
  UInputAction *Action = Owner.CreateDefaultSubobject<UInputAction>(Name);
  Action->ValueType = EInputActionValueType::Axis1D;
  return Action;
}
} // namespace

ALostsenseKnightCharacter::ALostsenseKnightCharacter() {
  PrimaryActorTick.bCanEverTick = true;

  bUseControllerRotationPitch = false;
  bUseControllerRotationRoll = false;
  bUseControllerRotationYaw = false;

  UCharacterMovementComponent *Movement = GetCharacterMovement();
  Movement->bOrientRotationToMovement = true;
  Movement->RotationRate = FRotator(0.0, 720.0, 0.0);
  Movement->MaxWalkSpeed = 600.0;
  Movement->MaxAcceleration = 2400.0;
  Movement->BrakingDecelerationWalking = 1800.0;

  CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(RootComponent);
  CameraBoom->TargetArmLength = 900.0;
  CameraBoom->bDoCollisionTest = true;
  CameraBoom->bUsePawnControlRotation = false;
  CameraBoom->SetRelativeRotation(FRotator(-55.0, -45.0, 0.0));

  FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
  FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
  FollowCamera->bUsePawnControlRotation = false;

  GameplayMappingContext =
      CreateDefaultSubobject<UInputMappingContext>(TEXT("GameplayMapping"));

  MoveForwardAction = CreateDigitalAction(*this, TEXT("MoveForward"));
  MoveBackwardAction = CreateDigitalAction(*this, TEXT("MoveBackward"));
  MoveLeftAction = CreateDigitalAction(*this, TEXT("MoveLeft"));
  MoveRightAction = CreateDigitalAction(*this, TEXT("MoveRight"));
  LookXAction = CreateAxisAction(*this, TEXT("LookX"));
  LookYAction = CreateAxisAction(*this, TEXT("LookY"));
  PrimaryAttackAction = CreateDigitalAction(*this, TEXT("PrimaryAttack"));
  HeavyAttackAction = CreateDigitalAction(*this, TEXT("HeavyAttack"));
  DodgeAction = CreateDigitalAction(*this, TEXT("Dodge"));
  GuardAction = CreateDigitalAction(*this, TEXT("Guard"));
  ActiveSlot1Action = CreateDigitalAction(*this, TEXT("ActiveSlot1"));
  ActiveSlot2Action = CreateDigitalAction(*this, TEXT("ActiveSlot2"));
  ActiveSlot3Action = CreateDigitalAction(*this, TEXT("ActiveSlot3"));
  ActiveSlot4Action = CreateDigitalAction(*this, TEXT("ActiveSlot4"));
  AdvanceSkillAction = CreateDigitalAction(*this, TEXT("AdvanceStarterSkill"));
  EquipPickupAction = CreateDigitalAction(*this, TEXT("EquipFirstPickup"));

  GameplayMappingContext->MapKey(MoveForwardAction, EKeys::W);
  GameplayMappingContext->MapKey(MoveBackwardAction, EKeys::S);
  GameplayMappingContext->MapKey(MoveLeftAction, EKeys::A);
  GameplayMappingContext->MapKey(MoveRightAction, EKeys::D);
  GameplayMappingContext->MapKey(LookXAction, EKeys::MouseX);
  GameplayMappingContext->MapKey(LookYAction, EKeys::MouseY);
  GameplayMappingContext->MapKey(PrimaryAttackAction, EKeys::LeftMouseButton);
  GameplayMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
  GameplayMappingContext->MapKey(DodgeAction, EKeys::SpaceBar);
  GameplayMappingContext->MapKey(GuardAction, EKeys::LeftShift);
  GameplayMappingContext->MapKey(ActiveSlot1Action, EKeys::Q);
  GameplayMappingContext->MapKey(ActiveSlot2Action, EKeys::Two);
  GameplayMappingContext->MapKey(ActiveSlot3Action, EKeys::R);
  GameplayMappingContext->MapKey(ActiveSlot4Action, EKeys::F);
  GameplayMappingContext->MapKey(AdvanceSkillAction, EKeys::K);
  GameplayMappingContext->MapKey(EquipPickupAction, EKeys::T);
}

void ALostsenseKnightCharacter::BeginPlay() {
  Super::BeginPlay();

  const APlayerController *PlayerController =
      Cast<APlayerController>(GetController());
  if (PlayerController == nullptr || GameplayMappingContext == nullptr) {
    return;
  }

  const ULocalPlayer *LocalPlayer = PlayerController->GetLocalPlayer();
  if (LocalPlayer == nullptr) {
    return;
  }

  if (UEnhancedInputLocalPlayerSubsystem *InputSubsystem =
          LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
    InputSubsystem->AddMappingContext(GameplayMappingContext, 0);
  }
}

void ALostsenseKnightCharacter::Tick(const float DeltaSeconds) {
  Super::Tick(DeltaSeconds);

  if (UGameInstance *GameInstance = GetGameInstance()) {
    if (ULostsenseRuntimeSubsystem *Runtime =
            GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()) {
      Runtime->AdvancePlayerTime(DeltaSeconds);
      if (Runtime->GetPlayerHealth() <= 0.0) {
        GetCharacterMovement()->DisableMovement();
        bGuardHeld = false;
        return;
      }
    }
  }

  if (!bGuardHeld) {
    return;
  }

  GuardRefreshRemaining -= DeltaSeconds;
  if (GuardRefreshRemaining <= 0.0F) {
    const bool Activated = ActivateLoadoutSlot(static_cast<int32>(
        Lostsense::Gameplay::AbilityLoadoutSlot::ClassMechanic));
    GuardRefreshRemaining = Activated ? 0.20F : 0.05F;
  }
}

void ALostsenseKnightCharacter::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  UEnhancedInputComponent *Enhanced =
      Cast<UEnhancedInputComponent>(PlayerInputComponent);
  if (Enhanced == nullptr) {
    return;
  }

  Enhanced->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this,
                       &ALostsenseKnightCharacter::MoveForward);
  Enhanced->BindAction(MoveBackwardAction, ETriggerEvent::Triggered, this,
                       &ALostsenseKnightCharacter::MoveBackward);
  Enhanced->BindAction(MoveLeftAction, ETriggerEvent::Triggered, this,
                       &ALostsenseKnightCharacter::MoveLeft);
  Enhanced->BindAction(MoveRightAction, ETriggerEvent::Triggered, this,
                       &ALostsenseKnightCharacter::MoveRight);
  Enhanced->BindAction(LookXAction, ETriggerEvent::Triggered, this,
                       &ALostsenseKnightCharacter::LookX);
  Enhanced->BindAction(LookYAction, ETriggerEvent::Triggered, this,
                       &ALostsenseKnightCharacter::LookY);
  Enhanced->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::PrimaryAttack);
  Enhanced->BindAction(HeavyAttackAction, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::HeavyAttack);
  Enhanced->BindAction(DodgeAction, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::Dodge);
  Enhanced->BindAction(GuardAction, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::GuardStarted);
  Enhanced->BindAction(GuardAction, ETriggerEvent::Completed, this,
                       &ALostsenseKnightCharacter::GuardCompleted);
  Enhanced->BindAction(ActiveSlot1Action, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::ActivateSlot1);
  Enhanced->BindAction(ActiveSlot2Action, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::ActivateSlot2);
  Enhanced->BindAction(ActiveSlot3Action, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::ActivateSlot3);
  Enhanced->BindAction(ActiveSlot4Action, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::ActivateSlot4);
  Enhanced->BindAction(AdvanceSkillAction, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::AdvanceStarterSkill);
  Enhanced->BindAction(EquipPickupAction, ETriggerEvent::Started, this,
                       &ALostsenseKnightCharacter::EquipFirstPickup);
}

void ALostsenseKnightCharacter::MoveForward(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    const FRotator Yaw(0.0, CameraBoom->GetComponentRotation().Yaw, 0.0);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), 1.0);
  }
}

void ALostsenseKnightCharacter::MoveBackward(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    const FRotator Yaw(0.0, CameraBoom->GetComponentRotation().Yaw, 0.0);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), -1.0);
  }
}

void ALostsenseKnightCharacter::MoveLeft(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    const FRotator Yaw(0.0, CameraBoom->GetComponentRotation().Yaw, 0.0);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), -1.0);
  }
}

void ALostsenseKnightCharacter::MoveRight(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    const FRotator Yaw(0.0, CameraBoom->GetComponentRotation().Yaw, 0.0);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), 1.0);
  }
}

void ALostsenseKnightCharacter::LookX(const FInputActionValue &Value) {
  FRotator Rotation = CameraBoom->GetRelativeRotation();
  Rotation.Yaw += Value.Get<float>() * 1.5;
  CameraBoom->SetRelativeRotation(Rotation);
}

void ALostsenseKnightCharacter::LookY(const FInputActionValue &Value) {
  FRotator Rotation = CameraBoom->GetRelativeRotation();
  Rotation.Pitch =
      FMath::Clamp(Rotation.Pitch + Value.Get<float>(), -70.0, -35.0);
  CameraBoom->SetRelativeRotation(Rotation);
}

void ALostsenseKnightCharacter::PrimaryAttack(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    ActivateLoadoutSlotAgainstNearestEnemy(static_cast<int32>(
        Lostsense::Gameplay::AbilityLoadoutSlot::PrimaryAttack));
  }
}

void ALostsenseKnightCharacter::HeavyAttack(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    ActivateLoadoutSlotAgainstNearestEnemy(static_cast<int32>(
        Lostsense::Gameplay::AbilityLoadoutSlot::SecondaryAttack));
  }
}

void ALostsenseKnightCharacter::Dodge(const FInputActionValue &Value) {
  if (!Value.Get<bool>()) {
    return;
  }

  if (ActivateLoadoutSlot(
          static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Dodge))) {
    LaunchCharacter(GetActorForwardVector() * 650.0 + FVector(0.0, 0.0, 60.0),
                    true, false);
  }
}

void ALostsenseKnightCharacter::GuardStarted(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    if (UGameInstance *GameInstance = GetGameInstance()) {
      if (ULostsenseRuntimeSubsystem *Runtime =
              GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()) {
        static_cast<void>(Runtime->BeginPerfectGuardWindow());
      }
    }
    bGuardHeld = true;
    GuardRefreshRemaining = 0.0F;
  }
}

void ALostsenseKnightCharacter::GuardCompleted(const FInputActionValue &Value) {
  static_cast<void>(Value);
  bGuardHeld = false;
  GuardRefreshRemaining = 0.0F;
}

void ALostsenseKnightCharacter::ActivateSlot1(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    ActivateLoadoutSlotAgainstNearestEnemy(
        static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Active1));
  }
}

void ALostsenseKnightCharacter::ActivateSlot2(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    ActivateLoadoutSlotAgainstNearestEnemy(
        static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Active2));
  }
}

void ALostsenseKnightCharacter::ActivateSlot3(const FInputActionValue &Value) {
  if (!Value.Get<bool>()) {
    return;
  }
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  UWorld *World = GetWorld();
  const int32 SlotIndex =
      static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Active3);
  if (Runtime == nullptr || World == nullptr) {
    return;
  }
  const int32 MaximumTargets = Runtime->GetLoadoutSlotMaximumTargets(SlotIndex);
  if (MaximumTargets <= 1) {
    ActivateLoadoutSlotAgainstNearestEnemy(SlotIndex);
    return;
  }

  struct FCandidate final {
    ALostsenseEnemyCharacter *Enemy = nullptr;
    double DistanceSquared = 0.0;
    uint64 CombatantId = 0U;
  };
  std::vector<FCandidate> Candidates;
  const FVector Origin = GetActorLocation();
  const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
  constexpr double SweepRange = 430.0;
  constexpr double MinimumForwardDot = 0.25;
  for (TActorIterator<ALostsenseEnemyCharacter> It(World); It; ++It) {
    ALostsenseEnemyCharacter *Enemy = *It;
    if (Enemy == nullptr || Enemy->IsDefeated()) {
      continue;
    }
    const FVector Delta = Enemy->GetActorLocation() - Origin;
    const double DistanceSquared = Delta.SizeSquared2D();
    if (DistanceSquared > SweepRange * SweepRange ||
        FVector::DotProduct(Forward, Delta.GetSafeNormal2D()) <
            MinimumForwardDot) {
      continue;
    }
    Candidates.push_back({Enemy, DistanceSquared, Enemy->GetCombatantId()});
  }
  std::sort(Candidates.begin(), Candidates.end(),
            [](const FCandidate &Left, const FCandidate &Right) {
              if (Left.DistanceSquared != Right.DistanceSquared) {
                return Left.DistanceSquared < Right.DistanceSquared;
              }
              return Left.CombatantId < Right.CombatantId;
            });
  if (Candidates.empty()) {
    return;
  }

  const std::size_t Count =
      std::min(Candidates.size(), static_cast<std::size_t>(MaximumTargets));
  std::vector<Lostsense::Gameplay::AbilityTarget> Targets;
  Targets.reserve(Count);
  for (std::size_t Index = 0U; Index < Count; ++Index) {
    Targets.push_back(Candidates[Index].Enemy->BuildAbilityTarget());
  }
  if (!Runtime->ActivatePlayerLoadoutSlotAgainstMany(SlotIndex, Targets)) {
    return;
  }
  for (std::size_t Index = 0U; Index < Count; ++Index) {
    Candidates[Index].Enemy->FinalizePlayerAbility(*Runtime);
  }
}

void ALostsenseKnightCharacter::ActivateSlot4(const FInputActionValue &Value) {
  if (Value.Get<bool>()) {
    ActivateLoadoutSlot(
        static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Active4));
  }
}

void ALostsenseKnightCharacter::AdvanceStarterSkill(
    const FInputActionValue &Value) {
  if (!Value.Get<bool>()) {
    return;
  }

  ULostsenseRuntimeSubsystem *Runtime =
      GetGameInstance()->GetSubsystem<ULostsenseRuntimeSubsystem>();
  if (Runtime == nullptr) {
    return;
  }

  using namespace Lostsense::Gameplay::FirstSlice;
  if (Runtime->AllocateSkillNode(static_cast<int32>(FirstMeasure.Value))) {
    return;
  }
  if (Runtime->AllocateSkillNode(static_cast<int32>(BellstepNode.Value))) {
    static_cast<void>(Runtime->EquipAbilityInSlot(
        static_cast<int32>(Lostsense::Gameplay::AbilityLoadoutSlot::Active1),
        static_cast<int32>(Bellstep.Value)));
  }
}

void ALostsenseKnightCharacter::EquipFirstPickup(
    const FInputActionValue &Value) {
  if (!Value.Get<bool>()) {
    return;
  }

  if (ULostsenseRuntimeSubsystem *Runtime =
          GetGameInstance()->GetSubsystem<ULostsenseRuntimeSubsystem>()) {
    static_cast<void>(Runtime->EquipFirstInventoryItem());
  }
}

bool ALostsenseKnightCharacter::ActivateLoadoutSlot(const int32 SlotIndex) {
  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  return Runtime != nullptr && Runtime->ActivatePlayerLoadoutSlot(SlotIndex);
}

bool ALostsenseKnightCharacter::ActivateLoadoutSlotAgainstNearestEnemy(
    const int32 SlotIndex) {
  ALostsenseEnemyCharacter *Enemy = FindNearestLivingEnemy(260.0F);
  if (Enemy == nullptr) {
    return false;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseRuntimeSubsystem *Runtime =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseRuntimeSubsystem>()
          : nullptr;
  return Runtime != nullptr &&
         Enemy->ReceivePlayerLoadoutSlot(*Runtime, SlotIndex);
}

ALostsenseEnemyCharacter *
ALostsenseKnightCharacter::FindNearestLivingEnemy(const float Radius) const {
  ALostsenseEnemyCharacter *Best = nullptr;
  double BestDistanceSquared = FMath::Square(static_cast<double>(Radius));
  for (TActorIterator<ALostsenseEnemyCharacter> It(GetWorld()); It; ++It) {
    ALostsenseEnemyCharacter *Enemy = *It;
    if (Enemy == nullptr || Enemy->IsDefeated()) {
      continue;
    }

    const double DistanceSquared =
        FVector::DistSquared(GetActorLocation(), Enemy->GetActorLocation());
    if (DistanceSquared < BestDistanceSquared) {
      BestDistanceSquared = DistanceSquared;
      Best = Enemy;
    }
  }
  return Best;
}
