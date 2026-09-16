#include "DeepRouteState.h"

#include <array>
#include <charconv>
#include <utility>

namespace Lostsense::Gameplay {
namespace {
constexpr std::uint32_t Bit(DeepRouteMilestone milestone) noexcept {
  return 1U << static_cast<std::uint32_t>(milestone);
}

bool IsAllowed(DeepMechanismId id, DeepMechanismState state) noexcept {
  switch (id) {
  case DeepMechanismId::LanternRailSwitch:
    return state == DeepMechanismState::Locked ||
           state == DeepMechanismState::Available ||
           state == DeepMechanismState::Primary ||
           state == DeepMechanismState::Alternate;
  case DeepMechanismId::ServiceCageBrake:
    return state == DeepMechanismState::Locked ||
           state == DeepMechanismState::Available ||
           state == DeepMechanismState::Restored;
  case DeepMechanismId::RoyalPressureDoor:
  case DeepMechanismId::VentilationIntake:
  case DeepMechanismId::ReliefVent:
  case DeepMechanismId::VaurReturnShortcut:
    return state == DeepMechanismState::Locked ||
           state == DeepMechanismState::Available ||
           state == DeepMechanismState::Open;
  }
  return false;
}

bool ParseUnsigned(std::string_view text, std::uint32_t &value) noexcept {
  if (text.empty()) {
    return false;
  }
  const auto *first = text.data();
  const auto *last = first + text.size();
  const auto result = std::from_chars(first, last, value);
  return result.ec == std::errc{} && result.ptr == last;
}
} // namespace

bool DeepRouteState::Complete(DeepRouteMilestone milestone) {
  const auto index = static_cast<std::uint32_t>(milestone);
  if (index > static_cast<std::uint32_t>(
                  DeepRouteMilestone::PumpCathedralApproachOpened)) {
    return false;
  }
  const auto bit = Bit(milestone);
  if ((CompletedMask_ & bit) != 0U) {
    return true;
  }
  if (index > 0U) {
    const auto required = (1U << index) - 1U;
    if ((CompletedMask_ & required) != required) {
      return false;
    }
  }

  switch (milestone) {
  case DeepRouteMilestone::OdranDefeated:
  case DeepRouteMilestone::LanternRailDiscovered:
    break;
  case DeepRouteMilestone::ServiceBrakeRestored:
    if (ServiceBrake_ != DeepMechanismState::Restored) {
      return false;
    }
    break;
  case DeepRouteMilestone::RoyalThresholdOpened:
    if (PressureDoor_ != DeepMechanismState::Open) {
      return false;
    }
    break;
  case DeepRouteMilestone::VentilationNaveStabilized:
    if (VentilationIntake_ != DeepMechanismState::Open ||
        ReliefVent_ != DeepMechanismState::Open) {
      return false;
    }
    break;
  case DeepRouteMilestone::PumpCathedralApproachOpened:
    break;
  }

  CompletedMask_ |= bit;
  switch (milestone) {
  case DeepRouteMilestone::OdranDefeated:
    RailSwitch_ = DeepMechanismState::Available;
    break;
  case DeepRouteMilestone::LanternRailDiscovered:
    ServiceBrake_ = DeepMechanismState::Available;
    break;
  case DeepRouteMilestone::ServiceBrakeRestored:
    PressureDoor_ = DeepMechanismState::Available;
    break;
  case DeepRouteMilestone::RoyalThresholdOpened:
    VentilationIntake_ = DeepMechanismState::Available;
    ReliefVent_ = DeepMechanismState::Available;
    break;
  case DeepRouteMilestone::VentilationNaveStabilized:
    ReturnShortcut_ = DeepMechanismState::Available;
    break;
  case DeepRouteMilestone::PumpCathedralApproachOpened:
    break;
  }
  return true;
}

bool DeepRouteState::IsComplete(DeepRouteMilestone milestone) const noexcept {
  const auto index = static_cast<std::uint32_t>(milestone);
  return index <= static_cast<std::uint32_t>(
                      DeepRouteMilestone::PumpCathedralApproachOpened) &&
         (CompletedMask_ & Bit(milestone)) != 0U;
}

bool DeepRouteState::SetMechanism(DeepMechanismId id,
                                  DeepMechanismState state) {
  if (!IsAllowed(id, state)) {
    return false;
  }

  DeepMechanismState *slot = nullptr;
  switch (id) {
  case DeepMechanismId::LanternRailSwitch:
    slot = &RailSwitch_;
    break;
  case DeepMechanismId::ServiceCageBrake:
    slot = &ServiceBrake_;
    break;
  case DeepMechanismId::RoyalPressureDoor:
    slot = &PressureDoor_;
    break;
  case DeepMechanismId::VentilationIntake:
    slot = &VentilationIntake_;
    break;
  case DeepMechanismId::ReliefVent:
    slot = &ReliefVent_;
    break;
  case DeepMechanismId::VaurReturnShortcut:
    slot = &ReturnShortcut_;
    break;
  }
  if (slot == nullptr || *slot == DeepMechanismState::Locked) {
    return state == DeepMechanismState::Locked;
  }

  if (id != DeepMechanismId::LanternRailSwitch &&
      (*slot == DeepMechanismState::Restored ||
       *slot == DeepMechanismState::Open) &&
      state != *slot) {
    return false;
  }

  const auto previous = *slot;
  *slot = state;
  if (!ValidState(*this)) {
    *slot = previous;
    return false;
  }
  return true;
}

DeepMechanismState
DeepRouteState::Mechanism(DeepMechanismId id) const noexcept {
  switch (id) {
  case DeepMechanismId::LanternRailSwitch:
    return RailSwitch_;
  case DeepMechanismId::ServiceCageBrake:
    return ServiceBrake_;
  case DeepMechanismId::RoyalPressureDoor:
    return PressureDoor_;
  case DeepMechanismId::VentilationIntake:
    return VentilationIntake_;
  case DeepMechanismId::ReliefVent:
    return ReliefVent_;
  case DeepMechanismId::VaurReturnShortcut:
    return ReturnShortcut_;
  }
  return DeepMechanismState::Locked;
}

std::vector<DeepMechanismSnapshot> DeepRouteState::Mechanisms() const {
  return {{DeepMechanismId::LanternRailSwitch, RailSwitch_},
          {DeepMechanismId::ServiceCageBrake, ServiceBrake_},
          {DeepMechanismId::RoyalPressureDoor, PressureDoor_},
          {DeepMechanismId::VentilationIntake, VentilationIntake_},
          {DeepMechanismId::ReliefVent, ReliefVent_},
          {DeepMechanismId::VaurReturnShortcut, ReturnShortcut_}};
}

std::string DeepRouteState::Serialize() const {
  return "DR1|" + std::to_string(CompletedMask_) + "|" +
         std::to_string(static_cast<unsigned>(RailSwitch_)) + "|" +
         std::to_string(static_cast<unsigned>(ServiceBrake_)) + "|" +
         std::to_string(static_cast<unsigned>(PressureDoor_)) + "|" +
         std::to_string(static_cast<unsigned>(VentilationIntake_)) + "|" +
         std::to_string(static_cast<unsigned>(ReliefVent_)) + "|" +
         std::to_string(static_cast<unsigned>(ReturnShortcut_));
}

std::optional<DeepRouteState>
DeepRouteState::Deserialize(std::string_view text) {
  if (text.size() > 256U || !text.starts_with("DR1|")) {
    return std::nullopt;
  }
  std::array<std::uint32_t, 7> values{};
  auto remainder = text.substr(4);
  for (std::size_t index = 0; index < values.size(); ++index) {
    const auto separator = remainder.find('|');
    const bool final = index + 1U == values.size();
    if ((!final && separator == std::string_view::npos) ||
        (final && separator != std::string_view::npos)) {
      return std::nullopt;
    }
    const auto token = final ? remainder : remainder.substr(0, separator);
    if (!ParseUnsigned(token, values[index])) {
      return std::nullopt;
    }
    if (!final) {
      remainder.remove_prefix(separator + 1U);
    }
  }

  DeepRouteState state;
  state.CompletedMask_ = values[0];
  const auto maxState = static_cast<std::uint32_t>(DeepMechanismState::Open);
  for (std::size_t index = 1; index < values.size(); ++index) {
    if (values[index] > maxState) {
      return std::nullopt;
    }
  }
  state.RailSwitch_ = static_cast<DeepMechanismState>(values[1]);
  state.ServiceBrake_ = static_cast<DeepMechanismState>(values[2]);
  state.PressureDoor_ = static_cast<DeepMechanismState>(values[3]);
  state.VentilationIntake_ = static_cast<DeepMechanismState>(values[4]);
  state.ReliefVent_ = static_cast<DeepMechanismState>(values[5]);
  state.ReturnShortcut_ = static_cast<DeepMechanismState>(values[6]);
  if (!ValidState(state)) {
    return std::nullopt;
  }
  return state;
}

bool DeepRouteState::ValidMilestoneOrder(std::uint32_t mask) noexcept {
  constexpr auto count = static_cast<std::uint32_t>(
                             DeepRouteMilestone::PumpCathedralApproachOpened) +
                         1U;
  if ((mask >> count) != 0U) {
    return false;
  }
  bool foundGap = false;
  for (std::uint32_t index = 0; index < count; ++index) {
    const bool set = (mask & (1U << index)) != 0U;
    if (!set) {
      foundGap = true;
    } else if (foundGap) {
      return false;
    }
  }
  return true;
}

bool DeepRouteState::ValidState(const DeepRouteState &state) noexcept {
  if (!ValidMilestoneOrder(state.CompletedMask_)) {
    return false;
  }
  const std::array pairs{
      std::pair{DeepMechanismId::LanternRailSwitch, state.RailSwitch_},
      std::pair{DeepMechanismId::ServiceCageBrake, state.ServiceBrake_},
      std::pair{DeepMechanismId::RoyalPressureDoor, state.PressureDoor_},
      std::pair{DeepMechanismId::VentilationIntake, state.VentilationIntake_},
      std::pair{DeepMechanismId::ReliefVent, state.ReliefVent_},
      std::pair{DeepMechanismId::VaurReturnShortcut, state.ReturnShortcut_},
  };
  for (const auto &[id, mechanismState] : pairs) {
    if (!IsAllowed(id, mechanismState)) {
      return false;
    }
  }
  if (!state.IsComplete(DeepRouteMilestone::OdranDefeated) &&
      state.RailSwitch_ != DeepMechanismState::Locked) {
    return false;
  }
  if (!state.IsComplete(DeepRouteMilestone::LanternRailDiscovered) &&
      state.ServiceBrake_ != DeepMechanismState::Locked) {
    return false;
  }
  if (!state.IsComplete(DeepRouteMilestone::ServiceBrakeRestored) &&
      state.PressureDoor_ != DeepMechanismState::Locked) {
    return false;
  }
  if (state.IsComplete(DeepRouteMilestone::ServiceBrakeRestored) &&
      state.ServiceBrake_ != DeepMechanismState::Restored) {
    return false;
  }
  if (!state.IsComplete(DeepRouteMilestone::RoyalThresholdOpened) &&
      (state.VentilationIntake_ != DeepMechanismState::Locked ||
       state.ReliefVent_ != DeepMechanismState::Locked)) {
    return false;
  }
  if (state.IsComplete(DeepRouteMilestone::RoyalThresholdOpened) &&
      state.PressureDoor_ != DeepMechanismState::Open) {
    return false;
  }
  if (!state.IsComplete(DeepRouteMilestone::VentilationNaveStabilized) &&
      state.ReturnShortcut_ != DeepMechanismState::Locked) {
    return false;
  }
  if (state.IsComplete(DeepRouteMilestone::VentilationNaveStabilized) &&
      (state.VentilationIntake_ != DeepMechanismState::Open ||
       state.ReliefVent_ != DeepMechanismState::Open)) {
    return false;
  }
  return true;
}

} // namespace Lostsense::Gameplay
