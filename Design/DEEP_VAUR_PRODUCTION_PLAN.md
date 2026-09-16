# LOSTSENSE — Deep Vaur Production Execution Plan

Status: **IMPLEMENTATION PLAN — subordinate to canonical authorities**
Base: merged PR #9 (`657636dca35dc4c2a8077176055e64efcb609030`)

This file does not redefine canon. `WORLD_AUTHORITY.md`, `STORY_AUTHORITY.md`, `DUNGEON_BOSS_AUTHORITY.md`, and the other canonical design authorities remain binding.

## Objective

Continue downward from the completed Bellgrave → Upper Vaur → Coinless Shaft → Odran vertical slice and make the Vaur network begin to feel like a continuous underground world rather than a sequence of disconnected arenas.

The next production path is:

**Upper Vaur Goldworks → abandoned industrial shafts / Lantern Rail approach → Royal Deepworks threshold → Pump Cathedral approach → later Crown Excavation / King's Bore.**

This block prepares the geography and systems required for Act IV without skipping the campaign's authored order.

## Production pillars

1. **Readable depth.** Timber, practical lamps and Charter rails gradually give way to royal stone, heraldry, oversized pumps and failed gilded machinery.
2. **Continuous geography.** Shafts, cages, rails, ventilation trunks and shortcuts must preserve believable relationships to Bellgrave/Ravelwood above and Royal Deepworks below.
3. **Mechanisms are gameplay.** Rail switches, cage lifts, brakes, pressure valves and ventilation routing alter traversal and encounters; they are not decorative buttons.
4. **Authored spaces.** Critical routes are hand-authored. No randomized-room substitute for campaign dungeons.
5. **Portable authority stays authoritative.** Unreal actors adapt combat, items, skills, story and persistence; they do not duplicate those systems.
6. **Persistent consequences.** Restored lifts, opened shortcuts and pressure states must be representable by authoritative persistent world/story state rather than transient actor-only booleans where progression depends on them.
7. **No false runtime claims.** Source/static completion is not UE 5.8 UHT/UBT/PIE/package/Pixel Streaming verification.

## Implementation sequence

### A. Deep-route state

- Extend the typed story/world progression model only where required for durable deep-route gates.
- Add stable IDs for restored mechanisms and discovered deep-route anchors.
- Preserve transactional save/load behavior and reject impossible restored states.

### B. Mine-network mechanisms

Build reusable source-level mechanism contracts for:

- cage lift call/send/locked states;
- rail switch routing and cart hazard lanes;
- brake restoration;
- ventilation/pressure valves;
- pressure doors and relief vents;
- authored shortcut unlocks.

Mechanisms must expose presentation state without making presentation authoritative.

### C. Abandoned industrial shafts / Lantern Rail approach

Materialize a coherent route beneath the existing Upper Vaur slice:

- descending rail exchange;
- derailed maintenance gallery;
- lift/brake service chamber;
- worker staging alcoves and abandoned shift evidence;
- optional repair crawl / shortcut;
- first royal masonry intrusion visible before the threshold.

Combat spaces must retain traversal exits and not become sealed rectangles without geographic justification.

### D. Deep-mine ecology

Expand existing ecology with deeper variants and roles rather than health-only reskins:

- Echo Miner pressure groups;
- Haul Construct route denial / charge geometry;
- royal-era gilded dead at the transition;
- pressure-mutant foreshadowing near ventilation failures;
- an elite encounter that teaches rail/pressure interaction before Pump Cathedral.

All attacks remain readable through explicit windup/attack/recovery presentation hooks.

### E. Royal Deepworks threshold / Pump Cathedral approach

Create the first Royal Deepworks production graybox showing the material and scale transition:

- monumental pump intake;
- pressure lock sequence;
- vertical ventilation nave;
- safe relief route versus dangerous high-pressure shortcut;
- visible deeper bore infrastructure;
- authored return shortcut toward the Vaur network.

Do not prematurely implement the entire Act IV boss chain in one undifferentiated commit.

### F. Verification and repair

Every implementation slice must preserve:

- GCC Release + warnings-as-errors;
- Clang Release + warnings-as-errors;
- ASan/UBSan;
- clang-format;
- Unreal layered static filters;
- portable deterministic tests;
- new deep-route persistence/mechanism regression tests;
- source acceptance checks for authored route/mechanism integration.

Any discovered state divergence, duplicate authority, unsafe restore path, non-deterministic gameplay ordering or input collision is fixed before the block is treated as complete.

## Acceptance boundary

This PR can become source/CI merge-ready without a local Unreal host only if that limitation remains explicit. The following remain separate real-engine gates when an authorized UE 5.8 host exists: UHT, UBT, PIE, collision/traversal feel, rail/counterweight/pressure physics, animation/audio/Niagara binding, packaged build and iPad Pixel Streaming acceptance.
