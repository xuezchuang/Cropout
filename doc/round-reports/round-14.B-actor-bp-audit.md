# Round 14.B — Actor BP `list_variables` audit + lift-path map

## Scope
Apply the R14.A `list_variables` audit to the six non-savegame
actor Blueprints in the project, decide which BP-local vars can be
pruned (because they have native counterparts on the parent C++
class), and lay out the per-BP lift roadmap for the next round.

## Audit findings

### `BP_Interactable` (Interactable)
7 BP-local vars:
```python
["BoundGap", "Require Build", "RT_Draw", "OutlineDraw",
 "Enable Ground Blend", "Progression State", "Mesh List"]
```
- **Has native parent?** No (`BP_Interactable` is currently a
  pure-data BP; no `ACropoutInteractable` parent class exists).
- **R14.B action:** none. Removing any of these would orphan the
  BP graph's K2Node_VariableGet/Set references with no fallback.
- **Lift path:** R15.B candidate — create `ACropoutInteractable`
  (UCLASS Blueprintable : AActor) and mirror these 7 fields plus
  the `GetProgressionState` UFUNCTION referenced from
  `CropoutGameInstance.cpp`'s `HandleEventUpdateAllInteractables`.

### `BP_BuildingBase` (Interactable/Building)
2 BP-local vars:
```python
["Current Stage", "Build Difficulty"]
```
- **Has native parent?** No.
- **R14.B action:** none.
- **Lift path:** R16.B candidate.

### `BP_Resource` (Interactable/Resources)
5 BP-local vars:
```python
["Resource Type", "Resource Amount", "Collection Time",
 "CollectionValue", "Use Random Mesh"]
```
- **Has native parent?** No.
- **R14.B action:** none.
- **Lift path:** R17.B candidate — natural alignment with the
  round 12.C `ECropoutResourceType` UENUM (`Resource Type` field
  is a perfect mirror target).

### `BP_Villager` (Villagers)
7 BP-local vars:
```python
["ResourcesHeld", "Quantity", "Target Ref", "New Job",
 "Active  Behavior", "Work Anim", "Target Tool"]
```
- **Has native parent?** No.
- **R14.B action:** none.
- **Lift path:** R18.B candidate — `APawn`-derived
  `ACropoutVillager` with the AI / job controller / inventory
  fields mirrored.

### `BP_Player` (Player) — special case
39 BP-local vars (truncated for size):
```python
["ZoomCurve", "ZoomValue", "Selected", "InputType", "Spawn",
 "StepRotation", "Can Drop", "Post Process Settings", "Target",
 "Aperture (F-stop)", "Sensor Width (Mm)", "Focal Distance",
 "TH_Hover", "HoverActor", "TargetHandle", "Zoom", "ZoomTimeCheck",
 "Target Zoom", "ZoomDirection", "Stored Move", "ZoomHandle",
 "TrackHandle", "Limit", "UI_HUD", "Focus length", "BuildMod",
 "Zoom Compare", "DragAxis", "Edge Move Distance",
 "Target Spawn Class", "MID_Placeable", "SpawnOverlay", "NS_Path",
 "Path Points", "Block Drag", "Move Spawn", "Resource Cost",
 "VillagerTarget", "Hover Actor"]
```
- **Has native parent?** Yes — `UCropoutPlayerPawn` from round 1
  (R2A — `Source/CropoutSampleProject/CropoutPlayerPawn.h`).
  That native class currently exposes **only** the 12
  `Enhanced Input` asset-reference UPROPERTYs (IA_/IMC_*). None of
  the 39 BP-local vars are Enhanced-Input asset references.
- **R14.B action:** none. Removing any of the 39 would orphan
  BP graph K2Node references for fields that don't yet have a
  native counterpart.
- **Lift path:** R19.B candidate — lift the gameplay-side vars
  (Zoom / Target / UI / Spawn overlay / Path Points / etc.) into
  `UCropoutPlayerPawn` so the BP graph can be slimmed down.

### `BP_PC` (Player) — special case
3 BP-local vars:
```python
["InputType", "KeySwitch", "Cursor Widget"]
```
- **Has native parent?** Yes — `UCropoutPlayerController` from
  round 1 (R2A scaffold, currently empty body — see
  `CropoutPlayerController.h` header note).
- **R14.B action:** none.
- **Lift path:** R20.B candidate — add 3 UPROPERTYs to
  `UCropoutPlayerController` and migrate the BP graph to
  inherited versions.

