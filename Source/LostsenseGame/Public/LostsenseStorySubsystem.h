#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LostsenseStorySubsystem.generated.h"

UENUM(BlueprintType)
enum class ELostsenseStoryBeat : uint8 {
  ReturnedAwakened,
  MetMara,
  MetHadrun,
  BellgraveDepartureAllowed,
  EnteredRavelwood,
  FoundNinthDescentPlate,
  EnteredUpperVaur,
  EnteredCoinlessShaft,
  OdranEncounterStarted,
  OdranDefeated,
  NinthDescentRecordRecovered,
  BellgraveChanged
};

UENUM(BlueprintType)
enum class ELostsenseObjectiveState : uint8 { Locked, Active, Completed };

UENUM(BlueprintType)
enum class ELostsenseDeepRouteMilestone : uint8 {
  OdranDefeated,
  LanternRailDiscovered,
  ServiceBrakeRestored,
  RoyalThresholdOpened,
  VentilationNaveStabilized,
  ActFourClearanceGranted,
  PumpCathedralApproachOpened
};

UENUM(BlueprintType)
enum class ELostsenseDeepMechanism : uint8 {
  LanternRailSwitch,
  ServiceCageBrake,
  RoyalPressureDoor,
  VentilationIntake,
  ReliefVent,
  VaurReturnShortcut
};

UENUM(BlueprintType)
enum class ELostsenseDeepMechanismState : uint8 {
  Locked,
  Available,
  Primary,
  Alternate,
  Restored,
  Open
};

UCLASS()
class LOSTSENSEGAME_API ULostsenseStorySubsystem final
    : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  ULostsenseStorySubsystem();
  virtual ~ULostsenseStorySubsystem() override;

  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Story")
  bool IsStoryReady() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Story")
  bool HasBeat(ELostsenseStoryBeat Beat) const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Story")
  bool CompleteBeat(ELostsenseStoryBeat Beat);

  UFUNCTION(BlueprintPure, Category = "Lostsense|Story")
  ELostsenseObjectiveState GetObjectiveState(int32 ObjectiveId) const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Story")
  int32 GetCurrentObjectiveId() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|DeepRoute")
  bool HasDeepRouteMilestone(ELostsenseDeepRouteMilestone Milestone) const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|DeepRoute")
  bool CompleteDeepRouteMilestone(ELostsenseDeepRouteMilestone Milestone);

  UFUNCTION(BlueprintPure, Category = "Lostsense|DeepRoute")
  ELostsenseDeepMechanismState
  GetDeepMechanismState(ELostsenseDeepMechanism Mechanism) const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|DeepRoute")
  bool SetDeepMechanismState(ELostsenseDeepMechanism Mechanism,
                             ELostsenseDeepMechanismState State);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Story|Save")
  bool SaveStoryToText(FString &OutPayload) const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Story|Save")
  bool LoadStoryFromText(const FString &Payload);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|DeepRoute|Save")
  bool SaveDeepRouteToText(FString &OutPayload) const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|DeepRoute|Save")
  bool LoadDeepRouteFromText(const FString &Payload);

private:
  struct FStoryRuntime;
  TUniquePtr<FStoryRuntime> StoryRuntime;
};
