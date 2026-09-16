# LOSTSENSE — Act II Ravelwood Production Plan

Status: **IMPLEMENTATION ACTIVE — ecology and acceptance pass**
Base: merged PR #10 (`ea17b03ad9ae63027fd66ce46c9a3b8841e17927`)

## Canonical correction before expansion

PR #10 correctly materialized deeper Vaur geography after Act I, because Act I opens deeper mine gates. However the canonical campaign does **not** enter Pump Cathedral / King's Bore until Act IV, after Acts II and III. PR #11 therefore treats the Pump Cathedral approach as a physically discoverable but campaign-sealed threshold until later campaign authority grants Act IV clearance. Geography may foreshadow later content; campaign progression may not skip it.

## Act II objective

Implement the next campaign order from `STORY_AUTHORITY.md`:

**Black Sap (80010) → The Lives in Bark (80011) → Fen Without Footsteps (80012) → The Thorn Choir (80013) → Borrowed Mother (80014).**

Primary production geography:

**changed Bellgrave → deeper Ravelwood → Morrowstep / infected woodland → root tunnel toward Giltfen → Thorn Choir Abbey approach.**

## Ordered checklist

- [x] Seal the PR10 Pump Cathedral threshold behind future Act IV campaign clearance.
- [x] Add deterministic Act II story/objective authority gated by completed Act I.
- [x] Extend transactional story save/load to include Act II without breaking PR9/PR10 state.
- [x] Materialize deeper Ravelwood authored geography and Morrowstep social anchor.
- [x] Add Black Sap / infected-villager interaction mechanics without forcing lethal resolution.
- [x] Add root-memory traversal and authored root-tunnel route toward Giltfen.
- [ ] Expand Act II enemy ecology: Rootbound, echo-stag pressure and thorn penitents.
- [x] Materialize Thorn Choir Abbey approach and campaign gate, not the final boss encounter prematurely.
- [x] Add portable regression and static source acceptance gates.
- [ ] Run exact-head CI, audit, repair and merge only when green.

## Current production shape

Act II now begins only after changed Bellgrave. The portable authority owns ordered quest progression and the preserve/burn witness-root decision, while the Unreal adapter includes Act II in the same transactional story payload used by existing save/load. Deeper Ravelwood extends physically from the Act I edge into a stilted Morrowstep social anchor, an explicitly nonlethal infected-villager interaction, a remembered root road toward Giltfen and a sealed Thorn Choir threshold. Mother Veyr's final encounter is intentionally not faked in this foundation pass.

## Boundaries

Existing deterministic combat, inventory, equipment, skill, event, persistence, Act I and Deep Vaur authorities remain authoritative. No procedural-room replacement for authored campaign spaces. No UE runtime claim without an authorized Unreal Engine 5.8 host.
