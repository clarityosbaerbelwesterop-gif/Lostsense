#include "LostsenseStorySubsystem.h"

#include "Lostsense/Gameplay/Story/StoryRuntime.h"
#include "Lostsense/Gameplay/World/DeepRouteState.h"

#include <string>

namespace {
Lostsense::Gameplay::FirstSliceStoryBeat
ToPortableBeat(const ELostsenseStoryBeat Beat) {
  return static_cast<Lostsense::Gameplay::FirstSliceStoryBeat>(
      static_cast<uint8>(Beat));
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
  if (!IsStoryReady() || !StoryRuntime->Story.CompleteBeat(ToPortableBeat(Beat))) {
    return false;
  }
  if (Beat == ELostsenseStoryBeat::OdranDefeated) {
    return StoryRuntime->DeepRoute.Complete(
        Lostsense::Gameplay::DeepRouteMilestone::OdranDefeated);
  }
  return true;
}

ELostsenseObjectiveState
ULostsenseStorySubsystem::GetObjectiveState(const int32 ObjectiveId) const {
  if (!IsStoryReady() || ObjectiveId <= 0) {
    return ELostsenseObjectiveState::Locked;
  }
  return ToPresentationState(
      StoryRuntime->Story.Objective(static_cast<uint32>(ObjectiveId)));
}

int32 ULostsenseStorySubsystem::GetCurrentObjectiveId() const {
  if (!IsStoryReady()) {
    return 0;
  }
  constexpr int32 ObjectiveIds[] = {80001, 80002, 80003, 80004, 80005, 80006};
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
  return IsStoryReady() && StoryRuntime->DeepRoute.Complete(
                               ToPortableMilestone(Milestone));
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
  return IsStoryReady() && StoryRuntime->DeepRoute.SetMechanism(
                               ToPortableMechanism(Mechanism),
                               ToPortableMechanismState(State));
}

bool ULostsenseStorySubsystem::SaveStoryToText(FString &OutPayload) const {
  if (!IsStoryReady()) {
    return false;
  }
  std::string Payload;
  if (!Lostsense::Gameplay::FirstSliceStoryCodec::Serialize(
          StoryRuntime->Story.CaptureState(), Payload)) {
    return false;
  }
  OutPayload = UTF8_TO_TCHAR(Payload.c_str());
  return true;
}

bool ULostsenseStorySubsystem::LoadStoryFromText(const FString &Payload) {
  if (!IsStoryReady()) {
    return false;
  }
  Lostsense::Gameplay::FirstSliceStoryState State;
  const std::string Utf8Payload(TCHAR_TO_UTF8(*Payload));
  return Lostsense::Gameplay::FirstSliceStoryCodec::Deserialize(Utf8Payload,
                                                                State) &&
         StoryRuntime->Story.RestoreState(State);
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
  const auto Candidate = Lostsense::Gameplay::DeepRouteState::Deserialize(Utf8Payload);
  if (!Candidate.has_value()) {
    return false;
  }
  if (Candidate->IsComplete(Lostsense::Gameplay::DeepRouteMilestone::OdranDefeated) !=
      StoryRuntime->Story.HasBeat(
          Lostsense::Gameplay::FirstSliceStoryBeat::OdranDefeated)) {
    return false;
  }
  StoryRuntime->DeepRoute = *Candidate;
  return true;
}
