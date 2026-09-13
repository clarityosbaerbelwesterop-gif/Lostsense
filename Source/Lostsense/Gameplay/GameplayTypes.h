#pragma once

#include <compare>
#include <cstddef>
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

struct SkillNodeId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const SkillNodeId &, const SkillNodeId &) noexcept = default;
};

struct SkillTreeId final {
  std::uint16_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const SkillTreeId &, const SkillTreeId &) noexcept = default;
};

struct SkillExclusiveGroupId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const SkillExclusiveGroupId &,
              const SkillExclusiveGroupId &) noexcept = default;
};

struct AbilityMutationId final {
  std::uint32_t Value{0};
  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const AbilityMutationId &,
              const AbilityMutationId &) noexcept = default;
};

enum class AbilityLoadoutSlot : std::uint8_t {
  PrimaryAttack,
  SecondaryAttack,
  Active1,
  Active2,
  Active3,
  Active4,
  Dodge,
  ClassMechanic,
  Ultimate,
  Count,
};

using AbilityLoadoutSlotMask = std::uint16_t;
inline constexpr std::size_t AbilityLoadoutSlotCount =
    static_cast<std::size_t>(AbilityLoadoutSlot::Count);

[[nodiscard]] constexpr AbilityLoadoutSlotMask
AbilityLoadoutSlotBit(const AbilityLoadoutSlot slot) noexcept {
  const auto index = static_cast<std::uint8_t>(slot);
  return index < static_cast<std::uint8_t>(AbilityLoadoutSlot::Count)
             ? static_cast<AbilityLoadoutSlotMask>(1U << index)
             : 0U;
}

inline constexpr AbilityLoadoutSlotMask AllAbilityLoadoutSlots =
    static_cast<AbilityLoadoutSlotMask>((1U << AbilityLoadoutSlotCount) - 1U);

} // namespace Lostsense::Gameplay
