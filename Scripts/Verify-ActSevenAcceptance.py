#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
checks={
"Source/Lostsense/Gameplay/Story/ActSevenChoiceRuntime.h":[
 "BellgraveCivilianArchive","CrownlessCivilianArchive"],
"Source/Lostsense/Gameplay/Story/ActSevenChoiceRuntime.cpp":[
 "A7C1|"],
"Tests/Gameplay/ActSevenChoiceRuntimeTests.cpp":[
 "cannot be overwritten","round-trips exactly"],
"Source/LostsenseGame/Private/LostsenseStorySubsystem.cpp":[
 "LOSTSENSE_ACT_SEVEN","ResolveActSevenArchiveChoice","Objective(80062U)",
 "ArchiveQuestCompleted != ArchiveChoiceResolved"],
"Source/LostsenseGame/Private/LostsenseActSevenWorldSubsystem.cpp":[
 "A Mercy Measured in Names","20012","30047U","SaelRhyne","PaperDead"],
"Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp":[
 "SaelRhyne","BossPhase = 3","CompleteCampaignQuest(80064)",
 "GetObjectiveState(80064)"],
}
bad=[]
for p,toks in checks.items():
 path=R/p
 if not path.is_file(): bad.append(f"missing {p}"); continue
 s=path.read_text(encoding="utf-8")
 for t in toks:
  if t not in s: bad.append(f"{p}: missing {t!r}")
if bad:
 print("Act VII acceptance FAILED")
 for x in bad: print(" -",x)
 sys.exit(1)
print("Act VII source acceptance passed")
