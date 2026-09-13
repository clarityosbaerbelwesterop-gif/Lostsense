#pragma once

#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"

#include <array>
#include <cstdint>

namespace Lostsense::Gameplay {

enum class LoadoutResult : std::uint8_t {
  Success,
  InvalidRuntime,
  InvalidSlot,
  UnknownAbility,
  NotUnlocked,
  WrongClass,
  SlotNotAllowed,
  DuplicateNotAllowed,
  InvalidState,
};

struct AbilityLoadoutState final {
  std::array<AbilityId, AbilityLoadoutSlotCount> Slots{};
};

class AbilityLoadout final {
public:
  explicit AbilityLoadout(AbilityRuntime &abilities) noexcept
      : abilities_{abilities} {}

  [[nodiscard]] bool IsValid() const noexcept { return abilities_.IsValid(); }
  [[nodiscard]] const AbilityRuntime *AbilityAuthority() const noexcept {
    return &abilities_;
  }
  [[nodiscard]] LoadoutResult Equip(AbilityLoadoutSlot slot,
                                    AbilityId ability) noexcept;
  [[nodiscard]] bool Unequip(AbilityLoadoutSlot slot) noexcept;
  [[nodiscard]] AbilityId AbilityAt(AbilityLoadoutSlot slot) const noexcept;
  [[nodiscard]] bool UsesAbility(AbilityId ability) const noexcept;

  [[nodiscard]] AbilityLoadoutState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(const AbilityLoadoutState &state) noexcept;

private:
  [[nodiscard]] static bool IsSlotValid(AbilityLoadoutSlot slot) noexcept;
  [[nodiscard]] LoadoutResult
  ValidateAssignment(AbilityLoadoutSlot slot, AbilityId ability,
                     const std::array<AbilityId, AbilityLoadoutSlotCount>
                         &candidate) const noexcept;
  [[nodiscard]] bool
  ValidateState(const AbilityLoadoutState &state) const noexcept;

  AbilityRuntime &abilities_;
  std::array<AbilityId, AbilityLoadoutSlotCount> slots_{};
};

} // namespace Lostsense::Gameplay
