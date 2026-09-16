#include "LostsenseStorySubsystem.h"

#include "Lostsense/Gameplay/Story/ActTwoStoryRuntime.h"
#include "Lostsense/Gameplay/Story/StoryRuntime.h"
#include "Lostsense/Gameplay/World/DeepRouteState.h"

#include <string>
#include <string_view>

namespace {
constexpr std::string_view DeepRouteMarker{"LOSTSENSE_DEEP_ROUTE\n"};
constexpr std::string_view ActTwoMarker{"\nLOSTSENSE_ACT_TWO\n"};

Lostsense::Gameplay::FirstSliceStoryBeat
ToPortableBeat(const ELostsenseStoryBeat Beat) {
  return static_cast<Lostsense::Gameplay::FirstSliceStoryBeat>(
      static_cast<uint8>(Beat));
}

Lostsense::Gameplay::ActTwoStoryBeat
ToPortableActTwoBeat(const ELostsenseActTwoStoryBeat Beat) {
  return static_cast<Lostsense::Gameplay::ActTwoStoryBeat>(
      static_cast<uint8>(Beat));
}

Lostsense::Gameplay::WitnessRootDecision
ToPortableRootDecision(const ELostsenseWitnessRootDecision Decision) {
  return static_cast<Lostsense::Gameplay::WitnessRootDecision>(
      static_cast<uint8>(Decision));
}

Lostsense::Gameplay::DeepRouteMilestone
ToPortableMilestone(const ELostsenseDeepRouteMilestone Milestone) {
  return static_cast<Lostsense::Gameplay::DeepRouteMilestone>(
      static_cast<uint8>(Milestone));
}

Lostsense::Gameplay::DeepMechanismId
ToPortableMechanism(const ELostsenseDeepMechanism Mechanism) {
  return static_cast<Lostsense::Gameplay::DeepMechanismId>(
      static_cast<uint8>(Mechanism));
}

Lostsense::Gameplay::DeepMechanismState
ToPortableMechanismState(const ELostsenseDeepMechanismState State) {
  return static_cast<Lostsense::Gameplay::DeepMechanismState>(
      static_cast<uint8>(State));
}

ELostsenseDeepMechanismState ToPresentationMechanismState(
    const Lostsense::Gameplay::DeepMechanismState State) {
  return static_cast<ELostsenseDeepMechanismState>(static_cast<uint8>(State));
}

ELostsenseObjectiveState
ToPresentationState(const Lostsense::Gameplay::ObjectiveState State) {
  switch (State) {
  case Lostsense::Gameplay::ObjectiveState::Locked:
    return ELostsenseObjectiveState::Locked;
  case Lostsense::Gameplay::ObjectiveState::Active:
    return ELostsenseObjectiveState::Active;
  case Lostsense::Gameplay::ObjectiveState::Completed:
    return ELostsenseObjectiveState::Completed;
  }
  return ELostsenseObjectiveState::Locked;
}
} // namespace

struct ULostsenseStorySubsystem::FStoryRuntime {
  Lostsense::Gameplay::FirstSliceStoryRuntime Story{
      Lostsense::Gameplay::BuildFirstSliceObjectives()};
  Lostsense::Gameplay::ActTwoStoryRuntime ActTwo;
  Lostsense::Gameplay::DeepRouteState DeepRoute;
};

ULostsenseStorySubsystem::ULostsenseStorySubsystem() = default;
ULostsenseStorySubsystem::~ULostsenseStorySubsystem() = default;

void ULostsenseStorySubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  StoryRuntime = MakeUnique<FStoryRuntime>();
}

void ULostsenseStorySubsystem::Deinitialize() {
  StoryRuntime.Reset();
  Super::Deinitialize();
}

bool ULostsenseStorySubsystem::IsStoryReady() const {
  return StoryRuntime.IsValid() && StoryRuntime->Story.IsValid();
}

bool ULostsenseStorySubsystem::HasBeat(const ELostsenseStoryBeat Beat) const {
  return IsStoryReady() && StoryRuntime->Story.HasBeat(ToPortableBeat(Beat));
}

