# Round 12.B — `UCropoutSpawner` 1:1 mirror of `BP_Spawner`

## Scope
Mirror `BP_Spawner.uasset`'s user-defined functions into a native
`AActor`-derived `UCropoutSpawner` so the BP child can be
reparented (round 12.B+) without losing the function-graph contract.

## Function enumeration via direct MCP

Using the same `get_connected_subgraph` pattern that landed R6+R7+R8
and R12.A, three user-defined functions were discovered in
`BP_Spawner` (under plugin path
`/IslandGenerator/Spawner/BP_Spawner.BP_Spawner`):

| Display name (BP) | BP internal name | C++ UFUNCTION |
|---|---|---|
| `Spawn Mesh Only` | `SpawnMeshOnly` | `SpawnMeshOnly(AActor*)` |
| `Spawn Random` | `SpawnRandom` | `SpawnRandom(AActor*)` |
| `Load Spawn` | `LoadSpawn` | `LoadSpawn()` |

### Note on the originally-planned 4th function
`BeginASyncSpawning` is referenced in
`BP_GM.BP_GM:EventGraph K2Node_CustomEvent_1` (the BP_GM-side
Custom event entry-point that triggers async spawn setup), but is
NOT a function defined on `BP_Spawner`. Likewise
`SpawnLoadedInteractables` is a BP-side local function on `BP_GM`
(not on `BP_Spawner`). The original "4 spawner functions" assumption
in the planner was off by one — the actual count is 3, and round
12.B lifts exactly those 3.

## Files added

### `Source/CropoutSampleProject/CropoutSpawner.h`
- `UCLASS(Blueprintable)` `ACropoutSpawner : public AActor`.
- Empty constructor (mirrors `AActor`'s default).
- 3 `UFUNCTION(BlueprintCallable, Category = "Cropout|Spawner")`
  declarations matching the BP-internal function names 1:1.

### `Source/CropoutSampleProject/CropoutSpawner.cpp`
- `SpawnMeshOnly(SpawnRef)`: forwards to BP-side
  `Function=SpawnMeshOnly` via `ProcessEvent` so the existing
  BP child behaviour is preserved until round 12.B+ redirects the
  BP graph to `Call Parent`.
- `SpawnRandom(SpawnRef)`: same forward-to-BP pattern.
- `LoadSpawn()`: same forward-to-BP pattern (no parameters).

The forward-to-BP pattern is the same shape as the
`GetBPSaveGameFunc` reflection helpers in
`CropoutGameInstance.cpp` — keeps the runtime behaviour identical
during the lift period, and lets the native UFUNCTIONs be the
authoritative implementations once the BP graph is updated to
`Call Parent.<X>`.

## Build status
```
[1/5] Compile [x64] CropoutSpawner.cpp
[2/5] Compile [x64] Module.CropoutSampleProject.gen.cpp
[3/5] Link [x64] UnrealEditor-CropoutSampleProject-0037.lib
[4/5] Link [x64] UnrealEditor-CropoutSampleProject-0037.dll
[5/5] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 6.85 seconds
```
DLL tag now `-0037`. `.sln` regenerated, `Result: Succeeded`
(3.96s).

## BP_Spawner disk state
- `is_dirty(BP_Spawner)` → `false` (BP-side untouched).
- `save_assets([BP_Spawner])` → `returnValue: true` (no-op write,
  disk unchanged). MD5 on disk: `d92c9635e937c29d8016faa4a2cc4513`.

## What this enables
Existing BP-side callers (BP_GM `EventBeginPlay`, BP_GM
`HandleBeginASyncSpawning`) reference `BP_Spawner` by class name.
Today those calls go through `BP_Spawner_C` (BP-defined child)
which still serves the BP-side implementations; the forward-to-BP
reflection in `UCropoutSpawner::SpawnMeshOnly/SpawnRandom/LoadSpawn`
ensures the runtime path is identical whether the BP child is
reparented or not.

A future round can:
1. `set_parent(BP_Spawner, ACropoutSpawner)` to give the BP child
   the native parent class.
2. For each of the 3 functions, either delete the BP-side body or
   replace the function entry with `Call Parent.<X>` (the same
   9-step MCP reroute pattern from R6+R7).
3. Once BP-side bodies are deleted, replace the `ProcessEvent`
   stubs in `UCropoutSpawner.cpp` with real implementations
   (spawn meshes using `UGameplayStatics::SpawnActor` + the
   `ST_SpawnData` / `ST_SpawnInstance` row structs).

## Files written
- `Source/CropoutSampleProject/CropoutSpawner.h`
- `Source/CropoutSampleProject/CropoutSpawner.cpp`
- `doc/round-reports/round-12.B-spawner-actor.md` (this file)

## Next (Round 12.C)
- `E_ResourceType` BP enum → native `UENUM` with values
  aligned to BP side. Round 4 attempted this with a cast to
  `ECropoutResourceType` from `int32` in `TMap`; that was rolled
  back. The right approach for round 12.C is:
  1. Define `ECropoutResourceType` in
     `Source/CropoutSampleProject/CropoutResourceType.h` (already
     present as round-4 stub).
  2. Migrate the BP-side `TMap<E_ResourceType, int32>` storage on
     `BPSaveGM` to a `TMap<FName, int32>` keying on `EnumValue`
     reflection, OR replace the BP-side variable with a
     `TMap<ECropoutResourceType, int32>` after migrating the BP
     child to `UCropoutSaveGame` (already done in R8).
  3. Update `UCropoutSaveGame::Resources` to
     `TMap<ECropoutResourceType, int32>` (currently
     `TMap<int32, int32>`).
