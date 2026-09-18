#include "Lostsense/Gameplay/Story/CampaignProgressRuntime.h"
#include "TestHarness.h"

#include <string>

int main() {
  using namespace Lostsense::Gameplay;
  Lostsense::Tests::TestSuite test{"Campaign progress"};

  CampaignProgressRuntime runtime;
  test.Expect(runtime.CurrentQuestId() == 0U,
              "continuation campaign starts locked");
  test.Expect(runtime.Begin(), "Act III continuation can begin once");
  test.Expect(runtime.CurrentQuestId() == 80020U,
              "Act III begins at Crownless Reach");
  test.Expect(runtime.Objective(80020U) == ObjectiveState::Active,
              "first Act III quest becomes active");
  test.Expect(runtime.Objective(80021U) == ObjectiveState::Locked,
              "future quest remains locked");
  test.Expect(!runtime.CompleteQuest(80021U),
              "campaign rejects out-of-order completion");
  test.Expect(runtime.CompleteQuest(80020U),
              "active quest completes deterministically");
  test.Expect(runtime.Objective(80020U) == ObjectiveState::Completed &&
                  runtime.Objective(80021U) == ObjectiveState::Active,
              "completion advances exactly one quest");

  std::string payload;
  test.Expect(CampaignProgressCodec::Serialize(runtime.CaptureState(), payload),
              "campaign state serializes");
  CampaignProgressState decoded;
  test.Expect(CampaignProgressCodec::Deserialize(payload, decoded),
              "campaign state deserializes");
  CampaignProgressRuntime restored;
  test.Expect(restored.RestoreState(decoded) &&
                  restored.CurrentQuestId() == 80021U,
              "campaign continuation restores without drift");

  for (std::uint32_t index = 12U;
       index < CampaignCatalog::Quests().size(); ++index) {
    test.Expect(restored.CompleteQuest(CampaignCatalog::Quests()[index].Id),
                "remaining canonical quest completes in order");
  }
  test.Expect(restored.IsFinished() && restored.CurrentQuestId() == 0U,
              "What Remains closes the campaign");
  test.Expect(!restored.CompleteQuest(80084U),
              "finished campaign cannot complete twice");

  CampaignProgressState invalid{1U, true, 10U};
  test.Expect(!restored.RestoreState(invalid),
              "restore rejects pre-Act-III continuation index");
  return test.Finish();
}