### `BPI_Villager` (Villagers) — interface
`list_variables` ⇒ `[]` (BPs of interface type have no local
state). Already clean.

## Total BP-local vars across the audit

| BP | Var count |
|---|---|
| BP_Interactable | 7 |
| BP_BuildingBase | 2 |
| BP_Resource | 5 |
| BP_Villager | 7 |
| BP_Player | 39 |
| BP_PC | 3 |
| **Total** | **63** |

## Decision: no removals in R14.B

The R14.A rationale (remove only BP-local vars that have an
inherited native UPROPERTY of the same name and type) does not
apply to any of the 63 actor-BP vars in this round: 60 of them
have no native counterpart yet, and 3 (the BP_PC trio) only have
a C++ parent class that is itself an empty R2A scaffold.

Performing `remove_variable` on any of these would (a) leave the
BP graph with K2Node_VariableGet/Set dangling, and (b) invalidate
runtime behaviour for which the BP-side vars are the only source
of truth until the corresponding native fields are added and the
BP graph is redirected (R12.B pattern).

The right unblocking step is to extend the lift pipeline actor by
actor (R15.B, R16.B, R17.B, R18.B, R19.B, R20.B), each of which
will produce a native parent class with the corresponding 1:1
mirrored UPROPERTYs at which point the BP-local vars become
removable in a follow-on `remove_variable` pass.

## What R14.B delivered

- **63-var enumeration** of the 6 main actor BPs, written to
  `C:/Temp/r14B_vars.json` (one MCP-call-per-BP list_variables
  each, ~14s/server-round-trip, total ~90s wall-clock).
- Per-BP lift-path notes mapping each var to a future native class
  migration round.
- Disk state preserved (no `remove_variable` issued, so MD5 of all
  six assets unchanged):
  ```
  BP_Interactable.uasset  494035fc8f09bc9327b3076451971f0f
  BP_BuildingBase.uasset  028bea06a2373c39478e882a90d25ebb
  BP_Resource.uasset      09807ad6e0dbf0c6373ca33419b166b1
  BP_Villager.uasset      f2a49eadc6f7979d0e223c8888ae646e
  BP_Player.uasset        ef9ed137690f30b69c55ed44bda0d1f5
  BP_PC.uasset            71ea9fc8b70fee2336159eb874a00964
  ```
- No `save_assets` issued (no in-memory edits); `is_dirty` would
  return `false` for all 6 if probed.

## Future-round pipeline suggested by R14.B

| Round | Native class | BP-local vars it would subsume |
|---|---|---|
| R15.B | `ACropoutInteractable` | 7 (BoundGap / Require Build / RT_Draw / OutlineDraw / Enable Ground Blend / Progression State / Mesh List) |
| R16.B | `ACropoutBuildingBase` | 2 (Current Stage / Build Difficulty) |
| R17.B | `ACropoutResource` | 5 (Resource Type / Resource Amount / Collection Time / CollectionValue / Use Random Mesh) |
| R18.B | `ACropoutVillager` | 7 (ResourcesHeld / Quantity / Target Ref / New Job / Active Behavior / Work Anim / Target Tool) |
| R19.B | `UCropoutPlayerPawn` *filled* | 39 (full Zoom/Target/UI/Spawn overlay/path-points/villager-target set) |
| R20.B | `UCropoutPlayerController` *filled* | 3 (InputType / KeySwitch / Cursor Widget) |

Each round uses the R12.B pipeline:
1. `read_graph_dsl(<asset>:EventGraph)` to confirm the BP graph
   shape.
2. `get_connected_subgraph(K2Node_FunctionEntry_N)` per BP-defined
   function to enumerate user-defined functions and their
   signatures.
3. Write `UCropoutXxx.h/.cpp` with native UPROPERTYs +
   `UFUNCTION`s mirroring each BP-defined function, with
   `ProcessEvent` reflection as the round-12.B-compatible fallback
   so BP graphs continue to work.
4. `UBT` build (DLL tag bumps).
5. Once native fields exist, BP-local vars can be deleted (R14.B
   follow-on sweep).

## Files touched
- `doc/round-reports/round-14.B-actor-bp-audit.md` (this file).
- `C:/Temp/r14B_vars.json` (machine-readable audit, not in repo).
- `C:/Temp/r14B_probe.py` + `C:/Temp/r14B_summary.py` (probing
  scripts, not in repo).
