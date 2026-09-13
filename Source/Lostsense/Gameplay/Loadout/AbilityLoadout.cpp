#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"

#include <cstddef>

namespace Lostsense::Gameplay {

bool AbilityLoadout::IsSlotValid(const AbilityLoadoutSlot slot) noexcept {
  return static_cast<std::size_t>(slot) < AbilityLoadoutSlotCount;
}

LoadoutResult AbilityLoadout::ValidateAssignment(
    const AbilityLoadoutSlot slot, const AbilityId ability,
    const std::array<AbilityId, AbilityLoadoutSlotCount> &candidate)
    const noexcept {
  if (!abilities_.IsValid()) {
    return LoadoutResult::InvalidRuntime;
  }
  if (!IsSlotValid(slot)) {
    return LoadoutResult::InvalidSlot;
  }
  if (!ability.IsValid()) {
    return LoadoutResult::UnknownAbility;
  }

  const AbilityDefinition *definition = abilities_.FindDefinition(ability);
  if (definition == nullptr) {
    return LoadoutResult::UnknownAbility;
  }
  if (!abilities_.IsUnlocked(ability)) {
    return LoadoutResult::NotUnlocked;
  }
  if (definition->RequiredClass.IsValid() &&
      definition->RequiredClass != abilities_.OwnerClass()) {
    return LoadoutResult::WrongClass;
  }
  if (definition->AllowedLoadoutSlots == 0U ||
      (definition->AllowedLoadoutSlots &
       static_cast<AbilityLoadoutSlotMask>(~AllAbilityLoadoutSlots)) != 0U ||
      (definition->AllowedLoadoutSlots & AbilityLoadoutSlotBit(slot)) == 0U) {
    return LoadoutResult::SlotNotAllowed;
  }

  if (!definition->AllowDuplicateInLoadout) {
    const auto selectedIndex = static_cast<std::size_t>(slot);
    for (std::size_t index = 0; index < candidate.size(); ++index) {
      if (index != selectedIndex && candidate[index] == ability) {
        return LoadoutResult::DuplicateNotAllowed;
      }
    }
  }
  return LoadoutResult::Success;
}

LoadoutResult AbilityLoadout::Equip(const AbilityLoadoutSlot slot,
                                    const AbilityId ability) noexcept {
  if (!IsSlotValid(slot)) {
    return LoadoutResult::InvalidSlot;
  }
  auto candidate = slots_;
  candidate[static_cast<std::size_t>(slot)] = ability;
  const LoadoutResult validation = ValidateAssignment(slot, ability, candidate);
  if (validation != LoadoutResult::Success) {
    return validation;
  }
  slots_ = candidate;
  return LoadoutResult::Success;
}

bool AbilityLoadout::Unequip(const AbilityLoadoutSlot slot) noexcept {
  if (!IsSlotValid(slot)) {
    return false;
  }
  AbilityId &entry = slots_[static_cast<std::size_t>(slot)];
  if (!entry.IsValid()) {
    return false;
  }
  entry = {};
  return true;
}

AbilityId
AbilityLoadout::AbilityAt(const AbilityLoadoutSlot slot) const noexcept {
  return IsSlotValid(slot) ? slots_[static_cast<std::size_t>(slot)]
                           : AbilityId{};
}

bool AbilityLoadout::UsesAbility(const AbilityId ability) const noexcept {
  if (!ability.IsValid()) {
    return false;
  }
  for (const AbilityId equipped : slots_) {
    if (equipped == ability) {
      return true;
    }
  }
  return false;
}

AbilityLoadoutState AbilityLoadout::CaptureState() const noexcept {
  return {slots_};
}

bool AbilityLoadout::ValidateState(
    const AbilityLoadoutState &state) const noexcept {
  if (!abilities_.IsValid()) {
    return false;
  }
  for (std::size_t index = 0; index < state.Slots.size(); ++index) {
    const AbilityId ability = state.Slots[index];
    if (!ability.IsValid()) {
      continue;
    }
    const auto slot = static_cast<AbilityLoadoutSlot>(index);
    if (ValidateAssignment(slot, ability, state.Slots) !=
        LoadoutResult::Success) {
      return false;
    }
  }
  return true;
}

bool AbilityLoadout::RestoreState(const AbilityLoadoutState &state) noexcept {
  if (!ValidateState(state)) {
    return false;
  }
  slots_ = state.Slots;
  return true;
}

} // namespace Lostsense::Gameplay
