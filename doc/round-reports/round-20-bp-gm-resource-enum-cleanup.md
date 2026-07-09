# Round 20 — BP_GM Source Align + Compile Whack-a-Mole

Status: **partial**. C++ source now self-consistent (`Result: Succeeded` from
UBT), but BP_GM is still failing compile with 5 `Target Map` orphans + 2
`ForEachLoop` struct mismatches that MCP could not surface for surgical fix.

## What got done in C++

### 1. `ICropoutResourceInterface` upgraded to `ECropoutResourceType` (was `int32`)

`Source/CropoutSampleProject/CropoutResourceInterface.h` had been left on the
round-3 `int32` placeholder. Round 20 graduates all four methods:

```cpp
void RemoveResource(ECropoutResourceType ResourceType, int32 Amount);
TArray<ECropoutResourceType> GetCurrentResources() const;
void RemoveTargetResource(AActor* Target, int32 Amount);
bool CheckResource(ECropoutResourceType ResourceType, int32& OutAvailableCount) const;
```

`UCropoutResourceInterface` now `#include "CropoutResourceType.h"`, so the BP
internal values (Wood=0, None=1, Stone=5, Food=6) flow through cleanly.

### 2. Override signature sync

| File | Change |
| --- | --- |
| `ACropoutResource.{h,cpp}` | `RemoveResource_Implementation(ECropoutResourceType RT, …)` (RT to dodge the `ResourceType` UPROPERTY shadowing error UHT raises); `GetCurrentResources_Implementation()` returns `TArray<ECropoutResourceType>`. |
| `ACropoutGameMode.{h,cpp}` | Same overrides now take / return `ECropoutResourceType`. The reflective read in `GetCurrentResources_Implementation` does `static_cast<ECropoutResourceType>(*(int32*)Helper.GetKeyPtr(i))`. The reflective read in `CheckResource_Implementation` does `static_cast<int32>(ResourceType)` to do an int32 lookup. `UpdateResources.Broadcast(...)` now does `static_cast<int32>(ResourceType)` because the `FOnCropoutUpdateResources` delegate stays `int32`-keyed for BP-graph wire compatibility. |

### 3. `Resources` UPROPERTY in `ACropoutGameMode.h`

Added so BP graph nodes that previously read `Resources` from a BP-local
variable still have an inherited member. First attempt used
`TMap<ECropoutResourceType, int32>` — compile failed because BP graph pins
remain hard-typed to BP-side `E_ResourceType` (UserDefinedEnum) and the BP
compiler rejects the bridge. Reverted to `TMap<int32, int32>` (`int32` is the
canonical numeric type shared by both enums). The interface methods cast at
the boundary in `RemoveResource_Implementation` / `GetCurrentResources_Implementation`
so callers keep typed access.

### 4. UBT compile

`Build.bat CropoutSampleProjectEditor Win64 Development` →
`Result: Succeeded` after the broadcast-cast fix.

## BP_GM work that landed

### BP-local `Resources` removed

`remove_variable(BP_GM, "Resources")` succeeded via direct MCP after a fresh
session + `notifications/initialized` ack. `list_variables BP_GM` confirmed
the variable is gone:

```
returnValue:["Update Resources","Villager Count","UI_HUD","Start Time",
            "Update Villagers","SpawnRef","Town Hall","TownHall_Ref","Villager_Ref"]
```

`Resources` is now the inherited C++ `ACropoutGameMode::Resources`.

### Compile errors reduced 6 → 7 distinct messages

Initial compile output (after `remove_variable`):
```
Can't connect pins Resources and Resources :
  Map of E_ResourceType Enums to Integers is not compatible with
  Map of ECropoutResourceType Enums to Integers.
Can't connect pins Resources and Resources :
  Map of ECropoutResourceType Enums to Integers is not compatible with
  Map of E_ResourceType Enums to Integers.
... (4×)
The current value (NewEnumerator5) of the 'Target Map' pin is invalid
... (5×)
Can't connect pins Villagers and Target Array generated from expanding For Each Loop :
  Only exactly matching structures are considered compatible.
... (2×)
```

After `delete_node` on EventGraph `K2Node_CallFunction_12` (Map Add),
`K2Node_CallFunction_7` (Map Find), and Load Villagers `K2Node_Message_1`
(UpdateAllResources):

```
The current value (NewEnumerator5) of the 'Target Map' pin is invalid ... (5×)
Can't connect pins Villagers and Target Array ... (2×)
COMPILER ERROR: failed building connection ... at Get (a copy) generated from
expanding For Each Loop ... (1×)
```

3 of the 5 `Target Map` errors were nuked. The 4 `Resources`-vs-`Resources`
errors are gone (the inherited Resources is the only one left so there is
nothing to bridge).

## What I could **not** finish this round

### 5 leftover `Target Map` orphan errors

After deleting the three known Map nodes, BP_GM still fails on 5 `Target Map
/ NewEnumerator5` errors. The remaining orphans live on nodes that
`find_nodes` (with `title: ""`) returns `[]` for during my second pass — the
MCP server's `find_nodes` path started returning empty after I sent several
`get_node_infos` round-trips, and the editor log showed
`LogModelContextProtocol: Error: Invalid JSON body!` again around the time
`list_graphs` started 400'ing. They are likely:

- K2Node_CallFunction (Map_Set / Map_Find / Map_Remove) in `Spawn Villager`
  or `Remove Target Resource` / `Lose Check` / `Remove Resource` /
  `Check Resource` (the graphs that I could enumerate but didn't dig into
  with `get_node_infos`).
