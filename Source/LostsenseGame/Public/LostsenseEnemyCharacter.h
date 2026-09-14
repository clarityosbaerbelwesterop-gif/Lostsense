#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LostsenseEnemyCharacter.generated.h"

class ULostsenseRuntimeSubsystem;

UCLASS()
class LOSTSENSEGAME_API ALostsenseEnemyCharacter final : public ACharacter {
  GENERATED_BODY()

public:
  ALostsenseEnemyCharacter();
  virtual ~ALostsenseEnemyCharacter() override;

  void ConfigureEnemy(uint64 InCombatantId, bool bInElite, uint32 InItemLevel);
  bool ReceivePlayerAbility(ULostsenseRuntimeSubsystem &Runtime,
                            uint32 AbilityId);
  bool ReceivePlayerLoadoutSlot(ULostsenseRuntimeSubsystem &Runtime,
                                int32 SlotIndex);

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  bool IsDefeated() const;

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaSeconds) override;

private:
  void HandleDefeat(ULostsenseRuntimeSubsystem &Runtime);

  struct FPortableEnemy;
  TUniquePtr<FPortableEnemy> PortableEnemy;
  uint64 PendingCombatantId = 1001U;
  uint32 ItemLevel = 1U;
  bool bElite = false;
  bool bDefeatHandled = false;
  float AttackCooldownRemaining = 0.0F;
};
