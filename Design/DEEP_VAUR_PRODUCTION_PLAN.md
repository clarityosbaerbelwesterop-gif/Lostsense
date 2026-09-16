# LOSTSENSE — Deep Vaur Production Execution Plan

Status: **IMPLEMENTATION PLAN — subordinate to canonical authorities**
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
- [ ] Wire deep-route state into the Unreal story/world adapter and save/load transaction.
- [ ] Build reusable rail/lift/brake/pressure/shortcut mechanism actors.
- [ ] Materialize abandoned industrial shafts / Lantern Rail approach.
- [ ] Expand deep-mine ecology and mechanism-teaching encounters.
- [ ] Materialize Royal Deepworks threshold / Pump Cathedral approach.
- [ ] Add deep-route static acceptance and portable regression gates.
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

`Gameplay/World/DeepRouteState` owns the ordered source-of-truth for the production route after Odran. It rejects skipped milestones, exposes typed mechanism state, bounds its text codec and validates restored combinations. Unreal may project or mutate it only through the adapter; Actor-local flags must not become a second progression authority.

## Acceptance boundary

This PR can become source/CI merge-ready without a local Unreal host only if that limitation remains explicit. The following remain separate real-engine gates when an authorized UE 5.8 host exists: UHT, UBT, PIE, collision/traversal feel, rail/counterweight/pressure physics, animation/audio/Niagara binding, packaged build and iPad Pixel Streaming acceptance.
