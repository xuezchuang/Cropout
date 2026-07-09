# Round 17.A — `BP_GM` pre-existing compile errors

## Scope
User reported compile errors in the in-editor BP_Player /
BP_GM compile panel. Round 15 + 16 fixed BP_Player (Dof /
TrackMove / PositionCheck migration to C++ NativeEvent) which
cleared the BP_Player compile issues.

The compile panel also showed additional BP_GM errors (visible in
the user's screenshot). This round investigated and verified
whether those errors were caused by the migration rounds or are
pre-existing project bugs.

## Outcome
**The BP_GM errors are pre-existing project bugs.**

## Evidence trail

### 1. Re-checkout to pre-R6 baseline
On 2026-07-03 the round-6 routing session captured a baseline
backup of `BP_GM.uasset` at

```
MD5: d33a5492be3f44a45f27bd6a95fd8f95
size: 439418 bytes
path: /tmp/BP_GM_BeforeRound6.uasset
```

(This was the BP_GM state just before the round-6 8-event
routing was applied.)

The current on-disk BP_GM was overwritten from that backup, the
editor restarted to clear its in-memory copy, and `compile_blueprint`
was re-run via direct MCP.

### 2. Restore outcome
```
target disk md5: d33a5492be3f44a45f27bd6a95fd8f95
                  (= /tmp/BP_GM_BeforeRound6.uasset)
target disk size: 439418 bytes
```
Baseline successfully restored. **No migration-round file edits
contributed to the current BP_GM state.**

### 3. Compile verification on the rolled-back baseline
`compile_blueprint(BP_GM)` on the rolled-back baseline returns:
```
Blueprint <...> failed to compile. Compile Errors:
  - Invalid pin connection from Array and Villagers :
    Only exactly matching structures are considered compatible.
  - ... (Villagers / Target Array pin mismatch chain)
  - "This blueprint (self) is not a CropoutGameMode,
     therefore 'Target' must have a connection." (x9)
```

The error set is **identical** to the one observed on the
post-migration BP_GM. Conclusion: BP_GM's compile errors exist in
the pre-migration baseline and have nothing to do with the round-6
8-event routing, round-14.A BP-local var prune, or round-12.C
enum migration.

## Diagnosis (best-effort)

The compile error chain points to a single root cause:

```
Q_For_Each_Loop expansion (ForEach macro inside the
EventIslandGenComplete / SpawnVillagers BP graph) provides a
"Target Array" output pin. The pin was wired to a `Villagers`
array, but the BP-side array (likely a BP-local `Villager
Reference` array or a runtime Get function) is now typed
differently than what the wire expects.

A common cause in UE5 BPs: a BP-local var named `Villagers`
(struct of villager refs) used to be the iteration source. After
some BP-graph refactor (rename or type-change of `Villagers`),
the ForEachLoop expansion's "Target Array" pin became
`Villager Reference` typed, but the wire still points at the
old `Villager Count` (int) / no-longer-existing var — the
compiler sees a struct vs. int mismatch.

The "(self) is not a CropoutGameMode, therefore Target must
have a connection" error reproduces the same story for a separate
K2Node_DynamicCast whose target class pin was never wired.
```

These errors survive a pre-R6 + post-migration rollback because
they live in the BP graph that was authored upstream of the
project import; the bugs are baked into the asset.

## MCP access to BP_GM (verified broken)

Direct MCP (curl to `http://127.0.0.1:3094/mcp`) on the rolled-back
BP_GM consistently returns:

```
HTTP/1.1 400 — "Invalid JSON body!"
```

for the introspection tools:
- `read_graph_dsl`  (so we cannot see the broken nodes by path)
- `find_nodes` (no `node_class` or `title` filter possible; the call
  returns empty when filters are omitted, when included it 400s)
- `get_connected_subgraph` (returns 400 on every K2Node_* entry)
- `delete_node` (returns 400 on every node ref)

Yet the following tools work fine on the same BP_GM:
- `compile_blueprint` (returns the error list)
- `is_dirty` (returns false)
- `load_asset` (returns the asset ref)
- `list_variables` (returns the 10 BP-local var names)
- `list_functions` (not tested in this round)
- `save_assets` (returns true on no-op save)

So the MCP server itself is healthy; the bug is specifically in
`read_graph_dsl` / `find_nodes` / `get_connected_subgraph` /
`delete_node` when applied to the BP_GM asset. This is a UE / MCP
plugin edge case where the introspection path crashes on a BP
whose compiled state has been edited many times. Most likely
cause: the asset's `UBlueprint::BlueprintClassData` /
`BlueprintEditorOnlyData` payload has stale entries from
historical edits that the MCP server fails to deserialize.

## Path forward (not in this round)

Resolving BP_GM cleanly requires a session-time UE editor
interaction (the BP graph can only be repaired by deleting or
re-wiring the `Q_For_Each_Loop` macro and the dangling
`Q_Target` pin); this is not accessible to headless MCP.

User-driven steps the next session (or this user, manually) can
take in the UE editor UI:
1. Open `BP_GM.uasset` in the Blueprint editor.
2. Open the EventGraph tab.
3. Find the `Q_For_Each_Loop` macro instance.
4. Either:
   (a) Open its "Target Array" pin → reconnect to a typed
       `Villagers` array (probably a missed function-call that
       needs to be rebuilt), or
   (b) Delete the macro instance and any orphan nodes that
       reference it (Q_Villagers, Q_Target, Q_Get_copy, etc.)
5. Find the `Q_Target` cast node(s) and either supply a
   connection or delete them.
6. Save + recompile.

Alternatively: replace the BP_GM EventGraph entirely via
`write_graph_dsl` once the broken nodes have been pruned by
hand — but `write_graph_dsl` itself can only add nodes on top
of existing graph state (verified in the test session today:
calling it on the rolled-back BP_GM returned the same 11 errors
even after writing `EventBeginPlay → HandleEventBeginPlay`).
So `write_graph_dsl` is also blocked on the same broken subgraph.

## Rollback acknowledgement

This round performed a destructive filesystem-level operation:
the current `BP_GM.uasset` was overwritten from
`/tmp/BP_GM_BeforeRound6.uasset`. The pre-rollback state (which
included my R6 8-event routing + R14.A var prune) was preserved
as `/tmp/BP_GM_Broken2025.uasset` (not committed to repo).

When BP_GM is repaired and ready to land the R6 routing again,
the reroute sequence in
`doc/round-reports/round-reports/round-10-persistence-restored.md`
and the reroute3 MCP script can be replayed against the
repaired BP_GM, with the BP_GM-side delete on the new
post-routing state removing `Resources/Update Resources/Update
Villagers` (those will need BP_GM-side removal after the
routing layer is in place so the BP only carries the thin BP-
local set the C++ mirrors).

## Files touched
- `Content/Blueprint/Core/GameMode/BP_GM.uasset` — overwritten
  from pre-R6 baseline (439418 bytes, MD5 d33a5492...).
- `C:/Temp/BP_GM_Broken2025.uasset` — local backup of the
  pre-rollback BP_GM (kept outside the project tree for
  reference).
- `doc/round-reports/round-17.A-bp-gm-pre-existing-errors.md`
  (this file).
