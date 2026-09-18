#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace Lostsense::Gameplay {

enum class CampaignAct : std::uint8_t {
  BellThatForgot = 1,
  RootsRememberBlood,
  CrownWithoutName,
  GoldRemembers,
  CityUnderSky,
  HollowMeridian,
  SecondSilence,
  RedArchive,
  FirstWitness,
};

struct CampaignQuestDefinition {
  std::uint32_t Id{};
  CampaignAct Act{CampaignAct::BellThatForgot};
  std::string_view Title;
  std::string_view Objective;
  std::uint32_t PrimaryDungeonId{};
  std::uint32_t BossId{};
};

struct CampaignRegionDefinition {
  std::uint32_t Id{};
  std::string_view Name;
  std::string_view Layer;
};

class CampaignCatalog final {
public:
  [[nodiscard]] static const std::vector<CampaignQuestDefinition> &Quests();
  [[nodiscard]] static const std::vector<CampaignRegionDefinition> &Regions();
  [[nodiscard]] static std::optional<CampaignQuestDefinition>
  FindQuest(std::uint32_t questId);
  [[nodiscard]] static std::optional<CampaignQuestDefinition>
  NextQuest(std::uint32_t questId);
  [[nodiscard]] static std::size_t QuestIndex(std::uint32_t questId) noexcept;
  [[nodiscard]] static bool IsCanonical() noexcept;
};

} // namespace Lostsense::Gameplay
