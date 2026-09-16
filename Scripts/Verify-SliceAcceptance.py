#!/usr/bin/env python3
"""Static acceptance regression gates for Lostsense PR #9.

These checks protect the source contracts that can be verified on ordinary CI.
They deliberately do not claim to replace UE 5.8 UHT/UBT, PIE, packaged runtime,
controller/touch hardware, audio/animation asset validation or Pixel Streaming.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ERRORS: list[str] = []
PASSES: list[str] = []


def text(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        ERRORS.append(f"missing required slice file: {path}")
        return ""
    return target.read_text(encoding="utf-8")


def require(group: str, haystack: str, needles: tuple[str, ...]) -> None:
    failures = [needle for needle in needles if needle not in haystack]
    if failures:
        for needle in failures:
            ERRORS.append(f"[{group}] missing source invariant: {needle}")
        return
    PASSES.append(group)


def main() -> int:
    controller = text("Source/LostsenseGame/Private/LostsensePlayerController.cpp")
    hud = text("Source/LostsenseGame/Private/LostsenseDevelopmentHUD.cpp")
    menu = text("Source/LostsenseGame/Private/LostsenseMenuProjection.cpp")
    presentation = text(
        "Source/LostsenseGame/Private/LostsenseEnemyPresentationSubsystem.cpp"
    )
    presentation_header = text(
        "Source/LostsenseGame/Public/LostsenseEnemyPresentationSubsystem.h"
    )
    bridge = text("Source/LostsenseGame/Private/LostsenseInputBridgeLibrary.cpp")
    bridge_header = text("Source/LostsenseGame/Public/LostsenseInputBridgeLibrary.h")
    mechanism = text("Source/LostsenseGame/Private/LostsenseMechanismActor.cpp")
    mechanism_header = text("Source/LostsenseGame/Public/LostsenseMechanismActor.h")
    enemy = text("Source/LostsenseGame/Private/LostsenseEnemyCharacter.cpp")

    require(
        "inventory-and-scar-ui",
        controller + hud + menu,
        (
            "ToggleInventoryMenu",
            "ToggleScarAtlasMenu",
            "EKeys::I",
            "EKeys::Tab",
            "CaptureInventory",
            "CaptureScarAtlas",
            "AllocateSkillNode",
            "SCAR ATLAS // KNIGHT 102",
            "INVENTORY // FIELD LOADOUT",
        ),
    )

    require(
        "menu-input-isolation",
        controller,
        (
            "ApplyMenuInputMode",
            "GuardReleased(this)",
            "ControlledPawn->DisableInput(this)",
            "ControlledPawn->EnableInput(this)",
        ),
    )

    require(
        "odran-and-enemy-presentation-hooks",
        presentation + presentation_header,
        (
            "AttackTelegraph",
            "OdranBellGesture",
            "BossPhaseTransition",
            "Defeat",
            "BlueprintAssignable",
            "OnPresentationCue.Broadcast",
        ),
    )

    require(
        "touch-and-pixel-streaming-input-bridge",
        bridge + bridge_header,
        (
            "UBlueprintFunctionLibrary",
            "static void Move",
            "static void Look",
            "static void PrimaryAttack",
            "static void GuardPressed",
            "static void GuardReleased",
            "static void ActivateSkill",
            "static void Interact",
            "static void ToggleInventory",
            "static void ToggleScarAtlas",
            "GameplaySuppressed",
        ),
    )

    require(
        "counterweight-source-contract",
        mechanism + mechanism_header,
        (
            "ELostsenseMechanismKind::Counterweight",
            "bRaised = !bRaised",
            "ApplyCounterweightState()",
            "bRaised ? 0.0F : -520.0F",
            "ECollisionEnabled::QueryAndPhysics",
            "Restore counterweight",
        ),
    )

    require(
        "odran-death-transition-contract",
        enemy,
        (
            "bBossTransitionCommitted",
            "BossPhase = 2",
            "Story->CompleteBeat(ELostsenseStoryBeat::OdranDefeated)",
            "SetLifeSpan(bBoss ? 8.0F : 4.0F)",
        ),
    )

    print("LOSTSENSE PR #9 slice acceptance static gates")
    for item in PASSES:
        print(f"PASS [{item}]")
    if ERRORS:
        for error in ERRORS:
            print(f"FAIL {error}")
        print(f"RESULT: {len(ERRORS)} failure(s)")
        return 1

    print(f"RESULT: all {len(PASSES)} source acceptance groups passed")
    print(
        "NOTE: counterweight, input, presentation and gameplay still require a real UE 5.8 runtime acceptance pass."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