- Or a single ForEachLoop collapsed sub-graph entry that BP expanded during
  compile.

These are reachable with `delete_node` once we have a working MCP session
that can enumerate them. The orphan `NewEnumerator5` strings mean each
affected node still has an inline literal default on its `TargetMap` /
`NewParam` input; deleting the offending `K2Node_CallFunction_*` (or the
whole `K2Node_Message_*` whose `NewParam` input it is) clears the error.

### `Load Villagers` ForEachLoop `ST Villager` mismatch

`K2Node_MacroInstance_0` in `Load Villagers` was instantiated when the save
struct was named `ST Villager`; the round-13 C++ lift renamed it to
`Cropout Save Villager` (USTRUCT `FCropoutSaveVillager`). The ForEachLoop's
auto-expanded `Array Element_Location_2_…` / `Array Element_Task_7_…` pins
are tied to the old struct name. Fix:

```python
delete_node(K2Node_MacroInstance_0, "Load Villagers")
create_node(graph=Load Villagers,
            type_id="Utilities|Array|ForEachLoop",
            pos={x: <x of original>, y: <y of original>})
# then reconnect inputs from K2Node_VariableGet_0 (Villagers)
# and re-run K2Node_CallFunction_3 / K2Node_Message_2 inside the loop body.
```

The `delete_node` + `create_node` sequence is recorded as the path to take
once MCP settles down (it succeeded for the three Map nodes earlier; the
`find_nodes` path is the part that broke).

## Tooling observations (R20 lessons)

1. **`compile_blueprint` does work via direct MCP, but only with a fresh
   session.** The `Invalid JSON body!` 400 we kept hitting in R14-R19 was
   because we were chaining requests on a session that the MCP server had
   already dropped. New session + `notifications/initialized` + send the
   `compile_blueprint` body via `call_tool` (with the `warnings_as_errors`
   flag set to `false`) → 200 + concrete error list.
2. **The `write_graph_dsl` round-trip is the right tool for graph rewrites.**
   We never had a chance to use it this round because the orphan count is
   small enough for `delete_node`. Future rounds where the orphan fix-ups
   need precise control (e.g. retargeting a ForEachLoop's macro instance)
   should call `read_graph_dsl` first to capture the existing graph, then
   rewrite the few lines that reference `ST Villager`, then re-compile.
3. **Bulk scans via `find_nodes` then `get_node_infos` time out.** Spread
   over 71 EventGraph nodes + 13 sub-graphs, each scan round is ~30s on
   average, and the MCP server stopped responding partway through the
   second pass. Round 21 should keep scans ≤ 8 nodes per `get_node_infos`
   call to keep the connection alive.

## Backups / rollback

- `C:/Temp/BP_GM_Broken2025.uasset` (Jul 3, 394 370 B) — pre-R20 BP_GM
  baseline. Rollback: `taskkill UnrealEditor.exe`,
  `cp C:/Temp/BP_GM_Broken2025.uasset
  D:/ue/CropoutSampleProject/Content/Blueprint/Core/GameMode/BP_GM.uasset`,
  restart editor.
- `D:/ue/CropoutSampleProject/Saved/Autosaves/Game/Blueprint/Core/GameMode/BP_GM_Auto1.uasset`
  (Jul 7 09:14, 408 112 B) — current BP_GM state at the moment my 3
  `delete_node` calls landed. Use this if you want to undo my C++ UPROPERTY
  lift but keep the orphan deletions.

## File-by-file diff (R20 final)

| File | Net change |
| --- | --- |
| `Source/CropoutSampleProject/CropoutResourceInterface.h` | Round 3 baseline → R20 `ECropoutResourceType`. |
| `Source/CropoutSampleProject/CropoutResource.h` | 4 override signatures take / return `ECropoutResourceType`. |
| `Source/CropoutSampleProject/CropoutResource.cpp` | Stubs use the new signature. |
| `Source/CropoutSampleProject/CropoutGameMode.h` | New `TMap<int32, int32> Resources` UPROPERTY (round-20-introduced) + 4 interface overrides. |
| `Source/CropoutSampleProject/CropoutGameMode.cpp` | `RemoveResource_Implementation(ECropoutResourceType, int)`; `GetCurrentResources_Implementation` returns `TArray<ECropoutResourceType>`; `CheckResource_Implementation` casts enum → int32 for the map lookup; `UpdateResources.Broadcast(...)` casts back to int32. |
| `Content/Blueprint/Core/GameMode/BP_GM.uasset` | `Resources` BP-local var gone; 3 Map / Message nodes deleted; still has 5 orphan `Target Map` errors + 2 ForEachLoop struct errors. |

## R21 plan (when MCP comes back to life)

1. Resume scanning: `find_nodes title=""` across
   `Spawn Villager`, `Lose Check`, `Remove Target Resource`,
   `Remove Resource`, `Check Resource`, `Get Current Resources` to find the
   remaining 5 `Target Map` orphans; `delete_node` each.
2. `delete_node K2Node_MacroInstance_0` in `Load Villagers`,
   `create_node type_id="Utilities|Array|ForEachLoop"`, rewire the loop
   body from the saved graph.
3. Recompile BP_GM. Expect 0 fatals.
4. Same audit pass on BP_GI (autosave backups:
   `Saved/Autosaves/Game/Blueprint/Core/GameMode/BP_GI_Auto*.uasset`).
