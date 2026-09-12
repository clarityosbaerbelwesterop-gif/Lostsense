#include "Lostsense/Core/DeterministicRandom.h"

namespace Lostsense::Core {
namespace {

constexpr std::uint64_t Multiplier = 6364136223846793005ULL;
constexpr double TwoToThe53 = 9007199254740992.0;

} // namespace

DeterministicRandom::DeterministicRandom(const std::uint64_t seed,
                                         const std::uint64_t sequence) noexcept
    : increment_{(sequence << 1U) | 1U} {
  static_cast<void>(NextUInt32());
  state_ += seed;
  static_cast<void>(NextUInt32());
}

std::uint32_t DeterministicRandom::NextUInt32() noexcept {
  const std::uint64_t previous = state_;
  state_ = previous * Multiplier + increment_;
  const auto xorshifted = static_cast<std::uint32_t>(
      ((previous >> 18U) ^ previous) >> 27U);
  const auto rotation = static_cast<std::uint32_t>(previous >> 59U);
  return (xorshifted >> rotation) |
         (xorshifted << ((0U - rotation) & 31U));
}

std::uint64_t DeterministicRandom::NextUInt64() noexcept {
  const auto high = static_cast<std::uint64_t>(NextUInt32());
  const auto low = static_cast<std::uint64_t>(NextUInt32());
  return (high << 32U) | low;
}

double DeterministicRandom::NextUnit() noexcept {
  const auto high = static_cast<std::uint64_t>(NextUInt32() >> 5U);
  const auto low = static_cast<std::uint64_t>(NextUInt32() >> 6U);
  const std::uint64_t bits = (high << 26U) | low;
  return static_cast<double>(bits) / TwoToThe53;
}

std::uint32_t
DeterministicRandom::NextBounded(const std::uint32_t exclusiveUpper) noexcept {
  if (exclusiveUpper == 0U) {
    return 0U;
  }

  const std::uint32_t threshold =
      static_cast<std::uint32_t>(0U - exclusiveUpper) % exclusiveUpper;
  std::uint32_t sample = 0U;
  do {
    sample = NextUInt32();
  } while (sample < threshold);
  return sample % exclusiveUpper;
}

RandomState DeterministicRandom::CaptureState() const noexcept {
  return RandomState{state_, increment_};
}

bool DeterministicRandom::RestoreState(const RandomState state) noexcept {
  if ((state.Increment & 1U) == 0U) {
    return false;
  }
  state_ = state.State;
  increment_ = state.Increment;
  return true;
}

} // namespace Lostsense::Core
