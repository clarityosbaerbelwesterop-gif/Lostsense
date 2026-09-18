#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = {
    "Source/Lostsense/Gameplay/Story/CampaignCatalog.cpp": [
        "80030U", "Below the Charter Line", "80031U", "Breath for the Dead",
        "80032U", "Royal Pressure", "30024U", "80033U", "The King's Bore",
        "80034U", "A City in the Rock", "30026U",
    ],
    "Source/LostsenseGame/Private/LostsenseStorySubsystem.cpp": [
        "QuestId == 80024", "ActFourClearanceGranted",
        "VentilationNaveStabilized",
    ],
    "Source/LostsenseGame/Private/LostsenseDeepMechanismActor.cpp": [
        "GetObjectiveState(80030)", "ActFourClearanceGranted",
        "PumpCathedralApproachOpened",
    ],
    "Source/LostsenseGame/Private/LostsenseActFourStoryActor.cpp": [
        "80030", "80031", "80033", "PumpCathedralApproachOpened",
        "CompleteCampaignQuest",
    ],
    "Source/LostsenseGame/Private/LostsenseActFourWorldSubsystem.cpp": [
        "Pump Cathedral", "30024U", "BishopPiston", "King's Bore",
        "30026U", "GildedLung", "NamarithBreach",
    ],
    "Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp": [
        "BishopPiston", "GildedLung", "CompleteCampaignQuest(80032)",
        "CompleteCampaignQuest(80034)", "BossPhase == 2", "BossPhase = 3",
    ],
}
failures = []
for rel, tokens in checks.items():
    path = ROOT / rel
    if not path.is_file():
        failures.append(f"missing {rel}")
        continue
    text = path.read_text(encoding="utf-8")
    for token in tokens:
        if token not in text:
            failures.append(f"{rel}: missing {token!r}")
if failures:
    print("Act IV acceptance FAILED")
    for failure in failures:
        print(f" - {failure}")
    sys.exit(1)
print("Act IV source acceptance passed")
