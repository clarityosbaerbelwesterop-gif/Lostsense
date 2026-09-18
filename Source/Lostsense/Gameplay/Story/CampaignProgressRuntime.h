#pragma once

#include "CampaignCatalog.h"
#include "StoryRuntime.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Lostsense::Gameplay {

struct CampaignProgressState final {
  std::uint32_t SchemaVersion{1U};
  bool Started{false};
  std::uint32_t NextQuestIndex{0U};

  friend bool operator==(const CampaignProgressState &,
                         const CampaignProgressState &) = default;
};

class CampaignProgressRuntime final {
public:
  static constexpr std::uint32_t CurrentSchemaVersion = 1U;
  static constexpr std::uint32_t FirstContinuationQuestIndex = 11U;

  [[nodiscard]] bool Begin();
  [[nodiscard]] bool CompleteQuest(std::uint32_t questId);
  [[nodiscard]] ObjectiveState Objective(std::uint32_t questId) const noexcept;
  [[nodiscard]] std::uint32_t CurrentQuestId() const noexcept;
  [[nodiscard]] bool IsStarted() const noexcept { return State_.Started; }
  [[nodiscard]] bool IsFinished() const noexcept;
  [[nodiscard]] CampaignProgressState CaptureState() const noexcept {
    return State_;
  }
  [[nodiscard]] bool RestoreState(const CampaignProgressState &state) noexcept;

private:
  [[nodiscard]] static bool
  ValidState(const CampaignProgressState &state) noexcept;

  CampaignProgressState State_{};
};

class CampaignProgressCodec final {
public:
  [[nodiscard]] static bool Serialize(const CampaignProgressState &state,
                                      std::string &output);
  [[nodiscard]] static bool Deserialize(std::string_view text,
                                        CampaignProgressState &output);
};

} // namespace Lostsense::Gameplay
