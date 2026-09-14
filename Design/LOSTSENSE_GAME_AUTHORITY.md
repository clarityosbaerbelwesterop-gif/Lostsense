# LOSTSENSE — Game Authority

Status: **CANONICAL DESIGN AUTHORITY**  
Scope: identity, terminology, content-ID policy, art direction, world rules, implementation truth.  
Authority precedence: this file defines global canon; specialized files in `Design/` refine it. If two authorities conflict, the more specialized authority wins only when it explicitly says it is overriding this file.

## 1. Product identity

**LOSTSENSE** is an original dark-fantasy action RPG set in **Avarra**, a vertically interconnected world whose geology, civilizations and metaphysics are layered from surface settlements down through mines, buried cities, the Abyss, Hell and the final archival depths.

The intended player experience combines:

- deep ARPG loot and build construction;
- responsive high-quality action combat;
- readable, learnable boss encounters where telegraphs and punish windows matter;
- Terraria-like curiosity about what exists below every visible place;
- a connected world rather than a lobby of disconnected biome levels;
- extreme long-term build diversity without filler systems or content-count inflation.

LOSTSENSE must not be a reskin of another ARPG. Its identity comes from **senseprints, witness gold, resonant architecture, vertical history and the conflict between lived contradiction and perfect remembered order**.

## 2. The Lostsense phenomenon

Every living act of perception leaves a weak informational residue called a **senseprint**. Most materials lose senseprints almost immediately. Deep Avarran gold retains them.

That gold is commonly called **witness gold**. When heavily saturated, witness gold can preserve fragments of sight, pain, intention, spatial expectation and memory for centuries.

**Lostsense** is a fracture between physical reality and accumulated senseprints. When it intensifies, the world and the remembered expectation of the world stop agreeing. Examples include:

- a door opening onto the place generations of miners expected behind it rather than the current tunnel;
- heat visibly warping air around a cold object;
- the dead repeating another person's memories;
- architecture preserving routes removed centuries earlier;
- cause and effect appearing displaced in time without breaking combat readability.

Gameplay rule: Lostsense may alter space, enemy behavior, quests, traversal and encounter rules, but must never deliberately sabotage accessibility or basic combat readability. It is a world rule, not an excuse for arbitrary mechanics.

## 3. Ancient truth

The buried civilization of **Namarith** learned to refine witness gold into a continent-scale consensus-memory machine: **the Loom Below**.

The Loom was designed to reduce famine, war and catastrophic uncertainty by preserving shared truth. Its operators eventually tried to remove contradiction itself. The result split reality into three linked layers:

1. **Avarra** — physical lived reality.
2. **The Abyss** — the deficit space created where reality and archived expectation no longer reconcile.
3. **The Red Archive**, called **Hell** by later cultures — the deep rejection/burn layer where corrupted or unwanted senseprints were sent for destruction.

Neither the Abyss nor Hell is a conventional afterlife. Both became real ecosystems and political territories after millennia of exposure to living senseprints.

## 4. Player identity

The player is a customizable protagonist with one authored narrative role: **the Returned Surveyor**.

The Returned joined the **Ninth Descent**, an expedition into the royal deep mines. The expedition vanished. Seven years later the player emerges from a sealed shaft beneath Bellgrave with missing memories, an abnormal resistance to Lostsense, and an impossible senseprint that appears in Namarith records older than the current civilizations.

The six classes are not six separate protagonists. Each class is the Returned's remembered discipline before the Ninth Descent. Dialogue, quests and cinematics address the player as the Returned/Surveyor and support customization.

## 5. Central conflict

**Prelate Ilyr Vael**, leader of the Candescent Synod, believes the world's accumulated contradictions are causing Lostsense. He seeks the **Second Silence**: a controlled reset that would erase enough current senseprints to stabilize reality.

The deeper antagonist is **Aster Null, the First Witness**, a Namarith archivist who merged with the Loom. Aster manipulates Vael and multiple factions because its final objective is a contradiction-free world in which every consciousness is collapsed into one perfectly consistent total memory.

The story asks whether identity is worth the instability created by difference, error, grief and conflicting memory.

## 6. Content ID policy

Production-authored content uses stable numeric IDs. Existing runtime/test IDs are **non-canon test fixtures** and do not reserve production meaning.

