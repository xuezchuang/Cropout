# Round 12.C — `ECropoutResourceType` UENUM migration + `TMap` key switch

## Scope
Replace the round-8 `TMap<int32, int32>` resource storage on the
C++ side with a strongly-typed `TMap<ECropoutResourceType, int32>`,
aligning the native enum's internal numeric values with the BP-side
`E_ResourceType` UserDefinedEnum (verified via binary inspection of
`Content/Blueprint/Interactable/Extras/E_ResourceType.uasset`).

## What changed

### `Source/CropoutSampleProject/CropoutResourceType.h`
Reduced the round-4 UENUM stub from 8 speculative values (None/Wood/
Stone/Food/Corn/Wheat/Lettuce/Pumpkin) to **the 4 actual BP-side
values**, with the internal numeric values aligned to the BP-side
canonical mapping:

| `ECropoutResourceType` | Internal value | BP-side `NewEnumeratorN` |
|---|---|---|
| `Wood`    | `0` | `NewEnumerator0` |
| `None`    | `1` | `NewEnumerator1` |
| `Stone`   | `5` | `NewEnumerator5` |
| `Food`    | `6` | `NewEnumerator6` |

Discovery path:
1. `re.findall(rb"[\x20-\x7e]{3,80}", E_ResourceType.uasset)`
   surfaced `'(0 - Value).DisplayNameMap'..'(3 - Value).DisplayNameMap'`
   + `E_ResourceType::NewEnumerator0/1/5/6`.
2. The `(N - Value).DisplayNameMap` strings are byte-pair-context
   tags showing the BP-side internal-value-to-display-name mapping:
   `0 -> Wood`, `1 -> None`, `2 -> Stone`, `3 -> Food`.

The 2/3/4 numeric gaps are BP-editor history (deleted/re-added
enumerator) — preserving the BP values exactly avoids invalidating
serialised `TMap<E_ResourceType, int32>` payloads.

The 4 speculative entries (`Corn / Wheat / Lettuce / Pumpkin`) are
removed; the round-4 stub header noted "BP `E_ResourceType` may have
slightly different names or fewer / more entries; the round-4
alignment pass is the time to fix that" — round 12.C does so.

### `Source/CropoutSampleProject/CropoutSaveGame.h` + `.cpp`
```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
TMap<ECropoutResourceType, int32> Resources;
```
`SetResources(const TMap<ECropoutResourceType, int32>&)` signature
matches.

### `Source/CropoutSampleProject/CropoutGameInstanceInterface.h`
`UpdateAllResources(...)` parameter is now
`const TMap<ECropoutResourceType, int32>&` (was int32).

### `Source/CropoutSampleProject/CropoutGameInstance.h` + `.cpp`
- `HandleEventUpdateAllResources(...)` signature matches.
- `UpdateAllResources_Implementation(...)` matches.
- The `CallBPSaveGameFuncWithMap` reflection helper params struct
  switched from `TMap<int32, int32>` to
  `TMap<ECropoutResourceType, int32>`.

### `Source/CropoutSampleProject/CropoutGameMode.cpp`
Both FScriptMapHelper reflective reads (in
`HandleEventAddResource`-side and `RemoveResource_Implementation`)
now cast the raw FProperty-stored int32 key through
`static_cast<ECropoutResourceType>` before passing the strongly-typed
map into `Execute_UpdateAllResources`.

## Build status

```
[1/4] Compile [x64] CropoutGameMode.cpp
[2/4] Link [x64] UnrealEditor-CropoutSampleProject-0039.lib
[3/4] Link [x64] UnrealEditor-CropoutSampleProject-0039.dll
[4/4] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 4.52 seconds
```
DLL tag now `-0039`. `.sln` regenerated, `Result: Succeeded`
(3.53s).

## BP_SaveGM disk state
- `is_dirty(BP_SaveGM)` → `false`.
- `save_assets([BP_SaveGM])` → `returnValue: true` (no-op write,
  disk unchanged). MD5 on disk: `caf6cfd3...` (same as after R8 — the
  R12.C C++ changes don't invalidate the BP-side reparent).

## Storage layout — current state

`BP_SaveGM_C` (data-only BP) currently exposes:
- Inherited from `UCropoutSaveGame`: `Interactables`,
  `Villagers`, `Resources` (`TMap<ECropoutResourceType, int32>`),
  `Seed`.
- BP-local: `Seed_0`, `Play Time` (the original BP-side SaveRef
  locals, surfaced by round 11's `list_variables`).

`Resources` on the BP-side as a BP-local `TMap<E_ResourceType, int32>`
was the round-8 assumption. With R8's `set_parent`, the inherited
C++ `Resources` (`TMap<ECropoutResourceType, int32>`) was actually
a SEPARATE property — the BP-side `Resources_0` was the local BP
copy. R12.C keeps this duality intact: the BP child inherits the
strongly-typed native `Resources`, and a future round that deletes
the BP-side `Resources_0` would let the inherited member take over
the persistence path.

## Caveats / open items
- `BP_SaveGM` does NOT have its BP-side `Resources_0` variable type
  changed (it remains `TMap<E_ResourceType, int32>` from the BP
  editor). Replacing a BP variable's type via MCP requires deleting
  the variable and creating a new one with the matching name and
  the new type — a multi-step operation not attempted in this
  round. Once a follow-up round does that, the inherited native
  `Resources` becomes the authoritative storage.
- `Corn / Wheat / Lettuce / Pumpkin` were removed from the native
  enum. If those values later surface in the BP enum (e.g. via a
  future content update), the migration is to add them back with
  appropriate internal values (the new BP-side round will assign
  new NewEnumeratorN indices).
- Native-side `ECropoutResourceType` is `uint8`-backed (UE BP
  enum interop). Reflection of BP-side UserDefinedEnum values
  still goes through raw `int32`, so the
  `static_cast<ECropoutResourceType>(int32)` bridge at every
  FProperty read remains the canonical translation point.

## Files touched
- `Source/CropoutSampleProject/CropoutResourceType.h` (4 values,
  aligned)
- `Source/CropoutSampleProject/CropoutSaveGame.h` / `.cpp`
  (`Resources` key type)
- `Source/CropoutSampleProject/CropoutGameInstanceInterface.h`
  (`UpdateAllResources` parameter)
- `Source/CropoutSampleProject/CropoutGameInstance.h` / `.cpp`
  (`HandleEventUpdateAllResources`, `UpdateAllResources_Implementation`,
  `CallBPSaveGameFuncWithMap`)
- `Source/CropoutSampleProject/CropoutGameMode.cpp` (two
  FScriptMapHelper reflective reads)
- `doc/round-reports/round-12.C-resource-type-uenum.md` (this file)

## Next (post-R12.C)
- Round 13 candidate: delete the BP-side `Resources_0` variable on
  `BP_SaveGM` via MCP so the inherited native `Resources` becomes
  the single source of truth. This requires `delete_variable` on
  the BP asset (tool availability to be probed).
- Or: declare `Resources` on `BP_SaveGM_C` as a `TMap<ECropoutResourceType, int32>`
  BP-side variable with the same name, so the C++ UPROPERTY is
  shadowed (rather than duplicated). Editing the BP variable type
  via MCP isn't natively supported; this would need a manual
  editor pass.
