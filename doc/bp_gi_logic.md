# BP_GI Logic Audit (2026-07-07)

This document captures what BP_GI (`/Game/Blueprint/Core/GameMode/BP_GI.uasset`)
currently contains, why each graph is the way it is, and where the compile
errors originate. Read this before touching the asset again.

## Asset shape

- **Parent class**: `UCropoutGameInstance` (C++ native, Round 7+, see
  `Source/CropoutSampleProject/CropoutGameInstance.h`).
- **Member variables**: 0 BP-defined (all 8 mirrored UPROPERTYs come from C++).
- **Event dispatchers**: 0 BP-defined.
- **Referenced by**: `BP_GM`, `BP_MainMenuGM`, `Village`, `Demo`, `BPF_Cropout`,
  `UI_EndGame`, `UI_Pause`, `UI_MainMenu`, `UIE_Slider`. Changing the public
  surface (events / interface messages) cascades into all of these.

## Graph inventory (7 graphs)

### 1. `EventGraph`

Forwards 9 incoming events to C++ handlers. Each node is a one-liner:

| Event                                  | Handler C++ call                       | Notes |
|----------------------------------------|----------------------------------------|-------|
| `SetSaveData|EventUpdateAllInteractables` | `HandleEventUpdateAllInteractables`   | OK    |
| `EventInit`                            | `HandleEventInit`                      | OK    |
| `SetSaveData|EventUpdateAllResources`  | `HandleEventUpdateAllResources`        | **Type mismatch** — see below |
| `SetSaveData|EventClearSave`           | `HandleEventClearSave(false)`          | **Default value issue** — see below |
| `EventLoadLevel`                       | `HandleEventLoadLevel`                 | OK    |
| `SetSaveData|EventUpdateAllVillagers`  | `HandleEventUpdateAllVillagers`        | OK    |
| `SetSaveData|EventSaveGame`            | `HandleEventSaveGame`                  | OK    |
| `Custom|OpenLevel`                     | `HandleOpenLevel(0)`                   | OK    |
| `Custom|AddLoadingUI`                  | `HandleAddLoadingUI`                   | OK    |

#### Issues on this graph

- **`SetSaveData|EventUpdateAllResources`** (event dispatcher binding):
  the `NewParam` data output is typed `Map of E_ResourceType Enums to Integers`
  (BP UserDefinedEnum), but the C++ `HandleEventUpdateAllResources` expects
  `Map of ECropoutResourceType Enums to Integers` (native UENUM). Source of
  the `Map of E_ResourceType Enums to Integers is not compatible with Map of
  ECropoutResourceType Enums to Integers` compiler error.
- **`SetSaveData|EventClearSave`** has a `Resources` data pin (likely inherited
  from an older event definition) whose default value is `Wood`. Map inputs
  cannot have a single enum value as default — source of `The current value
  (Wood) of the 'Resources' pin is invalid: Map inputs (like 'Resources') must
  have an input wired into them`. Also: `Cannot order parameters ClearSeed in
  function ClearSave` — the function's `ClearSeed` parameter appears to lack
  its order metadata.

The dispatcher `SetSaveData` is defined by the BP-side `BPI_GI` interface
(`/Game/Blueprint/Core/Save/BPI_GI.uasset`), which BP auto-generates from
the interface methods. BP_GI's EventGraph binds to those dispatcher events;
BP_GM (or whoever) broadcasts them.

### 2. `Clear Save` (no parameters) — **CLEARED BY THIS AUDIT**

Was: `GetSaveRef` (USaveGame*) → `SetPlayTime` / `GetVillagers` /
`GetInteractables` / `GetResources` / `SetResources` on the BP-side
`BP_SaveGM` / `UCropoutSaveGame`. Body is dead code: nobody calls this BP
function — the EventGraph's `SetSaveData|EventClearSave` handler calls
`HandleEventClearSave` directly in C++.

**Now**: empty body (only `K2Node_FunctionEntry_0` remains; 10 broken nodes
were deleted by `delete_node` one at a time).

### 3. `ClearSave (Clear Seed: bool)` — empty stub

Sets persistent frame `Clear Seed = false`, then jumps to
`ExecuteUbergraphBPGI(0)`. This is the function-bound entry for
`SetSaveData|EventClearSave` in the ubergraph fallback. Not actually called
by anything in the EventGraph (which goes through `HandleEventClearSave`
directly).

### 4. `SaveGame` — empty stub

Just `ExecuteUbergraphBPGI(0)`. Empty stub; not called.

### 5. `Play Music (Audio, Volume, Persist)` — **BROKEN, NEEDS FIX**

Current body:

```
Branch (Condition = true, hard-coded — dead if-branch)
  → Audio|Modulation|SetGlobalControlBusMixValue(Bus="…Cropout_Music_WinLose", Value=0.5)
  → Audio|Modulation|SetGlobalControlBusMixValue(Bus="…Cropout_Music_Stop", Value=0.0)
  → Audio|CreateSound2D(Sound=Audio, VolumeMultiplier=Volume, …, bPersist=Persist)
  → |SetAudio(Output_Get)        ← BROKEN
  → Audio|Components|Audio|Play(self=Output_Get)
```

**Issue**: `|SetAudio` is a setter on `self.Audio` (UCropoutGameInstance
member variable). After the Round 7 C++ migration that member is:
- typed `TObjectPtr<USoundMix>`
- marked `BlueprintReadOnly`

So `CreateSound2D` returns `UAudioComponent*` and we try to assign it to a
`USoundMix*` `BlueprintReadOnly` variable — the `Set Audio` errors.

