# LOSTSENSE — Weapon and Item Authority

Status: **CANONICAL DESIGN AUTHORITY**  
Purpose: weapon-family rules, rarity semantics and the first 48 production weapon concepts. Existing test `ItemDefinition` content is non-canon fixture data.

## 1. Content model

Weapons remain data, not one C++ class per item. Production weapons map to the existing `ItemDefinition`, `ItemInstance`, `AffixDefinition`, `EffectId` and `AbilityMutationId` systems.

A weapon identity may come from:

- base family moveset / timing profile in Unreal presentation;
- authored base modifiers;
- compatible affix pools;
- stable Effect references;
- stable AbilityMutation references;
- unique or boss-specific fixed mechanics.

No weapon is considered implemented until these references exist in production data and the engine presentation can actually use the corresponding weapon family.

## 2. Rarity semantics

| Tier | Production meaning |
|---|---|
| Common | dependable family baseline, few/no affixes |
| Magic | one focused mechanical direction |
| Rare | multiple compatible affixes creating build specialization |
| Epic | high-quality rare with stronger access to advanced affix pools |
| Legendary | authored rule interaction or ability mutation, not only bigger numbers |
| Mythic | late-game authored combination with strict acquisition context |
| Ancient | Namarith-origin mechanics interacting with senseprints/Lostsense |
| Abyssal | mechanics built around anchors, absence, silence or Fracture interactions |
| Transcendent | endgame/Ascended reward tier with bounded build-defining behavior |

**Unique** is an authored identity flag, not merely a rarity color. Unique weapons may have controlled fixed rarity and authored mechanics.

## 3. Weapon families by class

### Assassin
Paired Daggers, Long Knives, Hook Blades, Needle Swords, Hand Sickles, Chain Blades, Toxin Darts, Short Sabres.

### Knight
Longswords, Greatswords, Axes, Greataxes, Hammers, Mauls, Spears/Polearms, Shields, War Picks, Flanged Maces.

### Witch
Ritual Knives, Bone Crooks, Hex Chains, Reliquaries, Blood Sickles, Spirit Lanterns, Curse Idols, Thorn Whips.

### Ranger
Longbows, Recurves, Shortbows, Heavy Crossbows, Repeating Crossbows, Hand Crossbows, Javelins, Hunting Spears, Trap Launchers, Slingbows.

### Wizard
Staves, Focus Rods, Prism Orbs, Runic Blades, Channel Gauntlets, Lens Scepters, Conductor Forks, Archive Codices.

### Arsonist
Flame Projectors, Pressure Lances, Bomb Slings, Ignition Pistols, Oil Throwers, Furnace Hammers, Canister Launchers, Spark Carbines.

Cross-class compatibility is authored at the item/class level. It is never inferred from the display name.

## 4. First production weapon catalog — 48 weapons

### Assassin — 10001–10008

| ID | Weapon | Family / tier | Build identity |
|---:|---|---|---|
| 10001 | Quiet Ledger | Paired Daggers / Rare | alternating left/right hits build a short Execution mark faster when attacking from different arcs |
| 10002 | Venn's Last Needle | Needle Sword / Legendary | precise rear thrust can convert one Poison stack into immediate stagger pressure instead of damage-over-time |
| 10003 | Widow's Hinge | Hook Blade / Rare | hook hit briefly improves side/rear movement around the struck target |
| 10004 | Miremilk Knives | Paired Daggers / Epic | Poison application is stronger on targets standing in wet/corrosive fields; no generic poison multiplier everywhere |
| 10005 | Charter Seal-Cutter | Long Knife / Rare | bonus Armor Break against guarded/human enemies after a successful dodge-through |
| 10006 | False Shadow Chain | Chain Blade / Legendary | authored mutation creates a delayed senseprint echo of one chain finisher at the marked target's prior position |
| 10007 | Null-Silk Sickle | Hand Sickle / Abyssal | successful execution grants a short anchor state resisting displacement, but Tempo gain pauses during it |
| 10008 | Knife of the Uncounted | Short Sabre / Unique | boss/secret weapon: first clean hit after leaving enemy sight records a strike; the next Execution can replay a bounded fraction as physical damage |

### Knight — 10021–10028

| ID | Weapon | Family / tier | Build identity |
|---:|---|---|---|
| 10021 | Gravesong Longsword | Longsword / Rare | balanced baseline; counter hits improve Guard Integrity recovery briefly |
| 10022 | Bellguard Sabre | Longsword / Epic | perfect guard primes the next sweeping attack with resonance stagger |
| 10023 | Charter Split-Axe | Axe / Rare | heavy hit against armor exposes a short Armor Break window rather than raw permanent penetration |
| 10024 | Two-Oath Greatsword | Greatsword / Legendary | stance-changing heavy attack alternates Bellward defensive follow-up and Hollow high-risk follow-up |
| 10025 | Rookbreaker Maul | Maul / Epic | slow committed attacks create exceptional stagger against elites but poor recovery on miss |
| 10026 | March Spear | Spear / Rare | maintained spacing increases Resolve gain from precise thrusts; loses bonus at point-blank range |
| 10027 | Tongueless Shield | Shield / Legendary | perfect guard emits a small resonance ring but reduces passive block efficiency, rewarding timing |
| 10028 | Odran's Clapper | Bell-Maul / Boss Unique | defeating Odran unlocks a counter shock AbilityMutation; charged impact can leave a resonant punish zone |

### Witch — 10041–10048

