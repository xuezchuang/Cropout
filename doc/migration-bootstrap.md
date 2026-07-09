# Migration Bootstrap — deterministic MCP replay sequence

## Why this file exists

The R6 (BP_GM 8-event routing) and R7 (BP_GI 9-event routing) graph
edits exist **only in the in-memory UPackage held by the running
editor (PID 56328 as of 2026-07-03 11:06)**. The disk file
timestamps show:

```
BP_GM.uasset: Modify 2026-07-03 12:02:24 (auto-backup before R6 routing)
BP_GI.uasset: Modify 2026-07-03 11:06:15 (pre-R7)
```

The MCP harness that was performing the routing started returning
`Function "<name>", input params are required by the function input
schema Json, but incoming function input params Json is empty` for
**every** `call_tool` invocation (verified across `save_assets`,
`is_dirty`, `find_nodes`, `read_graph_dsl`, `connect_pins`).
`UnrealEditor-Cmd -ExecutePythonScript` cannot reach the running
editor's in-memory packages from a separate process — it would
launch its own editor, load from disk (Auto1 pre-routing), and
**overwrite the in-memory R6/R7 routing with a stale save**, so that
path is destructive and is not used.

This file captures the exact `mcp__ue-mcp__call_tool` sequence that
was issued during R6+R7 so the work can be deterministically
re-driven when the harness recovers (or re-issued under a different
ZCode session with a working MCP backplane).

## Pre-conditions

1. MCP `save_assets` and `connect_pins` working again
   (currently failing with `params required... empty`).
