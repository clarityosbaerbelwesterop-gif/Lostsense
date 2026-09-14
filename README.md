# Lostsense

Portable deterministic action-RPG runtime plus an authored Unreal Engine 5.8 integration foundation.

The portable C++ runtime remains the gameplay authority for combat, effects, abilities, progression, items, inventory, equipment, loot, persistence and gameplay events. The `Design/` directory contains the binding game/story/world/content authority. Unreal-specific source is presentation and platform integration and must not duplicate those portable authorities.

## Verification truth

- Portable C++: built and tested through the repository CMake/CTest CI.
- Game canon: designed and version-controlled under `Design/`.
- Unreal Engine source: authored for UE 5.8 where present.
- Unreal compile/runtime: must only be claimed after a real UnrealBuildTool/editor/game run. The current Work environment used for the first UE source pass does not contain UnrealEditor or UnrealBuildTool.

See `Design/LOSTSENSE_GAME_AUTHORITY.md` and `Design/VERTICAL_SLICE_AUTHORITY.md` for the production canon and first playable target.
