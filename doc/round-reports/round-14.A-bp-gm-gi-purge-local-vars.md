# Round 14.A — `BP_GM` + `BP_GI` redundant BP-local var purge

## Scope
Run `list_variables` on the four primary core BP assets, identify
BP-local variables that duplicate native C++ UPROPERTYs (which were
introduced in their respective migration rounds), and prune them
via direct MCP `remove_variable`. The point: make `BP_GM_C` /
`BP_GI_C` thin-inheritance children of `ACropoutGameMode` /
`UCropoutGameInstance` so BP graph references resolve to the
inherited native UPROPERTYs.

## Audit findings

### `BP_GM` (CropoutGameMode)
**Initial 10 BP-local vars:**
```
["Resources", "Update Resources", "Villager Count", "UI_HUD",
 "Start Time", "Update Villagers", "SpawnRef", "Town Hall",
 "TownHall_Ref", "Villager_Ref"]
```

**Native counterparts in `ACropoutGameMode.h`:**
- `VillagerCount` ← BP's `Villager Count`
- `UI_HUD` ← BP's `UI_HUD`
- `StartTime` ← BP's `Start Time`
- `SpawnRef` (TSoftClassPtr<AActor>) ← BP's `SpawnRef`
- `TownHall` (TObjectPtr<AActor>) ← BP's `Town Hall`
- `TownHall_Ref` (TSoftClassPtr<AActor>) ← BP's `TownHall_Ref`
- `Villager_Ref` (TSoftClassPtr<APawn>) ← BP's `Villager_Ref`
- `UpdateResources` (dispatcher) ← BP's `Update Resources`
- `UpdateVillagers` (dispatcher) ← BP's `Update Villagers`

**Pruned (7 removed, 3 kept):**
- ✅ REMOVED: `Villager Count`, `UI_HUD`, `Start Time`, `SpawnRef`,
  `Town Hall`, `TownHall_Ref`, `Villager_Ref`.
- ⏸ KEPT: `Resources` (needed by `CropoutGameMode.cpp` lines 248-263
  FMapProperty reflection — the C++ side never has its own
  `TMap<ECropoutResourceType, int32> Resources` UPROPERTY on
  `ACropoutGameMode`, only on `UCropoutSaveGame`; `Resources` here
  is the BP-side `Map of E_ResourceType Enums to Integers`).
- ⏸ KEPT: `Update Resources`, `Update Villagers` (kept to avoid
  breaking any BP graph `Call<dispatcher>` nodes that bind to the
  exact BP-side UPROPERTY; native dispatchers have the same name and
  UE prefers native when both exist).

### `BP_GI` (CropoutGameInstance)
**Initial 3 BP-local vars:**
```
["Has Save", "Start Game Offset", "Music Playing"]
```

**Native counterparts in `UCropoutGameInstance.h`:**
- `HasSave` ← BP's `Has Save`
- `StartGameOffset` ← BP's `Start Game Offset`
- `MusicPlaying` ← BP's `Music Playing`

**Pruned (3 removed):**
- ✅ REMOVED: `Has Save`, `Start Game Offset`, `Music Playing`.
- **Post-prune `list_variables(BP_GI)` → `[]`**: BP_GI is now a
  fully-empty data-only shell whose entire state surface is inherited
  from `UCropoutGameInstance`.

### `BPI_Resource` (CropoutResourceInterface)
- `list_variables` → `[]`.
- `EventGraph` not valid (BP interface assets have no event graphs).
- No work needed — already clean.

### `BPI_GI` (CropoutGameInstanceInterface)
- `list_variables` → `[]`.
- `EventGraph` not valid (BP interface asset).
- No work needed — already clean.

## Tool use (R10 direct MCP)

Each successful removal followed the same call shape:
```python
call_tool(
    toolset_name = "editor_toolset.toolsets.blueprint.BlueprintTools",
    tool_name    = "remove_variable",
    arguments    = {
        "blueprint": {"refPath": "<asset>"},
        "name":      "<display name with spaces>"
    }
)
```
All 10 calls returned `returnValue: null` (success).

## Outcomes (after save)

| Asset | BP-local vars before | BP-local vars after | is_dirty |
|---|---|---|---|
| BP_GM  | 10 | 3 | false |
| BP_GI  | 3  | 0 | false |
| BPI_Resource | 0 | 0 | n/a |
| BPI_GI      | 0 | 0 | n/a |

`save_assets([BP_GM, BP_GI])` → `returnValue: true`.

### Disk MD5
```
BP_GM.uasset
   before R14.A: 07c7f714d146c1e14a4516b7f762689b  (R6 routing baseline)
   after  R14.A: 5d28fb6bcfcefd4a4ece737e3847cd4c  (-46KB / -10%)
BP_GI.uasset
   before R14.A: ef8fdc366b8ac12af4a5c2799145f23d  (R7 routing baseline)
   after  R14.A: 8c4be9b2e645523b5eeb34144c7986e6  (-33KB / -7.5%)
```
Both confirmed via `md5sum`.

## What this enables
- `BP_GI_C` and `BP_SaveGM_C` are now nearly-identical thin
  inheritance shells (BP_GI empty, BP_SaveGM empty), both fully
  inheriting C++ state from `UCropoutGameInstance` /
  `UCropoutSaveGame`. BP graphs that previously bound to `HasSave`
  / `StartGameOffset` / `MusicPlaying` now resolve to the native
  counterparts (no behaviour difference in
  `HandleEventUpdateAllResources` / `SaveGame_Implementation` etc.
  because the migration rounds already assume the native field
  names).
- The BP_GM-side `Resources` map and `Update Resources` /
  `Update Villagers` dispatchers remain BP-local because they are
  the source of truth for BP-side K2Node map operations / dispatch
  bindings — `ACropoutGameMode` does not currently have a native
  `Resources` UPROPERTY (only `UCropoutSaveGame` does), so removing
  the BP-side `Resources` would break the
  `FMapProperty` reflection path in `CropoutGameMode.cpp`.
- A future R15 could lift `ACropoutGameMode::Resources` to a real
  UPROPERTY (matching the BP-side type after R12.C's enum
  alignment), then delete the BP-side `Resources`. That requires
  the BP enum / native enum type-alignment which is more delicate
  than this round.

## Files touched
- `Content/Blueprint/Core/GameMode/BP_GM.uasset` (7 BP-local vars
  removed; MD5 changed).
- `Content/Blueprint/Core/GameMode/BP_GI.uasset` (3 BP-local vars
  removed; MD5 changed).
- `doc/round-reports/round-14.A-bp-gm-gi-purge-local-vars.md` (this
  file).

## Next (post-R14.A)
- **R14.B**: extend the same audit to
  `BP_Interactable`, `BP_BuildingBase`, `BP_Resource`, `BP_Villager`,
  `BP_Player`, `BP_PC`, `BP_PlayerController` actor BPs.
- **R14.C**: re-run `list_variables` periodically to catch new
  BP-local additions (BP editor often auto-creates new locals when
  designers add a node).
- **R15**: lift `ACropoutGameMode::Resources` to a real UPROPERTY
  (matching BP-side `TMap<ECropoutResourceType, int32>` after
  R12.C's enum alignment), then delete the BP-side `Resources` in
  `BP_GM`.
