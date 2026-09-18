# Lostsense full-game production gate

This branch begins the transition from the validated Act I/II source slice into a shippable game shell.

## What is authoritative now

The portable campaign catalog registers all 47 canonical main quests across Acts I–IX and all 15 canonical world regions. It is data authority for journal/map presentation only; it does not falsely mark unbuilt acts as playable.

The Lostsense game-user-settings class provides persistent frame-rate, camera-shake and damage-number preferences on top of Unreal's existing resolution, fullscreen, VSync and scalability persistence.

## Higgsfield CLI

The official MIT-licensed higgsfield-ai/cli is the production interface for concept/keyframe, video, 3D and animation generation. Assets/Higgsfield/manifest.json contains original Lostsense prompts and Scripts/Plan-HiggsfieldAssets.py emits reviewable CLI commands.

The CLI binary being open source does not make model generations unmetered. Generation must not silently spend credits or start a trial. Generated media must be visually reviewed before it becomes game content.

## Ship gate

A Steam/Epic/Xbox/PlayStation build is not called complete until a real Unreal Engine 5.8 host has passed UHT/UBT, cooked/package builds, controller navigation, save migration, performance captures and a start-to-credits playthrough. Console packaging additionally requires the platform-holder SDK and developer entitlement.
