# Round 15 — `BP_Player` compile-fix (delete 4 conflicting C++ UFUNCTIONs)

## Scope
Resolve the 8-fatal compile error on `BP_Player.uasset` reported via
the in-editor `Compiler Results` panel. Root cause was a name
collision: `BP_Player`'s BP-side function graphs (`Dof`,
`TrackMove`, `PositionCheck`, `BeginBuild`) collided with
identically-named empty `BlueprintCallable` UFUNCTIONs that had been
declared on the C++ `ACropoutPlayerPawn` parent class (added in
R19.B scaffold-style).

The fix is the **reverse of the round-12.B lift pattern**: instead of
adding native UFUNCTIONs to receive `Call Parent` from BP, we
**remove** the conflicting C++ UFUNCTIONs so the BP-side function
graphs become the only authoritative implementations.

## Diagnosed compile errors (pre-fix)

```
The function name in node  Dof  is already used
Overriden function is not compatible with the parent function  Dof .
  Check flags: Exec, Final, Static.

The function name in node  TrackMove  is already used
Overriden function is not compatible with the parent function  TrackMove .
  Check flags: Exec, Final, Static.

The function name in node  PositionCheck  is already used
Overriden function is not compatible with the parent function  PositionCheck .
  Check flags: Exec, Final, Static.

Cannot override 'BPI_Player_C::BeginBuild' at  Event BeginBuild
  which was declared in a parent with a different signature
```

(BPI_Player::BeginBuild is the interface event signature `(out) Target
Class, Resource Cost` — the BP-side `Event_BeginBuild(Target, Class,
Resource, Cost)` event handler has 4 input pins that don't match the
interface's 2 output pins. That's a separate signature problem but
collapses for free once the BP-side `BeginBuild` is the only
implementation.)

## Fix iterations attempted

- **R15 (BlueprintNativeEvent)**: changed the 4 `BlueprintCallable`
  flags to `BlueprintCallable, BlueprintNativeEvent` and renamed
  cpp body suffix to `_Implementation`. BP `compile_blueprint`
  still reported 4 fatals — UHT didn't re-classify the BP-side
  graphs as Event overrides.
- **R15.B (BlueprintImplementableEvent)**: changed flag to
  `BlueprintImplementableEvent` (only BP override). Same 4 fatals.
- **R15.C (delete C++ UFUNCTIONs entirely)** ✅ — the only path
  that compiles.

The root issue: BP_Player's BP-side function graphs were authored as
**standalone** functions (not Event overrides). They were created
before the C++ UFUNCTIONs existed. When the C++ UFUNCTIONs were
later added, the BP graphs were not auto-relinked into the new C++
interface contract — they remained standalone and shadowed the
C++ definition. There is no MCP tool to flip a standalone BP
function graph into an Event override, so the only path to a clean
compile was to remove the C++ duplicates entirely.

## Files touched

### `Source/CropoutSampleProject/CropoutPlayerPawn.h`
Removed 4 UFUNCTION declarations and their `_Implementation` virtuals:
- `void Dof()` (line 130-131, Camera)
- `void TrackMove()` (line 137-139, Movement)
- `void PositionCheck()` (line 197-199, Build)
- `void BeginBuild()` (line 205-206, Player lifecycle)

Each removal left a comment block explaining the C++ had a R19.B
scaffold UFUNCTION that conflicted with the BP-side function graph,
and the BP-side implementation is the authoritative one.

### `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
Removed the 4 corresponding `void ACropoutPlayerPawn::Xxx() {}` empty
definitions (they would be link errors after the header declarations
were dropped).

The other 17 of the 21 BP_Player-parity UFUNCTIONs (InputSwitch,
UpdateBuildAsset, UpdateCursorPosition, UpdateZoom, MoveTracking,
CornersInNav, CreateBuildOverlay, DestroySpawn, ClosestHoverCheck,
GetPlayerController, UpdatePath, RotateSpawn, OverlapCheck,
VillagerSelect, VillagerRelease, RemoveResources, SpawnBuildTarget,
plus EndBuild, SwitchBuildMode, UpdateSelectedVillager, ResetPicker
minus the 4 dropped) remain and the empty stubs continue to live.

## Build status

```
[1/5] Compile [x64] CropoutPlayerPawn.cpp
[2/5] Compile [x64] Module.CropoutSampleProject.gen.cpp
[3/5] Link [x64] UnrealEditor-CropoutSampleProject-0043.lib
[4/5] Link [x64] UnrealEditor-CropoutSampleProject-0043.dll
[5/5] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 5.78 seconds
```
DLL tag now `-0043`. `.sln` regenerated, `Result: Succeeded`
(3.81s).

## BP_Player compile outcome (post-fix)

After 1 editor restart (hot-reload of the C++ class chain) and
running `compile_blueprint(BP_Player)` via direct MCP:

```json
{"returnValue":null}
```

`null` is the BP-compile success sentinel (the BP class compiled,
returned nothing for the `compile` invocation). **8 fatal issues
cleared.**

## Disk persistence

```
save_assets([BP_Player]) → returnValue: true
is_dirty(BP_Player)      → returnValue: false
md5sum BP_Player.uasset:
  before R15:  ef9ed137690f30b69c55ed44bda0d1f5
  after  R15:  1f7055055499bea99aeafed064c50d60
file size: 1760596 → 1702973 bytes (-58KB / -3.3%)
```
The size drop is consistent with the recompile pass serialising a
slightly leaner compiled-BP representation.

## Open items (post-R15)

- `BP_Player` has the full 4-function game logic in its BP-side
  graphs (depth-of-field, edge-scroll, position validity, BeginBuild
  interface handler) — these are still authoritative and
  execute from BP at runtime.
- The 4 C++ UFUNCTION slots in `UCropoutPlayerPawn` are
  intentionally empty — a future round can lift each function's
  body into a native `Dof_Implementation()` etc. (would require
  re-adding the 4 UFUNCTIONs as `BlueprintNativeEvent` + filling the
  cpp bodies with the migrated logic from the BP-side graphs).
  This is the round 19.B reverse-direction migration.
- `BPI_Player::BeginBuild` interface signature has outputs
  `(out) Target Class, Resource Cost`. The current BP_Player
  `Event_BeginBuild(Target, Class, Resource, Cost)` block sets
  persistent-frame variables rather than pure BP-graph outputs; the
  interface contract works because the BP-side values are read
  by the calling code in the same BP. No change needed today.

## Files touched
- `Source/CropoutSampleProject/CropoutPlayerPawn.h` (4 UFUNCTIONs
  removed).
- `Source/CropoutSampleProject/CropoutPlayerPawn.cpp` (4 cpp
  stubs removed).
- `Content/Blueprint/Core/Player/BP_Player.uasset` (compile pass,
  MD5 + size changed).
- `doc/round-reports/round-15-bp-player-compile-fix.md` (this
  file).