| Range | Authority |
|---:|---|
| 100–199 | Classes |
| 1,000–1,999 | Regions / major subregions |
| 2,000–2,999 | Factions |
| 3,000–3,999 | Major characters |
| 10,000–19,999 | Items / weapons |
| 20,000–29,999 | Dungeons / authored encounter spaces |
| 30,000–39,999 | Bosses / boss-tier encounters |
| 40,000–49,999 | Abilities |
| 50,000–59,999 | Skill nodes |
| 60,000–69,999 | Effects / statuses |
| 70,000–79,999 | Loot tables |
| 80,000–89,999 | Main/side story quests |

Stable IDs are never reassigned after accepted content ships. Names may be localized; IDs remain authoritative.

## 7. Canonical class IDs

| ID | Class |
|---:|---|
| 101 | Assassin |
| 102 | Knight |
| 103 | Witch |
| 104 | Ranger |
| 105 | Wizard |
| 106 | Arsonist |

## 8. Visual language

LOSTSENSE avoids generic dark-fantasy visual noise. Its repeated visual grammar is:

- **memory metallurgy:** witness gold appears as bruised, oxidized, veined conductor material rather than treasure-room shine;
- **resonant infrastructure:** stabilization bells, tuning forks, suspended plates, vibration ribs and weighted chains are practical architecture;
- **visible vertical history:** walls and mines expose older structures beneath newer ones instead of hiding transitions behind portals;
- **asymmetric repair:** settlements are built around warped preexisting structures, giving strong silhouettes rather than uniform medieval streets;
- **regional color logic:** each region owns a narrow functional palette, and VFX accents respect it;
- **Abyss language:** absence, occluded geometry, matte mineral surfaces, silent moving structures and negative-space silhouettes rather than purple fog caves;
- **Hell language:** furnaces, memory ash, pressure vessels, black-red ceramic, slag canals and bureaucratic industrial monuments rather than endless generic lava.

Major bosses, weapons, regions and characters need at least one idea that would be recognizably Lostsense if shown without a logo.

## 9. World scale principle

Long-term targets remain ambitious—six classes, 1,000+ meaningful skill nodes, hundreds of dungeons and boss-tier encounters, deep weapon catalogs and 1–4 player co-op—but production proceeds through authored slices. Quantity never justifies duplicates, reskins or unverified claims.

## 10. Implementation truth levels

Every content/system reference should use one of these truth levels:

- **DESIGNED:** canon exists in authority documents.
- **AUTHORED SOURCE:** code/project/configuration exists in the repository but may not have been compiled in its target engine.
- **COMPILED:** target toolchain successfully built it.
- **RUNTIME-TESTED:** actual game/editor execution proved the described behavior.
- **PLAYABLE:** user input reaches real gameplay and changes authoritative state through the complete intended loop.

No document may call designed material implemented or call authored Unreal source compiled until UnrealBuildTool actually succeeds.

## 11. Production order after this authority

1. Build the Unreal project shell and portable-runtime bridge without duplicating gameplay rules.
2. Implement Knight as the first production slice class.
3. Implement Bellgrave March → Ravelwood Edge → Weeping Cut → Upper Vaur Goldworks → Coinless Shaft → Odran.
4. Prove combat, loot, equipment, skills, events and save/load in-engine.
5. Expand vertically into the Royal Deepworks and Namarith layers before broad horizontal content production.

## 12. Engine and rendering target — verified 2026-09-14

The authored engine target for the first integration is **Unreal Engine 5.8**, because Epic's current official 5.8 documentation is available and documents the required UE5 feature set. The Work environment used to author this authority does **not** contain UnrealEditor or UnrealBuildTool, so repository Unreal source created in this milestone is `AUTHORED SOURCE`, not `COMPILED`.

Rendering architecture rules:

- normal gameplay must not require Path Tracing;
- TSR is the engine-native baseline temporal upscaling path;
- Nanite, Lumen and Virtual Shadow Maps are target technologies where scene/material/platform constraints justify them;
- World Partition is the intended large-world streaming architecture once real world assets exist;
- Niagara and Chaos are presentation/physics tools, not replacements for portable gameplay authority;
- NVIDIA DLSS remains optional. NVIDIA's current official Unreal plugin offering is **DLSS 4.5** for UE 5.8, including Super Resolution/DLAA, Frame Generation/Multi Frame Generation, Dynamic MFG, Ray Reconstruction and Reflex. Do not label an integration “DLSS 5” without an official production plugin/API actually used by the project;
- long-term user-facing upscaling abstraction should permit TSR, DLSS, FSR and XeSS rather than hard-coding gameplay to one vendor path;
- Performance, Quality, Ultra and Cinematic/Path-Tracing tiers are long-term configuration targets, not current measured profiles.

No FPS number becomes canon without a captured build/platform/profile measurement.
