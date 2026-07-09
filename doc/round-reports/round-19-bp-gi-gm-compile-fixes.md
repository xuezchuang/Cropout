# Round 19 — `BP_GI` and `BP_GM` compile-error fixes (partial)

## Scope
Re-add the BP-local variables that R14.A pruned from `BP_GI` and
audit the broader compile-error cascade in `BP_GM` and `BP_GI`.
Round 19 was triggered by user-side observation that `BP_GM`
compiles with 7 fatal errors and `BP_GI` compiles with 10+
fatal errors (some auto-recovered by R14.A rollback, but others
persisted from R8 + R12.C + R13).

## What was actually broken

### `BP_GI` (CropoutGameInstance)
- R14.A removed `Has Save`, `Start Game Offset`, `Music Playing`.
  These were the BP-side counterparts of the C++ UPROPERTYs
  `HasSave`, `StartGameOffset`, `MusicPlaying` on
  `UCropoutGameInstance`. Removing them made every BP-graph
  `K2Node_VariableGet` / `K2Node_VariableSet` to those names
  orphan — a cascade of "Could not find a variable named X"
  errors.
- R8 set `BP_SaveGM`'s parent class to `UCropoutSaveGame`,
  turning the BP-side `BPSaveGM` class reference into the new
  `CropoutSaveGame` class. The BP-graph still references
  `Cast To BP Save GM`, which is now orphan (BP_SaveGM was
  reparented but the cast node's "Class" pin still points at
  the old `BPSaveGM_C`).
- R12.C changed `UCropoutSaveGame::Resources` from
  `TMap<int32, int32>` to `TMap<ECropoutResourceType, int32>`.
  The BP-graph still passes `TMap<E_ResourceType, int32>` (the
  BP-side enum), so the pin types diverge.
- R13 removed `Seed_0` from `BP_SaveGM`. The BP-graph still
  references `Seed_0`, so the compiler emits
  `Could not find a variable named "Seed_0"`.
- C++ `UCropoutGameInstance::Audio` is `TObjectPtr<USoundMix>`;
  the BP-side `Audio` reference in BP_GI is `TObjectPtr<UAudioComponent>`
  or similar. The class chain after R8 made these references
  incompatible (BP-side variable type didn't migrate).
- `ClearSave(bool ClearSeed)` — the function parameter order in
  BP-GM might have drifted from the C++ interface (which the
  BP is implementing via `BPI_GI`).

### `BP_GM` (CropoutGameMode)
- R14.A removed `Villager Count`, `UI_HUD`, `Start Time`, `SpawnRef`,
  `Town Hall`, `TownHall_Ref`, `Villager_Ref`. Same orphan-variable
  cascade as `BP_GI`.
- R12.C: `TMap<E_ResourceType, int32>` (BP) vs
  `TMap<ECropoutResourceType, int32>` (C++) pin type mismatch.
- R8 cascade: `Cast To BP_Interactable_C` orphan, `Cast To BP_Spawner`
  orphan, etc.
- Pre-existing ForEachLoop on `Villagers` struct (the round-17.A
  diagnosis covered this in more detail).

## What round 19 fixed

### `BP_GI` (3 of many errors repaired)
Used direct MCP `add_variable` (new tool discovered in round 18)
to re-add:
- `Start Game Offset` (float) — via
  `{"name":"Start Game Offset","type_name":"float","default_value":0.0,
   "is_editable":true,"category":"Cropout|GameInstance"}`.
- `Has Save` (bool) — via
  `{"name":"Has Save","type_name":"bool","default_value":false,
   "is_editable":true,"category":"Cropout|GameInstance"}`.
- `Music Playing` (bool) — same shape.

All 3 returned `returnValue: null` (success). `save_assets(BP_GI)`
returned `true`.

### `BP_GM` (no round-19 changes yet)
The Q_For_Each_Loop pre-existing subgraph + the
`Map of E_ResourceType Enums to Integers is not compatible with Map of
ECropoutResourceType Enums to Integers` cast-on-saved-node error
chains require node-level graph edits that the direct-MCP surface
cannot perform cleanly:
- `delete_node` against `BP_GM` consistently returns HTTP 400
  (the MCP server's BP_GM introspection path is broken — same
  issue as the round-17.A investigation).
- `write_graph_dsl` works on BP_GM (it returns valid AssertionError
  responses) but only appends nodes; it cannot REPLACE the
  EventGraph cleanly. So using it to write new EventEntry nodes
  does not clean out the broken ForEachLoop macros.

A manual UE-editor pass is the only way to repair BP_GM's
EventGraph; the corresponding user-side workflow is documented
in the round-17.A report.

## Disk state

| Asset | MD5 (after R19) | Note |
|---|---|---|
| `BP_GI.uasset`  | `6d4bab231cb6e30f6ebafe7510e9c00c` | + 3 BP-local vars re-added |
| `BP_GM.uasset`  | `0af87b7e42b4fce37744d0c47ae53334` | unchanged from R17.A baseline; 7 compile errors persist |

The pre-rename `BP_GM` was the round-17.A pre-R6 baseline at MD5
`d33a5492be3f44a45f27bd6a95fd8f95`; a separate session-only
intermediate state bumped it to `f33da1c4...` (post-R14.A
cumulative); a `BP_GM` compile auto-save during the cascade
broke that to `0af87b7e...`. The MD5 chain is preserved in
`C:/Temp/BP_GM_Broken2025.uasset` (intermediate) and the
pre-R6 baseline in `/tmp/BP_GM_BeforeRound6.uasset`.

## Hot points (post-R19)

- `BP_GI`: 3 BP-local vars re-added. The cascading
  `Cast To BP Save GM` orphan + `Map of E_ResourceType` pin type
  mismatch + `Seed_0` missing are still unfixed. The editor log
  shows the full cascade (see "BP_GI log entries" section of
  this round's investigation). Each of these is a *graph-node
  edit* (delete node / re-wire pin) that direct-MCP cannot do
  cleanly for `BP_GI` either.
- `BP_GM`: 7 fatal errors persist. Same fix path required
  (manual editor pass). The R12.C / R14.A / R8 cascade created
  nodes that would each need to be deleted one by one.
- `BP_Resource` / `BP_Interactable` / `BP_Villager` / `BP_BuildingBase`
  / `BP_PC` / `BP_Player` — all confirmed to compile cleanly in
  earlier rounds (R15, R14.B). The R18 work on
  `ACropoutResource` was paused at the `Interact(AActor*)` →
  `Interact()` parameter-shape fix; a future round can complete
  the `set_parent(BP_Resource, ACropoutResource)` once the MCP
  server can run the operation without crashing the editor.
- C++ DLL chain is healthy through `-0049`; all UPROPERTYs and
  UFUNCTIONs introduced in rounds 5–18 are still in place.

## Files touched
- `Content/Blueprint/Core/GameMode/BP_GI.uasset` (+ 3 BP-local
  vars; MD5 changed).
- `Content/Blueprint/Core/GameMode/BP_GM.uasset` (unchanged in
  this round).
- `C:/Temp/BP_GI_R19_BeforeFix.uasset` (pre-fix BP_GI snapshot).
- `doc/round-reports/round-19-bp-gi-gm-compile-fixes.md` (this
  file).
