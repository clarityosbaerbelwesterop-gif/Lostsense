#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

checks = {
    "Source/Lostsense/Gameplay/World/DeepRouteState.cpp": [
        "ValidMilestoneOrder",
        "DeepRouteState::Deserialize",
        "const auto previous = *slot",
        "PumpCathedralApproachOpened",
    ],
    "Tests/Gameplay/DeepRouteStateTests.cpp": [
        "reject skipped progression",
        "serialization is deterministic",
        "rejects impossible milestone gaps",
    ],
    "Source/LostsenseGame/Private/LostsenseStorySubsystem.cpp": [
        "LOSTSENSE_DEEP_ROUTE",
        "DeepRouteState::Deserialize",
        "ValidatedStory.RestoreState",
        "FirstSliceStoryBeat::OdranDefeated",
    ],
    "Source/LostsenseGame/Private/LostsenseDeepMechanismActor.cpp": [
        "LanternRailDiscovered",
        "ServiceBrakeRestored",
        "RoyalThresholdOpened",
        "VentilationNaveStabilized",
        "PumpCathedralApproachOpened",
    ],
    "Source/LostsenseGame/Private/LostsenseDeepMoverActor.cpp": [
        "LanternRailCart",
        "ServiceCage",
        "VentilationRotor",
        "GetDeepMechanismState",
    ],
    "Source/LostsenseGame/Private/LostsenseDeepWorldSubsystem.cpp": [
        "SpawnLanternRail",
        "SpawnRoyalThreshold",
        "GildedDead",
        "PressureMutant",
        "RailMarshal",
        "PumpCathedralApproachGate",
    ],
    "Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp": [
        "ELostsenseEnemyArchetype::GildedDead",
        "ELostsenseEnemyArchetype::PressureMutant",
        "ELostsenseEnemyArchetype::RailMarshal",
        "DamageType::Arcane",
    ],
    "CMakeLists.txt": [
        "Source/Lostsense/Gameplay/World/DeepRouteState.cpp",
        "Tests/Gameplay/DeepRouteStateTests.cpp",
        "gameplay.deep_route_state",
    ],
}

failures = []
for relative, required in checks.items():
    path = ROOT / relative
    if not path.is_file():
        failures.append(f"missing {relative}")
        continue
    text = path.read_text(encoding="utf-8")
    for token in required:
        if token not in text:
            failures.append(f"{relative}: missing contract token {token!r}")

# Progression authority must not be recreated in the world builder.
world_text = (ROOT / "Source/LostsenseGame/Private/LostsenseDeepWorldSubsystem.cpp").read_text(encoding="utf-8")
for forbidden in ("CompletedMask_", "DeepRouteState DeepRoute", "bOdranDefeated"):
    if forbidden in world_text:
        failures.append(f"Deep world builder contains forbidden authority token {forbidden!r}")

if failures:
    print("Deep Vaur acceptance FAILED:")
    for failure in failures:
        print(f" - {failure}")
    sys.exit(1)

print("Deep Vaur source acceptance passed")
