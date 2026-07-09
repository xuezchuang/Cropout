# MCP Tool Usage Notes (BP_GI audit session, 2026-07-07)

Quick-reference for tools I exercised during the BP_GI fix. Save this
so the next session doesn't waste time re-discovering what works.

## BlueprintTools (`editor_toolset.toolsets.blueprint.BlueprintTools`)

### Graph-level operations

| Tool | What it does | Args | Notes |
|---|---|---|---|
| `read_graph_dsl` | Returns DSL S-expr of a graph | `{graph: {refPath: "<asset>:GraphName"}}` | graph ref path format is `<asset path>:<GraphName>`. GraphName is case-sensitive. Spaces in the editor display name are part of the GraphName. |
| `write_graph_dsl` | Writes a graph from DSL and compiles | `{graph, code}` | **Only persists if BP compiles successfully.** If BP has compile errors anywhere, the write is rolled back. Use as a quick "is the BP clean yet?" probe. |
| `find_nodes` | Lists all nodes in a graph | `{graph, title: ""}` | title is a substring filter; pass `""` to list all. |
| `get_connected_subgraph` | Subgraph reachable from a node | `{node}` | Includes the start node. |
| `create_node` | Adds a node by type_id | `{graph, type_id, pos}` | type_id format is `Category\|Title`. |
| `delete_node` | Removes a single node | `{node}` | Only deletes the node, NOT its function graph. |

### Function-graph lifecycle

| Tool | What it does | Args | Notes |
|---|---|---|---|
| `remove_function_graph` | **Removes an entire BP-defined function graph** (so the function no longer shadows the C++ parent) | `{blueprint: {refPath: ...}, graph_name: "<Editor display name>"}` | graph_name is the **editor display name**, with spaces (e.g. `"Play Music"`, not `"PlayMusic"`). C++ UFUNCTIONs that have no BP-defined override return "Graph <name> does not exist". |

This is the tool I missed for the longest time. Use it instead of
delete_node-by-node for dead BP function graphs.

### Other BP tools exercised

| Tool | Notes |
|---|---|
| `get_parent` | Returns the BP's native parent class as `/Script/...` refPath. |
| `list_event_dispatchers` | Empty for BP_GI / BP_GM / BPI_GI. The dispatcher `SetSaveData` events come from the BP-side interface implementation, not from a BP-defined dispatcher. |

## AssetTools (`editor_toolset.toolsets.asset.AssetTools`)

| Tool | Notes |
|---|---|
| `is_dirty` | `{asset_path: "/Game/..."}` — true if BP has unsaved graph changes. |
| `save_assets` | `{asset_paths: [...]}` — persists packages. Does NOT force a recompile; compile happens on next BP-touching operation. |
| `find_assets` | `{folder_path, name}` — name is substring, case-insensitive. |
| `get_referencers` | `{asset_path}` — list of asset paths that depend on this asset. |

## ObjectTools (`editor_toolset.toolsets.object.ObjectTools`)

| Tool | Notes |
|---|---|
| `list_properties` | `{instance}` — empty for BPI_GI (interfaces have no BP properties). |
| `get_properties` | `{instance, properties}` — wildcards like `ImplementedInterfaces` are rejected. |

## How to find what graphs a BP actually has

The graph name in MCP refPaths is case-sensitive and uses the **editor
display name**. Practical discovery sequence:

1. `find_nodes(graph: {refPath: "<asset>:EventGraph"}, title: "")` to
   list EventGraph nodes (those nodes' type_ids reveal bound events).
2. Try common graph names with `find_nodes` first (passes silently if
   missing) before any destructive call:
   - `<FuncName>` (no spaces, e.g. `LoadGame`)
   - `<Display Name>` (with spaces, e.g. `Load Game`)
   - `<FuncName>_<n>` if there's a name collision with C++ UFUNCTION
3. `read_graph_dsl` to confirm content; `write_graph_dsl` to verify the
   BP compiles (it will roll back and dump errors if anything is broken).

## Known graph inventory for `/Game/Blueprint/Core/GameMode/BP_GI`

Verified via `find_nodes` + `read_graph_dsl` (Round N after the
`Clear Save` body was emptied):

| Editor display name | RefPath suffix | State after audit |
|---|---|---|
| `EventGraph` | `EventGraph` | 7 event forwarders to C++ `HandleEvent*` |
| `Clear Save` | `Clear Save` | REMOVED via `remove_function_graph` |
| `Play Music` | `Play Music` | REMOVED via `remove_function_graph` |
| `TransitionIn` | `TransitionIn` | REMOVED (was empty stub) |
| `TransitionOut` | `TransitionOut` | REMOVED (was empty stub) |
| `LoadGame` | `LoadGame` | REMOVED via `remove_function_graph` |
| `Get Seed` | `Get Seed` | REMOVED via `remove_function_graph` (had `GetSeed_0` broken call) |
| `Check Save Bool` | `Check Save Bool` | REMOVED (was `return false`) |
| `Stop Music` | `Stop Music` | REMOVED (was orphan SetGlobalControlBusMixValue) |
| `ClearSave (Clear Seed)` | `ClearSave` | Stub via `K2Node_SetVariableOnPersistentFrame` + `ExecuteUbergraphBPGI(0)`. C++ `ClearSave(bool)` covers runtime; this stub is harmless but `remove_function_graph` reports "does not exist" for it. |
| `SaveGame`, `AddLoadingUI`, `OpenLevel`, `Start New Level`, `Update Save Data`, `Island Gen Complete`, `Update Save Asset`, `Clear Save Asset`, `ScaleUp` | various | C++ UFUNCTIONs that the BP never overrode — no BP graph to remove. |

## Compile-error taxonomy observed in BP_GI

Before cleanup, the recurring errors were:

- `Save Game Object Reference is not compatible with Cropout Save Game Object Reference` / `... BP Save GM Object Reference` — caused by BP-defined `Clear Save` calling BP_SaveGM setter/getter methods through a `USaveGame*` cast.
- `Map of E_ResourceType ... is not compatible with Map of ECropoutResourceType ... at Get Current Resources` — BP enum vs C++ enum mismatch on the dispatcher signature.
- `Sound Mix Object Reference is not compatible with Audio Component Object Reference at Set Audio` — `Play Music`'s `|SetAudio` node trying to push an AudioComponent into a BlueprintReadOnly USoundMix member.
- `Cannot order parameters ClearSeed in function ClearSave` and `Wood pin invalid on Map inputs` — dispatcher event defaults from a stale signature.
- `Could not find a variable named "Seed_0" in 'BP_SaveGM_C'` — BP's `GetSeed` local function called `GetSeed_0` on a `BP_SaveGM` whose actual seed member is `Seed` (C++ `UCropoutSaveGame::Seed`).

All of these resolved once the dead BP function graphs were removed and
the broken EventGraph dispatcher bindings (`SetSaveData|EventUpdateAllResources`,
`SetSaveData|EventClearSave`) were deleted.