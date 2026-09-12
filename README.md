# LOSTSENSE

LOSTSENSE is being built as a data-driven, cooperative action RPG. The repository
currently contains the first engine-independent gameplay foundation: a deterministic
combat damage pipeline that can be shared by authoritative server code, simulations,
and automated tests before an Unreal-facing adapter is added.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Enable runtime memory and undefined-behaviour checks with:

```bash
cmake -S . -B build-sanitize -DLOSTSENSE_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

## Combat foundation

`Lostsense::Combat::DamageCalculator` is a stateless, deterministic pipeline. The
caller supplies the critical-hit roll, making the outcome reproducible for server
authority, network correction, replays, and save/load verification. Its order of
operations is documented in the public header and covered by focused tests.

The module intentionally has no engine dependency. Unreal data assets can translate
their values into `DamageRequest` while C++ remains responsible for the rules. This
keeps authored skills, weapons, enemies, and bosses out of hard-coded class trees.
