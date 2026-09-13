#pragma once

#include "Lostsense/Gameplay/Persistence/CharacterPersistence.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Lostsense::Gameplay {

enum class SaveDecodeStatus : std::uint8_t {
  Success,
  PayloadTooLarge,
  Malformed,
  UnsupportedVersion,
  MigrationFailed,
};

struct SaveDecodeResult final {
  SaveDecodeStatus Status{SaveDecodeStatus::Malformed};
  std::uint32_t SourceVersion{0U};
};

class SaveCodec final {
public:
  [[nodiscard]] static bool Serialize(const CharacterSaveState &state,
                                      std::string &payload);
  [[nodiscard]] static SaveDecodeResult
  Deserialize(std::string_view payload, CharacterSaveState &state);
};

} // namespace Lostsense::Gameplay
