#pragma once

#include "CoreMinimal.h"
#include "LostsenseEnemyCharacter.h"
#include "Subsystems/WorldSubsystem.h"

#include "LostsenseEnemyPresentationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ELostsenseEnemyPresentationCueKind : uint8 {
  StateChanged,
  AttackTelegraph,
  OdranBellGesture,
  BossPhaseTransition,
  Defeat
};

USTRUCT(BlueprintType)
struct FLostsenseEnemyPresentationCue {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  int64 CombatantId = 0;

  UPROPERTY(BlueprintReadOnly)
  ELostsenseEnemyArchetype Archetype =
      ELostsenseEnemyArchetype::CharterDeserter;

  UPROPERTY(BlueprintReadOnly)
  ELostsenseEnemyAiState AiState = ELostsenseEnemyAiState::Idle;

  UPROPERTY(BlueprintReadOnly)
  ELostsenseEnemyPresentationCueKind Kind =
      ELostsenseEnemyPresentationCueKind::StateChanged;

  UPROPERTY(BlueprintReadOnly)
  int32 BossPhase = 0;

  UPROPERTY(BlueprintReadOnly)
  float SuggestedDurationSeconds = 0.0F;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FLostsenseEnemyPresentationCueDelegate,
    const FLostsenseEnemyPresentationCue &, Cue);

UCLASS()
class LOSTSENSEGAME_API ULostsenseEnemyPresentationSubsystem final
    : public UTickableWorldSubsystem {
  GENERATED_BODY()

public:
  virtual void Tick(float DeltaTime) override;
  virtual TStatId GetStatId() const override;

  UPROPERTY(BlueprintAssignable, Category = "Lostsense|Presentation")
  FLostsenseEnemyPresentationCueDelegate OnPresentationCue;

private:
  struct FObservedEnemy final {
    ELostsenseEnemyAiState AiState = ELostsenseEnemyAiState::Idle;
    int32 BossPhase = 0;
    bool bDefeated = false;
    bool bInitialized = false;
  };

  void EmitCue(const ALostsenseEnemyCharacter &Enemy,
               ELostsenseEnemyPresentationCueKind Kind,
               float SuggestedDurationSeconds);

  TMap<uint64, FObservedEnemy> ObservedEnemies;
};
