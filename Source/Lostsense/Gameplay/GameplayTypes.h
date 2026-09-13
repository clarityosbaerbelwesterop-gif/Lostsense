#pragma once

#include <compare>
#include <cstdint>

namespace Lostsense::Gameplay {

struct EffectId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const EffectId &, const EffectId &) noexcept = default;
};

struct EffectInstanceId final {
  std::uint64_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const EffectInstanceId &,
              const EffectInstanceId &) noexcept = default;
};

struct EffectStackGroupId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const EffectStackGroupId &,
              const EffectStackGroupId &) noexcept = default;
};

struct GameplayTagId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const GameplayTagId &, const GameplayTagId &) noexcept = default;
};

struct AbilityId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const AbilityId &, const AbilityId &) noexcept = default;
};

struct CooldownGroupId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const CooldownGroupId &,
              const CooldownGroupId &) noexcept = default;
};

struct ClassId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const ClassId &, const ClassId &) noexcept = default;
};

} // namespace Lostsense::Gameplay
