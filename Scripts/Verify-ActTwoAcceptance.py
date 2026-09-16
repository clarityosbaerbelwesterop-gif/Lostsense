#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = {
    "Source/Lostsense/Gameplay/Story/ActTwoStoryRuntime.cpp": [
        "UnlockFromActOne", "BellgraveChanged", "InfectedVillagerStabilized",
        "ResolveWitnessRoot", "ActTwoStoryCodec::Deserialize",
    ],
    "Tests/Gameplay/ActTwoStoryRuntimeTests.cpp": [
        "Act II remains locked before changed Bellgrave",
        "nonlethal villager stabilization",
        "Act II restore rejects beat gaps",
    ],
    "Source/LostsenseGame/Private/LostsenseStorySubsystem.cpp": [
        "LOSTSENSE_ACT_TWO", "ActTwoStoryCodec::Serialize",
        "ActTwoStoryCodec::Deserialize", "UnlockFromActOne",
    ],
    "Source/LostsenseGame/Private/LostsenseActTwoStoryActor.cpp": [
        "Stabilize senseprint without killing host",
        "GiltfenRootTunnelOpened", "ThornChoirDiscovered",
    ],
    "Source/LostsenseGame/Private/LostsenseActTwoWorldSubsystem.cpp": [
        "Morrowstep", "RootTunnel", "ThornChoirThreshold",
        "nonlethal resolution",
    ],
    "Source/LostsenseGame/Private/LostsenseDeepMechanismActor.cpp": [
        "ActFourClearanceGranted",
        "PumpCathedralApproachOpened",
    ],
    "CMakeLists.txt": [
        "ActTwoStoryRuntime.cpp", "ActTwoStoryRuntimeTests.cpp",
        "gameplay.act_two_story",
    ],
}

failures=[]
for rel, tokens in checks.items():
    path=ROOT/rel
    if not path.is_file():
        failures.append(f"missing {rel}")
        continue
    text=path.read_text(encoding="utf-8")
    for token in tokens:
        if token not in text:
            failures.append(f"{rel}: missing {token!r}")

if failures:
    print("Act II acceptance FAILED")
    for failure in failures:
        print(f" - {failure}")
    sys.exit(1)
print("Act II source acceptance passed")