bool ULostsenseStorySubsystem::CompleteBeat(const ELostsenseStoryBeat Beat) {
  if (!IsStoryReady() ||
      !StoryRuntime->Story.CompleteBeat(ToPortableBeat(Beat))) {
    return false;
  }
  if (Beat == ELostsenseStoryBeat::OdranDefeated &&
      !StoryRuntime->DeepRoute.Complete(
          Lostsense::Gameplay::DeepRouteMilestone::OdranDefeated)) {
    return false;
  }
  if (Beat == ELostsenseStoryBeat::BellgraveChanged) {
    return StoryRuntime->ActTwo.UnlockFromActOne(StoryRuntime->Story);
  }
  return true;
}

bool ULostsenseStorySubsystem::HasActTwoBeat(
    const ELostsenseActTwoStoryBeat Beat) const {
  return IsStoryReady() &&
         StoryRuntime->ActTwo.HasBeat(ToPortableActTwoBeat(Beat));
}

bool ULostsenseStorySubsystem::CompleteActTwoBeat(
    const ELostsenseActTwoStoryBeat Beat) {
  return IsStoryReady() &&
         StoryRuntime->ActTwo.CompleteBeat(ToPortableActTwoBeat(Beat));
}

bool ULostsenseStorySubsystem::ResolveWitnessRoot(
    const ELostsenseWitnessRootDecision Decision) {
  return IsStoryReady() && StoryRuntime->ActTwo.ResolveWitnessRoot(
                               ToPortableRootDecision(Decision));
}

ELostsenseObjectiveState
ULostsenseStorySubsystem::GetObjectiveState(const int32 ObjectiveId) const {
  if (!IsStoryReady() || ObjectiveId <= 0) {
    return ELostsenseObjectiveState::Locked;
  }
  if (ObjectiveId >= 80010 && ObjectiveId <= 80014) {
    return ToPresentationState(
        StoryRuntime->ActTwo.Objective(static_cast<uint32>(ObjectiveId)));
  }
  return ToPresentationState(
      StoryRuntime->Story.Objective(static_cast<uint32>(ObjectiveId)));
}

int32 ULostsenseStorySubsystem::GetCurrentObjectiveId() const {
  if (!IsStoryReady()) {
    return 0;
  }
  constexpr int32 ObjectiveIds[] = {80001, 80002, 80003, 80004, 80005, 80006,
                                    80010, 80011, 80012, 80013, 80014};
  for (const int32 Id : ObjectiveIds) {
    if (GetObjectiveState(Id) == ELostsenseObjectiveState::Active) {
      return Id;
    }
  }
  return 0;
}

bool ULostsenseStorySubsystem::HasDeepRouteMilestone(
    const ELostsenseDeepRouteMilestone Milestone) const {
  return IsStoryReady() &&
         StoryRuntime->DeepRoute.IsComplete(ToPortableMilestone(Milestone));
}

bool ULostsenseStorySubsystem::CompleteDeepRouteMilestone(
    const ELostsenseDeepRouteMilestone Milestone) {
  return IsStoryReady() &&
         StoryRuntime->DeepRoute.Complete(ToPortableMilestone(Milestone));
}

ELostsenseDeepMechanismState ULostsenseStorySubsystem::GetDeepMechanismState(
    const ELostsenseDeepMechanism Mechanism) const {
  if (!IsStoryReady()) {
    return ELostsenseDeepMechanismState::Locked;
  }
  return ToPresentationMechanismState(
      StoryRuntime->DeepRoute.Mechanism(ToPortableMechanism(Mechanism)));
}

bool ULostsenseStorySubsystem::SetDeepMechanismState(
    const ELostsenseDeepMechanism Mechanism,
    const ELostsenseDeepMechanismState State) {
  return IsStoryReady() &&
         StoryRuntime->DeepRoute.SetMechanism(ToPortableMechanism(Mechanism),
                                              ToPortableMechanismState(State));
}

bool ULostsenseStorySubsystem::SaveStoryToText(FString &OutPayload) const {
  if (!IsStoryReady()) {
    return false;
  }
  std::string StoryPayload;
  std::string ActTwoPayload;
  if (!Lostsense::Gameplay::FirstSliceStoryCodec::Serialize(
          StoryRuntime->Story.CaptureState(), StoryPayload) ||
      !Lostsense::Gameplay::ActTwoStoryCodec::Serialize(
          StoryRuntime->ActTwo.CaptureState(), ActTwoPayload)) {
    return false;
  }
  StoryPayload.append(DeepRouteMarker);
  StoryPayload.append(StoryRuntime->DeepRoute.Serialize());
  StoryPayload.append(ActTwoMarker);
  StoryPayload.append(ActTwoPayload);
  OutPayload = UTF8_TO_TCHAR(StoryPayload.c_str());
  return true;
}