2. BP_GM and BP_GI loaded into the running editor
   (default — they're the GlobalDefault GameMode / GameInstance).
3. BP_GM and BP_GI on disk are back at the pre-R6 state
   (auto-backup at `/tmp/BP_GM_BeforeRound6.uasset`, MD5
   `d33a5492be3f44a45f27bd6a95fd8f95`). If the in-memory state is
   lost but the disk state was already advanced, replace with
   `/tmp/BP_GM_BeforeRound6.uasset` first.
4. `UCropoutGameInstance.h/.cpp`, `UCropoutSaveGame.h/.cpp` exist
   on disk (already done, DLL -0034 compiled).
5. UEditor running, PID-sticky across session.

## R6 — BP_GM 8-event routing (re-drive)

For each of the 8 events in `BP_GM.EventGraph`, run:

```
mcp__ue-mcp__call_tool editor_toolset.toolsets.blueprint.BlueprintTools.create_node
  graph: { refPath: "/Game/Blueprint/Core/GameMode/BP_GM.BP_GM:EventGraph" }
  node_class: { refPath: "/Script/BlueprintGraph.K2Node_CallFunction" }   # parent function
  node_params: {
    function_reference: { class: "UCropoutGameMode", member: "<HandlerName>" },
    ...
  }
```

Mapping (8 entries):

| BP event | Handler UFUNCTION on `UCropoutGameMode` |
|---|---|
| `EventBeginPlay` | `HandleEventBeginPlay` |
| `EventInit` (GameMode) | `HandleEventInit` |
| `EventAddResource` | `HandleEventAddResource` |
| `EventConsumeResource` | `HandleEventConsumeResource` |
| `EventUpdateSave` | `HandleEventUpdateSave` |
| `EventSaveGame` | `HandleEventSaveGame` |
| `EventIslandGenComplete` | `HandleEventIslandGenComplete` |
| `EndGame` | `HandleEndGame` |

The create_node output returns the new node id (e.g. `K2Node_CallFunction_N`).

After creating all 8 `K2Node_CallFunction_N` nodes, for each:
```
mcp__ue-mcp__call_tool editor_toolset.toolsets.blueprint.BlueprintTools.connect_pins
  output_pin: { direction: "EGPD_Output", index_id: 1, node: { refPath: "<EventX node path>" } }
  input_pin:  { direction: "EGPD_Input",  index_id: 0, node: { refPath: "K2Node_CallFunction_N path" } }
```

`connect_pins` returns `null` on success (verified behaviour in
round 6) and automatically cleans up orphan body nodes downstream of
the reconnected entry point.

Persist:
```
mcp__ue-mcp__call_tool editor_toolset.toolsets.asset.AssetTools.save_assets
  asset_paths: [ "/Game/Blueprint/Core/GameMode/BP_GM" ]
```

If the harness bug recurs, retry with an explicit Python equivalent
via the editor's `Editor Asset Subsystem`:

```bash
UnrealEditor-Cmd.exe D:\ue\CropoutSampleProject\CropoutSampleProject.uproject \
  -ExecutePythonScript="import unreal; unreal.EditorAssetLibrary.save_asset('/Game/Blueprint/Core/GameMode/BP_GM')"
```

This will only work if a second editor is allowed to grab the package
while PID 56328 is still up — usually not the case in practice, but
documented for completeness.

## R7 — BP_GI 9-event routing (re-drive)

Same pattern as R6 but targeting `BP_GI.uasset` and using
`K2Node_CustomEvent_*` for `AddLoadingUI`/`OpenLevel` (the BP-side
custom events, not `K2Node_Event_*`).

```
graph: { refPath: "/Game/Blueprint/Core/GameMode/BP_GI.BP_GI:EventGraph" }
```

Mapping (9 entries):

| BP entry point | Handler UFUNCTION on `UCropoutGameInstance` | Connector node path prefix |
|---|---|---|
| `K2Node_Event_0` (EventInit) | `HandleEventInit` | event prefix |
| `K2Node_Event_2` (EventSaveGame) | `HandleEventSaveGame` | event prefix |
| `K2Node_Event_3` (EventUpdateAllInteractables) | `HandleEventUpdateAllInteractables` | event prefix |
| `K2Node_Event_4` (EventLoadLevel) | `HandleEventLoadLevel` | event prefix |
| `K2Node_Event_5` (EventUpdateAllResources) | `HandleEventUpdateAllResources` | event prefix |
| `K2Node_Event_6` (EventUpdateAllVillagers) | `HandleEventUpdateAllVillagers` | event prefix |
| `K2Node_Event_7` (EventClearSave) | `HandleEventClearSave` | event prefix |
| `K2Node_CustomEvent_0` (AddLoadingUI) | `HandleAddLoadingUI` | custom event prefix |
| `K2Node_CustomEvent_1` (OpenLevel) | `HandleOpenLevel` | custom event prefix |

The 9 `create_node` calls will return ids
`K2Node_CallFunction_46..54` (this is **editor-session dependent**;
new sessions may produce different ids — always read back via
`find_nodes` before issuing `connect_pins`).

Persist:
```
mcp__ue-mcp__call_tool editor_toolset.toolsets.asset.AssetTools.save_assets
  asset_paths: [ "/Game/Blueprint/Core/GameMode/BP_GI" ]
```

## R8 — `set_parent(BP_SaveGM, UCropoutSaveGame)`

```
mcp__ue-mcp__call_tool editor_toolset.toolsets.blueprint.BlueprintTools.set_parent
  asset_path:        "/Game/Blueprint/Core/Save/BP_SaveGM"
  new_parent_class:  { refPath: "/Script/CropoutSampleProject.CropoutSaveGame" }
```

Verify with:
```
read_graph_dsl(BP_SaveGM)  -> expect "GeneratedClass CropoutSaveGame" header
```

Persist (combined):
```
mcp__ue-mcp__call_tool editor_toolset.toolsets.asset.AssetTools.save_assets
  asset_paths: [ "/Game/Blueprint/Core/Save/BP_SaveGM" ]
```

## Verification gate

After all three `save_assets` calls succeed:

```
mcp__ue-mcp__call_tool editor_toolset.toolsets.asset.AssetTools.is_dirty
  asset_path: "/Game/Blueprint/Core/GameMode/BP_GM"
```

Should return `false` for all three. Then UBT:
```
cmd /C Build.bat CropoutSampleProjectEditor Win64 Development \
  -Project="D:\ue\CropoutSampleProject\CropoutSampleProject.uproject" \
  -WaitMutex -FromMsBuild
```
should succeed with the bootstrap-final DLL tag.

## Hardware fingerprint at write time

- Editor process: `UnrealEditor.exe` PID 56328
- Engine: UE 5.8 (build commit not recorded in this doc)
- Project commit: not under git, no sha
- MCP harness state: failing on all array + object params
- Save_assets outcome last 9 turns: rejected before reaching server
- Net state: R6+R7 live only in PID 56328's memory; R8/R8.5/R9 are on
  disk in C++; /tmp/BP_GM_BeforeRound6.uasset has the Auto1 backup

## Acceptance criteria (next-session run)

- [ ] R6 disk MD5 != `/tmp/BP_GM_BeforeRound6.uasset` MD5 (= modified)
- [ ] R7 disk MD5 != previous R7 disk MD5 (= modified)
- [ ] R8 BP_SaveGM `GeneratedClass` parent matches `CropoutSaveGame`
- [ ] `is_dirty(BP_GM) == false`, `is_dirty(BP_GI) == false`,
      `is_dirty(BP_SaveGM) == false`
- [ ] UBT compile of `CropoutSampleProjectEditor` succeeds with the
      same in-memory routing the running editor used

Until all five are green, the R6+R7+R8 disk state is not considered
persisted and the in-memory state remains authoritative (volatile).
