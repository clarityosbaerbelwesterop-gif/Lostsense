# LOSTSENSE — Act II Ravelwood Production Plan

Status: **IMPLEMENTATION ACTIVE — canonical gate first**
Base: merged PR #10 (`ea17b03ad9ae63027fd66ce46c9a3b8841e17927`)

## Canonical correction before expansion

PR #10 correctly materialized deeper Vaur geography after Act I, because Act I opens deeper mine gates. However the canonical campaign does **not** enter Pump Cathedral / King's Bore until Act IV, after Acts II and III. PR #11 therefore treats the Pump Cathedral approach as a physically discoverable but campaign-sealed threshold until later campaign authority grants Act IV clearance. Geography may foreshadow later content; campaign progression may not skip it.

## Act II objective

Implement the next campaign order from `STORY_AUTHORITY.md`:

**Black Sap (80010) → The Lives in Bark (80011) → Fen Without Footsteps (80012) → The Thorn Choir (80013) → Borrowed Mother (80014).**

Primary production geography:

**changed Bellgrave → deeper Ravelwood → Morrowstep / infected woodland → root tunnel toward Giltfen → Thorn Choir Abbey approach.**

## Ordered checklist

- [ ] Seal the PR10 Pump Cathedral threshold behind future Act IV campaign clearance.
- [ ] Add deterministic Act II story/objective authority gated by completed Act I.
- [ ] Extend transactional story save/load to include Act II without breaking PR9/PR10 state.
- [ ] Materialize deeper Ravelwood authored geography and Morrowstep social anchor.
- [ ] Add Black Sap / infected-villager interaction mechanics without forcing lethal resolution.
- [ ] Add root-memory traversal and authored root-tunnel route toward Giltfen.
- [ ] Expand Act II enemy ecology: Rootbound, echo-stag pressure and thorn penitents.
- [ ] Materialize Thorn Choir Abbey approach and campaign gate, not the final boss encounter prematurely.
- [ ] Add portable regression and static source acceptance gates.
- [ ] Run exact-head CI, audit, repair and merge only when green.

## Boundaries

Existing deterministic combat, inventory, equipment, skill, event, persistence, Act I and Deep Vaur authorities remain authoritative. No procedural-room replacement for authored campaign spaces. No UE runtime claim without an authorized Unreal Engine 5.8 host.