bool ULostsenseStorySubsystem::LoadStoryFromText(const FString &Payload) {
  if (!IsStoryReady()) {
    return false;
  }

  const std::string Utf8Payload(TCHAR_TO_UTF8(*Payload));
  const auto DeepPosition = Utf8Payload.find(DeepRouteMarker);
  if (DeepPosition == std::string::npos) {
    return false;
  }
  const auto DeepStart = DeepPosition + DeepRouteMarker.size();
  const auto ActTwoPosition = Utf8Payload.find(ActTwoMarker, DeepStart);

  const std::string_view StoryPayload(Utf8Payload.data(), DeepPosition);
  const std::string_view DeepPayload(
      Utf8Payload.data() + DeepStart,
      (ActTwoPosition == std::string::npos ? Utf8Payload.size() : ActTwoPosition) -
          DeepStart);

  Lostsense::Gameplay::FirstSliceStoryState StoryCandidate;
  const auto DeepCandidate =
      Lostsense::Gameplay::DeepRouteState::Deserialize(DeepPayload);
  if (!Lostsense::Gameplay::FirstSliceStoryCodec::Deserialize(StoryPayload,
                                                              StoryCandidate) ||
      !DeepCandidate.has_value()) {
    return false;
  }

  Lostsense::Gameplay::FirstSliceStoryRuntime ValidatedStory{
      Lostsense::Gameplay::BuildFirstSliceObjectives()};
  if (!ValidatedStory.RestoreState(StoryCandidate) ||
      ValidatedStory.HasBeat(
          Lostsense::Gameplay::FirstSliceStoryBeat::OdranDefeated) !=
          DeepCandidate->IsComplete(
              Lostsense::Gameplay::DeepRouteMilestone::OdranDefeated)) {
    return false;
  }

  Lostsense::Gameplay::ActTwoStoryRuntime ActTwoCandidate;
  if (ActTwoPosition == std::string::npos) {
    if (ValidatedStory.HasBeat(
            Lostsense::Gameplay::FirstSliceStoryBeat::BellgraveChanged) &&
        !ActTwoCandidate.UnlockFromActOne(ValidatedStory)) {
      return false;
    }
  } else {
    Lostsense::Gameplay::ActTwoStoryState State;
    const auto ActTwoStart = ActTwoPosition + ActTwoMarker.size();
    const std::string_view ActTwoPayload(Utf8Payload.data() + ActTwoStart,
                                         Utf8Payload.size() - ActTwoStart);
    if (!Lostsense::Gameplay::ActTwoStoryCodec::Deserialize(ActTwoPayload, State) ||
        !ActTwoCandidate.RestoreState(State) ||
        ActTwoCandidate.IsUnlocked() !=
            ValidatedStory.HasBeat(
                Lostsense::Gameplay::FirstSliceStoryBeat::BellgraveChanged)) {
      return false;
    }
  }

  if (!StoryRuntime->Story.RestoreState(StoryCandidate) ||
      !StoryRuntime->ActTwo.RestoreState(ActTwoCandidate.CaptureState())) {
    return false;
  }
  StoryRuntime->DeepRoute = *DeepCandidate;
  return true;
}

bool ULostsenseStorySubsystem::SaveDeepRouteToText(FString &OutPayload) const {
  if (!IsStoryReady()) {
    return false;
  }
  const std::string Payload = StoryRuntime->DeepRoute.Serialize();
  OutPayload = UTF8_TO_TCHAR(Payload.c_str());
  return true;
}

bool ULostsenseStorySubsystem::LoadDeepRouteFromText(const FString &Payload) {
  if (!IsStoryReady()) {
    return false;
  }
  const std::string Utf8Payload(TCHAR_TO_UTF8(*Payload));
  const auto Candidate =
      Lostsense::Gameplay::DeepRouteState::Deserialize(Utf8Payload);
  if (!Candidate.has_value() ||
      Candidate->IsComplete(
          Lostsense::Gameplay::DeepRouteMilestone::OdranDefeated) !=
          StoryRuntime->Story.HasBeat(
              Lostsense::Gameplay::FirstSliceStoryBeat::OdranDefeated)) {
    return false;
  }
  StoryRuntime->DeepRoute = *Candidate;
  return true;
}
