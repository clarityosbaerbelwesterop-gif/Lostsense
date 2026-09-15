#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "LostsenseEnemyCharacter.generated.h"

class ULostsenseRuntimeSubsystem;

UENUM(BlueprintType)
enum class ELostsenseEnemyArchetype : uint8 {
  BellMaddenedCarrion,
  CharterDeserter,
  EchoMiner,
  HaulConstruct,
  ForemanKett,
  Odran
};

UENUM(BlueprintType)
enum class ELostsenseEnemyAiState : uint8 {
  Idle,
  Alert,
  Approach,
  Windup,
  Attack,
  Recover,
  Stagger,
  Dead
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseEnemyCharacter final : public ACharacter {
  GENERATED_BODY()

public:
  ALostsenseEnemyCharacter();
  virtual ~ALostsenseEnemyCharacter() override;

  void ConfigureEnemy(uint64 InCombatantId, bool bInElite, uint32 InItemLevel);
  void ConfigureEnemyArchetype(uint64 InCombatantId,
                               ELostsenseEnemyArchetype InArchetype,
                               uint32 InItemLevel);
  void ConfigureOdranBoss(uint64 InCombatantId, uint32 InItemLevel);
  bool ReceivePlayerAbility(ULostsenseRuntimeSubsystem &Runtime,
                            uint32 AbilityId);
  bool ReceivePlayerLoadoutSlot(ULostsenseRuntimeSubsystem &Runtime,
                                int32 SlotIndex);

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  bool IsDefeated() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  bool IsBoss() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  double GetCurrentHealth() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  double GetMaximumHealth() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  int32 GetBossPhase() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  ELostsenseEnemyArchetype GetArchetype() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Enemy")
  ELostsenseEnemyAiState GetAiState() const;

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaSeconds) override;

private:
  void EnterState(ELostsenseEnemyAiState NewState, float Duration = 0.0F);
  void TickLivingAi(float DeltaSeconds, ULostsenseRuntimeSubsystem &Runtime,
                    ACharacter &Player);
  void BeginAttack(double DistanceToPlayer);
  void ResolveCommittedAttack(ULostsenseRuntimeSubsystem &Runtime,
                              double DistanceToPlayer);
  void CommandNearbyConstructs();
  void HandleDefeat(ULostsenseRuntimeSubsystem &Runtime);

  struct FPortableEnemy;
  TUniquePtr<FPortableEnemy> PortableEnemy;
  uint64 PendingCombatantId = 1001U;
  uint32 ItemLevel = 1U;
  ELostsenseEnemyArchetype Archetype =
      ELostsenseEnemyArchetype::CharterDeserter;
  ELostsenseEnemyAiState AiState = ELostsenseEnemyAiState::Idle;
  bool bElite = false;
  bool bBoss = false;
  bool bDefeatHandled = false;
  bool bBossTransitionCommitted = false;
  bool bAttackCommitted = false;
  int32 BossPhase = 1;
  int32 AttackPatternIndex = 0;
  float StateTimeRemaining = 0.0F;
  float AttackCooldownRemaining = 0.0F;
  float BossPulseCooldownRemaining = 2.5F;
  float CommandCooldownRemaining = 3.0F;
};
