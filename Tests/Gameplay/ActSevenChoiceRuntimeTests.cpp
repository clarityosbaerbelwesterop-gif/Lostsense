#include "Lostsense/Gameplay/Story/ActSevenChoiceRuntime.h"
#include "TestHarness.h"

#include <string>

int main() {
  using namespace Lostsense::Gameplay;
  Lostsense::Tests::TestSuite test{"Act VII archive choice"};

  ActSevenChoiceRuntime runtime;
  test.Expect(runtime.Choice() == ActSevenArchiveChoice::None,
              "archive choice begins unresolved");
  test.Expect(!runtime.Resolve(ActSevenArchiveChoice::None),
              "None cannot resolve the choice");
  test.Expect(runtime.Resolve(ActSevenArchiveChoice::BellgraveCivilianArchive),
              "first explicit archive choice resolves");
  test.Expect(!runtime.Resolve(
                  ActSevenArchiveChoice::CrownlessCivilianArchive),
              "archive choice cannot be overwritten");

  std::string payload;
  test.Expect(ActSevenChoiceCodec::Serialize(runtime.CaptureState(), payload),
              "choice serializes");
  ActSevenChoiceState decoded;
  test.Expect(ActSevenChoiceCodec::Deserialize(payload, decoded),
              "choice deserializes");
  ActSevenChoiceRuntime restored;
  test.Expect(restored.RestoreState(decoded) &&
                  restored.Choice() ==
                      ActSevenArchiveChoice::BellgraveCivilianArchive,
              "choice round-trips exactly");
  test.Expect(!ActSevenChoiceCodec::Deserialize("A7C1|9", decoded),
              "invalid archive value is rejected");
  test.Expect(!ActSevenChoiceCodec::Deserialize("A7C2|1", decoded),
              "unknown codec version is rejected");
  return test.Finish();
}
