#include "CampaignEndingRuntime.h"

#include <charconv>

namespace Lostsense::Gameplay {

bool CampaignEndingRuntime::Resolve(const CampaignEnding ending) noexcept {
  if (Ending_ != CampaignEnding::None || ending == CampaignEnding::None) {
    return false;
  }
  Ending_ = ending;
  return true;
}

CampaignEnding CampaignEndingRuntime::Ending() const noexcept {
  return Ending_;
}

CampaignEndingState CampaignEndingRuntime::CaptureState() const noexcept {
  return {1U, Ending_};
}

bool CampaignEndingRuntime::RestoreState(
    const CampaignEndingState &state) noexcept {
  if (state.Version != 1U ||
      static_cast<std::uint8_t>(state.Ending) >
          static_cast<std::uint8_t>(CampaignEnding::Scatter)) {
    return false;
  }
  Ending_ = state.Ending;
  return true;
}

bool CampaignEndingCodec::Serialize(const CampaignEndingState &state,
                                    std::string &out) {
  CampaignEndingRuntime validation;
  if (!validation.RestoreState(state)) {
    return false;
  }
  out = "END1|" + std::to_string(static_cast<std::uint8_t>(state.Ending));
  return true;
}

bool CampaignEndingCodec::Deserialize(const std::string_view payload,
                                      CampaignEndingState &out) {
  constexpr std::string_view prefix{"END1|"};
  if (!payload.starts_with(prefix)) {
    return false;
  }
  std::uint32_t raw = 0U;
  const std::string_view value = payload.substr(prefix.size());
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), raw);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
      raw > static_cast<std::uint32_t>(CampaignEnding::Scatter)) {
    return false;
  }
  CampaignEndingState candidate{1U, static_cast<CampaignEnding>(raw)};
  CampaignEndingRuntime validation;
  if (!validation.RestoreState(candidate)) {
    return false;
  }
  out = candidate;
  return true;
}

} // namespace Lostsense::Gameplay
