#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
checks={
"Source/Lostsense/Gameplay/Story/CampaignEndingRuntime.h":[
 "Sever","Bind","Scatter"],
"Source/Lostsense/Gameplay/Story/CampaignEndingRuntime.cpp":["END1|"],
"Tests/Gameplay/CampaignEndingRuntimeTests.cpp":[
 "ending cannot be overwritten","round-trips exactly"],
"Source/LostsenseGame/Private/LostsenseStorySubsystem.cpp":[
 "LOSTSENSE_ENDING","ResolveCampaignEnding","Objective(80084U)",
 "EndingQuestCompleted != EndingResolved"],
"Source/LostsenseGame/Private/LostsenseActNineWorldSubsystem.cpp":[
 "Zero Gallery / Sensewell","20036","three consensus spines","30060U",
 "AsterNull","ELostsenseCampaignEnding::Sever",
 "ELostsenseCampaignEnding::Bind","ELostsenseCampaignEnding::Scatter"],
"Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp":[
 "AsterNull","BossPhase = 3","CompleteCampaignQuest(80083)",
 "GetObjectiveState(80083)"],
}
bad=[]
for p,toks in checks.items():
 path=R/p
 if not path.is_file(): bad.append(f"missing {p}");continue
 s=path.read_text(encoding="utf-8")
 for t in toks:
  if t not in s: bad.append(f"{p}: missing {t!r}")
if bad:
 print("Act IX acceptance FAILED")
 for x in bad: print(" -",x)
 sys.exit(1)
print("Act IX source acceptance passed")
