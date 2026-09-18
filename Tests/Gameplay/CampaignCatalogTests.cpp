#include "Lostsense/Gameplay/Story/CampaignCatalog.h"
#include "TestHarness.h"

int main() {
  using namespace Lostsense::Gameplay;
  Lostsense::Tests::TestSuite test{"Campaign catalog"};

  test.Expect(CampaignCatalog::IsCanonical(),
              "campaign catalog preserves canonical ordering");
  test.Expect(CampaignCatalog::Quests().size() == 47U,
              "all 47 main campaign quests are registered");
  test.Expect(CampaignCatalog::Regions().size() == 15U,
              "all 15 canonical regions are registered");

  const auto odran = CampaignCatalog::FindQuest(80006U);
  test.Expect(odran.has_value() && odran->BossId == 30001U,
              "Act I boss mapping remains canonical");

  const auto actTwo = CampaignCatalog::NextQuest(80006U);
  test.Expect(actTwo.has_value() && actTwo->Id == 80010U,
              "Act II follows Act I without inventing an intermediate quest");

  const auto actFour = CampaignCatalog::FindQuest(80030U);
  test.Expect(actFour.has_value() &&
                  actFour->Act == CampaignAct::GoldRemembers,
              "Pump Cathedral campaign authority remains Act IV");

  const auto finale = CampaignCatalog::FindQuest(80083U);
  test.Expect(finale.has_value() && finale->BossId == 30060U,
              "Aster remains the final combat boss");

  test.Expect(!CampaignCatalog::NextQuest(80084U).has_value(),
              "What Remains is the final campaign quest");
  return test.Finish();
}
