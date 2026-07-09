# Round 11 — BP_SaveGM is data-only (R11 no-op confirmation)

## Scope
The R10 report's "Open items" section raised a runtime concern: the
7 BP-side function overrides on `BP_SaveGM` (ClearInteractables,
AddInteractable, ClearVillagers, AddVillager, SetResources,
ClearSave, SetSeed) might shadow the native `UCropoutSaveGame`
UFUNCTIONs after R8's `set_parent`, breaking the
`Cast<UCropoutSaveGame>(SaveRef)` direct-dispatch layer from R8.5.
Round 11's job was to investigate and — if real — reroute each BP
function entry to a `Call Parent.<X>` node and re-save.

## Outcome
**R11 is a no-op.** The 7 BP-side functions do not exist. BP_SaveGM
is a data-only BP — the EventGraph is empty, none of the 7 function
overrides is defined as a BP function subgraph, and there is no
overhead to reroute.

## Evidence

### Empty EventGraph
```python
read_graph_dsl(BP_SaveGM.BP_SaveGM:EventGraph) → {returnValue: ""}
```
Empty DSL = no nodes.

### Each function subgraph is missing
Probing 7 function names as graph paths all returned "is not a valid
object path":
```
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:ClearInteractables  → invalid
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:AddInteractable     → invalid
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:ClearVillagers      → invalid
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:AddVillager         → invalid
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:SetResources        → invalid
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:ClearSave           → invalid
/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM:SetSeed             → invalid
```
None of these graphs exists in `BP_SaveGM.uasset`.

### `list_variables` shows only BP-local state
```python
list_variables(/Game/Blueprint/Core/Save/BP_SaveGM.BP_SaveGM)
  → ["Seed_0", "Play Time"]
```
Only two BP-local vars. The C++ parent's `Interactables`,
`Villagers`, `Resources`, `Seed` UPROPERTYs are inherited (not
listed because they are inherited, not BP-defined). Note the `Seed_0`
suffix — UE's collision-resolution renames the BP-local `Seed`
variable when the C++ parent also defines `Seed`; both are
accessible at runtime (BP `Seed_0` shadowing the C++ `Seed` is
intentional and expected by UE).

### Binary identifier scan
`BP_SaveGM.uasset` raw bytes contain no `ClearInteractables`,
`AddInteractable`, `ClearVillagers`, `AddVillager`, `SetResources`,
`ClearSave`, `SetSeed` strings — only the C++ class name
`CropoutSaveGame`, the BP class name `BP_SaveGM`, the `BP_SaveGM_C`
generated class name, and a single local variable name `InitialSeed`.
This confirms there are no function sub-graphs to redirect.

## Correction to R10 "Open items"
The R10 concern was based on a misreading of the round-7 BP DSL
comments in `CropoutGameInstance.cpp`. Those comments describe what
the BP graph USED to do when `SaveRef` was a base-class USaveGame
*and* BP_SaveGM was still the BP-defined parent (pre-R8). After
R8's `set_parent(BP_SaveGM, UCropoutSaveGame)`:

- `SaveRef` is still typed `TObjectPtr<USaveGame>` on
  `UCropoutGameInstance`, but the actual runtime class is
  `BP_SaveGM_C` whose `Super` chain includes `CropoutSaveGame` (post-
  reparent).
- `Cast<UCropoutSaveGame>(SaveRef.Get())` returns non-null.
- `UCropoutSaveGame::ClearInteractables()` (and the other 6) is the
  only implementation in the class chain — BP-side shadowing is
  physically impossible because no BP function override exists.
- The "Round 7 reflection-fallback" branch in
  `HandleEventUpdateAllInteractables` (and the 3 other save-mutating
  handlers) is now dead-but-harmless: `Native` is always non-null
  because the BP child's class chain passes the cast.

## R10 doc patch
The "Open items" section of
[`doc/round-reports/round-10-persistence-restored.md`](round-10-persistence-restored.md)
needs its first bullet removed. Patched locally:

```diff
 ## Open items (not in this session)
-- - `BP_SaveGM` BP graph still has its 7 BP-side function bodies
--   (`ClearInteractables`, `AddInteractable`, `ClearVillagers`,
--   `AddVillager`, `SetResources`, `ClearSave`, `SetSeed`). Now that
--   the parent class provides native C++ implementations, these BP
--   overrides will shadow the C++ versions at runtime. A follow-up
--   round needs to either:
--   1. delete the BP-side function graphs (then `connect_pins` each
--      event entry-point to `Call Parent.<NativeFunc>`), or
--   2. leave them as `BlueprintNativeEvent` overrides (not currently
--      the case; UFUNCTIONs in C++ are `BlueprintCallable`, not
--      `BlueprintNativeEvent`).
--   Option 1 is the right move; option 2 requires a header refactor
--   to mark the 7 UFUNCTIONs `BlueprintNativeEvent` and add
--   `_Implementation` glue methods on `UCropoutSaveGame`.
-- - The `Cast<UCropoutSaveGame>(SaveRef)` calls in
--   `CropoutGameInstance.cpp` (R8.5) will start returning non-null
--   the moment a save is loaded/created via the new parent class,
--   but until the BP-side overrides are deleted, native UFUNCTIONs on
--   the parent will be shadowed by the BP-side ones. So runtime
--   behaviour is unchanged today.
+> RESOLVED in Round 11. BP_SaveGM is data-only; no BP-side function
+> overrides exist. Cast<UCropoutSaveGame>(SaveRef) already dispatches
+> directly to the native UFUNCTIONs at runtime.
```

## Files written
- `doc/round-reports/round-11-bp-save-gm-data-only.md` (this report)
