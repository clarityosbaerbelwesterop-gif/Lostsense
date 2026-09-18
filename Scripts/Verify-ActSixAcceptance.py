#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
checks={
"Source/LostsenseGame/Private/LostsenseActSixStoryActor.cpp":[
 "80050","80051","80052","80053","CompleteCampaignQuest"],
"Source/LostsenseGame/Private/LostsenseActSixWorldSubsystem.cpp":[
 "Hollow Meridian","Quiet House of Bearings","Blind Astrarium","20028",
 "30041U","Kharos","Choirless Expanse"],
"Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp":[
 "NullMantle","ThoughtEater","Kharos","BossPhase = 3",
 "CompleteCampaignQuest(80054)","GetObjectiveState(80054)",
 "KharosBossLoot"],
"Source/Lostsense/Gameplay/Content/FirstSliceContent.h":[
 "KharosBlindline{10068U}","KharosBossLoot"],
"Source/Lostsense/Gameplay/Content/FirstSliceContent.cpp":[
 "KharosBlindline","KharosBossLoot"],
}
failures=[]
for rel,tokens in checks.items():
 p=ROOT/rel
 if not p.is_file():
  failures.append(f"missing {rel}");continue
 s=p.read_text(encoding="utf-8")
 for token in tokens:
  if token not in s: failures.append(f"{rel}: missing {token!r}")
if failures:
 print("Act VI acceptance FAILED")
 for f in failures: print(" -",f)
 sys.exit(1)
print("Act VI source acceptance passed")
