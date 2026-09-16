#include "Lostsense/Gameplay/World/DeepRouteState.h"
#include "TestHarness.h"

using namespace Lostsense::Gameplay;

int main() {
  Lostsense::Tests::TestSuite test{"deep route state"};

  DeepRouteState route;
  test.Expect(!route.Complete(DeepRouteMilestone::ServiceBrakeRestored),
              "deep milestones reject skipped progression");
  test.Expect(route.Complete(DeepRouteMilestone::OdranDefeated),
              "Odran completion starts deep route");
  test.Expect(route.Mechanism(DeepMechanismId::LanternRailSwitch) ==
                  DeepMechanismState::Available,
              "rail switch unlocks after Odran");
  test.Expect(route.Complete(DeepRouteMilestone::LanternRailDiscovered),
              "Lantern Rail discovery commits in order");
  test.Expect(route.SetMechanism(DeepMechanismId::LanternRailSwitch,
                                 DeepMechanismState::Alternate),
              "rail switch supports authored alternate route");
  test.Expect(!route.Complete(DeepRouteMilestone::ServiceBrakeRestored),
              "brake milestone rejects progress before physical restoration");
  test.Expect(route.SetMechanism(DeepMechanismId::ServiceCageBrake,
                                 DeepMechanismState::Restored),
              "service brake can be restored after discovery");
  test.Expect(route.Complete(DeepRouteMilestone::ServiceBrakeRestored),
              "restored brake advances route");
  test.Expect(!route.SetMechanism(DeepMechanismId::ServiceCageBrake,
                                  DeepMechanismState::Available),
              "restored brake cannot regress to available");
  test.Expect(!route.Complete(DeepRouteMilestone::RoyalThresholdOpened),
              "royal threshold requires the pressure door to open");
  test.Expect(route.SetMechanism(DeepMechanismId::RoyalPressureDoor,
                                 DeepMechanismState::Open),
              "royal pressure door opens after brake restoration");
  test.Expect(route.Complete(DeepRouteMilestone::RoyalThresholdOpened),
              "opened pressure door advances royal threshold");
  test.Expect(
      route.SetMechanism(DeepMechanismId::ReliefVent, DeepMechanismState::Open),
      "relief vent can establish safe pressure route");
  test.Expect(!route.Complete(DeepRouteMilestone::VentilationNaveStabilized),
              "one valve cannot falsely stabilize the ventilation nave");
  test.Expect(route.SetMechanism(DeepMechanismId::VentilationIntake,
                                 DeepMechanismState::Open),
              "ventilation intake can open after royal threshold");
  test.Expect(route.Complete(DeepRouteMilestone::VentilationNaveStabilized),
              "both open valves stabilize the ventilation nave");
  test.Expect(route.SetMechanism(DeepMechanismId::VaurReturnShortcut,
                                 DeepMechanismState::Open),
              "return shortcut persists after stabilization");
  test.Expect(!route.Complete(DeepRouteMilestone::PumpCathedralApproachOpened),
              "Pump Cathedral cannot open before Act IV clearance");
  test.Expect(route.Complete(DeepRouteMilestone::ActFourClearanceGranted),
              "future campaign authority can grant Act IV clearance");
  test.Expect(route.Complete(DeepRouteMilestone::PumpCathedralApproachOpened),
              "Pump Cathedral approach opens only after campaign clearance");

  const auto encoded = route.Serialize();
  const auto restored = DeepRouteState::Deserialize(encoded);
  test.Expect(restored.has_value(),
              "deep route round trips through bounded codec");
  if (restored) {
    test.Expect(restored->Serialize() == encoded,
                "deep route serialization is deterministic");
    test.Expect(restored->Mechanism(DeepMechanismId::LanternRailSwitch) ==
                    DeepMechanismState::Alternate,
                "rail route survives restore");
    test.Expect(restored->Mechanism(DeepMechanismId::VaurReturnShortcut) ==
                    DeepMechanismState::Open,
                "shortcut survives restore");
  }

  test.Expect(!DeepRouteState::Deserialize("DR1|4|1|0|0|0|0|0").has_value(),
              "restore rejects impossible milestone gaps");
  test.Expect(
      !DeepRouteState::Deserialize("DR1|7|3|1|1|0|0|0").has_value(),
      "restore rejects completed brake milestone without restored brake");
  test.Expect(!DeepRouteState::Deserialize("DR1|1|99|0|0|0|0|0").has_value(),
              "restore rejects invalid mechanism enum");
  test.Expect(!DeepRouteState::Deserialize(std::string(300, 'x')).has_value(),
              "restore rejects oversized payload");

  return test.Finish();
}
