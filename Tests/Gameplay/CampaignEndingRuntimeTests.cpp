#include "Lostsense/Gameplay/Story/CampaignEndingRuntime.h"
#include "TestHarness.h"

#include <string>

int main() {
  using namespace Lostsense::Gameplay;
  Lostsense::Tests::TestSuite test{"Campaign ending"};

  CampaignEndingRuntime runtime;
  test.Expect(runtime.Ending() == CampaignEnding::None,
              "ending begins unresolved");
  test.Expect(!runtime.Resolve(CampaignEnding::None),
              "None cannot resolve the ending");
  test.Expect(runtime.Resolve(CampaignEnding::Scatter),
              "explicit ending resolves");
  test.Expect(!runtime.Resolve(CampaignEnding::Sever),
              "ending cannot be overwritten");

  std::string payload;
  test.Expect(CampaignEndingCodec::Serialize(runtime.CaptureState(), payload),
              "ending serializes");
  CampaignEndingState decoded;
  test.Expect(CampaignEndingCodec::Deserialize(payload, decoded),
              "ending deserializes");
  CampaignEndingRuntime restored;
  test.Expect(restored.RestoreState(decoded) &&
                  restored.Ending() == CampaignEnding::Scatter,
              "ending round-trips exactly");
  test.Expect(!CampaignEndingCodec::Deserialize("END1|9", decoded),
              "invalid ending is rejected");
  return test.Finish();
}
