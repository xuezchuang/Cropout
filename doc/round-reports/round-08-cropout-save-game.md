# Round 8 — `UCropoutSaveGame` + `FCropoutSaveInteract` / `FCropoutSaveVillager` USTRUCTs scaffold

## Scope
Lift `BP_SaveGM.uasset`'s underlying variable structure (3 save arrays /
maps + 7 BP-side functions) into native C++ so that `BP_SaveGM` can be
reparented to `UCropoutSaveGame` once the MCP `set_parent` call succeeds
(harness currently blocking array params). Until that reparent happens,
`BP_SaveGM`'s BP-side function implementations continue to run; once
reparented, the BP-side overrides may be deleted (or kept as thin
overrides for level-specific tuning).

Source of truth for the BP-side shape: the `BP DSL:` comments in
`Source/CropoutSampleProject/CropoutGameInstance.cpp` lines 70-216
(inherited from round 7).

## New files

### `Source/CropoutSampleProject/CropoutSaveGame.h`
- **2 USTRUCTs** (`BlueprintType`):
  - `FCropoutSaveInteract` — 5 fields mirroring the per-actor
    `SetArrayElem(SaveRef.Interactables, ...)` call:
    `FTransform Transform`, `TObjectPtr<UClass> Class`,
    `int32 ProgressionState`, `FName Tag`, `bool bIsValid`.
  - `FCropoutSaveVillager` — 3 fields mirroring the per-pawn
    `SetArrayElem(SaveRef.Villagers, ...)` call:
    `FVector Location`, `FName Tag`, `bool bIsValid`.
- **1 UCLASS** `UCropoutSaveGame : public USaveGame` with:
  - 4 state fields: `TArray<FCropoutSaveInteract> Interactables`,
    `TArray<FCropoutSaveVillager> Villagers`,
    `TMap<int32,int32> Resources` (BP-side enum cast to int32 due
    to round-4 E_ResourceType bridge rollback), `int32 Seed` (tracks
    `PCGSettings.SetSeed` calls during `ClearSave(bClearSeed=true)`).
  - 7 BP-parity UFUNCTIONs (`BlueprintCallable`,
    `Category="Cropout|SaveGame"`): `ClearInteractables`,
    `AddInteractable(Transform, Class, ProgressionState, Tag, Source)`,
    `ClearVillagers`, `AddVillager(Location, Tag, Source)`,
    `SetResources(const TMap<int32,int32>&)`, `ClearSave`, `SetSeed`.

### `Source/CropoutSampleProject/CropoutSaveGame.cpp`
- Empty constructor (`USaveGame` defaults).
- 7 function bodies that mirror the BP DSL:
  - `Clear*()` → `TArray::Reset()`.
  - `Add*()` → build struct entry from params, append.
  - `SetResources(NewParam)` → `Resources = InResources`.
  - `ClearSave` → reset all 3 collections + `Seed = 0`.
  - `SetSeed` → `Seed = FMath::Rand() ^ FPlatformTime::Cycles()`.

## Build status

```
[1/5] Compile [x64] CropoutSaveGame.cpp
[2/5] Compile [x64] Module.CropoutSampleProject.gen.cpp
[3/5] Link [x64] UnrealEditor-CropoutSampleProject-0032.lib
[4/5] Link [x64] UnrealEditor-CropoutSampleProject-0032.dll
[5/5] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 5.21 seconds
```

.sln regenerated: `Result: Succeeded` (3.30s).

## What is still on the BP side (next layer of work)

`CropoutGameInstance.cpp` (round 7) still routes 4 of the
`CallBPSaveGameFunc*` reflection calls through
`UFunction::FindFunctionByName("ClearInteractables"|"AddInteractable"|
"SetResources"|"ClearSave"|"ClearVillagers"|"AddVillager"|"SetSeed")`
on the base-typed `TObjectPtr<USaveGame>` save ref.

Once `BP_SaveGM` is reparented to `UCropoutSaveGame` (and recompiled),
those reflection calls can be reduced to:
```cpp
if (UCropoutSaveGame* Native = Cast<UCropoutSaveGame>(SaveRef.Get()))
{
    Native->ClearInteractables();      // direct dispatch
    // ...
}
else
{
    CallBPSaveGameFunc(SaveRef, TEXT("ClearInteractables"));  // legacy fallback
}
```

That refactor is the natural follow-on once MCP recovers and the
reparent + save is confirmed.

## Persistence (BP_SaveGM reparent)

**Blocked by MCP harness bug.** The `set_parent(BP_SaveGM, UCropoutSaveGame)`
+ `save_assets(BP_SaveGM)` flow cannot run because the harness
strips array params from all MCP `call_tool` invocations. The same
harness bug is also blocking:
- `save_assets(BP_GM)` (round 6 in-memory 8-event routing)
- `save_assets(BP_GI)` (round 7 in-memory 9-event routing)

When the harness recovers, the execution sequence for round 8 is:
1. `set_parent(BP_SaveGM, UCropoutSaveGame)`
2. `read_graph_dsl(BP_SaveGM)` to verify the BP child compiled-in
   the new parent (or report a class mismatch).
3. Verify the BP graph no longer calls its own implementations of
   the 7 `UFUNCTION`s that now live on the C++ parent; either remove
   the BP-side overrides or route them as `Call Parent.<X>`.
4. `save_assets([BP_GM, BP_GI, BP_SaveGM])` to persist all three.

## Files written
- `Source/CropoutSampleProject/CropoutSaveGame.h` (new)
- `Source/CropoutSampleProject/CropoutSaveGame.cpp` (new)
- `doc/round-reports/round-08-cropout-save-game.md` (this report)
