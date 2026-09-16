# LOSTSENSE — Deep Vaur Production Execution Plan

Status: **SOURCE IMPLEMENTATION COMPLETE — exact-head acceptance pending**
Base: merged PR #9 (`657636dca35dc4c2a8077176055e64efcb609030`)

This file does not redefine canon. `WORLD_AUTHORITY.md`, `STORY_AUTHORITY.md`, `DUNGEON_BOSS_AUTHORITY.md`, and the other canonical design authorities remain binding.

## Objective

Continue downward from the completed Bellgrave → Upper Vaur → Coinless Shaft → Odran vertical slice and make the Vaur network begin to feel like a continuous underground world rather than a sequence of disconnected arenas.

The next production path is:

**Upper Vaur Goldworks → abandoned industrial shafts / Lantern Rail approach → Royal Deepworks threshold → Pump Cathedral approach → later Crown Excavation / King's Bore.**

## Ordered implementation checklist

- [x] Merge PR #9 only after exact-head CI is green.
- [x] Re-read world/story/dungeon authority and establish PR #10 from merged main.
- [x] Add a deterministic deep-route state authority with bounded serialization.
- [x] Wire deep-route state into the Unreal story/world adapter and save/load transaction.
- [x] Build reusable rail/lift/brake/pressure/shortcut mechanism actors.
- [x] Materialize abandoned industrial shafts / Lantern Rail approach.
- [x] Expand deep-mine ecology and mechanism-teaching encounters.
- [x] Materialize Royal Deepworks threshold / Pump Cathedral approach.
- [x] Add deep-route static acceptance and portable regression gates.
- [ ] Run exact-head CI, audit failures, repair, repeat until green.
- [ ] Final PR #10 acceptance review and merge only when the source/CI contract is satisfied.

## Production pillars

1. **Readable depth.** Timber, practical lamps and Charter rails gradually give way to royal stone, heraldry, oversized pumps and failed gilded machinery.
2. **Continuous geography.** Shafts, cages, rails, ventilation trunks and shortcuts must preserve believable relationships to Bellgrave/Ravelwood above and Royal Deepworks below.
3. **Mechanisms are gameplay.** Rail switches, cage lifts, brakes, pressure valves and ventilation routing alter traversal and encounters; they are not decorative buttons.
4. **Authored spaces.** Critical routes are hand-authored. No randomized-room substitute for campaign dungeons.
5. **Portable authority stays authoritative.** Unreal actors adapt combat, items, skills, story and persistence; they do not duplicate those systems.
6. **Persistent consequences.** Restored lifts, opened shortcuts and pressure states must be representable by authoritative persistent world/story state rather than transient actor-only booleans where progression depends on them.
7. **No false runtime claims.** Source/static completion is not UE 5.8 UHT/UBT/PIE/package/Pixel Streaming verification.

## Deep-route authority

`Gameplay/World/DeepRouteState` owns the ordered source-of-truth for the production route after Odran. It rejects skipped milestones, requires the corresponding mechanism state before route advancement, prevents completed one-way mechanisms from regressing, bounds its text codec and validates restored combinations. Unreal may project or mutate it only through the adapter; Actor-local flags must not become a second progression authority.

## Current source production

The Deep Vaur block now has source-authored continuous geometry below Odran: an abandoned Lantern Rail exchange with a maintenance branch, a brake-service chamber and moving service cage; a Royal Deepworks threshold with monumental pressure lock; a vertical ventilation nave with safe relief and direct pressure lanes; a persistent Vaur return shortcut; and the Pump Cathedral approach gate. Rail carts, cage movement and ventilation rotors project authoritative mechanism state rather than owning progression.

Deep ecology now introduces Gilded Dead as slow high-armor royal holdovers, Pressure Mutants with mixed physical/Arcane pressure bursts, and Rail Marshals as faster elite lane controllers alongside existing Echo Miners and Haul Constructs. These roles differ in armor, movement, range, windup/recovery cadence and attack composition rather than only health totals.

The save/load audit found one presentation divergence risk: a loaded deep-route state could update authority while already-spawned static mechanism Actors retained their old transform. Deep mechanism presentation now periodically reprojects the authoritative state, while rail carts, cages and ventilation movers already derive their targets every tick. Save restore therefore cannot leave the visible route on a stale pre-load mechanism state.

## Acceptance boundary

This PR can become source/CI merge-ready without a local Unreal host only if that limitation remains explicit. The following remain separate real-engine gates when an authorized UE 5.8 host exists: UHT, UBT, PIE, collision/traversal feel, rail/counterweight/pressure physics, animation/audio/Niagara binding, packaged build and iPad Pixel Streaming acceptance.
