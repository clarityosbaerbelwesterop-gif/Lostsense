# LOSTSENSE

LOSTSENSE is an original, data-driven action RPG in active development. The
current repository contains its engine-independent C++ gameplay core; it is not
yet an Unreal Engine project or a playable game build.

## Current runtime foundation

The portable C++20 layer currently provides:

- deterministic multi-type damage with armor, resistances, penetration,
  critical hits, blocking and per-type damage bonuses;
- a definition-driven attribute set with stable numeric IDs, bounded base
  values, deterministic additive/multiplicative modifiers and source-based
  removal;
- health and resource pools with explicit overkill, overheal, death, revive,
  underflow and overflow behavior;
- a reusable combatant model connecting attributes, power scaling, damage,
  health and resources for players, enemies, elites and bosses;
- a portable PCG random stream with stable cross-platform output and
  snapshot/restore support;
- deterministic gameplay effects with explicit stacking, timed DOT/HOT,
  status-owned attribute modifiers, immunity, cleanse/dispel, restrictions and
  transactional state restore; and
- a data-driven ability runtime with ownership, prerequisites, class and target
  validation, resource costs, cooldowns/groups, charges/recharge, direct combat
  resolution, effect application and transactional activation rollback;
- a validated graph-based skill/build runtime with deterministic allocation and
  refund, class/exclusive rules, skill-owned AttributeSet modifiers, ability
  unlock integration and stable mutation/tag extension hooks; and
- a portable ability loadout with authored slot compatibility, duplicate policy
  and transactional capture/restore across primary, secondary, four active,
  dodge, class-mechanic and ultimate slots; and
- validated item, rarity and affix definitions with persistent rolled item
  instances, transactional inventory/equipment ownership, exact equipment-owned
  AttributeSet modifiers, socket/unique hooks and deterministic loot generation
  driven only by the existing PCG random stream;
- a bounded typed gameplay event stream with monotonic sequencing, checkpoints,
  rollback and commit-only adapters around existing gameplay authorities; and
- versioned aggregate character persistence that composes the existing runtime
  snapshots, validates cross-system ownership/modifier invariants, restores
  transactionally, encodes doubles exactly and migrates supported V1 saves to
  the current V2 schema without raw-memory serialization.

Authoritative gameplay randomness is never hidden inside the damage math.
Callers can provide rolls directly or use `Core::DeterministicRandom`; the
combatant RNG overload consumes exactly two samples per attack for stable
stream advancement. Timed gameplay systems advance through explicit simulation
time rather than wall-clock state.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Enable strict warnings and runtime memory/undefined-behavior checks with:

```bash
cmake -S . -B build-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLOSTSENSE_WARNINGS_AS_ERRORS=ON \
  -DLOSTSENSE_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

When clang-format is installed, CMake also exposes:

```bash
cmake --build build --target format-check
```

GitHub Actions verifies Release builds with GCC and Clang, all correctness
tests, ASan/UBSan and formatting. No engine or paid service is required for the
portable core.

## Architecture boundary

- `Lostsense::Core` owns portable deterministic utilities.
- `Lostsense::Stats` owns extensible definitions, base values and modifiers.
- `Lostsense::Combat` owns damage rules, vital pools and combat resolution.
- `Lostsense::Gameplay` owns portable effects, abilities, skill/build graphs,
  loadouts, item/inventory/equipment state, loot, typed committed gameplay
  events and versioned aggregate persistence while consuming the existing
  combat, stats and deterministic RNG authorities.
- Future Unreal modules will adapt these systems to actors, components, input,
  replication, rendering and assets without moving authoritative rules into
  engine-only code.

The next milestone is the real Unreal integration foundation: verify the actual
engine/toolchain available, create the in-repository Unreal project and bridge
input/presentation to this portable authority without duplicating combat, loot,
progression or persistence rules. Unreal functionality is not claimed until it
is actually compiled and, where tooling permits, launched.

## Game canon and Unreal transition

The binding game/story/world/content authority now lives under `Design/`. Start with `Design/LOSTSENSE_GAME_AUTHORITY.md` and `Design/VERTICAL_SLICE_AUTHORITY.md`.

The Unreal transition targets Unreal Engine 5.8 and keeps the portable C++ runtime as the gameplay authority. Unreal-specific code is presentation/platform integration; it must not duplicate damage, effects, abilities, progression, item ownership, loot, persistence or event authority.

Truth boundary for the first Unreal source pass: the Work environment used to author it does not contain UnrealEditor or UnrealBuildTool. Unreal project/module code in this repository is therefore **authored source only** until a real UE 5.8 toolchain compiles and launches it.
