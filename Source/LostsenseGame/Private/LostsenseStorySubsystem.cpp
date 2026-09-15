#include "LostsenseStorySubsystem.h"

#include "Lostsense/Gameplay/Story/StoryRuntime.h"

#include <string>

namespace {
Lostsense::Gameplay::FirstSliceStoryBeat
ToPortableBeat(const ELostsenseStoryBeat Beat) {
  return static_cast<Lostsense::Gameplay::FirstSliceStoryBeat>(
      static_cast<uint8>(Beat));
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
  return IsStoryReady() &&
         StoryRuntime->Story.CompleteBeat(ToPortableBeat(Beat));
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
