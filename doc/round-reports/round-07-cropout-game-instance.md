# Round 7 — `UCropoutGameInstance` C++ lift + BP_GI graph routing

## Scope
Mirror Round 5's pattern on `BP_GI`: lift 8 BP-side variables + 9
EventGraph handlers + 5 BPI_GI native-interface overrides from
`Content/Blueprint/Core/GameMode/BP_GI.uasset` into
`Source/CropoutSampleProject/CropoutGameInstance.{h,cpp}`, then route
each BP event's `then` pin to a single `Call Parent.<Handler>` node so
the BP child becomes a thin override-only layer.

## C++ surface

### `Source/CropoutSampleProject/CropoutGameInstance.h`
- `UCLASS(Blueprintable)` `UCropoutGameInstance : public UGameInstance, public ICropoutGameInstanceInterface`.
- **8 UPROPERTYs** (BlueprintReadOnly + EditAnywhere, `Category="Cropout|GameInstance"`):
  `TargetSeed` (int32), `UI_Transition` (TObjectPtr<UUserWidget>), `HasSave` (bool),
  `SaveRef` (TObjectPtr<USaveGame>), `StartGameOffset` (float),
  `Audio` (TObjectPtr<USoundMix>), `MusicPlaying` (bool),
  `SoundMixes` (TArray<TObjectPtr<USoundMix>>).
- **9 event UFUNCTIONs** (`BlueprintCallable`, `Category="Cropout|GameInstance|Event"`):
  `HandleEventInit`, `HandleEventSaveGame`,
  `HandleEventUpdateAllInteractables`, `HandleEventLoadLevel`,
  `HandleEventUpdateAllResources(const TMap<int32,int32>&)`,
  `HandleEventUpdateAllVillagers`, `HandleEventClearSave(bool)`,
  `HandleAddLoadingUI`, `HandleOpenLevel(TSoftObjectPtr<UWorld>)`.
- **5 BPI_GI `_Implementation` overrides**: `UpdateAllResources`,
  `UpdateAllVillagers`, `UpdateAllInteractables`, `SaveGame`, `ClearSave`.

### `Source/CropoutSampleProject/CropoutGameInstance.cpp`
- Constructor empty (Round 5 pattern).
- BP-side cross-calls (`BPSaveGM.Get/Set Resources`, `GetVillagers`,
  `GetInteractables`) routed through `UFunction` reflection via
  the `CallBPSaveGameFunc` helper (round 7 stub; round 8+ lifts
  `BPSaveGM` to native `UCropoutSaveGame`).
- `HandleOpenLevel(TSoftObjectPtr<UWorld> Level)` calls BP-side
  `TransitionIn()` via reflection (until round 8+ lifts the
  BP_UI_Transition widget to native), then derives a level name
  from `Level.GetAssetName()` and calls
  `UGameplayStatics::OpenLevel(this, FName(*LevelName))`.
- `SaveGame_Implementation` binds a `FAsyncSaveGameToSlotDelegate`
  lambda; calls `UGameplayStatics::AsyncSaveGameToSlot(SaveRef, ...)`.

## BP_GI graph routing

Created 9 `Call Parent` nodes via MCP `create_node`:
| Event entry (EventGraph) | → Call Parent (Handler) |
|---|---|
| `K2Node_Event_0` (EventInit) | `K2Node_CallFunction_46` |
| `K2Node_Event_2` (EventSaveGame) | `K2Node_CallFunction_47` |
| `K2Node_Event_3` (EventUpdateAllInteractables) | `K2Node_CallFunction_48` |
| `K2Node_Event_4` (EventLoadLevel) | `K2Node_CallFunction_49` |
| `K2Node_Event_5` (EventUpdateAllResources) | `K2Node_CallFunction_50` |
| `K2Node_Event_6` (EventUpdateAllVillagers) | `K2Node_CallFunction_51` |
| `K2Node_Event_7` (EventClearSave) | `K2Node_CallFunction_52` |
| `K2Node_CustomEvent_0` (AddLoadingUI) | `K2Node_CallFunction_53` |
| `K2Node_CustomEvent_1` (OpenLevel) | `K2Node_CallFunction_54` |

All 9 `event.then → CallParent.execute` pin connections succeeded
(connect_pins returns `null` on success, same pattern as round 6's
8-event BP_GM routing). UE's automatic cleanup removed the orphan
body nodes downstream of each reconnected entry point.

## Build status

- **UBT build**: `Build.bat CropoutSampleProjectEditor Win64 Development -Project=...`
  → `Result: Succeeded` after fixes to `HandleOpenLevel` signature
  (`FName` → `TSoftObjectPtr<UWorld>`), forward declaration of
  `class UWorld`, and `#include "Engine/World.h"`.
- **.sln regen**: `Build.bat -Mode=GenerateProjectFiles` → wrote
  `CropoutSampleProject.sln` + `CropoutSampleProject.slnx`,
  `Result: Succeeded` (3.38s).

## Persistence (BP_GM + BP_GI)

**Blocked by MCP harness bug.** `save_assets` rejects the
`asset_paths` array parameter with:
> `could not convert incoming function input params Json to a UStruct`

Tried empty array, single-element, multi-element variants — all
fail. The session's in-memory BP_GM 8-event routing (round 6)
and BP_GI 9-event routing (round 7) are **not yet persisted to
disk**. They will be re-routed (or BP script-replayed) after the
harness bug is fixed.

Auto-backups on disk:
- `/tmp/BP_GM_BeforeRound6.uasset` (pre-routing)
- `/tmp/BP_GM_AfterRound6_Auto1.uasset` (MD5 `d33a5492be3f44a45f27bd6a95fd8f95`)

## Hot points (next rounds)
- **R8**: Lift `BPSaveGM` USaveGame subclass → `UCropoutSaveGame`
  native class (eliminate `CallBPSaveGameFunc` reflection calls
  in `UCropoutGameInstance`).
- **R9**: Lift `BP_Transition` widget class (`UI_Transition` ref)
  → native `UUserWidget` subclass, so `HandleOpenLevel`'s
  `TransitionIn()` call can drop the reflection hop.
- **R10**: Lift `BPF_Cropout` Blueprint function library
  (referenced from `BP_GM.HandleEventIslandGenComplete` etc.)
  → static `UCropoutBlueprintLibrary` UBlueprintFunctionLibrary.
- **R11**: Lift `BP_Spawner` (`BeginASyncSpawning`, `SpawnMeshOnly`,
  `SpawnLoadedInteractables`) → native `UCropoutSpawner` actor.
- **R12**: `E_ResourceType` BP enum → native `ECropoutResourceType`
  UENUM with values aligned to BP side (the round-4 attempt rolled
  back because BP-side `TMap<E_ResourceType,int32>` can't be type-
  matched from C++; round 12 will migrate the BP-side map storage
  type alongside).

## Files touched
- `Source/CropoutSampleProject/CropoutGameInstance.h` (new)
- `Source/CropoutSampleProject/CropoutGameInstance.cpp` (new)
- `Content/Blueprint/Core/GameMode/BP_GI.uasset` (in-memory
  routing of 9 EventGraph handlers — persist pending)
