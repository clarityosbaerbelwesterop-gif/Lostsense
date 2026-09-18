# LOSTSENSE — Act III Crownless Reach Production Plan

Status: implementation plan subordinate to STORY_AUTHORITY, WORLD_AUTHORITY, CHARACTER_AUTHORITY and DUNGEON_BOSS_AUTHORITY.

## Production target

Turn the canonical Act III chain into a traversable source-level production route without skipping the campaign order:

Crownless approach → civic plaza / Regent Solenne Mave → erased royal ledgers → Gilded Charter evidence hall → Palace of Empty Names → contradictory archive → Regent-Engine Caldris.

The route uses canonical region 1004, dungeon 20010 and boss 30017. It is a continuation of the Act II witness-root resolution and the deterministic campaign runtime.

## Gameplay rules

- Quest 80020–80024 progress only in canonical order.
- Solenne is the civic entry beat, not a disposable combat NPC.
- Court Shades create mobile pressure; Blank Knights are armored lane holders.
- Caldris cannot aggro or receive player ability damage before quest 80024 is active. This prevents sequence-break softlocks.
- The Palace descends physically toward Royal Deepworks to preserve the world-above/world-below relationship.
- Evidence interactions are explicit player actions and persist through the campaign save authority.
- Source/static CI does not substitute for UE 5.8 UHT/UBT/PIE, navigation, collision, animation, lighting or packaged-build validation.

## Acceptance

The source pass is complete when Crownless Reach has an authored continuous route, all four pre-boss quest interactions are wired, Caldris is a gated boss with canonical ID 30017, boss defeat advances 80024, save/restore remains deterministic, and CI verifies the source contracts.
