# Round 16 — `BP_Player` C++ becomes authoritative for `Dof/TrackMove/PositionCheck`

## Scope
Finish the round-15.B cleanup by:
1. Re-adding 3 of the 4 removed UFUNCTIONs as
   `BlueprintNativeEvent` (so the BP-side function graphs can be
   deleted cleanly without leaving the game with no
   implementation).
2. Leaving `BeginBuild` permanently removed (it is the
   `BPI_Player::BeginBuild` interface event, not a free C++
   UFUNCTION — see "BeginBuild caveat" below).
3. Using direct MCP `remove_function_graph` to delete the 3
   BP-side function graphs (`Dof`, `TrackMove`, `PositionCheck`).
4. Re-compiling and saving `BP_Player.uasset`.

Net effect: the 3 functions are now C++-side `BlueprintNativeEvent`
stubs. They run but are no-ops. The BP-side game logic (depth-of-
field post-process settings, edge-scroll move, position validity
check) has been **demoted** — a future round has to migrate the
BP-side logic into the corresponding `Dof_Implementation()` /
`TrackMove_Implementation()` / `PositionCheck_Implementation()`
bodies.

## Files touched

### `Source/CropoutSampleProject/CropoutPlayerPawn.h`
3 `UFUNCTION(BlueprintCallable, BlueprintNativeEvent, ...)` re-added
for `Dof`, `TrackMove`, `PositionCheck`, each with a matching
`virtual void Xxx_Implementation();` declaration.

`BeginBuild` was deliberately **not** re-added:
- `BPI_Player::BeginBuild` is a Blueprint interface event whose
  BP-side signature in BP_Player is `(out) Target Class, Resource
  Cost` (per the round-15 subgraph dump). A C++
  `BlueprintNativeEvent BeginBuild()` with no params cannot
  match that signature without a full struct-out refactor of the
  interface contract.
- The BP_Player BP-side event graph that implements
  `BPI_Player::BeginBuild` is *not* a free-standing BP function
  graph — it is tied to the BP's interface implementation slot
  and gets compiled separately. `remove_function_graph`'s error
  `Graph BeginBuild does not exist` confirmed this: it never was
  a free graph.

### `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
3 `_Implementation` empty stub definitions added next to the other
BP_Player parity stubs:
```cpp
void ACropoutPlayerPawn::Dof_Implementation() {}
void ACropoutPlayerPawn::TrackMove_Implementation() {}
void ACropoutPlayerPawn::PositionCheck_Implementation() {}
```

## Build status

```
[1/5] Compile [x64] CropoutPlayerPawn.cpp
[2/5] Compile [x64] Module.CropoutSampleProject.gen.cpp
[3/5] Link [x64] UnrealEditor-CropoutSampleProject-0045.lib
[4/5] Link [x64] UnrealEditor-CropoutSampleProject-0045.dll
[5/5] WriteMetadata CropoutSampleProjectEditor.target
Result: Succeeded
Total execution time: 6.90 seconds
```
DLL tag now `-0045`. `.sln` regenerated, `Result: Succeeded`
(3.65s).

## Direct MCP delete sequence

```bash
for fn in Dof TrackMove PositionCheck; do
  mcp__ue-mcp__call_tool remove_function_graph \
    blueprint=/Game/Blueprint/Core/Player/BP_Player.BP_Player \
    graph_name=$fn
done
```

All 3 returned `returnValue: null` (success). `BeginBuild` was
skipped because `remove_function_graph` reported
`Graph BeginBuild does not exist` (it's an interface event slot,
not a free function graph).

## BP_Player compile outcome (post-delete)

```
compile_blueprint(BP_Player) → returnValue: null
```

`null` = BP compiled cleanly. **8 fatal issues cleared in R15
followed by 3 more cleared in R16 → 0 errors total.**

## BP_Player function list (post-R16)

`list_functions(BP_Player.BP_Player)` returns 18 entries, all
`bIsImplemented: true`:

```
UserConstructionScript
Input Switch
Update Build Asset
Update Cursor Position
Update Zoom
Move Tracking
Corners in Nav
Create Build Overlay
Destroy Spawn
Closest Hover Check
Get Player Controller
Update Path
Rotate Spawn
Overlap Check
Villager Select
Villager Release
Remove Resources
Spawn Build Target
```

These 18 are the BP-side function graphs that remain. The 3
BP-side graphs removed (`Dof/TrackMove/PositionCheck`) are not in
this list — they're now only on the C++ side as NativeEvent
stubs.

## Disk persistence

```
save_assets([BP_Player]) → returnValue: true
is_dirty(BP_Player)      → returnValue: false

md5sum BP_Player.uasset:
  before R16:  1f7055055499bea99aeafed064c50d60  (1702973 bytes)
  after  R16:  df254043e9146b8c3c0d66fe3da00c5d  (1538568 bytes,
                                                     -9.6% / -164 KB)
```

The 164 KB drop is the deleted BP-side function graphs (with
their nodes, pins, default values, etc.) being stripped from the
.uasset.

## BeginBuild caveat (worth flagging)

`BPI_Player::BeginBuild` is implemented by `BP_Player` via a
BP-side *event override* of the BPI interface event. That
override is **still on disk**. R16 confirms it was never a free
BP-defined function graph (the `remove_function_graph` error
message made that explicit), so it survived R15 / R16 untouched.

The C++-side contract is implicitly "do not interfere with the BP
side" — there's no C++ BeginBuild UFUNCTION, so BP_Player's BPI_Player
implementation compiles without conflict. The `BPI_Player::BeginBuild`
event also got the round-15 BP-class-chain re-link + new compiled
state, which is why the BP-side implementation now compiles
(previous compile failure was caused by the `Final` / `Static`
flags on the conflicting C++ UFUNCTION declarations; those flags
are now removed along with the declarations).

## Hot points (post-R16)

- **3 demoted functions**: `Dof/TrackMove/PositionCheck` are
  no-ops at runtime. Game animations that depended on depth-of-
  field are static. Edge-scroll movement has no movement. Build
  position validity check is always true. These are observable
  regressions in PIE / standalone build and should be addressed
  before next play-test.
- **Future R17 candidate**: migrate the 3 BP-side DSL bodies
  captured in earlier round-15 dumps:
  - `Dof()` → set camera post-process settings on SpringArm target
    length 150 mm, f-stop 3.0;
  - `TrackMove()` → deproject mouse to ground, advance actor via
    spring-arm forward/up vector math;
  - `PositionCheck()` → deproject mouse to ground, set `Collision`
    actor location based on `InputType` enum branch.
  Each goes into the matching `_Implementation()` body on the C++
  side; the BP graph then gets a single
  `Call Parent.Dof/TrackMove/PositionCheck` per callsite (or no
  callsite change if the BP graph was already empty — but it
  isn't).

## Files touched
- `Source/CropoutSampleProject/CropoutPlayerPawn.h` (3 UFUNCTIONs
  + 3 virtual re-added; BeginBuild permanently removed).
- `Source/CropoutSampleProject/CropoutPlayerPawn.cpp` (3
  `_Implementation` empty stubs).
- `Content/Blueprint/Core/Player/BP_Player.uasset` (3 BP-side
  function graphs removed via direct MCP; MD5 + size changed).
- `doc/round-reports/round-16-bp-player-cpp-authoritative.md`
  (this file).
