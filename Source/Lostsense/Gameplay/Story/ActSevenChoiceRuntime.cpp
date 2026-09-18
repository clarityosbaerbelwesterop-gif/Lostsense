#include "ActSevenChoiceRuntime.h"

#include <charconv>

namespace Lostsense::Gameplay {

bool ActSevenChoiceRuntime::Resolve(const ActSevenArchiveChoice choice) noexcept {
  if (Choice_ != ActSevenArchiveChoice::None ||
      choice == ActSevenArchiveChoice::None) {
    return false;
  }
  Choice_ = choice;
  return true;
}

ActSevenArchiveChoice ActSevenChoiceRuntime::Choice() const noexcept {
  return Choice_;
}

ActSevenChoiceState ActSevenChoiceRuntime::CaptureState() const noexcept {
  return {1U, Choice_};
}

bool ActSevenChoiceRuntime::RestoreState(
    const ActSevenChoiceState &state) noexcept {
  if (state.Version != 1U ||
      static_cast<std::uint8_t>(state.Archive) >
          static_cast<std::uint8_t>(
              ActSevenArchiveChoice::CrownlessCivilianArchive)) {
    return false;
  }
  Choice_ = state.Archive;
  return true;
}

bool ActSevenChoiceCodec::Serialize(const ActSevenChoiceState &state,
                                    std::string &out) {
  ActSevenChoiceRuntime validation;
  if (!validation.RestoreState(state)) {
    return false;
  }
  out = "A7C1|" +
        std::to_string(static_cast<std::uint8_t>(state.Archive));
  return true;
}

bool ActSevenChoiceCodec::Deserialize(const std::string_view payload,
                                      ActSevenChoiceState &out) {
  constexpr std::string_view prefix{"A7C1|"};
  if (!payload.starts_with(prefix)) {
    return false;
  }
  std::uint32_t raw = 0U;
  const std::string_view value = payload.substr(prefix.size());
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), raw);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
      raw > static_cast<std::uint32_t>(
                ActSevenArchiveChoice::CrownlessCivilianArchive)) {
    return false;
  }
  ActSevenChoiceState candidate{
      1U, static_cast<ActSevenArchiveChoice>(raw)};
  ActSevenChoiceRuntime validation;
  if (!validation.RestoreState(candidate)) {
    return false;
  }
  out = candidate;
  return true;
}

} // namespace Lostsense::Gameplay
