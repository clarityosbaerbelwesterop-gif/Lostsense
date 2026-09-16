#include "Lostsense/Gameplay/Story/ActTwoStoryRuntime.h"
#include "TestHarness.h"

using namespace Lostsense::Gameplay;

int main() {
  Lostsense::Tests::TestSuite test{"Act II story runtime"};

  FirstSliceStoryRuntime actOne{BuildFirstSliceObjectives()};
  ActTwoStoryRuntime actTwo;
  test.Expect(!actTwo.UnlockFromActOne(actOne),
              "Act II remains locked before changed Bellgrave");
  test.Expect(actTwo.Objective(80010U) == ObjectiveState::Locked,
              "Black Sap is locked before Act I completion");

  for (std::uint32_t index = 0;
       index <=
       static_cast<std::uint32_t>(FirstSliceStoryBeat::BellgraveChanged);
       ++index) {
    test.Expect(actOne.CompleteBeat(static_cast<FirstSliceStoryBeat>(index)),
                "Act I canonical beats complete in order");
  }
  test.Expect(actTwo.UnlockFromActOne(actOne),
              "changed Bellgrave unlocks Act II");
  test.Expect(actTwo.Objective(80010U) == ObjectiveState::Active,
              "Black Sap becomes the first active Act II objective");
  test.Expect(!actTwo.CompleteBeat(ActTwoStoryBeat::GiltfenRootTunnelOpened),
              "Act II rejects skipped beats");

  test.Expect(actTwo.CompleteBeat(ActTwoStoryBeat::BlackSapTrailFound),
              "black sap trail can be established");
  test.Expect(actTwo.CompleteBeat(ActTwoStoryBeat::DomaIreMet),
              "Doma Ire meeting completes Black Sap");
  test.Expect(actTwo.Objective(80010U) == ObjectiveState::Completed &&
                  actTwo.Objective(80011U) == ObjectiveState::Active,
              "Lives in Bark activates after Black Sap");
  test.Expect(actTwo.CompleteBeat(ActTwoStoryBeat::InfectedVillagerStabilized),
              "nonlethal villager stabilization advances Act II");
  test.Expect(actTwo.CompleteBeat(ActTwoStoryBeat::GiltfenRootTunnelOpened),
              "root tunnel opens only after stabilization");
  test.Expect(actTwo.CompleteBeat(ActTwoStoryBeat::ThornChoirDiscovered),
              "Thorn Choir discovery advances authored order");
  test.Expect(actTwo.CompleteBeat(ActTwoStoryBeat::MotherVeyrDefeated),
              "Mother Veyr defeat precedes witness-root choice");
  test.Expect(!actTwo.CompleteBeat(ActTwoStoryBeat::WitnessRootResolved),
              "root resolution requires an explicit decision");
  test.Expect(actTwo.ResolveWitnessRoot(WitnessRootDecision::Preserve),
              "preserve decision resolves Borrowed Mother");
  test.Expect(actTwo.Objective(80014U) == ObjectiveState::Completed,
              "Borrowed Mother completes after boss and root decision");

  std::string payload;
  test.Expect(ActTwoStoryCodec::Serialize(actTwo.CaptureState(), payload),
              "Act II state serializes");
  ActTwoStoryState restored;
  test.Expect(ActTwoStoryCodec::Deserialize(payload, restored),
              "Act II state deserializes");
  ActTwoStoryRuntime restoredRuntime;
  test.Expect(restoredRuntime.RestoreState(restored) &&
                  restoredRuntime.RootDecision() ==
                      WitnessRootDecision::Preserve,
              "Act II root decision round trips deterministically");

  ActTwoStoryState impossible{true, 0b101U, WitnessRootDecision::None};
  test.Expect(!restoredRuntime.RestoreState(impossible),
              "Act II restore rejects beat gaps");

  return test.Finish();
}
