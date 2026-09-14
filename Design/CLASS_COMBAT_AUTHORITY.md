# LOSTSENSE — Class and Combat Authority

Status: **CANONICAL DESIGN AUTHORITY**  
Runtime note: this file defines content semantics for the existing portable combat/ability/skill authorities. It does not replace them.

## 1. Shared action language

All classes use the same high-level action budget so controls remain legible:

- Primary attack
- Secondary / heavy / contextual attack
- Dodge
- Class mechanic
- Active 1–4
- Ultimate

The portable `AbilityLoadout` remains the source for equipped ability ownership. Class identity comes from resources, mechanics, weapon families, effects and skill topology rather than bespoke duplicate combat frameworks.

## 2. Assassin — Class 101

**Fantasy:** a close-range execution specialist who creates brief positional advantages and converts them into lethal windows.

- **Resource:** **Tempo** — gained by clean movement, back/side hits, dodging through danger and executing marked targets; decays when combat stalls.
- **Class mechanic:** **Veil Step** — spend Tempo to cross a short distance through an enemy's threat line; does not grant unlimited immunity and must respect authored dodge/iframes.
- **Primary families:** paired daggers, long knives, hook blades, needle swords, hand sickles, garrotes/chain blades, toxin darts, short sabres.
- **Signature mechanics:** Rear Arc, Mark, Poison stacks, Bleed windows, Execution thresholds, decoy senseprints.
- **Ability categories:** mobility, mark, toxin, bleed, shadow decoy, finisher.
- **Ultimate identity:** **Black Interval** — creates a short execution state where marked enemies expose chained positional openings; not a global time stop.
- **Defense:** avoid, misdirect, counter and exit; weakest sustained face-tanking.

## 3. Knight — Class 102

**Fantasy:** deliberate weapon mastery, guard control and heavy physical authority.

- **Resource:** **Resolve** — built by blocking appropriate damage, parrying, landing heavy hits and maintaining pressure; spent on guard arts and empowered counters.
- **Class mechanic:** **Guard Stance** — directional/angle-aware guard with perfect-guard timing window. Shield builds maximize safety/control; two-hand builds convert Resolve into poise and impact.
- **Primary families:** longswords, greatswords, axes, greataxes, hammers, mauls, spears/polearms, shields, war picks, flanged maces.
- **Signature mechanics:** Guard Integrity, Parry, Stagger, Armor Break, Counter, weapon stance routes.
- **Ability categories:** guard art, gap closer, sweep/control, single-target breaker, rally, weapon technique.
- **Ultimate identity:** **Unbroken Measure** — enter a short mastery state that makes successful guards/counters accelerate a controlled offensive sequence.
- **Defense:** strongest direct mitigation and poise, but positioning and stamina/Resolve management prevent passive turtling.
- **Lore branches:** **Bellward Oath** (protective resonance) and **Hollow Oath** (turn controlled Lostsense exposure into power). Neither is generic holy/evil alignment.

## 4. Witch — Class 103

**Fantasy:** curses, blood prices, spirits and controlled corruption; wins by turning cost into leverage.

- **Resource:** **Blood Debt** — abilities may reserve health or accumulate Debt; resolving curses, sacrificing summons or consuming marked enemies pays it down. Excess Debt creates risk, not instant random death.
- **Class mechanic:** **Pact Slots** — bind a limited set of spirit/pact rules that alter curse behavior.
- **Primary families:** ritual knives, bone crooks, hex chains, reliquaries, blood sickles, spirit lanterns, curse idols, thorn whips.
- **Signature mechanics:** Curse links, spirit summons, delayed payment, self-cost, corruption conversion, enemy debuff propagation.
- **Ability categories:** curse, spirit, blood rite, ward, summon, sacrifice.
- **Ultimate identity:** **Many-Mouthed Covenant** — manifests active pacts simultaneously for a short period, then resolves accumulated Blood Debt.
- **Defense:** wards, life conversion, summons and enemy weakening rather than elemental barriers.

## 5. Ranger — Class 104

**Fantasy:** mobile precision hunter who controls approach paths and converts observation into weak-point damage.

