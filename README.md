# LOSTSENSE

LOSTSENSE is an original, data-driven action RPG in active development. The
repository now contains both its engine-independent C++ gameplay authority and
an Unreal Engine 5.8 source-level game adapter. The Unreal source has not yet
been compiled or launched on a genuine UE 5.8 host, so the repository does not
claim a runtime-verified, packaged or stream-verified game build yet.

## Current gameplay authority

The portable C++20 layer provides deterministic multi-type combat, attributes,
health/resources, gameplay effects, abilities, cooldowns/charges, skill graphs,
ability loadouts, item/rarity/affix definitions, inventory, equipment,
deterministic loot, typed committed gameplay events and versioned transactional
character persistence.

PR #9 extends that authority with:

- a typed first-slice Story Runtime covering Returned awakening through changed
  Bellgrave, prerequisite-safe objective progression and bounded deterministic
  story serialization/restore;
- atomic multi-target ability semantics with one resource/cooldown commit,
  duplicate/dead-target rejection and full target/RNG rollback on late failure;
- March Sweep authored for up to eight targets rather than the old single-target
  development contract; and
- a bounded Perfect Guard effect used by the Unreal input adapter while portable
  combat remains responsible for the actual block result and Resolve reward.

Authoritative gameplay randomness is never hidden inside presentation code.
Callers can provide rolls directly or use `Core::DeterministicRandom`; timed
systems advance through explicit simulation time rather than wall-clock state.

## Unreal vertical-slice source

The Unreal Engine 5.8 adapter now source-implements the first production route:

Bellgrave → Ravelwood Edge → Weeping Cut → Upper Vaur Goldworks → Coinless
Shaft → Odran → changed Bellgrave.

The current source-driven production graybox includes Bellgrave's stabilization
belfry, Mara Venn's bellsmith, Hadrun Pike's guard yard, Tamsin Coil's salvage
stall, Ninth Cage lift house, residential lanes and Ravelwood road; a forked
Ravelwood edge; descending Weeping Cut; vertically tiered Upper Vaur; authored
Coinless Shaft room progression; and Odran's counterweight chamber.

The slice also contains a reusable interaction contract, Mara/Hadrun/Tamsin NPC
actors, route story gates, Ninth Descent discoveries, interactive hanging
counterweights, reward lift logic, explicit-interaction world loot, distinct
Bell-Maddened Carrion / Charter Deserter / Echo Miner / Haul Construct / Foreman
Kett archetype tuning, readable AI windup/recovery states, one-shot Odran phase
transition protection, March Sweep arc acquisition and an objective-aware HUD.
Graybox geometry and primitive markers are implementation scaffolding, not final
art.

PR #9 now also includes a usable source-level inventory and Scar Atlas menu pass,
including authored item/node projection, prerequisite and exclusive-oath state,
Scar allocation and active-slot wiring. Inventory confirmation equips the
highlighted recovered gear through gameplay authority rather than falling back
to the first compatible item. Gameplay input is isolated while those menus are
open. A Blueprint-callable input bridge routes touch/virtual controls through
the same Knight interaction/combat paths for the future iPad Pixel Streaming UI
rather than creating parallel touch-only gameplay logic.

Enemy and boss presentation are separated from gameplay authority through a
world presentation subsystem that publishes Blueprint-assignable cues for AI
state changes, attack telegraphs, Odran bell gestures, the one-shot phase
transition and death presentation. Animation, audio and Niagara assets can bind
to those cues later without moving combat truth into presentation code.

## Build and test

Portable verification:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLOSTSENSE_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Sanitizer verification:

```bash
cmake -S . -B build-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLOSTSENSE_WARNINGS_AS_ERRORS=ON \
  -DLOSTSENSE_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

GitHub Actions verifies GCC Release, Clang Release, ASan/UBSan, clang-format and
layered Unreal source checks. The Unreal filters validate the project descriptor,
Game/Editor target shape, UHT reflected-header shape, repository includes,
one-way authority boundaries, production GameMode wiring and Pixel Streaming 2
readiness. PR #9 additionally has static slice-acceptance gates covering the
inventory/Scar UI source contract, menu input isolation, enemy/Odran presentation
hooks, touch/Pixel Streaming input bridge, counterweight source contract and
Odran transition/death contract. These checks are deliberately not presented as
a substitute for UHT, UBT, PIE, packaging or real-device/runtime verification.
The PR is not treated as merge-ready while any exact-head CI job is red.

## Architecture boundary

- `Lostsense::Core` owns portable deterministic utilities.
- `Lostsense::Stats` owns extensible definitions, base values and modifiers.
- `Lostsense::Combat` owns damage rules, vital pools and combat resolution.
- `Lostsense::Gameplay` owns effects, abilities, story state, progression,
  loadouts, items/inventory/equipment, loot, committed gameplay events and
  persistence.
- `LostsenseGame` adapts those authorities to Unreal actors, input, interaction,
  encounter orchestration and presentation. It must not create parallel combat,
  item ownership, loot RNG, skill ownership or story truth.

## Game canon and Unreal validation boundary

Binding game/story/world/content authority lives under `Design/`. Start with
`Design/LOSTSENSE_GAME_AUTHORITY.md` and `Design/VERTICAL_SLICE_AUTHORITY.md`.

The project targets Unreal Engine 5.8. Pixel Streaming 2 is enabled and a
packaged-runtime launcher is present for the eventual browser/iPad test path.
The current execution environment still does not provide `UnrealEditor`,
`UnrealBuildTool` or `RunUAT`, and no authorized UE 5.8 GPU host/runner is
currently attached. Therefore current Unreal work is **SOURCE-IMPLEMENTED** and
static-verified, not **COMPILED**, **RUNTIME-VERIFIED**, **PLAYABLE**,
**PACKAGED** or **STREAM-VERIFIED**. Counterweight traversal/combat geometry,
touch ergonomics and presentation asset binding remain real-engine/device gates.
Those labels become valid only after the real engine gates run.
