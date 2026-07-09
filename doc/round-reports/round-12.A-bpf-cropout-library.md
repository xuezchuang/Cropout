# Round 12.A — `UCropoutBlueprintLibrary` 1:1 mirror of `BPF_Cropout`

## Scope
Mirror `BPF_Cropout.uasset`'s two user-defined helper functions
(`Get Cropout GI`, `Get Cropout GM`) into a native
`UBlueprintFunctionLibrary` subclass `UCropoutBlueprintLibrary`,
compile, and confirm `BPF_Cropout` disk state is preserved.

## Function enumeration via direct MCP

The earlier round-of-rounds had assumed `BPF_Cropout`'s user-defined
functions were not enumerable from the binary alone; that was wrong.
Direct MCP `get_connected_subgraph` on each candidate
`K2Node_FunctionEntry_*` returned the right nodes once the literal
display-name-with-spaces was used as the graph path suffix
(`BPF_Cropout.BPF_Cropout:Get Cropout GI`) and the BP-internal
function names came back as `|GetCropoutGI` and `|GetCropoutGM`.

Binary scan output confirmed via MCP that BPF_Cropout has exactly
**two** user-defined functions:

| Display name (BP) | Internal name (BP) | subgraph path |
|---|---|---|
| `Get Cropout GI` | `GetCropoutGI` | `BPF_Cropout.BPF_Cropout:Get Cropout GI` |
| `Get Cropout GM` | `GetCropoutGM` | `BPF_Cropout.BPF_Cropout:Get Cropout GM` |

### BP-side body (per subgraph dump)
Both functions follow the same template:
1. `K2Node_FunctionEntry_0` (Exec, no params)
2. `K2Node_CallFunction` calling `GetGameInstance` (or `GetGameMode`)
   with the `WorldContextObject` pin
3. `K2Node_DynamicCast_0` casting the result to `BP_GI` /
   `BP_GM` (an `AsBP_GI` / `AsBP_GM` typed output pin)
4. `K2Node_FunctionResult_0` returning the cast result on the
   `then` chain

The signatures (per the BP graph dump):
```cpp
// BP-FunctionLibrary, static, BlueprintPure
UFUNCTION(BlueprintCallable, BlueprintPure)
static UCropoutGameInstance* GetCropoutGI(const UObject* WorldContextObject);

UFUNCTION(BlueprintCallable, BlueprintPure)
static ACropoutGameMode* GetCropoutGM(const UObject* WorldContextObject);
```

The `WorldContextObject` is required because `UGameplayStatics`
needs a non-null `UObject*` to derive a `UWorld*` when no PIE/Game
context is supplied.

## Files added

### `Source/CropoutSampleProject/CropoutBlueprintLibrary.h`
- `UCLASS()` (no `Blueprintable` — the library itself is not
  derived from; only its UFUNCTIONs are exposed to BP).
- Two `UFUNCTION(BlueprintCallable, BlueprintPure, ...,
  meta=(WorldContext = "WorldContextObject"))` static methods.

### `Source/CropoutSampleProject/CropoutBlueprintLibrary.cpp`
- `GetCropoutGI`: `Cast<UCropoutGameInstance>(
  UGameplayStatics::GetGameInstance(WorldContextObject))`.
- `GetCropoutGM`: `Cast<ACropoutGameMode>(
  UGameplayStatics::GetGameMode(WorldContextObject))`.

Both return `nullptr` on `WorldContextObject == nullptr` (matches
the BP-side `bSuccess=false` fallback).

## Build status
```
[1/5] Compile [x64] CropoutBlueprintLibrary.cpp
[2/5] Compile [x64] Module.CropoutSampleProject.gen.cpp
[3/5] Link [x64] UnrealEditor-CropoutSampleProject-0036.lib
[4/5] Link [x64] UnrealEditor-CropoutSampleProject-0036.dll
[5/5] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 5.65 seconds
```
DLL tag now `-0036`. `.sln` regenerated, `Result: Succeeded`
(3.96s).

## BPF_Cropout disk state
- `is_dirty(BPF_Cropout)` → `false` (BP-side untouched).
- `save_assets([BPF_Cropout])` → `returnValue: true` (no-op write,
  disk unchanged). The native C++ UFUNCTIONs are reachable from BP
  without any reparent step because `BPF_Cropout` is a
  `BlueprintFunctionLibrary` — its function-graph definitions stay,
  and callers can additionally resolve `UCropoutBlueprintLibrary::*`
  by signature from any BP graph.

## What this enables
Any BP graph that today calls `BPF_Cropout.GetCropoutGI(self)` or
`BPF_Cropout.GetCropoutGM(self)` can be redirected to the native
library by:
1. Replacing the `BPF_Cropout` call site with the native
   `UCropoutBlueprintLibrary` call (same signature).
2. Saving the BP graph (typically via `save_assets` on the
   relevant BP asset).

The BP-side `BPF_Cropout` library is left intact for now — there is
no pressing reason to delete the BP-side graphs as long as they
duplicate the native UFUNCTION behaviour. A future round may use
`delete_node` to simplify the BP-side call sites; for now we just
de-duplicate by exposing the C++ versions side-by-side.

## Files written
- `Source/CropoutSampleProject/CropoutBlueprintLibrary.h`
- `Source/CropoutSampleProject/CropoutBlueprintLibrary.cpp`
- `doc/round-reports/round-12.A-bpf-cropout-library.md` (this file)

## Next (Round 12.B / 12.C)
- `BP_Spawner` → native `UCropoutSpawner` actor (referenced from
  `BP_GM.EventBeginPlay` and `BP_GM.HandleBeginASyncSpawning` —
  uses `SpawnLoadedInteractables`/`BeginASyncSpawning`/`SpawnMeshOnly`/`SpawnRandom`).
  Direct MCP can probe each K2Node_BlueprintFunctionCall on the
  spawner asset to enumerate its 4 user-defined functions.
- `E_ResourceType` BP enum → native `UENUM` (round 12.C, requires
  migrating BP-side `TMap<E_ResourceType, int32>` storage to the
  native int32 fallback already in use by R8's `TMap<int32,int32>`
  in `UCropoutSaveGame`).
