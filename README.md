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
  snapshot/restore support; and
- serialization-ready value-state snapshots with transactional validation.

Authoritative gameplay randomness is never hidden inside the damage math.
Callers can provide rolls directly or use `Core::DeterministicRandom`; the
combatant RNG overload consumes exactly two samples per attack for stable
stream advancement.

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
- Future Unreal modules will adapt these systems to actors, components, input,
  replication, rendering and assets without moving authoritative rules into
  engine-only code.

The next planned block is the portable effects, abilities and skill-graph
foundation. Unreal integration follows only after the gameplay core has a
coherent, tested dependency chain.
