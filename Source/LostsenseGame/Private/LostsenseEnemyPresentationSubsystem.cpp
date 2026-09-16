#include "LostsenseEnemyPresentationSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"

void ULostsenseEnemyPresentationSubsystem::Tick(const float DeltaTime) {
  static_cast<void>(DeltaTime);
  UWorld *World = GetWorld();
  if (World == nullptr || !World->IsGameWorld()) {
    return;
  }

  TSet<uint64> SeenCombatants;
  for (TActorIterator<ALostsenseEnemyCharacter> It(World); It; ++It) {
    ALostsenseEnemyCharacter *Enemy = *It;
    if (Enemy == nullptr) {
      continue;
    }

    const uint64 CombatantId = Enemy->GetCombatantId();
    if (CombatantId == 0U) {
      continue;
    }
    SeenCombatants.Add(CombatantId);

    FObservedEnemy &Observed = ObservedEnemies.FindOrAdd(CombatantId);
    const ELostsenseEnemyAiState CurrentState = Enemy->GetAiState();
    const int32 CurrentPhase = Enemy->GetBossPhase();
    const bool bCurrentDefeated = Enemy->IsDefeated();

    if (!Observed.bInitialized) {
      Observed.AiState = CurrentState;
      Observed.BossPhase = CurrentPhase;
      Observed.bDefeated = bCurrentDefeated;
      Observed.bInitialized = true;
      continue;
    }

    if (CurrentState != Observed.AiState) {
      EmitCue(*Enemy, ELostsenseEnemyPresentationCueKind::StateChanged, 0.0F);
      if (CurrentState == ELostsenseEnemyAiState::Windup) {
        EmitCue(*Enemy, ELostsenseEnemyPresentationCueKind::AttackTelegraph,
                Enemy->IsBoss() ? 0.78F : 0.55F);
        if (Enemy->GetArchetype() == ELostsenseEnemyArchetype::Odran) {
          EmitCue(*Enemy, ELostsenseEnemyPresentationCueKind::OdranBellGesture,
                  CurrentPhase >= 2 ? 0.58F : 0.78F);
        }
      }
      Observed.AiState = CurrentState;
    }

    if (Enemy->IsBoss() && CurrentPhase != Observed.BossPhase) {
      EmitCue(*Enemy,
              ELostsenseEnemyPresentationCueKind::BossPhaseTransition, 1.15F);
      Observed.BossPhase = CurrentPhase;
    }

    if (bCurrentDefeated && !Observed.bDefeated) {
      EmitCue(*Enemy, ELostsenseEnemyPresentationCueKind::Defeat,
              Enemy->IsBoss() ? 8.0F : 4.0F);
    }
    Observed.bDefeated = bCurrentDefeated;
  }

  TArray<uint64> RemovedCombatants;
  for (const TPair<uint64, FObservedEnemy> &Entry : ObservedEnemies) {
    if (!SeenCombatants.Contains(Entry.Key)) {
      RemovedCombatants.Add(Entry.Key);
    }
  }
  for (const uint64 CombatantId : RemovedCombatants) {
    ObservedEnemies.Remove(CombatantId);
  }
}

TStatId ULostsenseEnemyPresentationSubsystem::GetStatId() const {
  RETURN_QUICK_DECLARE_CYCLE_STAT(ULostsenseEnemyPresentationSubsystem,
                                  STATGROUP_Tickables);
}

void ULostsenseEnemyPresentationSubsystem::EmitCue(
    const ALostsenseEnemyCharacter &Enemy,
    const ELostsenseEnemyPresentationCueKind Kind,
    const float SuggestedDurationSeconds) {
  FLostsenseEnemyPresentationCue Cue;
  Cue.CombatantId = static_cast<int64>(Enemy.GetCombatantId());
  Cue.Archetype = Enemy.GetArchetype();
  Cue.AiState = Enemy.GetAiState();
  Cue.Kind = Kind;
  Cue.BossPhase = Enemy.GetBossPhase();
  Cue.SuggestedDurationSeconds = FMath::Max(0.0F, SuggestedDurationSeconds);
  OnPresentationCue.Broadcast(Cue);
}
