#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Lostsense::Gameplay {

enum class DeepRouteMilestone : std::uint8_t {
  OdranDefeated = 0,
  LanternRailDiscovered,
  ServiceBrakeRestored,
  RoyalThresholdOpened,
  VentilationNaveStabilized,
  ActFourClearanceGranted,
  PumpCathedralApproachOpened,
};

enum class DeepMechanismId : std::uint8_t {
  LanternRailSwitch = 0,
  ServiceCageBrake,
  RoyalPressureDoor,
  VentilationIntake,
  ReliefVent,
  VaurReturnShortcut,
};

enum class DeepMechanismState : std::uint8_t {
  Locked = 0,
  Available,
  Primary,
  Alternate,
  Restored,
  Open,
};

struct DeepMechanismSnapshot {
  DeepMechanismId Id{};
  DeepMechanismState State{DeepMechanismState::Locked};
};

class DeepRouteState {
public:
  [[nodiscard]] bool Complete(DeepRouteMilestone milestone);
  [[nodiscard]] bool IsComplete(DeepRouteMilestone milestone) const noexcept;
  [[nodiscard]] bool SetMechanism(DeepMechanismId id, DeepMechanismState state);
  [[nodiscard]] DeepMechanismState Mechanism(DeepMechanismId id) const noexcept;
  [[nodiscard]] std::vector<DeepMechanismSnapshot> Mechanisms() const;

  [[nodiscard]] std::string Serialize() const;
  [[nodiscard]] static std::optional<DeepRouteState>
  Deserialize(std::string_view text);

private:
  std::uint32_t CompletedMask_{0};
  DeepMechanismState RailSwitch_{DeepMechanismState::Locked};
  DeepMechanismState ServiceBrake_{DeepMechanismState::Locked};
  DeepMechanismState PressureDoor_{DeepMechanismState::Locked};
  DeepMechanismState VentilationIntake_{DeepMechanismState::Locked};
  DeepMechanismState ReliefVent_{DeepMechanismState::Locked};
  DeepMechanismState ReturnShortcut_{DeepMechanismState::Locked};

  [[nodiscard]] static bool ValidMilestoneOrder(std::uint32_t mask) noexcept;
  [[nodiscard]] static bool ValidState(const DeepRouteState &state) noexcept;
};

} // namespace Lostsense::Gameplay
