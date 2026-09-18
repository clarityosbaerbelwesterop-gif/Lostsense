#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Lostsense::Gameplay {

enum class CampaignEnding : std::uint8_t {
  None = 0U,
  Sever = 1U,
  Bind = 2U,
  Scatter = 3U
};

struct CampaignEndingState final {
  std::uint32_t Version = 1U;
  CampaignEnding Ending = CampaignEnding::None;
};

class CampaignEndingRuntime final {
public:
  [[nodiscard]] bool Resolve(CampaignEnding ending) noexcept;
  [[nodiscard]] CampaignEnding Ending() const noexcept;
  [[nodiscard]] CampaignEndingState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(const CampaignEndingState &state) noexcept;

private:
  CampaignEnding Ending_ = CampaignEnding::None;
};

class CampaignEndingCodec final {
public:
  [[nodiscard]] static bool Serialize(const CampaignEndingState &state,
                                      std::string &out);
  [[nodiscard]] static bool Deserialize(std::string_view payload,
                                        CampaignEndingState &out);
};

} // namespace Lostsense::Gameplay