**C++ replacement**: `UCropoutGameInstance::PlayMusic(USoundBase*, float,
bool)` already implements this logic (see `CropoutGameInstance.cpp:472`),
including the `SetGlobalControlBusMixValue(Cropout_Music_Stop, 0.0)` call.
The BP-defined `Play Music` is dead code and should be emptied.

### 6. `TransitionIn` — empty stub
### 7. `TransitionOut` — empty stub

Both are BP-defined functions that would shadow the C++ parent's
`TransitionIn` / `TransitionOut` — but the C++ side has renamed those to
`DoTransitionIn` / `DoTransitionOut` for exactly this reason (see
`CropoutGameInstance.h:160` comment). C++ callers use the renamed name.

## What's actually wrong (compile-error mapping)

| Error message                                                                | Origin graph                              | Real fix |
|------------------------------------------------------------------------------|-------------------------------------------|----------|
| `Cannot order parameters ClearSeed in function ClearSave`                   | `SetSaveData|EventClearSave` event binding | delete the `Resources` default value + reorder `ClearSeed` |
| `The current value (Wood) of the 'Resources' pin is invalid: Map inputs must have an input wired` | `SetSaveData|EventClearSave` event binding | delete the `Resources` data pin from the event definition |
| `Map of E_ResourceType ... is not compatible with Map of ECropoutResourceType` | `SetSaveData|EventUpdateAllResources`     | change the event's `NewParam` type from `Map of E_ResourceType` to `Map of ECropoutResourceType` (or, if the binding type comes from BPI_GI, change BPI_GI's interface signature) |
| `Sound Mix Object Reference is not compatible with Audio Component Object Reference` / `CropoutGameInstance.Audio is not blueprint writable` | `Play Music` (line `\|SetAudio _returnvalue`) | delete the `SetAudio` node (and probably the entire body — C++ does this natively) |
| `Save Game Object Reference is not compatible with Cropout Save Game Object Reference` | `Clear Save` body                         | already fixed by emptying the body |
| `Only exactly matching structures are considered compatible` (Interactables ↔ New Param) | upstream graph that wired an `FCropoutSaveInteract` literal as default | investigate which graph references `New Param` as a struct default — probably another dead body or a default value on a function param |
| `CropoutSaveGame.Resources is not blueprint writable. Set Resources`         | `Clear Save` body                         | already fixed by emptying the body |
| `Could not find a variable named "Seed_0" in BP_SaveGM`                      | some body referenced `Seed_0` on BP_SaveGM | BP_SaveGM (parent UCropoutSaveGame) has `Seed`, not `Seed_0`. Find which graph uses `Seed_0` — most likely the empty `Clear Save` body I just emptied. Verify after recompile. |

## Recovery state

- `Saved/Autosaves/Game/Blueprint/Core/GameMode/BP_GI_Auto0..Auto9.uasset`
  are pre-fix snapshots (Auto9 = most recent before autosave was disabled
  at 2026-07-07 10:13). `Saved/Autosaves_disabled_20260707_101320/` has
  older snapshots.
- The current `Content/Blueprint/Core/GameMode/BP_GI.uasset` has the
  `Clear Save` body emptied (10 nodes deleted) but everything else still
  in its pre-audit broken state. Last write timestamp: 2026-07-07 20:45.

## Decision (proposed)

The C++ parent class (`UCropoutGameInstance`) is fully implemented for all
runtime behavior — `HandleEvent*` family for events, `PlayMusic` /
`StopMusic` / `ClearSaveAsset` / `UpdateSaveAsset` etc. for the local
helpers. BP_GI's only legitimate role is to forward events to the C++
handlers, which it already does via the 9 EventGraph one-liners.

The local BP-defined functions (`Clear Save`, `ClearSave`, `SaveGame`,
`Play Music`, `TransitionIn`, `TransitionOut`) are all **dead code**:
- 4 of them are already empty stubs.
- `Clear Save` and `Play Music` had non-empty bodies that are now broken
  because the C++ parent changed the types.

**Recommended**: empty the remaining two bodies (`Play Music` and
`ClearSave (Clear Seed)` — the latter is technically already empty but
its event binding has a bad default; we'd need to fix the dispatcher
binding), and fix the two event signatures on the EventGraph:

1. `SetSaveData|EventUpdateAllResources` event — change `NewParam` type
   to `Map of ECropoutResourceType Enums to Integers`.
2. `SetSaveData|EventClearSave` event — drop the spurious `Resources` data
   pin (or its `Wood` default), keep just `Clear Seed: bool`.

Both event-signature fixes are mid-risk: modifying a K2Node_Event node's
metadata via the available MCP tools has not been exercised yet. Need to
either (a) delete the event node and re-create with the correct signature,
or (b) find a way to edit the bound delegate signature in place.

## What to do next

Before touching BP_GI further, decide:

1. **Are we OK with the dead-BP-functions-emptied approach?** That is: BP_GI
   becomes a thin forwarder; all real logic lives in C++. (Yes/No)
2. **For the EventGraph event signatures**: prefer
   - (a) delete + re-add the broken event nodes (cleaner, more risk)
   - (b) accept the type mismatches and accept BP_GI not compiling, but
     make sure C++ still works at runtime (BP compile errors don't block
     editor use unless something forces a recompile of dependents).
3. **Should we also revisit BP_SaveGM and BPI_GI** to clean up the
   dispatcher signature mismatch at the source? Those are the upstream
   pieces that originally defined `Map of E_ResourceType`.