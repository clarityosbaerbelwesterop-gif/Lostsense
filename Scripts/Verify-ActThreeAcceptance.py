#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = {
    "Source/Lostsense/Gameplay/Story/CampaignCatalog.cpp": [
        "80020U", "A Capital That Denies Itself", "80024U",
        "The Regent-Engine", "20010U", "30017U",
    ],
    "Source/Lostsense/Gameplay/Story/CampaignProgressRuntime.cpp": [
        "CompleteQuest", "CurrentQuestId", "FirstContinuationQuestIndex",
    ],
    "Source/LostsenseGame/Private/LostsenseActThreeStoryActor.cpp": [
        "Regent Solenne Mave", "80020", "80021", "80022", "80023",
        "CompleteCampaignQuest",
    ],
    "Source/LostsenseGame/Private/LostsenseActThreeWorldSubsystem.cpp": [
        "CivicPlaza", "LedgerCourt", "CharterHall", "Palace",
        "EngineChamber", "ConfigureCaldrisBoss", "30017U",
    ],
    "Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp": [
        "CourtShade", "BlankKnight", "Caldris",
        "GetObjectiveState(80024)", "CompleteCampaignQuest(80024)",
    ],
    "Source/LostsenseGame/Private/LostsenseStorySubsystem.cpp": [
        "LOSTSENSE_CAMPAIGN", "CampaignProgressCodec::Serialize",
        "CampaignProgressCodec::Deserialize",
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
    print("Act III acceptance FAILED")
    for failure in failures:
        print(f" - {failure}")
    sys.exit(1)
print("Act III source acceptance passed")