- **Resource:** **Focus** — built by maintaining target sight, precision hits and movement without taking direct hits; spent on high-precision shots and trap chaining.
- **Class mechanic:** **Mark Field** — observation reveals authored weak points or creates temporary marks on standard enemies.
- **Primary families:** longbows, recurves, shortbows, heavy crossbows, repeating crossbows, hand crossbows, javelins, hunting spears, trap launchers, slingbows.
- **Signature mechanics:** weak points, pin, mark, trap link, reposition, projectile timing.
- **Ability categories:** precision shot, mobility shot, trap, mark, volley, companion utility only where authored.
- **Ultimate identity:** **Line of Truth** — temporarily exposes a sequence of high-value weak-point lines through enemies/terrain; still requires aim and timing.
- **Defense:** distance control, evasive movement, snares and line-of-sight manipulation.

## 6. Wizard — Class 105

**Fantasy:** an elemental spatial controller who manipulates relationships among Fire, Frost and Lightning.

- **Resource:** **Resonance** — three elemental resonance bands accumulate through casting; specific combinations unlock reactions rather than only larger damage.
- **Class mechanic:** **Triad Weave** — previous two elements influence the next spell: e.g. Frost→Lightning creates conductive brittle zones; Fire→Frost creates steam/visibility/control; Lightning→Fire creates plasma arcs. Exact balance remains data-driven.
- **Primary families:** staves, focus rods, prism orbs, runic blades, channel gauntlets, lens scepters, conductor forks, archive codices.
- **Signature mechanics:** elemental fields, combos, spatial denial, teleport anchors, reaction setup.
- **Ability categories:** bolt, field, wall, displacement, reaction catalyst, elemental defense.
- **Ultimate identity:** **Threefold Equation** — stabilizes all three resonance bands and lets sequence order deliberately reshape a battlefield.
- **Defense:** barriers, space control, short teleports; weaker when cornered without setup.

## 7. Arsonist — Class 106

**Fantasy:** industrial incendiary combat—oil, pressure, ignition and chain reaction engineering rather than conventional fire magic.

- **Resource:** **Pressure** — devices build line pressure/heat; high Pressure improves output but risks **Overpressure**, forcing vent decisions.
- **Class mechanic:** **Fuel State** — terrain/enemies can be Dry, Oiled, Vaporized or Ignited. The class creates and detonates state chains.
- **Primary families:** flame projectors, pressure lances, bomb slings, ignition pistols, oil throwers, furnace hammers, canister launchers, spark carbines.
- **Signature mechanics:** oil spread, ignition, chained explosions, persistent hazard zones, pressure venting, environmental denial.
- **Ability categories:** fuel delivery, ignition, pressure burst, mine/bomb, hazard field, emergency vent.
- **Ultimate identity:** **Redline Plant** — deploy a temporary pressure manifold that links nearby fuel zones for controlled chain reactions; it can overheat if mismanaged.
- **Defense:** area denial, knockback, smoke/steam cover and proactive zoning; not Wizard-style magical shielding.

## 8. Skill tree — The Scar Atlas

The canonical player-facing skill structure is **the Scar Atlas**, representing reconstructed pathways of the Returned's fractured senseprint.

Long-term topology target: **1,000+ meaningful nodes**, not filler.

### Regions of the Atlas

- Six large class regions, roughly 120–140 nodes each at mature production scale.
- Shared central **Witness / Survival** routes.
- Weapon-specialization corridors that cross class-region boundaries where compatible.
- Status/element routes connecting poison, bleed, resonance, guard, curse, mark and ignition systems.
- **Vows** = Keystone nodes that impose meaningful rule changes/tradeoffs.
- **Fractures** = Transformation nodes using existing `AbilityMutationId` hooks.
- Hybrid routes for deliberate cross-mechanic builds, not unrestricted class homogenization.

### First Knight production subset

Vertical slice target is 36 authored Knight nodes:

- 10 Guard/Resolve nodes
- 8 one-hand/shield nodes
- 7 two-hand/impact nodes
- 5 Bellward Oath nodes
- 4 Hollow Oath nodes
- 2 Vows/Fractures

Representative Vow: **Stand Where It Falls** — perfect guards generate additional Resolve and stagger protection, but dodge distance is reduced while Resolve is above a threshold.

Representative Fracture: **Clapper's Return** — transforms an authored shield-counter ability so the return shock originates from the guarded enemy rather than the player, trading radius for single-target stagger.

These examples require real runtime content integration before being called implemented.
