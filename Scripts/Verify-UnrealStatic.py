#!/usr/bin/env python3
"""Layered repository-side validation for the Lostsense Unreal adapter.

This deliberately does not pretend to replace UHT/UBT or an Unreal runtime.
It catches project/config/module/reflection/include/authority-boundary regressions
on ordinary CI runners before a real UE 5.8 machine or Pixel Streaming host is used.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ERRORS: list[str] = []
PASSES: list[str] = []


def fail(filter_name: str, message: str) -> None:
    ERRORS.append(f"[{filter_name}] {message}")


def passed(filter_name: str, message: str) -> None:
    PASSES.append(f"[{filter_name}] {message}")


def read(path: str) -> str:
    file_path = ROOT / path
    if not file_path.is_file():
        fail("layout", f"missing required file: {path}")
        return ""
    return file_path.read_text(encoding="utf-8")


def filter_project_descriptor() -> None:
    name = "uproject"
    try:
        project = json.loads(read("Lostsense.uproject"))
    except json.JSONDecodeError as exc:
        fail(name, f"invalid JSON: {exc}")
        return

    if project.get("FileVersion") != 3:
        fail(name, "FileVersion must remain 3")
    if project.get("EngineAssociation") != "5.8":
        fail(name, "EngineAssociation must remain 5.8")

    modules = {entry.get("Name"): entry for entry in project.get("Modules", [])}
    expected_modules = {"Lostsense", "LostsenseGame"}
    if set(modules) != expected_modules:
        fail(name, f"modules must be exactly {sorted(expected_modules)}")
    else:
        if modules["Lostsense"].get("Type") != "Runtime":
            fail(name, "Lostsense must be a Runtime module")
        if modules["LostsenseGame"].get("Type") != "Runtime":
            fail(name, "LostsenseGame must be a Runtime module")

    plugins = {entry.get("Name"): entry for entry in project.get("Plugins", [])}
    for plugin in ("EnhancedInput", "PixelStreaming2"):
        if not plugins.get(plugin, {}).get("Enabled", False):
            fail(name, f"{plugin} must be explicitly enabled")

    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, "descriptor, UE 5.8 modules and required plugins are coherent")


def filter_targets_and_modules() -> None:
    name = "ubt-shape"
    game_target = read("Source/LostsenseGame.Target.cs")
    editor_target = read("Source/LostsenseGameEditor.Target.cs")
    game_rules = read("Source/LostsenseGame/LostsenseGame.Build.cs")
    portable_rules = read("Source/Lostsense/Lostsense.Build.cs")

    requirements = (
        (game_target, "Type = TargetType.Game", "Game target type"),
        (editor_target, "Type = TargetType.Editor", "Editor target type"),
        (game_target, "BuildSettingsVersion.Latest", "Game latest build settings"),
        (editor_target, "BuildSettingsVersion.Latest", "Editor latest build settings"),
        (game_target, "EngineIncludeOrderVersion.Latest", "Game include order"),
        (editor_target, "EngineIncludeOrderVersion.Latest", "Editor include order"),
        (game_target, "CppStandardVersion.Cpp20", "Game C++20"),
        (editor_target, "CppStandardVersion.Cpp20", "Editor C++20"),
        (game_rules, '"Lostsense"', "adapter depends on portable module"),
        (game_rules, '"EnhancedInput"', "adapter depends on EnhancedInput"),
        (portable_rules, "CppStandardVersion.Cpp20", "portable module C++20"),
    )
    for haystack, needle, label in requirements:
        if needle not in haystack:
            fail(name, f"missing {label}: {needle}")

    if "LostsenseGame" in portable_rules:
        fail(name, "portable Lostsense module must not depend on LostsenseGame")

    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, "Game/Editor targets and one-way module dependency are coherent")


def filter_reflection_headers() -> None:
    name = "uht-shape"
    public_dir = ROOT / "Source/LostsenseGame/Public"
    headers = sorted(public_dir.glob("*.h"))
    if not headers:
        fail(name, "no Unreal public headers found")
        return

    reflected = 0
    for header in headers:
        text = header.read_text(encoding="utf-8")
        if not any(token in text for token in ("UCLASS(", "USTRUCT(", "UINTERFACE(")):
            continue
        reflected += 1
        expected = f'#include "{header.stem}.generated.h"'
        includes = [line.strip() for line in text.splitlines() if line.strip().startswith("#include")]
        if expected not in includes:
            fail(name, f"{header.name}: missing {expected}")
        elif includes[-1] != expected:
            fail(name, f"{header.name}: generated.h must be the final include")
        if "GENERATED_BODY()" not in text:
            fail(name, f"{header.name}: reflected type lacks GENERATED_BODY()")

    if reflected == 0:
        fail(name, "no reflected Unreal types discovered")
    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, f"{reflected} reflected headers satisfy UHT source-shape invariants")


def filter_repository_includes() -> None:
    name = "includes"
    source_root = ROOT / "Source"
    adapter_root = ROOT / "Source/LostsenseGame"
    public_root = adapter_root / "Public"
    checked = 0

    for cpp in sorted((adapter_root / "Private").glob("*.cpp")):
        text = cpp.read_text(encoding="utf-8")
        for include in re.findall(r'^\s*#include\s+"([^"]+)"', text, flags=re.MULTILINE):
            candidate: Path | None = None
            if include.startswith("Lostsense/"):
                candidate = source_root / include
            elif include.startswith("Lostsense") and "/" not in include:
                candidate = public_root / include
            if candidate is not None:
                checked += 1
                if not candidate.is_file():
                    fail(name, f"{cpp.name}: repository include does not resolve: {include}")

    if checked == 0:
        fail(name, "no repository-owned adapter includes were checked")
    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, f"{checked} repository-owned adapter includes resolve")


def filter_authority_boundaries() -> None:
    name = "authority"
    adapter_root = ROOT / "Source/LostsenseGame"
    portable_root = ROOT / "Source/Lostsense"

    forbidden_rng = (
        "std::mt19937",
        "std::random_device",
        "FRandomStream",
        "FMath::Rand(",
        "FMath::RandRange(",
        "FMath::FRand(",
        "FMath::FRandRange(",
        "srand(",
    )
    for path in sorted(adapter_root.rglob("*")):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        for token in forbidden_rng:
            if token in text:
                fail(name, f"{path.relative_to(ROOT)} introduces parallel RNG token {token}")

    unreal_tokens = ("CoreMinimal.h", "UObject/", "Engine/", "UCLASS(", "USTRUCT(", "GENERATED_BODY()")
    for path in sorted(portable_root.rglob("*")):
        if path.suffix not in {".h", ".cpp"}:
            continue
        if path.as_posix().endswith("Source/Lostsense/Private/LostsenseModule.cpp"):
            continue
        text = path.read_text(encoding="utf-8")
        for token in unreal_tokens:
            if token in text:
                fail(name, f"{path.relative_to(ROOT)} leaks Unreal dependency into portable authority: {token}")

    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, "no parallel adapter RNG and no Unreal leakage into portable gameplay authorities")


def filter_config_and_entrypoint() -> None:
    name = "config"
    engine = read("Config/DefaultEngine.ini")
    required = (
        "GlobalDefaultGameMode=/Script/LostsenseGame.LostsenseGameMode",
        "DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput",
        "DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent",
    )
    for line in required:
        if line not in engine:
            fail(name, f"missing engine config invariant: {line}")

    game_mode = read("Source/LostsenseGame/Private/LostsenseGameMode.cpp")
    for token in ("ALostsenseKnightCharacter::StaticClass()", "ALostsensePlayerController::StaticClass()", "ALostsenseDevelopmentHUD::StaticClass()", "SpawnDevelopmentArena()"):
        if token not in game_mode:
            fail(name, f"development entrypoint missing: {token}")

    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, "GameMode, Enhanced Input and development encounter entrypoint are wired")


def filter_pixel_streaming_readiness() -> None:
    name = "pixel-streaming"
    project = read("Lostsense.uproject")
    verify_script = read("Scripts/Verify-Unreal.ps1")
    if '"Name": "PixelStreaming2"' not in project:
        fail(name, "PixelStreaming2 plugin is not enabled")
    for token in ("UnrealEditor", "RunUAT", "BuildCookRun"):
        if token not in verify_script:
            fail(name, f"Unreal verification script lacks {token} stage")
    if not any(error.startswith(f"[{name}]") for error in ERRORS):
        passed(name, "PixelStreaming2 is enabled and real-engine verification/package hooks are present")


def main() -> int:
    filters = (
        filter_project_descriptor,
        filter_targets_and_modules,
        filter_reflection_headers,
        filter_repository_includes,
        filter_authority_boundaries,
        filter_config_and_entrypoint,
        filter_pixel_streaming_readiness,
    )
    for check in filters:
        check()

    print("LOSTSENSE Unreal static verification")
    for message in PASSES:
        print(f"PASS {message}")
    if ERRORS:
        for message in ERRORS:
            print(f"FAIL {message}", file=sys.stderr)
        print(f"RESULT: {len(ERRORS)} failure(s), {len(PASSES)} filter(s) passed", file=sys.stderr)
        return 1
    print(f"RESULT: all {len(PASSES)} layered Unreal filters passed")
    print("NOTE: this is static verification; UHT/UBT, PIE/package and Pixel Streaming runtime remain real-engine gates.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
