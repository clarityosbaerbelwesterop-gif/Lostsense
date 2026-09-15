#include "Lostsense/Gameplay/Story/StoryRuntime.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void Require(const bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}
} // namespace

int main() {
  using namespace Lostsense::Gameplay;

  FirstSliceStoryRuntime story{BuildFirstSliceObjectives()};
  Require(story.IsValid(), "canonical first-slice story must validate");
  Require(story.Objective(80001U) == ObjectiveState::Locked,
          "opening objective waits for Returned awakening");

  Require(story.CompleteBeat(FirstSliceStoryBeat::ReturnedAwakened),
          "Returned awakening commits once");
  Require(!story.CompleteBeat(FirstSliceStoryBeat::ReturnedAwakened),
          "story beats are idempotent and cannot double-complete");
  Require(story.Objective(80001U) == ObjectiveState::Active,
          "Returned objective activates after awakening");

  Require(story.CompleteBeat(FirstSliceStoryBeat::MetMara),
          "Mara interaction advances story");
  Require(story.Objective(80001U) == ObjectiveState::Completed,
          "Mara completes Returned objective");
  Require(story.Objective(80002U) == ObjectiveState::Locked,
          "Bell investigation remains locked until Hadrun");
  Require(story.CompleteBeat(FirstSliceStoryBeat::MetHadrun),
          "Hadrun interaction advances story");
  Require(story.Objective(80002U) == ObjectiveState::Active,
          "Bell investigation activates after both NPC meetings");

  Require(story.CompleteBeat(FirstSliceStoryBeat::BellgraveDepartureAllowed),
          "Bellgrave road can unlock");
  Require(story.CompleteBeat(FirstSliceStoryBeat::EnteredRavelwood),
          "Ravelwood entry records");
  Require(story.Objective(80003U) == ObjectiveState::Active,
          "Ninth Descent plate objective activates on route");

  const FirstSliceStoryState checkpoint = story.CaptureState();
  std::string payload;
  Require(FirstSliceStoryCodec::Serialize(checkpoint, payload),
          "story state serializes");

  FirstSliceStoryState decoded;
  Require(FirstSliceStoryCodec::Deserialize(payload, decoded),
          "story state deserializes");
  Require(decoded == checkpoint, "story codec round-trips exactly");

  FirstSliceStoryRuntime restored{BuildFirstSliceObjectives()};
  Require(restored.RestoreState(decoded), "valid story state restores");
  Require(restored.CaptureState() == checkpoint,
          "restored story state matches checkpoint");

  Require(restored.CompleteBeat(FirstSliceStoryBeat::FoundNinthDescentPlate),
          "Ninth Descent plate can be recovered");
  Require(restored.CompleteBeat(FirstSliceStoryBeat::EnteredUpperVaur),
          "Upper Vaur entry records");
  Require(restored.CompleteBeat(FirstSliceStoryBeat::EnteredCoinlessShaft),
          "Coinless Shaft entry records");
  Require(restored.CompleteBeat(FirstSliceStoryBeat::OdranEncounterStarted),
          "Odran encounter starts");
  Require(restored.CompleteBeat(FirstSliceStoryBeat::OdranDefeated),
          "Odran defeat records once");
  Require(!restored.CompleteBeat(FirstSliceStoryBeat::OdranDefeated),
          "Odran defeat cannot double-complete");
  Require(restored.Objective(80006U) == ObjectiveState::Locked,
          "return objective waits for Ninth Descent record");
  Require(
      restored.CompleteBeat(FirstSliceStoryBeat::NinthDescentRecordRecovered),
      "Ninth Descent record can be recovered");
  Require(restored.Objective(80006U) == ObjectiveState::Active,
          "return objective activates after boss and record");
  Require(restored.CompleteBeat(FirstSliceStoryBeat::BellgraveChanged),
          "changed Bellgrave records");
  Require(restored.Objective(80006U) == ObjectiveState::Completed,
          "Act I slice objective completes on changed Bellgrave");

  FirstSliceStoryState corrupt = checkpoint;
  corrupt.SchemaVersion = 99U;
  Require(!restored.RestoreState(corrupt),
          "future story schema is rejected transactionally");
  Require(restored.HasBeat(FirstSliceStoryBeat::BellgraveChanged),
          "failed restore does not mutate live story");

  std::string malformed = payload + "TRAILING\n";
  Require(!FirstSliceStoryCodec::Deserialize(malformed, decoded),
          "trailing story payload is rejected");
  Require(!FirstSliceStoryCodec::Deserialize("LOSTSENSE_STORY 1\nBEATS 2\n",
                                             decoded),
          "malformed story payload is rejected");

  std::cout << "StoryRuntimeTests passed\n";
  return 0;
}