| ID | Weapon | Family / tier | Build identity |
|---:|---|---|---|
| 10041 | Patient's Thorn | Ritual Knife / Rare | paying Blood Debt with a curse resolution briefly empowers spirit damage |
| 10042 | Crook of Three Names | Bone Crook / Epic | one active spirit can inherit a defeated cursed enemy's utility tag for a bounded duration |
| 10043 | Red Tithe Chain | Hex Chain / Legendary | linking two cursed enemies shares part of curse resolution but increases Blood Debt generated |
| 10044 | Quiet Reliquary | Reliquary / Rare | ward strength rises when no summon is active, enabling non-summon Witch builds |
| 10045 | Widowroot Sickle | Blood Sickle / Epic | close attacks can harvest curse duration into immediate healing at efficiency loss |
| 10046 | Lantern of Borrowed Breath | Spirit Lantern / Legendary | destroyed summon leaves a stationary spirit ward instead of vanishing outright |
| 10047 | Assembly Idol | Curse Idol / Abyssal | curses persist more reliably across displacement/anchor effects but direct damage is reduced |
| 10048 | Veyr's Living Rosary | Thorn Whip / Boss Unique | every third curse propagation can root both caster and target briefly, creating deliberate high-risk setup |

### Ranger — 10061–10068

| ID | Weapon | Family / tier | Build identity |
|---:|---|---|---|
| 10061 | Marchline Recurve | Recurve Bow / Rare | moving perpendicular to target line builds Focus faster on precise hits |
| 10062 | Saltglass Longbow | Longbow / Epic | charged shots reveal/extend authored weak-point marks through thin destructible cover where supported |
| 10063 | Charter Windlass | Heavy Crossbow / Rare | reload is slow; first shot after a full reload has high stagger and pin potential |
| 10064 | Nine-Tooth Repeater | Repeating Crossbow / Legendary | controlled burst consumes Focus to place sequential marks instead of simply increasing fire rate |
| 10065 | Hush Quay Handbow | Hand Crossbow / Rare | trap deployment does not break short movement strings; lower raw damage |
| 10066 | Quill's Survey Javelin | Javelin / Epic | embedded javelin functions as a temporary weak-point beacon for follow-up ranged attacks |
| 10067 | No-Horizon Spear | Hunting Spear / Abyssal | attacks from elevated/falling traversal can create an anchor pin, rewarding vertical play |
| 10068 | Kharos' Blindline | Longbow / Boss Unique | hitting a marked weak point can briefly reveal a second 'absence line' that pierces only the marked target, not all enemies |

### Wizard — 10081–10088

| ID | Weapon | Family / tier | Build identity |
|---:|---|---|---|
| 10081 | Three-Phase Staff | Staff / Rare | modest bonuses rotate with the last elemental Resonance band used |
| 10082 | Frostglass Rod | Focus Rod / Epic | Frost field edges increase Lightning reaction reliability rather than flat spell power |
| 10083 | Cinder Prism | Prism Orb / Legendary | Fire→Frost reaction creates a controllable steam pocket that can later conduct Lightning |
| 10084 | Meridian Runeblade | Runic Blade / Rare | melee spell hits reposition small elemental fields toward the target |
| 10085 | Coil Gauntlets | Channel Gauntlets / Epic | channel duration raises Lightning spatial control but increases interruption risk |
| 10086 | Lens of Empty Noon | Lens Scepter / Abyssal | one spatial spell can anchor to empty terrain and persist through local geometry shift events |
| 10087 | Fork of the Third Storm | Conductor Fork / Legendary | Lightning following two different elements forks into distinct reaction effects, encouraging sequence variety |
| 10088 | Ysil's Civic Equation | Archive Codex / Boss Unique | Triad Weave can store one completed element sequence and replay its field shape once at reduced potency |

### Arsonist — 10101–10108

| ID | Weapon | Family / tier | Build identity |
|---:|---|---|---|
| 10101 | Bellgrave Torchworks | Flame Projector / Rare | reliable short cone; vents Pressure faster while not firing |
| 10102 | Charter Pressure Pike | Pressure Lance / Epic | thrust can atomize Oiled targets into Vaporized state for delayed ignition setups |
| 10103 | Cinder-Sling No. 4 | Bomb Sling / Rare | arcing bombs spread small authored fuel patches rather than pure explosion spam |
| 10104 | Spark Clerk | Ignition Pistol / Legendary | low base damage; precisely ignites the oldest prepared fuel state first to control chain order |
| 10105 | Hush Quay Flooder | Oil Thrower / Epic | broad oil control with reduced immediate ignition; strong setup tool |
| 10106 | Furnace Foreman's Hammer | Furnace Hammer / Rare | melee vent strike dumps Pressure into knockback/stagger instead of flame output |
| 10107 | Vask Six-Chamber | Canister Launcher / Legendary | cycles authored canister states; Overpressure changes the next canister behavior instead of random malfunction |
| 10108 | Vael's Ash Processional | Flame Projector / Boss Unique | ignition can consume a removable harmful Effect on the player to alter the next ash zone, tying Synod purge tech into risk/reward |

## 5. Boss weapon rule

Boss weapons must reproduce **part of a boss's combat idea**, not merely its visual model. Prefer existing portable hooks:

- `EffectId` for status behavior;
- `AbilityMutationId` for ability transformation;
- authored Item/Affix modifiers for stats;
- Unreal weapon family presentation for animation/timing.

Special-case gameplay code is acceptable only when a mechanic cannot be represented through existing authorities and the new behavior is reusable across multiple authored items.

## 6. Launch-scale planning

The long-term target of roughly 300 meaningful weapon/content variants per class is a **content lifecycle target**, not a PR count. Mature catalogs expand through family variants, faction gear, boss weapons, Abyss/Hell equipment and endgame transformations. Recolored numerical clones do not count as meaningful variants.
