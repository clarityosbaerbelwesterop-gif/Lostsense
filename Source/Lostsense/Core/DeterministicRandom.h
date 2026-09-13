#pragma once

#include <cstdint>

namespace Lostsense::Core {

struct RandomState final {
  std::uint64_t State{0};
  std::uint64_t Increment{1};

  [[nodiscard]] friend constexpr bool
  operator==(const RandomState &, const RandomState &) noexcept = default;
};

// PCG-XSH-RR with explicitly defined seeding and sample conversion. Unlike the
// standard-library distributions, this produces the same stream on every
// conforming platform and can be snapshotted for replay/save state.
class DeterministicRandom final {
public:
  explicit DeterministicRandom(std::uint64_t seed = 0,
                               std::uint64_t sequence = 1) noexcept;

  [[nodiscard]] std::uint32_t NextUInt32() noexcept;
  [[nodiscard]] std::uint64_t NextUInt64() noexcept;
  [[nodiscard]] double NextUnit() noexcept;
  [[nodiscard]] std::uint32_t
  NextBounded(std::uint32_t exclusiveUpper) noexcept;

  [[nodiscard]] RandomState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(RandomState state) noexcept;

private:
  std::uint64_t state_{0};
  std::uint64_t increment_{1};
};

} // namespace Lostsense::Core
