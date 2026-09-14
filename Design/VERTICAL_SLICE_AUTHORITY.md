# LOSTSENSE — First Vertical Slice Authority

Status: **CANONICAL PRODUCTION TARGET**  
Truth level today: **DESIGNED**. Portable runtime systems are implemented; Unreal slice is not yet compiled or playable.

## 1. Slice promise

The first playable Lostsense slice must prove that the repository can deliver an actual game loop, not only portable simulations.

Route:

**Bellgrave → Ravelwood Edge → Weeping Cut → Upper Vaur Goldworks → Coinless Shaft → Odran, the Bell Without Tongue → return to changed Bellgrave.**

The player uses **Knight (Class 102)**.

## 2. Minimum player loop

A playable slice requires all of the following through real input:

1. Launch into Bellgrave.
2. Move and control an isometric/action camera.
3. Talk/interact with Mara Venn and Hadrun Pike.
4. Use Primary Attack, Heavy Attack, Dodge and Guard.
5. Equip four active ability slots; at least two have authored functional Knight abilities in the slice.
6. Fight a standard enemy family and an Elite.
7. Take damage, die and restart from an authored anchor.
8. Kill enemies that call portable `LootRuntime`.
9. See a world drop, pick it up, add it to portable Inventory.
10. Open minimal inventory UI and equip an item through portable `EquipmentRuntime`.
11. Observe the resulting authoritative AttributeSet change affecting combat.
12. Gain a skill point, allocate a real Scar Atlas node, unlock/apply its effect or ability, and assign an ability to a slot.
13. Enter Weeping Cut and the Coinless Shaft as authored spaces rather than test boxes.
14. Fight Odran with at least two phases and real telegraphs.
15. Receive Odran's loot/story consequence.
16. Save, exit/restart where environment allows, load and restore equivalent portable character state.

## 3. Opening locations

### Bellgrave

Small production settlement, not a full city. Required spaces:

- central stabilization belfry and Returned emergence point;
- Mara's bellsmith/workbench;
- Hadrun's guard yard;
- Tamsin's salvage stall;
- Ninth Cage lift house;
- two residential lanes proving lived-in scale;
- road to Ravelwood edge;
- mine access that visually descends beneath the belfry.

### Ravelwood Edge

First wilderness encounter space demonstrating:

- readable traversal outside settlement;
- one fork/secret;
- Rootbound enemy ecology;
- vertical visual hint that roots have penetrated mine ceilings below;
- access to Weeping Cut.

### Weeping Cut

Compact cave with a non-boss climax. It introduces memory seepage through stone, contains the Ninth Descent plate and creates a physical shortcut to Upper Vaur.

### Upper Vaur Goldworks

First mine region section with rails, lift cages, working/failed machinery and multiple height levels. It must visibly be beneath Bellgrave/Ravelwood, not a disconnected cave tileset.

### Coinless Shaft — Dungeon 20,003

Required authored flow:

**sealed entry → descending rail gallery → collapsed counting room → split route/secret maintenance crawl → haul-engine encounter → Bellwarden elite → silent pre-boss belfry → Odran arena → bell-core reward lift.**

Signature mechanic: sections of the shaft are stabilized by hanging counterweights. The player can restore or release selected weights to change traversal and combat cover. This is authored level logic, not a random dungeon generator.

## 4. Slice enemies

### Bell-Maddened Carrion
Fast scavenger beast attracted to resonance. Teaches dodge/spacing.

### Charter Deserter
Human melee baseline with readable guarded and unguarded states.

### Echo Miner
Slow miner repeating an archived work cycle until disturbed; heavy telegraphed pick attacks and short memory-repeat combo.

### Haul Construct
Industrial machine enemy; exposes a side weak point after charge recovery.

### Elite — Foreman Kett, Still on Shift
An Echo Miner elite that orders nearby constructs into synchronized work patterns and teaches effect/control interruption.

## 5. Slice Knight content

### Core actions

- Light weapon chain
- Heavy committed strike
- Dodge
- Guard
- Perfect Guard → Resolve gain
- Stagger response

### Initial active abilities

Production IDs reserved in class range:

- 40120 **Bellstep** — short armored advance ending in controlled shield/weapon check.
- 40121 **Measure Breaker** — Resolve-spending single-target guard/stagger breaker.
- 40122 **March Sweep** — wide control attack with lower single-target value.
- 40123 **Answering Guard** — timed guard art that primes a counter on success.

Only abilities genuinely wired through `AbilityRuntime` and engine presentation count as implemented.

### Slice Scar Atlas

Author 36 Knight nodes per `CLASS_COMBAT_AUTHORITY.md`; only a smaller initial unlock path must be exposed in the first playable loop. Skill UI may be developer-focused but must operate on real `SkillTreeRuntime` state.

## 6. Slice equipment

Prioritize production implementation of these weapons/items:

- 10021 Gravesong Longsword
- 10022 Bellguard Sabre
- 10026 March Spear
- 10027 Tongueless Shield
- 10028 Odran's Clapper

Add a bounded set of armor/material drops sufficient to test affixes and slots. Do not add dozens of meaningless names to inflate the slice.

## 7. Odran — first main boss

Boss ID: **30,001**.

**Arena:** circular counterweight chamber inside Coinless Shaft. A huge stabilization bell hangs above but has no clapper. The arena floor has four resonance plates and two breakable hanging counterweight lines.

**Phase 1 — Warden:** Odran fights as a disciplined heavy Bellwarden: shield shoulder, maul arcs, delayed overhead, guard break. Core lesson is telegraph recognition and punish windows.

**Transition:** at ~65% health Odran strikes a resonance plate, shedding corroded outer armor. The bell above begins moving without sound.

**Phase 2 — Tongueless:** Odran's attacks leave visible resonance fronts from floor plates. He can charge a plate; the player may use movement/guard timing to avoid or redirect pressure. Counterweights change safe geometry but do not randomly delete space.

**Signature punish:** after a failed three-hit bell charge, Odran's fused arm locks against his armor, exposing a clear stagger window.

**Death:** Odran stops fighting before the final blow long enough to intentionally point toward the hidden Ninth Descent record. His armor collapses while the suspended bell finally rings audibly.

**Rewards:** Odran's Bell Core, boss loot table, later 10028 Odran's Clapper crafting path, story memory revealing the sealing order.

## 8. Unreal integration boundary

Unreal presentation owns:

- Pawn/Character transform and movement;
- input;
- camera;
- animation/montage presentation;
- collision/hit acquisition adapter;
- actor spawning;
- world drops;
- UI;
- VFX/audio;
- level/world streaming.

Portable Lostsense owns:

- damage calculation;
- Combatant health/resource state;
- Effects;
- Ability activation/cost/cooldown;
- Skill ownership;
- Inventory/Equipment/Loot;
- deterministic RNG;
- persistence schema/validation;
- committed gameplay event stream.

The Unreal adapter must never introduce a second authoritative damage/loot/inventory implementation.

## 9. Slice acceptance gates

The slice becomes **PLAYABLE** only when all of these are proven on an actual Unreal build:

- UBT compile succeeds;
- game/editor launches;
- input changes a real player actor;
- portable runtime changes from real combat/input;
- enemy death produces real portable loot and world representation;
- inventory/equipment changes combat stats;
- skill allocation changes gameplay;
- Odran phase behavior executes in runtime;
- save/load restores portable state;
- no claim of measured FPS until profiling capture exists.
