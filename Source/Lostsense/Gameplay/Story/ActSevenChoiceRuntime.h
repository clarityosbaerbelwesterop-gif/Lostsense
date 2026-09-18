#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Lostsense::Gameplay {

enum class ActSevenArchiveChoice : std::uint8_t {
  None = 0U,
  BellgraveCivilianArchive = 1U,
  CrownlessCivilianArchive = 2U
};

struct ActSevenChoiceState final {
  std::uint32_t Version = 1U;
  ActSevenArchiveChoice Archive = ActSevenArchiveChoice::None;
};

class ActSevenChoiceRuntime final {
public:
  [[nodiscard]] bool Resolve(ActSevenArchiveChoice choice) noexcept;
  [[nodiscard]] ActSevenArchiveChoice Choice() const noexcept;
  [[nodiscard]] ActSevenChoiceState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(const ActSevenChoiceState &state) noexcept;

private:
  ActSevenArchiveChoice Choice_ = ActSevenArchiveChoice::None;
};

class ActSevenChoiceCodec final {
public:
  [[nodiscard]] static bool Serialize(const ActSevenChoiceState &state,
                                      std::string &out);
  [[nodiscard]] static bool Deserialize(std::string_view payload,
                                        ActSevenChoiceState &out);
};

} // namespace Lostsense::Gameplay
