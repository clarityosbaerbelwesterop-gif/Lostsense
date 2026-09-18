#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
checks={
"Source/LostsenseGame/Private/LostsenseActFiveStoryActor.cpp":["80040","80041","80042","80043","CompleteCampaignQuest"],
"Source/LostsenseGame/Private/LostsenseActFiveWorldSubsystem.cpp":["Namarith","30034U","KeeperYsil","Reconstruction"],
"Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp":["KeeperYsil","CompleteCampaignQuest(80044)","GetObjectiveState(80044)"],
}
bad=[]
for p,toks in checks.items():
 s=(R/p).read_text()
 for t in toks:
  if t not in s: bad.append(f"{p}: {t}")
if bad:
 print("\n".join(bad));sys.exit(1)
print("Act V source acceptance passed")
