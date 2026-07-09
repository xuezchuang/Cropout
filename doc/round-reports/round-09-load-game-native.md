# Round 9 — `UCropoutGameInstance::HandleLoadGame()` native impl

## Scope
Eliminate the BP-side `LoadGame` event-graph reflection hop called
from `HandleEventInit`. The native side gains a `HandleLoadGame()`
UFUNCTION that performs the synchronous load-from-slot; the BP-side
graph can later (`set_parent` step) be reduced to a single
`Call Parent.HandleLoadGame` node.

## What changed

### `Source/CropoutSampleProject/CropoutGameInstance.h`
Added a new public UFUNCTION next to the 9 event handlers:
```cpp
/** BP parity: `LoadGame`. */
UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
void HandleLoadGame();
```

### `Source/CropoutSampleProject/CropoutGameInstance.cpp`

**New body** for `HandleLoadGame()`:
```cpp
if (UGameplayStatics::DoesSaveGameExist(TEXT("SAVE"), 0))
{
    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TEXT("SAVE"), 0))
    {
        SaveRef = Loaded;
        HasSave = true;
        return;
    }
}
HasSave = false;
```

This is the synchronous equivalent of the BP-side LoadGame event
graph (the BP graph uses the same `UGameplayStatics` nodes under
the hood).

**Updated caller** in `HandleEventInit()`: replaces the previous
`FindFunctionByName(TEXT("LoadGame"))` reflection with a precedence
order that prefers `HandleLoadGame` (always present, since it's a
native UFUNCTION) and falls back to the BP-side `"LoadGame"` only if
the native one is somehow missing:
```cpp
if (UFunction* NativeFunc = GetClass()->FindFunctionByName(TEXT("HandleLoadGame")))
{
    ProcessEvent(NativeFunc, nullptr);
}
else if (UFunction* LegacyFunc = GetClass()->FindFunctionByName(TEXT("LoadGame")))
{
    ProcessEvent(LegacyFunc, nullptr);
}
```

Until the BP-side `LoadGame` event graph is reparented and the
routing nodes are connected, the `HandleLoadGame` UFUNCTION will
dispatch even though the BP-side `LoadGame` event graph would
otherwise have run (it has both). This is a deliberate pre-emption
that ships the right behaviour before MCP recovery.

## Build status

```
[1/6] Compile [x64] Module.CropoutSampleProject.gen.cpp
[2/6] Compile [x64] CropoutGameInstance.cpp
[3/6] Compile [x64] CropoutGameMode.cpp
[4/6] Link [x64] UnrealEditor-CropoutSampleProject-0034.lib
[5/6] Link [x64] UnrealEditor-CropoutSampleProject-0034.dll
[6/6] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 5.28 seconds
```

.sln regenerated: `Result: Succeeded` (3.33s).

## What still needs MCP to land
1. `set_parent(BP_SaveGM, UCropoutSaveGame)` + `save_assets(BP_SaveGM)`.
2. `read_graph_dsl(BP_GI)` to find the `EventInit` body (post-R7
   routing, it is `EventInit.then -> Call Parent.HandleEventInit`)
   and locate the BP-side `LoadGame` node chain.
3. Delete the BP-side `LoadGame` event graph, or insert a single
   `Call Parent.HandleLoadGame` if the BP graph contains unique
   side effects (legacy "create empty SaveRef" branch).

## Files touched
- `Source/CropoutSampleProject/CropoutGameInstance.h` (new UFUNCTION)
- `Source/CropoutSampleProject/CropoutGameInstance.cpp`
  (new body, `HandleEventInit` reflection hop replaced).
