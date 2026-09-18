#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
checks={
"Source/LostsenseGame/Private/LostsenseActEightStoryActor.cpp":[
 "80070","80071","80072","80073","80075","CompleteCampaignQuest"],
"Source/LostsenseGame/Private/LostsenseActEightWorldSubsystem.cpp":[
 "Ashwake Wastes","Ember Court","Vask","Foundry Cathedral","20034",
 "30055U","IlyrVael","LastArchiveDoor"],
"Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp":[
 "Ashbound","IlyrVael","BossPhase = 3","CompleteCampaignQuest(80074)",
 "GetObjectiveState(80074)"],
}
bad=[]
for p,toks in checks.items():
 path=R/p
 if not path.is_file(): bad.append(f"missing {p}");continue
 s=path.read_text(encoding="utf-8")
 for t in toks:
  if t not in s: bad.append(f"{p}: missing {t!r}")
if bad:
 print("Act VIII acceptance FAILED")
 for x in bad: print(" -",x)
 sys.exit(1)
print("Act VIII source acceptance passed")
