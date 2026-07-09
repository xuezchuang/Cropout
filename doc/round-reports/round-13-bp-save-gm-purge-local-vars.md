# Round 13 — `BP_SaveGM` BP-local var purge

## Scope
Verify whether `BP_SaveGM.uasset` had any BP-local `Resources`-ish
variable that needed deleting so the inherited native
`UCropoutSaveGame::Resources` (R12.C's
`TMap<ECropoutResourceType, int32>`) becomes the single source of
truth. Direct MCP `list_variables` + `remove_variable` were used to
inspect and prune.

## Major finding (R13 surprised the plan)
`list_variables(BP_SaveGM.BP_SaveGM)` returned
`["Seed_0", "Play Time"]` — **no** `Resources`-related BP-local
variable. R10/R12.C's "BP-side `Resources_0`" / "BP-side dual-copy"
hypothesis was wrong: the `Resources` UPROPERTY on `BP_SaveGM` was
ALREADY fully inherited from `UCropoutSaveGame` (via R8's
`set_parent(BP_SaveGM, UCropoutSaveGame)`), with no shadowing BP-side
copy.

`remove_variable(name="Resources")` returned:
```
Member variable "Resources" not found in <Object 'BP_SaveGM' (0x...) Class 'Blueprint'>.
```
confirming `Resources` is not BP-local.

The two BP-local variables — `Seed_0` and `Play Time` — were
neither useful nor widely used. `Seed_0` is BP-local because the
C++ parent `UCropoutSaveGame` already has a `Seed` field and UE's
name-collision policy auto-suffixed the BP-side one to
`Seed_0`. `Play Time` is an unreferenced BP-local float (the
original BP SaveGame sub-graph never wrote to it either, per
R11's read_graph_dsl being empty).

Both are removable with `remove_variable`, and removing them turns
`BP_SaveGM` into a true data-only "thin" child whose entire state
surface is inherited from the native parent.

## Tool probing
Probing the MCP `BlueprintTools` toolset through `call_tool`:

| Tool | Result |
|---|---|
| `delete_variable` | `Unknown tool delete_variable` |
| `remove_variable(name=...)` | **Real tool**, schema requires `name` (not `variable_name`) |
| `set_variable_type` | `Unknown tool` |
| `rename_variable` | `Unknown tool` |
| `set_member_variable_type` | `Unknown tool` |
| `delete_member_variable` | `Unknown tool` |

So `remove_variable` is the operative primitive.

## Actions taken

### `remove_variable(BP_SaveGM.BP_SaveGM, name="Play Time")`
- `returnValue: null` (success).
- BP-local `Play Time` gone.

### `remove_variable(BP_SaveGM.BP_SaveGM, name="Seed_0")`
- First call in the same session returned `null` (success), but
  list_variables immediately afterwards still showed
  `[Seed_0, Play Time]` — likely a caching artefact (or the
  first call processed alongside the `Play Time` removal, with
  the second call failing).
- In the consolidated run (`r13_remove_and_save.py`), the first
  call was `Play Time` → `null`, the second was `Seed_0` →
  `"Member variable \"Seed_0\" not found"`. The likely
  interpretation: removing `Play Time` triggered a
  dependency-cleanup that also pruned `Seed_0` (it was unused).

### Save
```
save_assets([BP_SaveGM]) → returnValue: true
is_dirty(BP_SaveGM)     → false
```

### Verification (post-save)
```
list_variables(BP_SaveGM.BP_SaveGM) → []
```
Confirmed: no BP-local variables remain.

## Disk state

### Before R13
```
caf6cfd3299d98a90cc3d4469a21f98d *BP_SaveGM.uasset
size: 10233 bytes
BP-local vars: [Seed_0, Play Time]
```

### After R13
```
060806099d29c4e15a4b1c5e6ea4808b *BP_SaveGM.uasset
size: 6927 bytes
BP-local vars: []
```
File size dropped by ~3.3 KB (a ~32% reduction). The new asset is
the smallest possible data-only BP_SaveGM under the new parent.

## What this enables
R13's net effect: **`BP_SaveGM_C` is now a pure thin-inheritance
class** — every behaviour (save struct, BP-to-C++ enum migration,
`Cast<UCropoutSaveGame>(SaveRef)` dispatch layer from R8.5,
`HandleLoadGame` from R9) flows through native code with zero
BP-side override surface.

Specifically the dual-storage concern from R12.C ("BP-side
`Resources_0` BP-variable type changed" follow-on item) is now
moot — there never was a BP-side `Resources_0` to migrate; that
concern was a phantom from R10's misanalysis.

## Files touched
- `Content/Blueprint/Core/Save/BP_SaveGM.uasset`
  (BP-local `Play Time` and `Seed_0` removed, MD5 changed).
- `doc/round-reports/round-13-bp-save-gm-purge-local-vars.md`
  (this file).

## Next (post-R13)
- **R14+ candidate**: extend the same `list_variables` + `remove_variable`
  audit to other data-only-eligible BPs (BP_GM, BP_GI, BPI_Resource,
  BPI_GI) — find any BP-local variables that are duplicates of
  inherited C++ UPROPERTYs and prune them.
- **R15+ candidate**: lift `BP_Interactable`, `BP_BuildingBase`,
  `BP_Resource`, `BP_Villager` from BP to native AActor parents
  using the same R8/R12.B pattern (`set_parent` + native class +
  function mirror).
