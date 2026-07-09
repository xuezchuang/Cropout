# Round 8.5 — `CropoutGameInstance.cpp` native-vs-legacy dispatch

## Scope
Refactor the 4 reflection-heavy save handlers in
`CropoutGameInstance.cpp` so that, once `BP_SaveGM` is reparented to
`UCropoutSaveGame`, the runtime path takes a direct C++ method call
rather than a `UFunction::FindFunctionByName` + `ProcessEvent` round-
trip. The legacy reflection path is preserved as a fallback for the
pre-reparent world.

## What changed

### New helper (in anonymous namespace)
```cpp
UCropoutSaveGame* GetNativeSaveGame(UCropoutGameInstance* Self)
{
    return Cast<UCropoutSaveGame>(LoadBPSaveGameObject(Self));
}
```
- Returns the `SaveRef` cast as `UCropoutSaveGame*` when `BP_SaveGM`
  has been reparented to that class.
- Returns `nullptr` otherwise, so call sites fall back to the existing
  `CallBPSaveGameFunc*` reflection helpers.

### Touched handlers (4 of 9)
Each gains a top-of-function `UCropoutSaveGame* Native = GetNativeSaveGame(this);`
check. The body then runs once with `Native->Method(...)` and, if
`Native == nullptr`, re-routes through `CallBPSaveGameFunc*`.

| Handler | Native dispatch path | Legacy reflection path |
|---|---|---|
| `HandleEventUpdateAllInteractables` | `Native->ClearInteractables()` + `Native->AddInteractable(...)` | `CallBPSaveGameFunc(Legacy, "ClearInteractables")` + per-actor `FAddInteractable_Params` |
| `HandleEventUpdateAllResources` | `Native->SetResources(InResources)` | `CallBPSaveGameFuncWithMap(Legacy, "SetResources", ...)` |
| `HandleEventClearSave` | `Native->ClearSave()` + conditional `Native->SetSeed()` | `CallBPSaveGameFunc(Legacy, "ClearSave")` + conditional `"SetSeed"` |
| `HandleEventUpdateAllVillagers` | `Native->ClearVillagers()` + `Native->AddVillager(...)` | `CallBPSaveGameFunc(Legacy, "ClearVillagers")` + per-pawn `FAddVillager_Params` |

### Notable side-improvement
`HandleEventUpdateAllInteractables` now extracts
`BP_Interactable.GetProgressionState(Actor)` via a single
`Actor->ProcessEvent(ProgFunc, &Result)` call (was previously
embedded in the BP-side reflection helper). Also pulls
`Actor->Tags[0]` into a local `FName ActorTag`, removing the implicit
array-index reliance from the BP-side `Get(Actor.Tags, 0)`.

## Build status

```
[1/4] Compile [x64] CropoutGameInstance.cpp
[2/4] Link [x64] UnrealEditor-CropoutSampleProject-0033.lib
[3/4] Link [x64] UnrealEditor-CropoutSampleProject-0033.dll
[4/4] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 4.22 seconds
```

.sln regenerated: `Result: Succeeded` (3.36s).

## Behaviour

- **Today (BP_SaveGM NOT reparented):** all 4 handlers go through the
  legacy reflection path. The C++ `Native` pointer is `nullptr` (the
  BP-side `BPSaveGM_C` class is not derived from `UCropoutSaveGame`).
  Behaviour is identical to round 7.
- **After MCP recovers + reparent succeeds:** `Cast<UCropoutSaveGame>`
  returns the now-native save object, and the 4 handlers take the
  direct dispatch path. The reflection helpers remain in place as a
  safety net (and are still used by `HandleEventInit`'s `LoadGame`
  call, which itself will migrate when round 9+ lifts the BP-side
  `LoadGame` event graph).

## Files touched
- `Source/CropoutSampleProject/CropoutGameInstance.cpp`
  - Added `#include "CropoutSaveGame.h"`.
  - Added `GetNativeSaveGame(Self)` helper next to
    `LoadBPSaveGameObject`.
  - Rewrote `HandleEventUpdateAllInteractables`,
    `HandleEventUpdateAllResources`, `HandleEventClearSave`,
    `HandleEventUpdateAllVillagers` to dispatch through `Native`
    when available.

## Next (when MCP recovers)
- `set_parent(BP_SaveGM, UCropoutSaveGame)` +
  `save_assets([BP_SaveGM])`: confirms the BP child compiles the
  new parent (will likely produce a "save-time class mismatch"
  warning if any BP override signature drifts; the 7 BP-parity
  UFUNCTIONs in `CropoutSaveGame.h` were parameter-aligned to the
  round-7 BP DSL comments, so no signature drift is expected).
- Once saved, the legacy reflection branches in this round's 4
  handlers become dead code; a future round may optionally delete
  them, but the cost is negligible so the safety net is preserved
  for now.
