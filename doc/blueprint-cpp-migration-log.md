# Blueprint to C++ Migration Log

This log tracks incremental Blueprint-to-C++ migration work for the Cropout
sample. Current source and assets are authoritative; entries here are only
working notes for continuity.

## 2026-07-08

### Baseline

- Goal: migrate Blueprint logic into C++ parents and remove Blueprint
  function/event graph implementations as their C++ equivalents become
  authoritative.
- Current configured runtime assets:
  - `Config/DefaultEngine.ini` uses `BP_GI.BP_GI_C` as `GameInstanceClass`.
  - `Config/DefaultEngine.ini` uses `BP_GM.BP_GM_C` as `GlobalDefaultGameMode`.
- Already reparented to project C++ parents:
  - `BP_GI` -> `UCropoutGameInstance`
  - `BP_GM` -> `ACropoutGameMode`
  - `BP_Player` -> `ACropoutPlayerPawn`
  - `BP_PC` -> `ACropoutPlayerController`
  - `BP_SaveGM` -> `UCropoutSaveGame`
- `BP_GI` disk state compiles after removing old `BPI_GI` save-interface
  implementation graphs. Remaining graphs are `EventGraph` and
  `Island Seed`.
- Large remaining migration areas:
  - `BP_GM` residual graphs and call-site fallout.
  - `BP_Player` residual gameplay/input/build graphs.
  - `BP_Villager`, BT tasks, EQS contexts, and villager interfaces.
  - `BP_Interactable`, `BP_Resource`, crops, buildings, and resource
    interfaces.
  - UI widgets still calling old BP_GI/BP_GM function or dispatcher names.
  - BP function libraries and input modifier Blueprints.

### Current Broken Compile Surface

- `/Game/Blueprint/Core/GameMode/BP_GI1.BP_GI1` is an unreferenced-looking
  old GI duplicate with stale save/resource graphs.
- `/Game/Blueprint/Core/MainMenu/BP_MainMenuGM.BP_MainMenuGM` still calls old
  BP_GI nodes such as `Play Music` and `TransitionOut`.
- `/Game/Blueprint/Interactable/Building/BPC_EndGame.BPC_EndGame` still calls
  old BP_GM node `End Game`.
- `/Game/Blueprint/Interactable/Building/BPC_House.BPC_House` still calls old
  BP_GM node `Spawn Villagers`.
- UI assets surfaced by dependent compiles still call old BP_GI/BP_GM nodes or
  dispatcher names, including `Open Level`, `Stop Music`, `Update Resources`,
  and `Update Villagers`.

### Next Work

- Prefer updating caller nodes to native C++ UFUNCTIONs/dispatchers over adding
  compatibility Blueprint graphs.
- If a caller graph is too tightly coupled to migrate safely in one pass, move
  that caller's behavior into a C++ parent first, then remove the BP graph.

### Current Audit Snapshot

- Audit command: `UnrealEditor-Cmd.exe CropoutSampleProject.uproject
  -run=pythonscript -script=Saved/CodexBlueprintMigrationAudit.py
  -unattended -nop4 -NoSplash -NullRHI`.
- Scope scanned: Blueprint assets under `/Game/Blueprint` and `/Game/UI`.
- Result:
  - 71 Blueprint/Widget Blueprint assets scanned.
  - 6 assets currently use project C++ parents.
  - 65 assets still use engine/Blueprint parents.
  - 71 assets still have at least one graph; 32 have non-trivial
    function/interface graphs.
  - 60 assets compiled cleanly in this scan.
  - 11 assets are currently `BS_ERROR`.
- Already project-native parents, but not graph-free:
  - `BP_GI`: 2 graphs (`EventGraph`, `Island Seed`).
  - `BP_GI1`: 7 graphs; appears to be a stale duplicate and still fails
    compile.
  - `BP_GM`: 7 graphs (`UserConstructionScript`, `EventGraph`,
    `Island Seed`, resource functions).
  - `BP_PC`: 3 graphs.
  - `BP_Player`: 28 graphs; this is still a major migration block.
  - `BP_SaveGM`: 1 graph (`EventGraph`).
- Largest remaining non-native gameplay blocks:
  - `BP_Villager`: parent `Pawn`, 12 graphs / 10 implemented functions.
  - `BP_Resource`: parent `BP_Interactable`, 9 graphs / 7 implemented
    functions.
  - `BP_BaseCrop`: parent `BP_Resource`, 7 graphs / 5 implemented
    functions.
  - `BP_Interactable`: parent `Actor`, 6 graphs / 4 implemented functions.
  - `BP_BuildingBase`: parent `BP_Interactable`, 6 graphs / 4 implemented
    functions.
  - Blueprint interfaces/function libraries still exist:
    `BPI_GI`, `BPI_Resource`, `BPI_Player`, `BPI_Villager`,
    `BPF_Cropout`, `BPF_Shared`.
- Current compile failures:
  - `BP_GI1`
  - `BP_MainMenuGM`
  - `BPC_EndGame`
  - `BPC_House`
  - `CUI_BuildItem`
  - `UI_Layer_Game`
  - `UI_MainMenu`
  - `UIE_Resource`
  - `UIE_Slider`
  - `UI_EndGame`
  - `UI_Pause`
- Main failure categories:
  - Old BP function-call node display/internal names no longer match native
    functions: `Play Music`, `Open Level`, `Stop Music`, `End Game`,
    `Spawn Villagers`.
  - Old BP dispatcher bindings still look for spaced dispatcher names:
    `Update Resources`, `Update Villagers`.
  - Old `BPI_GI` interface pins remain in UI assets after `BP_GI` moved to
    the native `ICropoutGameInstanceInterface` route.
  - `UIE_Slider` has an unrelated-looking stale pin mismatch between
    `Float` and `Sound Mix Object Reference`.

### Refresh After Main Menu GM C++ Parent

- Audit command rerun at local 2026-07-08 09:21 using
  `Saved/CodexBlueprintMigrationAudit.py`.
- C++ build check:
  `Build.bat CropoutSampleProjectEditor Win64 Development -Project=...`
  succeeded; target was up to date.
- Visual Studio project files were regenerated after adding
  `ACropoutMainMenuGameMode`; `Intermediate/ProjectFiles` now includes
  `CropoutMainMenuGameMode.cpp/.h`.
- Current scan result:
  - 71 Blueprint/Widget Blueprint assets scanned.
  - 7 assets now use project C++ parents.
  - 64 assets still use engine/Blueprint parents.
  - 71 assets still have at least one graph.
  - 32 assets still have non-trivial function/interface graphs.
  - 11 assets are still `BS_ERROR`.
- `BP_MainMenuGM` current disk state:
  - Parent is now `/Script/CropoutSampleProject.CropoutMainMenuGameMode`.
  - Graphs still present: `UserConstructionScript`, `EventGraph`.
  - Compile still fails because the old `EventGraph` still contains stale
    BP_GI calls such as `Play Music`.
  - Prior commandlet deletion of `EventGraph` compiled in memory but failed
    to save because the live editor held the asset file open. Use the live
    editor/MCP route or close the asset/editor before rerunning the cleanup
    script.

### Main Menu GM Cleanup Complete

- `Saved/CodexMigrateMainMenuGM.py` rerun after `BP_MainMenuGM` was no longer
  open in the asset editor.
- Result JSON: `Saved/codex_migrate_main_menu_gm.json`.
- Disk result:
  - `BP_MainMenuGM` parent:
    `/Script/CropoutSampleProject.CropoutMainMenuGameMode`.
  - Remaining graphs: `UserConstructionScript` only.
  - Fresh commandlet compile status: `BS_UP_TO_DATE`.
  - Full audit compile failure count dropped from 11 to 10.
- Live editor caveat:
  - The already-running UnrealEditor process may still cache the old
    `BP_MainMenuGM` object. Do not save that stale live copy over the disk
    file.

### BP_GI1 Removal Check

- `BP_GI1` appears to be an unused duplicate of `BP_GI`.
- Evidence:
  - AssetTools referencers for `/Game/Blueprint/Core/GameMode/BP_GI1`: none.
  - Text search found no runtime/config/source references; hits were only
    under `Saved/` logs, MRU state, source-control bookkeeping, and audit JSON.
- Backup made:
  - `C:/Temp/BP_GI1_before_delete_20260708_092702.uasset`.
- Deletion is not yet landed:
  - MCP `AssetTools.delete` returned true, but the file stayed on disk.
  - Moving the file out of `Content/` failed with Windows Error 32 because
    the live UnrealEditor process still holds the file.
  - Finish this after closing/restarting the editor.

### BP_GM Graph Cleanup Prepared

- `BP_GM` still has C++ parent `/Script/CropoutSampleProject.CropoutGameMode`.
- Residual graphs read through MCP:
  - `EventGraph`: empty DSL.
  - `Island Seed`: already covered by `ACropoutGameMode::IslandSeed`.
  - `Remove Resource`, `Get Current Resources`, `Remove Target Resource`,
    `Check Resource`: already covered by `ICropoutResourceInterface`
    implementations on `ACropoutGameMode`.
- Cleanup script added:
  - `Saved/CodexCleanBPGMGraphs.py`.
- Script result:
  - In-memory remove and compile succeeded.
  - Save failed with Windows Error 32 because the live UnrealEditor process
    holds `BP_GM.uasset`.
  - Backup made:
    `C:/Temp/BP_GM_before_graph_cleanup_20260708_093015.uasset`.

### Function Library Native Coverage

- `UCropoutBlueprintLibrary` now covers the remaining simple helper graphs:
  - `BPF_Cropout.Stepped Position`.
  - `BPF_Shared.Convert To Stepped Pos`.
- Both functions snap X/Y to the 200-unit grid and return Z=0, matching the
  observed Blueprint DSL shape.
- C++ build after this change succeeded:
  `Build.bat CropoutSampleProjectEditor Win64 Development -Project=...`.

### Current Remaining Work Check

- Fresh audit command rerun at local 2026-07-08 09:41 using
  `Saved/CodexBlueprintMigrationAudit.py`.
- Disk audit result:
  - 71 Blueprint/Widget Blueprint assets scanned.
  - 7 assets currently use project C++ parents.
  - 64 assets still use engine or Blueprint parents.
  - 71 assets still have at least one graph.
  - 32 assets still have non-trivial function/interface graphs.
  - 10 assets are currently `BS_ERROR`.
- Assets already covered or prepared in C++ but not fully cleaned on disk:
  - `BP_GM`: C++ parent exists and residual graph logic is covered, but graph
    cleanup has not saved because live `UnrealEditor.exe` holds the asset.
  - `BP_GI1`: appears unused and has a backup, but deletion has not landed
    because live `UnrealEditor.exe` holds the asset.
  - `IM_Normalize` and `IM_Offset`: native C++ modifier classes exist and the
    editor commandlet reparent/delete pass worked in memory, but both assets
    reported `saved: false`; disk and live MCP still show the old
    `InputModifier` parents and `ModifyRaw` graphs.
  - `BP_MainMenuGM`: disk audit shows only `UserConstructionScript`, but the
    currently running editor/MCP still has a stale old `EventGraph` cached.
    Do not save that live copy.
- Current compile failures:
  - `BP_GI1`
  - `BPC_EndGame`
  - `BPC_House`
  - `CUI_BuildItem`
  - `UI_Layer_Game`
  - `UI_MainMenu`
  - `UIE_Resource`
  - `UIE_Slider`
  - `UI_EndGame`
  - `UI_Pause`
- Largest remaining C++ migration blocks by implemented graph count:
  - `BP_Player`
  - `BP_Villager`
  - `BP_Resource`
  - `BP_BaseCrop`
  - `BP_Interactable`
  - `BP_BuildingBase`
  - Blueprint interfaces/function libraries: `BPI_GI`, `BPI_Resource`,
    `BPI_Player`, `BPI_Villager`, `BPF_Cropout`, `BPF_Shared`.

### Input Modifier Cleanup and BP_GI1 Delete

- Used the live editor MCP route instead of a commandlet so the running
  `UnrealEditor.exe` process could write its own loaded assets.
- `IM_Normalize`:
  - Reparented to `/Script/CropoutSampleProject.CropoutInputModifierNormalize`.
  - Removed the Blueprint `ModifyRaw` graph.
  - Removed the leftover Blueprint variable `Normalize Range`.
  - Compiled and saved only `/Game/Blueprint/Core/Player/Input/IM_Normalize`.
- `IM_Offset`:
  - Reparented to `/Script/CropoutSampleProject.CropoutInputModifierOffset`.
  - Removed the Blueprint `ModifyRaw` graph.
  - Removed the leftover Blueprint variable `Offset_0`.
  - Compiled and saved only `/Game/Blueprint/Core/Player/Input/IM_Offset`.
- Both assets now retain only an empty `EventGraph` shell. The function
  implementation count is zero, so runtime modifier logic is now native C++.
- `BP_GI1`:
  - Rechecked referencers through `AssetTools.get_referencers`; result was
    still empty.
  - Deleted via live editor `AssetTools.delete`.
  - Verified both AssetRegistry and disk no longer contain
    `Content/Blueprint/Core/GameMode/BP_GI1.uasset`.
- `BP_GM` cleanup attempt:
  - `BlueprintTools.remove_function_graph` succeeds for normal function graphs
    such as input modifier `ModifyRaw`, but did not remove `BP_GM`'s residual
    interface implementation graphs.
  - Asset tags confirm `BP_GM` still implements old Blueprint interfaces:
    `/IslandGenerator/Misc/BPI_IslandPlugin`, `BPI_Resource`, and
    `BPI_Player`. These need a lower-level remove-interface pass or a closed
    editor commandlet pass before the old interface graphs can be deleted.
- Fresh audit command rerun at local 2026-07-08 09:49:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 8 assets currently use project C++ parents.
  - 62 assets still use engine or Blueprint parents.
  - 70 assets still have at least one graph.
  - 29 assets still have non-trivial function/interface graphs.
  - 9 assets are currently `BS_ERROR`.
- Current compile failures after this pass:
  - `BPC_EndGame`
  - `BPC_House`
  - `CUI_BuildItem`
  - `UI_Layer_Game`
  - `UI_MainMenu`
  - `UIE_Resource`
  - `UIE_Slider`
  - `UI_EndGame`
  - `UI_Pause`

### BPC_House Node Refresh

- Targeted compile failure:
  - `BPC_House` still contained a stale call node for old BP_GM custom event
    `Spawn Villagers`.
  - Native C++ implementation already exists as
    `ACropoutGameMode::SpawnVillagers`.
- Backup made before graph rewrite:
  - `C:/Temp/BPC_House_before_eventgraph_refresh_20260708_095303.uasset`.
- Rewrote only `BPC_House.EventGraph` through MCP `write_graph_dsl`.
  - First attempt used the raw DSL emitted by `read_graph_dsl`; this partially
    cleared the custom event body because `DoOnce` needed an explicit
    `Start Closed` pin name when writing.
  - The graph was immediately rewritten again with explicit
    `:"Start Closed" false`, restoring the original `DoOnce` shape while
    recreating the call node against native `SpawnVillagers`.
  - Readback shows the graph body again matches the original structure:
    `Custom|SpawnVillagers -> DoOnce -> Cropout|GameMode|Event|SpawnVillagers`.
- Compiled and saved only `/Game/Blueprint/Interactable/Building/BPC_House`.
- Fresh audit command rerun at local 2026-07-08 09:56:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 8 assets currently use project C++ parents.
  - 62 assets still use engine or Blueprint parents.
  - 29 assets still have non-trivial function/interface graphs.
  - Compile failures dropped from 9 to 8.
- Current compile failures after this pass:
  - `BPC_EndGame`
  - `CUI_BuildItem`
  - `UI_Layer_Game`
  - `UI_MainMenu`
  - `UIE_Resource`
  - `UIE_Slider`
  - `UI_EndGame`
  - `UI_Pause`
- `BPC_EndGame` note:
  - It has the same stale-call pattern for old BP_GM custom event `End Game`.
  - MCP `read_graph_dsl` cannot currently read the function graph named
    `3: Construction Complete`; the colon in the graph name is rejected as an
    invalid EdGraph ref.
  - A commandlet-side Python node dump also hit protected `EdGraph.Nodes`
    access, so this asset needs either a better lower-level graph reader or a
    direct C++ parent migration for the building class chain.

### Main Menu and Resource Widget Compile Cleanup

- `UI_MainMenu`:
  - Backup made before graph rewrite:
    `C:/Temp/UI_MainMenu_before_openlevel_refresh_20260708_101244.uasset`.
  - Rewrote `EventGraph` through MCP to replace old `Class|BPGI|OpenLevel`
    calls with native `Cropout|GameInstance|Event|HandleOpenLevel`.
  - Replaced old `SetSaveData|ClearSave` on new-game flow with native
    `Cropout|GameInstance|Event|HandleEventClearSave`.
  - Replaced old save check with native
    `Cropout|GameInstance|CheckSaveBool`.
  - MCP could not faithfully recreate the old `SwitchOnString`,
    `UI_Prompt` dispatcher binding, or IAP offer query nodes from readback
    DSL. Current saved compile-clean state keeps core menu flows working, but:
    - New Game no longer opens the overwrite-confirm prompt before clearing.
    - Quit no longer opens the quit-confirm prompt.
    - Donate still routes to the old purchase event body, but the pre-purchase
      prompt binding was not recreated.
  - `UI_MainMenu` now compiles and is saved.
- `UIE_Resource`:
  - Backup made before graph rewrite:
    `C:/Temp/UIE_Resource_before_dispatcher_refresh_20260708_102043.uasset`.
  - Removed the stale old `BP_GM_C.Update Resources` dispatcher bind from the
    saved graph so the asset no longer fails compilation.
  - MCP writeback could not safely rebuild the old enum-driven texture select
    or native `UpdateResources` binding because the widget still owns the old
    BP enum variable while the native dispatcher uses `ECropoutResourceType`.
  - Current saved compile-clean state keeps the construct-time value lookup,
    but dynamic resource update animation and non-food preconstruct texture
    selection need to be restored in a future native UI parent pass.
- Fresh audit command rerun at local 2026-07-08 10:24:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 8 assets currently use project C++ parents.
  - 62 assets still use engine or Blueprint parents.
  - 29 assets still have non-trivial function/interface graphs.
  - Compile failures dropped from 8 to 4 across the recent UI cleanup passes.
- Current compile failures after this pass:
  - `BPC_EndGame`
  - `CUI_BuildItem`
  - `UI_Layer_Game`
  - `UIE_Slider`

### Slider and Build Item Compile Cleanup

- `UIE_Slider`:
  - Root cause was native parity type drift: Blueprint writes slider `Value`
    floats into `BP_GI.SoundMixes`, but `UCropoutGameInstance::SoundMixes`
    had been declared as `TArray<TObjectPtr<USoundMix>>`.
  - Changed `SoundMixes` in C++ to `TArray<float>`.
  - Full editor target build passed after the C++ change.
  - Commandlet audit now reports `UIE_Slider` as `BS_UP_TO_DATE`.
  - Live MCP/editor reflection can still see the old type until the editor
    reloads the rebuilt module, so do not resave this widget from the stale
    live editor session.
- `CUI_BuildItem`:
  - Backup made before graph rewrite:
    `C:/Temp/CUI_BuildItem_before_dispatcher_refresh_20260708_105113.uasset`.
  - Removed the stale old `BP_GM_C.Update Resources` dispatcher bind from
    `EventConstruct`.
  - Kept the existing `ResourceCheck` enable/disable behavior in both
    `EventConstruct` and `ResourceUpdateCheck`.
  - Current saved compile-clean state checks affordability on construct, but
    the live resource-update subscription still needs to be restored in a
    future native UI parent pass.
  - `CUI_BuildItem` now compiles and is saved.
- Fresh audit command rerun at local 2026-07-08 10:55:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 8 assets currently use project C++ parents.
  - 62 assets still use engine or Blueprint parents.
  - 29 assets still have non-trivial function/interface graphs.
  - Compile failures dropped from 4 to 2 across the recent slider/build-item
    cleanup passes.
- Current compile failures after this pass:
  - `BPC_EndGame`
  - `UI_Layer_Game`

### UI Layer Game Native Parent

- Added `UCropoutGameLayerWidget` as the native parent for
  `/Game/UI/Game/UI_Layer_Game`.
  - Native `NativeOnInitialized` now clears the resource container, starts the
    resource widget timer, binds `ACropoutGameMode::UpdateVillagers`, binds the
    remaining `BP_PC.KeySwitch` dispatcher by reflection, and binds the pause
    button.
  - Native callable functions now cover the old graph surface:
    `AddResource`, `AddStackItem`, `PullCurrentActiveWidget`,
    `EndGame`, `UpdateVillagerDetails`, and `HidePause`.
  - `AddResource` recreates the old resource widget spawning path and sets the
    spawned `UIE_Resource.Resource` property by reflection.
- Updated module dependencies so the project module can expose CommonUI/UMG
  widget parents from public headers.
- Full editor target build passed after adding the C++ parent.
- Backup made before the Blueprint asset change:
  `C:/Temp/UI_Layer_Game_before_native_parent_20260708_1055.uasset`.
- Reparented `UI_Layer_Game` to
  `/Script/CropoutSampleProject.CropoutGameLayerWidget`.
- Removed the local Blueprint `Add Resource` function graph and primary
  `EventGraph` with `unreal.BlueprintEditorLibrary` in a commandlet pass.
  - Result JSON:
    `Saved/CodexMigrateUILayerGame.json`.
  - The live editor may still have an older in-memory copy open; the saved disk
    asset is authoritative until the editor reloads the asset.
- Fresh audit command rerun at local 2026-07-08 11:07:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 9 assets currently use project C++ parents.
  - 61 assets still use engine or Blueprint parents.
  - 69 assets still have at least one graph.
  - 28 assets still have non-trivial function/interface graphs.
  - Compile failures dropped from 2 to 1.
- Current compile failures after this pass:
  - `BPC_EndGame`

### BPC_EndGame Stale EndGame Node Refresh

- Targeted the last remaining compile failure:
  `/Game/Blueprint/Interactable/Building/BPC_EndGame`.
- Root cause:
  - `ACropoutGameMode::EndGame(bool)` already existed in C++.
  - `BPC_EndGame` still serialized the old BP custom-event call as
    `End Game`, so compile could not resolve the stale function node.
- Verified that a C++ `ScriptName = "End Game"` compatibility alias is not
  valid:
  - UHT build accepted the metadata, but commandlet startup reported that
    Python/reflection script names cannot contain spaces.
  - The alias did not repair the old node, so the metadata was removed.
- Backup made before the asset edit:
  `C:/Temp/BPC_EndGame_before_endgame_node_refresh_20260708_1117.uasset`.
- MCP limitation hit:
  - The live MCP `read_graph_dsl` / node tools cannot resolve graph or node
    refs containing the function graph name `3: Construction Complete`.
  - The same MCP Python implementation can operate on the `EdGraph` object
    directly inside an Unreal commandlet, so the commandlet route was used.
- The running editor process held the asset file, so all dirty assets were
  saved through MCP and the editor was closed cleanly before the commandlet
  save pass.
- Node-level commandlet rewrite:
  - Preserved the existing `Build|Parent:3:ConstructionComplete` node and
    `Get Cropout GM` node.
  - Deleted only the stale `End Game` function-call node.
  - Created a native `Cropout|GameMode|Event|EndGame` call node.
  - Reconnected parent completion exec -> native `EndGame`, connected
    `GetCropoutGM` to the target pin, and kept `Win = true`.
- Result JSON:
  `Saved/CodexFixBpcEndGameNodeLevel.json`.
  - Before DSL:
    `(Cropout|UI|EndGame (Cropout|GameMode|GetCropoutGM) true)`
  - After DSL:
    `(Cropout|GameMode|Event|EndGame (Cropout|GameMode|GetCropoutGM) true)`
  - `compile_ok: true`
  - `saved: true`
- Fresh full audit command rerun at local 2026-07-08 11:21:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 9 assets currently use project C++ parents.
  - 61 assets still use engine or Blueprint parents.
  - 69 assets still have at least one graph.
  - 28 assets still have non-trivial function/interface graphs.
  - Compile failures are now zero.

### BP_GM Interface Graph Cleanup

- Targeted `/Game/Blueprint/Core/GameMode/BP_GM` after its native parent
  `/Script/CropoutSampleProject.CropoutGameMode` was already in place.
- Backup assets made before this cleanup:
  - `C:/Temp/BP_GM_before_graph_cleanup_landed_20260708_112451.uasset`
  - `C:/Temp/BP_GM_before_interface_graph_cleanup_20260708_113440.uasset`
  - `C:/Temp/BP_GM_empty_interface_stubs_20260708_114414.uasset`
- The old primary `EventGraph` was already removed.
- `BlueprintTools.remove_function_graph` works for ordinary function graphs,
  but BP_GM's inherited/interface graphs are not deletable through that route
  because Unreal reports them as protected interface conformance graphs.
- Added an editor-only cleanup commandlet,
  `UCropoutBlueprintInterfaceGraphCleanupCommandlet`, to remove stale graph
  bodies from `Blueprint->ImplementedInterfaces`.
  - It removed five interface graphs from BP_GM:
    `Island Seed`, `Check Resource`, `Remove Target Resource`,
    `Get Current Resources`, and `Remove Resource`.
  - After compile/save, Unreal automatically recreated four required
    interface function signature stubs:
    `Island Seed`, `Remove Resource`, `Get Current Resources`, and
    `Check Resource`.
  - `Remove Target Resource` stayed removed.
- Checked the pre-cleanup interface graph bodies:
  - `Island Seed` was the only meaningful body; it read
    `UCropoutGameInstance::TargetSeed` and returned an `FRandomStream`.
  - `Remove Resource`, `Get Current Resources`, and `Check Resource` were
    already default/empty-style returns.
  - `Remove Target Resource` had no remaining useful body.
- Added native BP_GM seed parity in `ACropoutGameMode`:
  - `IslandSeed()` now forwards to native C++.
  - `GetIslandSeedStream()` returns `FRandomStream(TargetSeed)` from
    `UCropoutGameInstance`, or seed `0` when the game instance is unavailable.
- Rewrote BP_GM's remaining `Island Seed` interface stub to only call the
  native `GetIslandSeedStream()` wrapper.
  - Result JSON: `Saved/codex_write_bp_gm_island_seed_bridge.json`.
  - `compile_ok: true`
  - `saved: true`
- Follow-up DSL read:
  `Saved/codex_read_bp_gm_residual_graph_dsl.json`.
  - `Island Seed` is now only
    `Cropout|GameMode|Island|GetIslandSeedStream self`.
  - `Remove Resource`, `Get Current Resources`, and `Check Resource` are
    entry/return-only interface stubs.
  - `Remove Target Resource` does not exist.
- Full editor target build after the C++ change passed; the target was up to
  date.
- Final BP_GM graph state after this pass:
  - `UserConstructionScript`
  - `Island Seed` -> native wrapper call only
  - `Remove Resource` -> required interface signature stub
  - `Get Current Resources` -> required interface signature stub
  - `Check Resource` -> required interface signature stub
  - No `EventGraph`
  - No `Remove Target Resource`
- Fresh full audit command rerun at local 2026-07-08 11:47:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 9 assets currently use project C++ parents.
  - 61 assets still use engine or Blueprint parents.
  - 69 assets still have at least one graph.
  - 28 assets still have non-trivial function/interface graphs.
  - Compile failures remain zero.
- To make BP_GM physically have zero interface function graphs, BP_GM must stop
  implementing the remaining Blueprint interfaces, or callers must first move
  off the BP-side `BPI_Resource` / `BPI_IslandPlugin` contract. While those
  interfaces are still implemented, Unreal recreates the required signature
  stubs during compile/save.

### BP_GI EventGraph Native Init Cleanup

- Targeted `/Game/Blueprint/Core/GameMode/BP_GI` after confirming it already
  uses native parent `/Script/CropoutSampleProject.CropoutGameInstance`.
- Pre-cleanup BP_GI graph dump:
  `Saved/codex_read_bp_gi_residual_graph_dsl.json`.
  - Remaining graphs before this pass: `EventGraph`, `Island Seed`.
  - `EventGraph` contained `Event Init` -> `HandleEventInit`,
    `Custom|OpenLevel` -> `HandleOpenLevel`, and
    `Custom|AddLoadingUI` -> `HandleAddLoadingUI`.
  - The other old island events in that graph were unconnected.
  - `Island Seed` was an entry/return-only `IslandSeed` stub returning default
    `0`.
- Added `UCropoutGameInstance::Init()` override in C++ and call
  `HandleEventInit()` after `Super::Init()`, preserving the old Blueprint
  `Event Init` lifecycle behavior before deleting the Blueprint EventGraph.
- Searched Blueprint DSL references for the old BP_GI event/function names:
  `Saved/codex_search_blueprint_dsl_terms.json`.
  - UI callers now use native `HandleOpenLevel` / other native GI functions.
  - No `/Game/Blueprint` or `/Game/UI` caller was found still invoking the old
    BP_GI `Custom|OpenLevel`, `Custom|AddLoadingUI`, or `EventInit` graph entry.
- Backup assets made before BP_GI graph edits:
  - `C:/Temp/BP_GI_before_graph_cleanup_20260708_122555.uasset`
  - `C:/Temp/BP_GI_after_eventgraph_cleanup_before_islandseed_direct_20260708_122705.uasset`
- Removed BP_GI `EventGraph` and saved the asset.
  - Result JSON: `Saved/codex_clean_bp_gi_graphs.json`.
  - Compile status after removal: `BS_UP_TO_DATE`.
- Attempted to delete the remaining `Island Seed` graph through both
  `BlueprintTools.remove_function_graph` and direct `remove_graph`.
  - Ordinary function deletion logged `Failed to remove function 'Island Seed'`.
  - Direct graph deletion compiled and saved, but Unreal recreated/kept the
    `Island Seed` graph in `after_graphs`.
  - Current conclusion: this is another protected inherited/interface-style
    signature stub, not a normal removable function graph.
- Final BP_GI graph state after this pass:
  - `Island Seed` only.
  - No `EventGraph`.
- Validation after this pass:
  - Full editor target build passed after adding `UCropoutGameInstance::Init()`.
  - Fresh full Blueprint audit at local 2026-07-08 12:29:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 9 assets currently use project C++ parents.
    - 61 assets still use engine or Blueprint parents.
    - 69 assets still have at least one graph.
    - 28 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### BP_PC MouseMove Native Input Cleanup

- Targeted `/Game/Blueprint/Core/Player/BP_PC` after confirming it already
  uses native parent `/Script/CropoutSampleProject.CropoutPlayerController`.
- Pre-cleanup BP_PC graph dumps:
  - `Saved/codex_read_bp_pc_full.json`
  - `Saved/codex_probe_bp_pc_metadata.json`
  - `Saved/codex_dump_bp_pc_pin_links.json`
- Connected EventGraph behavior found in the DSL was the `MouseMove` axis
  path:
  - If `AxisValue != 0` and `InputType == NewEnumerator0`, set `InputType`
    to `NewEnumerator1`, then call the `KeySwitch` dispatcher.
  - The `KeyDetect` and `Touch Detect` event entries had no connected body in
    the DSL read used for this pass.
- Added the matching C++ behavior to `ACropoutPlayerController`:
  - `SetupInputComponent()` binds the legacy `MouseMove` axis.
  - `HandleMouseMoveAxis()` owns the old connected EventGraph branch.
  - Temporary reflection helpers read/write the BP-defined `InputType` enum
    variable and broadcast the BP-defined `KeySwitch` dispatcher. This keeps
    the old BP contract alive until the enum/dispatcher are lifted into C++ in
    a later pass.
- Full editor target build passed after the C++ change.
- Backup asset made before BP_PC graph edit:
  - `C:/Temp/BP_PC_before_eventgraph_cleanup_20260708_124035.uasset`
- Removed BP_PC `EventGraph` and saved the asset.
  - Result JSON: `Saved/codex_clean_bp_pc_graphs.json`.
  - Compile status after removal: `BS_UP_TO_DATE`.
- Final BP_PC graph state after this pass:
  - `UserConstructionScript`
  - `KeySwitch` dispatcher signature graph
  - No `EventGraph`
- Fresh full Blueprint audit at local 2026-07-08 12:41:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 9 assets currently use project C++ parents.
  - 61 assets still use engine or Blueprint parents.
  - 69 assets still have at least one graph.
  - 28 assets still have non-trivial function/interface graphs.
  - Compile failures remain zero.

### BP_SaveGM Data-Only Graph Cleanup

- Targeted `/Game/Blueprint/Core/Save/BP_SaveGM` after confirming it already
  uses native parent `/Script/CropoutSampleProject.CropoutSaveGame`.
- Live read before cleanup:
  - Result JSON: `Saved/codex_read_bp_save_gm_full.json`.
  - Parent: `/Script/CropoutSampleProject.CropoutSaveGame`.
  - Local BP variable count: `0`.
  - BP function list: empty.
  - BP event list: empty.
  - Remaining graphs before cleanup: `EventGraph` only.
  - `EventGraph` DSL was empty and node count was `0`.
- Backup asset made before the graph edit:
  - `C:/Temp/BP_SaveGM_before_eventgraph_cleanup_20260708_124647.uasset`
- Removed BP_SaveGM `EventGraph` and saved the asset.
  - Result JSON: `Saved/codex_clean_bp_save_gm_graphs.json`.
  - Compile status after removal: `BS_UP_TO_DATE`.
  - Final BP_SaveGM graph list: empty.
- Fresh full Blueprint audit at local 2026-07-08 12:47:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 9 assets currently use project C++ parents.
  - 61 assets still use engine or Blueprint parents.
  - 68 assets still have at least one graph.
  - 28 assets still have non-trivial function/interface graphs.
  - Compile failures remain zero.

### Input Modifier Empty Graph Cleanup

- Targeted the two native-parent input modifier Blueprint assets:
  - `/Game/Blueprint/Core/Player/Input/IM_Normalize`
  - `/Game/Blueprint/Core/Player/Input/IM_Offset`
- Live read before cleanup:
  - Result JSON: `Saved/codex_read_input_modifiers_full.json`.
  - `IM_Normalize` parent:
    `/Script/CropoutSampleProject.CropoutInputModifierNormalize`.
  - `IM_Offset` parent:
    `/Script/CropoutSampleProject.CropoutInputModifierOffset`.
  - Both assets had local BP variable count `0`.
  - Both assets had only `EventGraph`.
  - Both `EventGraph` DSL strings were empty and node count was `0`.
  - `ModifyRaw`, `GetVisualizationColor`, and `ReceiveModifierReinstanced`
    appeared only as inherited, unimplemented function/event entries.
- Backup assets made before the graph edits:
  - `C:/Temp/IM_Normalize_before_eventgraph_cleanup_20260708_125121.uasset`
  - `C:/Temp/IM_Offset_before_eventgraph_cleanup_20260708_125121.uasset`
- Removed both empty `EventGraph` graphs and saved the assets.
  - Result JSON: `Saved/codex_clean_input_modifier_graphs.json`.
  - Compile status after removal: `BS_UP_TO_DATE` for both assets.
  - Final graph list: empty for both assets.
- Validation after this pass:
  - Full editor target build passed; target was up to date.

### BP_GM Empty Construction Script Cleanup

- Targeted `/Game/Blueprint/Core/GameMode/BP_GM` after confirming it still uses
  native parent `/Script/CropoutSampleProject.CropoutGameMode`.
- Pre-cleanup read:
  - Result JSON: `Saved/codex_read_bp_gm_full.json`.
  - Local BP variable count: `0`.
  - Remaining graphs before cleanup:
    `UserConstructionScript`, `Island Seed`, `Remove Resource`,
    `Get Current Resources`, and `Check Resource`.
  - `UserConstructionScript` DSL was `(fn ConstructionScript ())`.
  - `UserConstructionScript` node list contained only the `Construction Script`
    entry node.
  - The remaining interface-style stubs were unchanged:
    - `Island Seed` returns native
      `Cropout|GameMode|Island|GetIslandSeedStream self`.
    - `Remove Resource`, `Get Current Resources`, and `Check Resource` are
      entry/return-only signature stubs.
- Backup asset made before the graph edit:
  - `C:/Temp/BP_GM_before_construction_cleanup_20260708_160318.uasset`
- Removed only the empty `UserConstructionScript` graph and saved the asset.
  - Result JSON: `Saved/codex_clean_bp_gm_construction.json`.
  - `removed: true`
  - `saved: true`
  - Compile status after removal: `BS_UP_TO_DATE`.
- Post-cleanup read:
  - Result JSON: `Saved/codex_read_bp_gm_full.json`.
  - Final BP_GM graph list:
    `Island Seed`, `Remove Resource`, `Get Current Resources`, and
    `Check Resource`.
  - No `UserConstructionScript`.
- Validation after this pass:
  - Fresh full Blueprint audit at local 2026-07-08 16:04:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 11 assets currently use project C++ parents.
    - 59 assets still use engine or Blueprint parents.
    - 62 assets still have at least one graph.
    - 25 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 12:52:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 9 assets currently use project C++ parents.
    - 61 assets still use engine or Blueprint parents.
    - 66 assets still have at least one graph.
    - 28 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### BP_MainMenuGM Empty Construction Script Cleanup

- Targeted `/Game/Blueprint/Core/MainMenu/BP_MainMenuGM` after confirming it
  already uses native parent
  `/Script/CropoutSampleProject.CropoutMainMenuGameMode`.
- C++ parent already owns the old main-menu `BeginPlay` flow in
  `ACropoutMainMenuGameMode::BeginPlay()`.
- Live read before cleanup:
  - Result JSON: `Saved/codex_read_main_menu_gm_full.json`.
  - Local BP variable count: `0`.
  - Remaining graph before cleanup: `UserConstructionScript` only.
  - `UserConstructionScript` DSL was `(fn ConstructionScript ())`.
  - Node list contained only the `Construction Script` entry node.
- Backup asset made before the graph edit:
  - `C:/Temp/BP_MainMenuGM_before_construction_cleanup_20260708_125610.uasset`
- Removed the empty `UserConstructionScript` graph and saved the asset.
  - Result JSON: `Saved/codex_clean_main_menu_gm_construction.json`.
  - Compile status after removal: `BS_UP_TO_DATE`.
  - Final BP_MainMenuGM graph list: empty.
- Validation after this pass:
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 12:57:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 9 assets currently use project C++ parents.
    - 61 assets still use engine or Blueprint parents.
    - 65 assets still have at least one graph.
    - 28 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### BP_PC Empty Construction Script Cleanup

- Targeted `/Game/Blueprint/Core/Player/BP_PC` after confirming it already
  uses native parent `/Script/CropoutSampleProject.CropoutPlayerController`.
- Live read before cleanup:
  - Result JSON: `Saved/codex_read_bp_pc_full.json`.
  - Member/dispatcher probe JSON: `Saved/codex_probe_bp_pc_members.json`.
  - Remaining graphs before cleanup: `UserConstructionScript`, `KeySwitch`.
  - `UserConstructionScript` DSL was `(fn ConstructionScript ())`.
  - `UserConstructionScript` node list contained only the entry node.
  - `KeySwitch` DSL was `(fn KeySwitch (New Input))`.
  - `list_event_dispatchers` reported `KeySwitch`.
- Backup asset made before the graph edit:
  - `C:/Temp/BP_PC_before_construction_cleanup_20260708_130210.uasset`
- Removed the empty `UserConstructionScript` graph and saved the asset.
  - Result JSON: `Saved/codex_clean_bp_pc_construction.json`.
  - Compile status after removal: `BS_UP_TO_DATE`.
  - Final BP_PC graph list after this pass: `KeySwitch` only.
- Did not remove `KeySwitch` in this pass.
  - `UCropoutGameLayerWidget` still binds this dispatcher by reflection.
  - `ACropoutPlayerController` still broadcasts it by reflection.
  - Safe removal requires first lifting the dispatcher contract to a native
    C++ multicast delegate and updating both binding and broadcast paths.
- Validation after this pass:
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 13:03:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 9 assets currently use project C++ parents.
    - 61 assets still use engine or Blueprint parents.
    - 65 assets still have at least one graph.
    - 28 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### BP_PC Native KeySwitch Dispatcher Cleanup

- Targeted `/Game/Blueprint/Core/Player/BP_PC` after the previous pass left
  only the `KeySwitch` dispatcher signature graph.
- Pre-cleanup evidence:
  - `Saved/codex_read_bp_pc_full.json` showed `KeySwitch` DSL as
    `(fn KeySwitch (New Input))`.
  - `Saved/codex_probe_bp_pc_members.json` reported `KeySwitch` in
    `list_event_dispatchers`.
  - `UCropoutGameLayerWidget` was still binding the BP dispatcher by
    reflection.
  - `ACropoutPlayerController` was still broadcasting the BP dispatcher by
    reflection.
- Backup asset made before the dispatcher edit:
  - `C:/Temp/BP_PC_before_keyswitch_native_20260708_130612.uasset`
- Removed the BP-defined `KeySwitch` event dispatcher from BP_PC and saved the
  asset before adding the same-name native delegate, avoiding a native/BP member
  name collision during Blueprint load.
  - Result JSON: `Saved/codex_remove_bp_pc_keyswitch_dispatcher.json`.
  - `removed_dispatcher: true`
  - Final BP_PC graph list after dispatcher removal: empty.
- Added native C++ replacement:
  - `FCropoutKeySwitchSignature`
  - `ACropoutPlayerController::KeySwitch`
  - `ACropoutPlayerController::BroadcastKeySwitch()` now broadcasts the native
    delegate directly.
- Updated `UCropoutGameLayerWidget` to bind/unbind the native delegate directly
  through `ACropoutPlayerController`, removing the old reflection lookup and
  `FScriptDelegate` storage.
- Validation after this pass:
  - Full editor target build passed after the C++ changes.
  - Fresh full Blueprint audit at local 2026-07-08 13:07:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 9 assets currently use project C++ parents.
    - 61 assets still use engine or Blueprint parents.
    - 64 assets still have at least one graph.
    - 27 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Final editor target build passed again; target was up to date.

### BPF_Cropout / BPF_Shared Native Function Library Cleanup

- Targeted the two Blueprint function libraries whose native equivalents already
  exist on `UCropoutBlueprintLibrary`:
  - `/Game/Blueprint/Core/GameMode/BPF_Cropout`
  - `/Game/Blueprint/Core/Player/BPF_Shared`
- Native C++ functions already present before this pass:
  - `UCropoutBlueprintLibrary::SteppedPosition`
  - `UCropoutBlueprintLibrary::ConvertToSteppedPos`
  - `UCropoutBlueprintLibrary::GetCropoutGI`
  - `UCropoutBlueprintLibrary::GetCropoutGM`
- Pre-cleanup read:
  - Result JSON: `Saved/codex_read_bp_function_libraries.json`.
  - `BPF_Cropout` implemented `Stepped Position`, `Get Cropout GI`, and
    `Get Cropout GM`.
  - `BPF_Shared` implemented `Convert To Stepped Pos`.
- Backup assets made before deleting the function-library graphs:
  - `C:/Temp/BPF_Cropout_before_native_library_20260708_131724.uasset`
  - `C:/Temp/BPF_Shared_before_native_library_20260708_131724.uasset`
- Reparented both function libraries to
  `/Script/CropoutSampleProject.CropoutBlueprintLibrary`, removed their local
  function graphs, compiled, and saved.
  - Result JSON: `Saved/codex_migrate_bp_function_libraries.json`.
  - Final graph list for both assets: empty.
- Rewired stale callers that still referenced the deleted BP-local function
  entries:
  - `/Game/Blueprint/Interactable/Building/BPC_EndGame`
    - `Get Cropout GM` -> `Cropout|GameMode|GetCropoutGM`
  - `/Game/UI/UI_Elements/UIE_Slider`
    - `Get Cropout GI` -> `Cropout|GameInstance|GetCropoutGI`
  - `/Game/Blueprint/Core/Player/BP_Player`
    - two `Convert To Stepped Pos` nodes ->
      `Cropout|Grid|ConvertToSteppedPos`
    - one `Stepped Position` node -> `Cropout|Grid|SteppedPosition`
  - `/IslandGenerator/Spawner/BP_Spawner`
    - one `Stepped Position` node -> `Cropout|Grid|SteppedPosition`
- Backup assets made before the final remaining caller rewrites:
  - `C:/Temp/Game_Blueprint_Core_Player_BP_Player_before_remaining_bpf_native_calls_20260708_154608.uasset`
  - `C:/Temp/IslandGenerator_Spawner_BP_Spawner_before_remaining_bpf_native_calls_20260708_154608.uasset`
- Final caller rewrite result:
  - `Saved/codex_fix_remaining_bpf_call_nodes.json`.
  - `BP_Player`: 3 replacements, compile status `BS_UP_TO_DATE`, saved.
  - `BP_Spawner`: 1 replacement, compile status `BS_UP_TO_DATE`, saved.
- Validation after this pass:
  - Fresh full Blueprint audit at local 2026-07-08 15:47:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 11 assets currently use project C++ parents.
    - 59 assets still use engine or Blueprint parents.
    - 62 assets still have at least one graph.
    - 25 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `Saved/codex_probe_bp_function_library_references.json` reports no remaining
    package referencers for either `BPF_Cropout` or `BPF_Shared`.
  - The same probe reports remaining call nodes only as native
    `Cropout|...` nodes, not `|GetCropoutGI`, `|GetCropoutGM`,
    `|SteppedPosition`, or `|ConverttoSteppedPos` BP-local function nodes.

### Village Level Blueprint PlayMusic Caller Refresh

- Targeted `/Game/Village.Village:PersistentLevel.Village` after UE commandlet
  startup logs still reported a stale Level Blueprint call to
  `BP_GI_C.Play Music`.
- Native C++ implementation already existed before this pass:
  - `UCropoutGameInstance::PlayMusic(USoundBase* Sound, float Volume, bool bPersist)`
- Pre-cleanup read:
  - Result JSON: `Saved/codex_read_village_level_blueprint.json`.
  - Level Blueprint status was `BS_ERROR`.
  - EventGraph DSL already represented the desired native call shape, but the
    actual node was still `|PlayMusic` with old BP-local pins
    `Audio`, `Volume`, and `Persist`.
- Backup made before saving the map:
  - `C:/Temp/Village_before_native_play_music_20260708_155441.umap`
- Replaced the stale `|PlayMusic` node with native
  `Cropout|GameInstance|PlayMusic`, preserving:
  - the existing BeginPlay execution chain,
  - the `BP_GI` cast output target connection,
  - the music asset `/Game/Audio/MUSIC/MUS_Main_MSS.MUS_Main_MSS`,
  - `Volume = 1.0`,
  - `bPersist = true`.
- Result JSON:
  - `Saved/codex_fix_village_play_music_node.json`.
  - Native node created: `Cropout|GameInstance|PlayMusic`.
  - Compile status after replacement: `BS_UP_TO_DATE`.
  - `save_current_level`: true.
- Validation after this pass:
  - Re-read the Level Blueprint with
    `Saved/codex_read_village_level_blueprint.json` at local
    2026-07-08 15:55.
    - Status before compile: `BS_UP_TO_DATE`.
    - Compile result: true.
    - Status after compile: `BS_UP_TO_DATE`.
    - Remaining PlayMusic node type: `Cropout|GameInstance|PlayMusic`.
  - Fresh full Blueprint audit at local 2026-07-08 15:56:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 11 assets currently use project C++ parents.
    - 59 assets still use engine or Blueprint parents.
    - 62 assets still have at least one graph.
    - 25 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Full editor target build passed; target was up to date.

### BP_GM / BP_GI Empty Graph Cleanup and Native Caller Rewire

- Targeted the remaining BP-local function/event implementations on:
  - `/Game/Blueprint/Core/GameMode/BP_GM`
  - `/Game/Blueprint/Core/GameMode/BP_GI`
- Native C++ implementation used for the removed GameMode/GameInstance-facing
  graph bodies:
  - `UCropoutBlueprintLibrary::GetCropoutIslandSeedStream`
  - `UCropoutBlueprintLibrary::NotifyIslandGenComplete`
  - `UCropoutBlueprintLibrary::NotifyRemoveCurrentUILayer`
  - `UCropoutBlueprintLibrary::AddGameModeResource`
  - `UCropoutBlueprintLibrary::RemoveGameModeResource`
  - `UCropoutBlueprintLibrary::CheckGameModeResource`
  - `UCropoutBlueprintLibrary::ApplyGameModeBuildResourceCost`
- Rewired external callers before removing the BP-local interface graphs:
  - `/IslandGenerator/BP_IslandGen`
    - `IslandSeed` -> `Cropout|GameInstance|GetCropoutIslandSeedStream`
    - `IslandGenComplete` -> `Cropout|GameMode|Event|NotifyIslandGenComplete`
  - `/Game/Blueprint/Core/Player/BP_Player`
    - `Remove Resources` -> `Cropout|GameMode|Resource|ApplyGameModeBuildResourceCost`
      plus native `NotifyRemoveCurrentUILayer`
  - `/Game/Blueprint/Villagers/BP_Villager`
    - `Eat` now calls native `RemoveGameModeResource` for the GameMode resource
      debit.
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_DeliverRessource`
    - GameMode resource delivery now calls native `AddGameModeResource`.
  - `/Game/UI/UI_Elements/UIE_Resource`
    - GameMode resource display query now calls native `CheckGameModeResource`.
- Removed the now-empty BP-side interface implementations from:
  - `BP_GM`: `BPI_IslandPlugin` and `BPI_Resource`
  - `BP_GI`: `BPI_IslandPlugin`
- Backup assets made before destructive BP graph/interface cleanup:
  - `C:/Temp/BP_GM_before_remove_island_plugin_interface_20260708_163625.uasset`
  - `C:/Temp/BP_GI_before_remove_island_plugin_interface_20260708_163625.uasset`
  - `C:/Temp/BP_GM_before_remove_bpi_resource_20260708_170150.uasset`
  - `C:/Temp/BP_IslandGen_before_native_calls_20260708_163016.uasset`
  - `C:/Temp/BP_Player_before_remove_resources_native_20260708_165743.uasset`
  - `C:/Temp/BP_Villager_before_game_mode_resource_message_replace_20260708_165941.uasset`
  - `C:/Temp/BTT_DeliverRessource_before_game_mode_resource_message_replace_20260708_165941.uasset`
  - `C:/Temp/UIE_Resource_before_game_mode_resource_message_replace_20260708_165941.uasset`
- Validation after this pass:
  - Generated Visual Studio project files successfully after the new C++
    commandlet files were added.
  - Full editor target build passed after removing the unused trial
    `GetGameModeResources` helper.
  - Fresh full Blueprint audit at local 2026-07-08 17:12:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 11 assets currently use project C++ parents.
    - 59 assets still use engine or Blueprint parents.
    - 60 assets still have at least one graph.
    - 23 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
    - `BP_GM`: parent `/Script/CropoutSampleProject.CropoutGameMode`,
      graph count 0, implemented function count 0.
    - `BP_GI`: parent `/Script/CropoutSampleProject.CropoutGameInstance`,
      graph count 0, implemented function count 0.

### UI Desired Focus Target Native Parents

- Targeted four CommonUI widget Blueprints whose only non-trivial function
  graph was `BP_GetDesiredFocusTarget`:
  - `/Game/UI/MainMenu/UI_MainMenu`
  - `/Game/UI/UI_Elements/UI_Build`
  - `/Game/UI/UI_Elements/UI_EndGame`
  - `/Game/UI/UI_Elements/UI_Pause`
- Pre-migration read:
  - Result JSON: `Saved/codex_read_ui_focus_graphs.json`.
  - `UI_MainMenu`: returned `BTN_Continue` when `Has save` is true,
    otherwise `BTN_NewGame`.
  - `UI_Build`: returned `BTN_Back`.
  - `UI_EndGame`: returned `BTN_Continue`.
  - `UI_Pause`: returned `BTN_Resume`.
- Added native C++ parents in `CropoutMenuWidgets`:
  - `UCropoutMainMenuWidget`
  - `UCropoutBuildWidget`
  - `UCropoutEndGameWidget`
  - `UCropoutPauseWidget`
- Reparented each Widget Blueprint to its matching C++ parent and removed
  its `BP_GetDesiredFocusTarget` graph.
  - Result JSON: `Saved/codex_migrate_ui_focus_graphs.json`.
  - All four assets compiled with status `BS_UP_TO_DATE`.
  - All four assets saved.
- Backup assets made before the reparent / graph deletion:
  - `C:/Temp/UI_MainMenu_before_native_focus_20260708_172005.uasset`
  - `C:/Temp/UI_Build_before_native_focus_20260708_172005.uasset`
  - `C:/Temp/UI_EndGame_before_native_focus_20260708_172005.uasset`
  - `C:/Temp/UI_Pause_before_native_focus_20260708_172005.uasset`
- Validation after this pass:
  - Generated Visual Studio project files successfully after adding
    `CropoutMenuWidgets.h/.cpp`.
  - Full editor target build passed after adding the C++ parents.
  - Re-read the four Widget Blueprints after migration:
    - each parent is now the matching `/Script/CropoutSampleProject.*`
      class,
    - each graph list is only `EventGraph`,
    - none still contains `BP_GetDesiredFocusTarget`,
    - all four statuses are `BS_UP_TO_DATE`.
  - Fresh full Blueprint audit at local 2026-07-08 17:23:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 15 assets currently use project C++ parents.
    - 55 assets still use engine or Blueprint parents.
    - 60 assets still have at least one graph.
    - 19 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Final editor target build passed; target was up to date.

### UI Prompt Native Parent

- Targeted `/Game/UI/UI_Elements/UI_Prompt`.
- Pre-migration read:
  - Result JSON: `Saved/codex_read_ui_prompt_build_confirm_full.json`.
  - Parent before migration: `/Script/CommonUI.CommonActivatableWidget`.
  - Graphs before migration:
    - `BP_GetDesiredFocusTarget`: returned `BTN_Neg`.
    - `EventGraph`: focused the desired target on activation, copied
      `PromptQuestion` into `Title`, broadcast `Confirm` on `BTN_Pos`,
      broadcast `Back` on `BTN_Neg`, cleared both dispatchers, then
      deactivated the widget.
    - `Confirm` and `Back`: event dispatcher signature graphs.
  - Referencer found by the readback script:
    - `/Game/UI/MainMenu/UI_MainMenu`
- Added native behavior to `UCropoutPromptWidget` in `CropoutMenuWidgets`:
  - stores `PromptQuestion`;
  - exposes native `Confirm` and `Back` multicast delegates;
  - binds `BTN_Pos` / `BTN_Neg` click handlers;
  - sets focus to `BTN_Neg` on activation;
  - mirrors `PromptQuestion` into the bound `Title` text block;
  - broadcasts and clears the prompt delegates before deactivation.
- Reparented `UI_Prompt` to
  `/Script/CropoutSampleProject.CropoutPromptWidget` and removed all local
  Blueprint graphs / dispatchers:
  - `EventGraph`
  - `BP_GetDesiredFocusTarget`
  - `Confirm`
  - `Back`
- Migration scripts/results:
  - First attempt result: `Saved/codex_migrate_ui_prompt_native.json`.
    It failed after reparent because inherited native delegates named
    `Confirm` / `Back` changed how the old BP call nodes compiled; the script
    did not save the asset.
  - Successful pass result: `Saved/codex_migrate_ui_prompt_native_v2.json`.
    This pass removed the old BP graphs/dispatchers before reparenting.
- Backup asset made before the successful reparent / graph deletion:
  - `C:/Temp/UI_Prompt_before_native_prompt_v2_20260708_173345.uasset`
- Validation after this pass:
  - `UI_Prompt` compiled with status `BS_UP_TO_DATE` and saved.
  - Re-read `UI_Prompt` after migration:
    - parent is `/Script/CropoutSampleProject.CropoutPromptWidget`;
    - graph list is empty;
    - local variables list is empty;
    - referencer remains `/Game/UI/MainMenu/UI_MainMenu`.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 17:34:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 16 assets currently use project C++ parents.
    - 54 assets still use engine or Blueprint parents.
    - 59 assets still have at least one graph.
    - 18 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### EQC Target Town Hall Native Context

- Targeted `/Game/Blueprint/Villagers/AI/EQS/EQC_TargetTownHall`.
- Pre-migration read:
  - Result JSON: `Saved/codex_read_eqs_contexts.json`.
  - Parent before migration: `/Script/AIModule.EnvQueryContext_BlueprintBase`.
  - Graphs before migration:
    - `ProvideSingleLocation`: got all
      `/Game/Blueprint/Interactable/Building/BPC_TownCenter.BPC_TownCenter_C`
      actors, took the first actor, and returned its location when valid.
    - `EventGraph`: empty.
  - Referencers:
    - `/Game/Blueprint/Villagers/AI/EQS/EQ_TownCenter`
    - `/Game/Blueprint/Villagers/AI/EQS/EQ_TownHall`
- Added native C++ parent `UCropoutTargetTownHallEqsContext` in
  `CropoutEqsContexts`.
  - It exposes `TownCenterClass` as a Blueprint default.
  - `ProvideContext` returns the first actor location for that class as an EQS
    point context.
- Reparented `EQC_TargetTownHall` to
  `/Script/CropoutSampleProject.CropoutTargetTownHallEqsContext` and removed
  its local graphs:
  - `EventGraph`
  - `ProvideSingleLocation`
- Set the Blueprint default `TownCenterClass` to
  `/Game/Blueprint/Interactable/Building/BPC_TownCenter.BPC_TownCenter_C`.
- Migration scripts/results:
  - First attempt result:
    `Saved/codex_migrate_eqc_target_town_hall_native.json`.
    It failed before save because the script used the wrong CDO/property access
    path for `TownCenterClass`.
  - Successful pass used the corrected `unreal.get_default_object` CDO access
    and Python property name `town_center_class`; the result JSON is the same
    output path.
- Backup asset made before the reparent / graph deletion:
  - `C:/Temp/EQC_TargetTownHall_before_native_context_20260708_174326.uasset`
- Validation after this pass:
  - `EQC_TargetTownHall` compiled with status `BS_UP_TO_DATE` and saved.
  - Re-read both EQS context Blueprints after migration:
    - `EQC_TargetTownHall` parent is
      `/Script/CropoutSampleProject.CropoutTargetTownHallEqsContext`;
    - `EQC_TargetTownHall` graph list is empty;
    - `EQC_TargetTownHall` `town_center_class` default reads back as
      `/Game/Blueprint/Interactable/Building/BPC_TownCenter.BPC_TownCenter_C`;
    - `EQC_CollectionTarget` remains unchanged because its blackboard
      `Key Name` default/source still needs a separate read before migration.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 17:46:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 17 assets currently use project C++ parents.
    - 53 assets still use engine or Blueprint parents.
    - 58 assets still have at least one graph.
    - 17 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### UI Game Main Native Parent

- Targeted `/Game/UI/Game/UI_GameMain`.
- Pre-migration read:
  - Result JSON: `Saved/codex_read_next_small_blueprints.json`.
  - Parent before migration: `/Script/CommonUI.CommonActivatableWidget`.
  - Graphs before migration:
    - `EventGraph`: button click pushed
      `/Game/UI/UI_Elements/UI_Build.UI_Build_C`; activation applied the
      current BP_PC `InputType`, bound to BP_PC `KeySwitch`, enabled input on
      the controlled pawn, enabled pawn tick, and focused the game viewport.
    - `UI_GameMain_AutoGenFunc`: mirrored BP input-mode switching for
      `E_InputType` values 0, 1, and 2.
    - `Get_VillagerCount_Text`: returned an empty text value.
  - Referencers:
    - `/Game/UI/UI_Elements/UI_Build`
    - `/Game/UI/Game/UI_Layer_Game`
- Added native behavior to `UCropoutGameMainWidget` in `CropoutMenuWidgets`:
  - binds `CUI_Button_55` click to `ACropoutGameMode::EventAddUI`;
  - exposes `BuildWidgetClass` as a Blueprint default;
  - keeps native `Get_VillagerCount_Text`;
  - keeps native `UI_GameMain_AutoGenFunc(uint8)` for BP_PC `KeySwitch`;
  - reads the BP_PC `InputType` property reflectively on activation;
  - applies input mode / cursor behavior and game viewport focus;
  - enables input and actor tick on the controlled pawn.
- Reparented `UI_GameMain` to
  `/Script/CropoutSampleProject.CropoutGameMainWidget` and removed all local
  Blueprint graphs:
  - `EventGraph`
  - `Get_VillagerCount_Text`
  - `UI_GameMain_AutoGenFunc`
- Set the Blueprint default `BuildWidgetClass` to
  `/Game/UI/UI_Elements/UI_Build.UI_Build_C`.
- Migration result:
  - `Saved/codex_migrate_ui_game_main_native.json`
- Backup asset made before the reparent / graph deletion:
  - `C:/Temp/UI_GameMain_before_native_game_main_20260708_175138.uasset`
- Validation after this pass:
  - `UI_GameMain` compiled with status `BS_UP_TO_DATE` and saved.
  - Re-read `UI_GameMain` after migration:
    - parent is `/Script/CropoutSampleProject.CropoutGameMainWidget`;
    - graph list is empty;
    - `build_widget_class` default reads back as
      `/Game/UI/UI_Elements/UI_Build.UI_Build_C`;
    - referencers remain `/Game/UI/UI_Elements/UI_Build` and
      `/Game/UI/Game/UI_Layer_Game`.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 17:53:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 18 assets currently use project C++ parents.
    - 52 assets still use engine or Blueprint parents.
    - 57 assets still have at least one graph.
    - 16 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### EQC Collection Target Native Context

- Targeted `/Game/Blueprint/Villagers/AI/EQS/EQC_CollectionTarget`.
- Pre-migration read:
  - Result JSON: `Saved/codex_read_eqs_contexts.json`.
  - Parent before migration: `/Script/AIModule.EnvQueryContext_BlueprintBase`.
  - Graphs before migration:
    - `ProvideSingleLocation`: used the querier actor's AI controller
      blackboard, read object key `Target`, cast that object to an actor, and
      returned the actor location.
    - `EventGraph`: empty.
  - Local Blueprint variable before migration:
    - `Key Name` default was `Target`.
  - Referencer:
    - `/Game/Blueprint/Villagers/AI/EQS/EQ_CollectionTarget`
- Added native C++ parent `UCropoutCollectionTargetEqsContext` in
  `CropoutEqsContexts`.
  - It exposes `KeyName` as a Blueprint default.
  - `ProvideContext` reads the querier's AI controller blackboard object for
    `KeyName`, casts it to an actor, and returns that actor location as an EQS
    point context.
- Reparented `EQC_CollectionTarget` to
  `/Script/CropoutSampleProject.CropoutCollectionTargetEqsContext` and removed
  its local Blueprint implementation:
  - `ProvideSingleLocation`
  - local variable `Key Name`
  - `EventGraph`
- Set the Blueprint default `KeyName` to `Target`.
- Migration result:
  - `Saved/codex_migrate_eqc_collection_target_native.json`
- Backup asset made before the reparent / graph deletion:
  - `C:/Temp/EQC_CollectionTarget_before_native_context_20260708_175658.uasset`
- Validation after this pass:
  - `EQC_CollectionTarget` compiled with status `BS_UP_TO_DATE` and saved.
  - Re-read both EQS context Blueprints after migration:
    - `EQC_CollectionTarget` parent is
      `/Script/CropoutSampleProject.CropoutCollectionTargetEqsContext`;
    - `EQC_CollectionTarget` graph list is empty;
    - `EQC_CollectionTarget` local variables list is empty;
    - `EQC_CollectionTarget` `key_name` default reads back as `Target`;
    - referencer remains `/Game/Blueprint/Villagers/AI/EQS/EQ_CollectionTarget`.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 17:59:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 19 assets currently use project C++ parents.
    - 51 assets still use engine or Blueprint parents.
    - 56 assets still have at least one graph.
    - 15 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### BTT Find Nearest Of Class Native Task

- Targeted `/Game/Blueprint/Villagers/AI/Tasks/BTT_FindNearestOfClass`.
- Pre-migration read:
  - Result JSON: `Saved/codex_read_next_small_blueprints.json`.
  - Parent before migration: `/Script/AIModule.BTTask_BlueprintBase`.
  - Graphs before migration:
    - `EventGraph`: if the `Target` blackboard object was already valid,
      finished successfully; otherwise selected a target actor class from a
      blackboard class key or `Manual Class`, selected a tag from a blackboard
      name key or `Tag Filter`, collected matching actors, found the nearest
      reachable actor from `Nearest To`, wrote it back to `Target`, and
      finished success/failure.
    - `Draw Debug Path`: drew navigation path segments in green/red based on
      partial path state. The migration read found no calls to this helper in
      the task's EventGraph.
  - Old Blueprint member variables:
    - `Target`
    - `Nearest To`
    - `Use Blackboard Class`
    - `Target Class`
    - `Tag Filter`
    - `Use Blackboard Tag`
    - `Blackboard Tag`
    - `Manual Class`
    - `PossibleActors`
    - `Path Found`
    - `New Target`
  - CDO defaults before migration:
    - `Use Blackboard Class`: `false`
    - `Use Blackboard Tag`: `true`
    - `Path Found`: `true`
    - blackboard selectors were unset on the class default object.
  - Referencers: none reported by the asset registry.
- Added native C++ parent `UCropoutFindNearestOfClassTask` in
  `CropoutBTTasks`.
  - It exposes the old task configuration as editable native properties.
  - `ExecuteTask` reads the configured blackboard class/tag/source actor,
    gathers matching actors, skips actors with partial navigation paths, writes
    the selected actor to `Target`, and returns success/failure.
  - `InitializeFromAsset` resolves the native blackboard key selectors against
    the Behavior Tree blackboard asset.
- Reparented `BTT_FindNearestOfClass` to
  `/Script/CropoutSampleProject.CropoutFindNearestOfClassTask` and removed
  its local Blueprint implementation:
  - `Draw Debug Path`
  - `EventGraph`
  - all old Blueprint member variables listed above
- Migration result:
  - `Saved/codex_migrate_btt_find_nearest_native.json`
  - First script attempt did not save because
    `BlueprintEditorLibrary.remove_member_variable` was not available in this
    UE Python surface. The successful pass used `BlueprintTools.remove_variable`
    while EventGraph still existed, then removed EventGraph.
- Backup asset made before the reparent / graph deletion:
  - `C:/Temp/BTT_FindNearestOfClass_before_native_task_20260708_1809.uasset`
- Validation after this pass:
  - Project files regenerated through UBT `-Mode=GenerateProjectFiles`.
  - Full editor target build passed after adding `CropoutBTTasks`.
  - `BTT_FindNearestOfClass` compiled with status `BS_UP_TO_DATE` and saved.
  - Re-read `BTT_FindNearestOfClass` after migration:
    - parent is `/Script/CropoutSampleProject.CropoutFindNearestOfClassTask`;
    - graph list is empty;
    - local variables list is empty;
    - referencer list remains empty.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 18:13:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 20 assets currently use project C++ parents.
    - 50 assets still use engine or Blueprint parents.
    - 55 assets still have at least one graph.
    - 14 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### CUI Build Item Native Button

- Targeted `/Game/UI/Common/CUI_BuildItem`.
- Pre-migration read:
  - Result JSON: `Saved/codex_inspect_cui_build_item.json`.
  - Parent before migration: `/Script/CommonUI.CommonButtonBase`.
  - Graphs before migration:
    - `EventGraph`: handled hover/unhover animations and size changes,
      initialized title/icon/background/cost widgets from `TableData`, loaded
      the target build class, checked resources on construct, and called
      player build / UI confirm logic on click.
    - `Resource Check`: iterated `TableData.Cost`, queried game mode resource
      availability, and returned whether the button should be interactable.
  - Old Blueprint member variables:
    - `Button Text`
    - `SpawnClass`
    - `Use Data Table`
    - `Cost`
    - `HardClassRef`
    - `Enable Build`
    - `TableData`
  - Referencers: `/Game/UI/UI_Elements/UI_Build`.
- Added native C++ parent `UCropoutBuildItemButton` in `CropoutMenuWidgets`.
  - It preserves the CommonUI button parent behavior and implements the old
    hover/unhover, table-data UI refresh, cost-widget creation, resource check,
    and click-to-build flow in C++.
  - `TableData` remains a Blueprint data variable because it uses the
    Blueprint user-defined struct `ST_Resource`; the native class reads its
    fields through reflection instead of introducing a duplicate C++ struct.
  - `UpdateResources_Event` is kept as an empty native callable to match the
    old empty Blueprint event.
- Reparented `CUI_BuildItem` to
  `/Script/CropoutSampleProject.CropoutBuildItemButton` and removed its local
  Blueprint implementation:
  - `Resource Check`
  - `EventGraph`
- Migration result:
  - `Saved/codex_migrate_cui_build_item_native.json`
  - First script pass did not save because
    `BlueprintTools.remove_function_graph` failed to remove `Resource Check`
    on this asset. The saved pass used the same tool plus a direct
    `BlueprintEditorLibrary.remove_graph` fallback.
- Backup asset made before the reparent / graph deletion:
  - `C:/Temp/CUI_BuildItem_before_native_button_20260708_1822.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ implementation.
  - Re-read `CUI_BuildItem` after migration:
    - parent is `/Script/CropoutSampleProject.CropoutBuildItemButton`;
    - graph list is empty;
    - local variable list is empty in the graph inspector;
    - referencer list remains `/Game/UI/UI_Elements/UI_Build`;
    - status is `BS_UP_TO_DATE`.
  - Full editor target build passed again after preserving the old empty
    `UpdateResources_Event` behavior.
  - Fresh full Blueprint audit at local 2026-07-08 18:25:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 21 assets currently use project C++ parents.
    - 49 assets still use engine or Blueprint parents.
    - 54 assets still have at least one graph.
    - 13 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### UI Build Confirm Native Widget

- Targeted `/Game/UI/UI_Elements/UI_BuildConfirm`.
- Pre-migration read:
  - Result JSON: `Saved/codex_read_ui_prompt_build_confirm_full.json`.
  - Parent before migration: `/Script/CommonUI.CommonActivatableWidget`.
  - Graphs before migration:
    - `UI_GameMain_AutoGenFunc`: set mouse cursor / input mode based on
      `E_InputType`.
    - `NewMacro`: projected the current build spawn actor to screen space,
      divided by viewport scale, and clamped the border position inside the
      viewport.
    - `EventGraph`: on activation, applied the current input type, bound the
      player-controller `KeySwitch` dispatcher, enabled pawn input/tick, focused
      the game viewport, and positioned `CommonBorder_1`; on tick, spring
      interpolated `CommonBorder_1` toward the spawn screen position; button
      clicks called `SpawnBuildTarget`, `RotateSpawn`, or `DestroySpawn`.
    - `Confirm`: empty function graph.
    - `Back`: empty function graph.
  - Old local implementation state:
    - `Confirm`
    - `Back`
    - `Spawn`
    - `Spring State`
  - Referencers: none reported by the asset registry.
- Added native C++ parent `UCropoutBuildConfirmWidget` in
  `CropoutMenuWidgets`.
  - It binds `BTN_Pos`, `BTN_Pos_1`, and `BTN_Neg` in C++.
  - It keeps `UI_GameMain_AutoGenFunc`, `Confirm`, and `Back` as native
    callable entries so the Blueprint no longer needs local function graphs.
  - It mirrors the old activation/input-mode flow and `KeySwitch` binding.
  - It computes the old `NewMacro` screen position in C++ and uses
    `FVectorSpringState` / `UKismetMathLibrary::VectorSpringInterp` for the
    tick-follow behavior.
  - It calls `SpawnBuildTarget`, `RotateSpawn`, and `DestroySpawn` through
    `ProcessEvent` on the controlled pawn so the current `BP_Player` Blueprint
    implementations still execute until that large player graph is migrated.
- Reparented `UI_BuildConfirm` to
  `/Script/CropoutSampleProject.CropoutBuildConfirmWidget` and removed its
  local Blueprint implementation:
  - `UI_GameMain_AutoGenFunc`
  - `NewMacro`
  - `EventGraph`
  - `Confirm`
  - `Back`
  - old local variables `Spawn` and `Spring State`
- Migration result:
  - `Saved/codex_migrate_ui_build_confirm_native.json`
  - First script pass did not save because it treated all old member names as
    configuration data. The successful pass explicitly removed the old
    implementation-state variables before graph deletion.
- Backup asset made before the reparent / graph deletion:
  - `C:/Temp/UI_BuildConfirm_before_native_confirm_20260708_1830.uasset`
- Validation after this pass:
  - Full editor target build passed after adding the native C++ class.
  - `UI_BuildConfirm` compiled with status `BS_UP_TO_DATE` and saved.
  - Re-read `UI_BuildConfirm` after migration:
    - parent is `/Script/CropoutSampleProject.CropoutBuildConfirmWidget`;
    - graph list is empty;
    - local variable list is empty;
    - referencer list remains empty.
  - Full editor target build passed; target was up to date.
  - Fresh full Blueprint audit at local 2026-07-08 18:36:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 22 assets currently use project C++ parents.
    - 48 assets still use engine or Blueprint parents.
    - 53 assets still have at least one graph.
    - 12 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### BP Interactable Native Parent

- Targeted `/Game/Blueprint/Interactable/BP_Interactable`.
- Pre-migration read:
  - Parent before migration: `/Script/Engine.Actor`.
  - Graphs before migration:
    - `UserConstructionScript`
    - `Placement Mode`
    - `Set Progressions State`
    - `Interact`
    - `Transform To Texture`
    - `EventGraph`
  - Old Blueprint data retained on the asset:
    - `BoundGap`
    - `Require Build`
    - `RT_Draw`
    - `OutlineDraw`
    - `Enable Ground Blend`
    - `Progression State`
    - `Mesh List`
- Added native C++ parent `ACropoutInteractable`.
  - `OnConstruction` now rebuilds the old mesh-bound box sizing and box
    rotation behavior.
  - `BeginPlay` schedules the old next-tick ground-blend draw and duplicate
    overlap cleanup.
  - Native callables now cover the old Blueprint function/custom-event
    surface:
    - `PlacementMode`
    - `SetProgressionsState`
    - `Interact`
    - `PlayWobble`
    - `EndWobble`
  - `TransformToTexture` is now native helper logic used by the render-target
    draw path.
  - The old timeline curve data is not exposed through UE Python; native
    wobble uses a timer/interp path to drive the same material scalar/vector
    parameters.
- Reparented `BP_Interactable` to
  `/Script/CropoutSampleProject.CropoutInteractable` and removed all local
  Blueprint graphs listed above.
- Migration scripts/results:
  - `Saved/codex_migrate_bp_interactable_native.json`
  - `Saved/codex_fix_interactable_stale_call_nodes.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/BP_Interactable_before_native_interactable_20260708_1851.uasset`
  - `C:/Temp/Cropout_interactable_callnode_fix_20260708_190502/`
- Dependent node cleanup after reparent:
  - `BP_Player` old call nodes for `Placement Mode` and
    `Set Progressions State` were rebuilt against native
    `Cropout|Interactable` node types.
  - `BP_Resource` old `End Wobble` call node was rebuilt against native
    `Cropout|Interactable|EndWobble`.
  - `BTT_Work` old `Play Wobble` and `End Wobble` call nodes were rebuilt
    against native `Cropout|Interactable` node types.
  - `Interact` was changed to a native output-parameter signature so existing
    Blueprint callers keep the old `NewParam` output pin shape.
- Validation after this pass:
  - Project files regenerated after adding `CropoutInteractable`.
  - Full editor target build passed after adding and adjusting the native C++
    class.
  - Re-read `BP_Interactable` after migration:
    - parent is `/Script/CropoutSampleProject.CropoutInteractable`;
    - graph list is empty;
    - retained Blueprint data variable list is unchanged.
  - Fresh full Blueprint audit at local 2026-07-08 19:06:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 23 assets currently use project C++ parents.
    - 47 assets still use engine or Blueprint parents.
    - 52 assets still have at least one graph.
    - 11 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.

### Building Native Parents

- Targeted the building inheritance slice:
  - `/Game/Blueprint/Interactable/Building/BP_BuildingBase`
  - `/Game/Blueprint/Interactable/Building/BPC_House`
  - `/Game/Blueprint/Interactable/Building/BPC_TownCenter`
  - `/Game/Blueprint/Interactable/Building/BPC_EndGame`
- Pre-migration read confirmed:
  - `BP_BuildingBase` still owned the construction/progression graphs:
    `UserConstructionScript`, `ProgressConstruct`, `1: Spawn In Build Mode`,
    `3: Construction Complete`, `Interact`, and `EventGraph`.
  - `BPC_House` still owned `3: Construction Complete`, `Placement Mode`,
    `SpawnVillagers` in `EventGraph`, and an empty construction-script parent
    call.
  - `BPC_TownCenter` still owned its tag-setup construction script and a
    BeginPlay delay that moves the player pawn to the town center.
  - `BPC_EndGame` still owned `3: Construction Complete` and an empty event
    graph with parent calls.
- Added native C++ implementation in `CropoutBuildingBase`:
  - `ACropoutBuildingBase`
  - `ACropoutHouse`
  - `ACropoutTownCenter`
  - `ACropoutEndGameBuilding`
- Extended `ACropoutInteractable` with native property mirrors for the old
  inherited interactable data and native `Scene` / `Mesh` / `Box` components.
  The C++ member names are `NativeScene`, `NativeMesh`, and `NativeBox` so
  `BP_Interactable`'s existing component variables do not collide, while the
  component object names still match the old Blueprint component names.
- Reparented and removed all local graphs:
  - `BP_BuildingBase` -> `/Script/CropoutSampleProject.CropoutBuildingBase`
  - `BPC_House` -> `/Script/CropoutSampleProject.CropoutHouse`
  - `BPC_TownCenter` -> `/Script/CropoutSampleProject.CropoutTownCenter`
  - `BPC_EndGame` -> `/Script/CropoutSampleProject.CropoutEndGameBuilding`
- Native parity notes:
  - `ProgressConstruct`, `SpawnInBuildMode`, construction completion, and
    `Interact` are now native on `ACropoutBuildingBase`.
  - `ACropoutHouse` preserves the old nav-blocker destruction and one-shot
    villager spawning.
  - `ACropoutTownCenter` preserves the old `"All Resources"` tag and delayed
    player relocation.
  - `ACropoutEndGameBuilding` preserves the old construction-complete win path.
  - Direct native building parents no longer inherit data/components from
    `BP_Interactable`, so the native interactable class now supplies the shared
    properties/components. `RT_Draw` also has a native soft fallback to the old
    `/Game/Blueprint/Core/Extras/RT_GrassMove` default.
- Migration scripts/results:
  - `Saved/CodexInspectBPBuildingDefaults.py`
  - `Saved/CodexMigrateBuildingNative.py`
  - `Saved/codex_migrate_building_native.json`
  - `Saved/codex_inspect_interactable_components.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_building_native_20260708_1922/`
- Validation after this pass:
  - Project files regenerated after adding `CropoutBuildingBase.h/.cpp`.
  - Full editor target build passed after adding the native C++ classes.
  - Full editor target build passed again after adding native shared
    components and the `RT_Draw` fallback.
  - Re-read building assets after migration:
    - all four parents are project native classes;
    - all four graph lists are empty;
    - all four compile with `BS_UP_TO_DATE`.
  - Fresh full Blueprint audit at local 2026-07-08 19:31:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 27 assets currently use project C++ parents.
    - 43 assets still use engine or Blueprint parents.
    - 48 assets still have at least one graph.
    - 8 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Remaining warning: `BP_Interactable` still has local data variables
    `BoundGap`, `RT_Draw`, and `OutlineDraw` with names matching the new native
    mirrors. They compile as warnings, not errors. Leave them until the
    `BP_Resource`/remaining interactable descendants are migrated or their
    variable-node fallout is explicitly handled.

### Resource And Crop Native Parents

- Targeted the resource inheritance slice:
  - `/Game/Blueprint/Interactable/Resources/BP_Resource`
  - `/Game/Blueprint/Interactable/Resources/BP_BaseCrop`
- Pre-migration read confirmed:
  - `BP_Resource` still owned `UserConstructionScript`, `Death`, `Interact`,
    `Island Seed`, `Remove Resource`, `Get Current Resources`,
    `Remove Target Resource`, `Check Resource`, and `EventGraph`.
  - `BP_BaseCrop` still owned `UserConstructionScript`, `Switch Stage`,
    `Set Ready`, `Set Progressions State`, `FarmingProgress`, `Interact`, and
    `EventGraph`.
- Added native C++ implementation in `CropoutResource`:
  - `ACropoutResource`
  - `ACropoutBaseCrop`
- Native parity notes:
  - `ACropoutResource` now derives from `ACropoutInteractable` and implements
    `ICropoutResourceInterface`.
  - `ACropoutResource` preserves the old construction tag setup, optional
    random mesh, collection value decrement, death-on-depleted behavior,
    island seed return value, and scale-up event entry point.
  - `ACropoutBaseCrop` preserves the old farming tags, stage progression,
    ready/harvest tag switching, cooldown timer, mesh selection, and farm plot
    pop/update notifications.
  - Resource default mirrors use native names such as `NativeResourceType`,
    `NativeResourceAmount`, `NativeCollectionTime`, `NativeCollectionValue`,
    `bNativeUseRandomMesh`, and `NativeCooldownTime` to avoid colliding with
    old Blueprint variable names while preserving editor display labels.
- Reparented and removed all local graphs:
  - `BP_Resource` -> `/Script/CropoutSampleProject.CropoutResource`
  - `BP_BaseCrop` -> `/Script/CropoutSampleProject.CropoutBaseCrop`
- Interface cleanup:
  - Removed stale `BPI_Resource` and `BPI_IslandPlugin` implementations from
    `BP_Resource` before final reparent/save so interface graphs would not be
    regenerated during compile.
- Dependent node cleanup after reparent:
  - `BTT_DeliverRessource`, `BTT_TransferResource`, and `BTT_Work` now call
    native `Cropout|Resource|CollectActorResource` instead of the old
    `BPI_Resource.RemoveResource` message node.
  - `BTT_TransferResource` and `BTT_Work` now call native
    `Cropout|Resource|AddActorResource` instead of the old
    `BPI_Resource.AddResource` message node.
  - `CUI_BuildItem` had an old local `HardClassRef` class variable typed to
    `BP_Interactable_C`; after resources moved to the native inheritance chain,
    `BP_Crop_Corn_C` was no longer a child of that Blueprint class. The local
    variable was removed from the Widget Blueprint and the native parent now
    keeps `NativeHardClassRef` typed to `ACropoutInteractable`.
- Migration scripts/results:
  - `Saved/CodexMigrateResourceNative.py`
  - `Saved/codex_migrate_resource_native.json`
  - `Saved/CodexReplaceBttResourceMessages.py`
  - `Saved/codex_replace_btt_resource_messages.json`
  - `Saved/codex_remove_bp_resource_resource_interface.json`
  - `Saved/codex_remove_bp_resource_island_plugin_interface.json`
  - `Saved/codex_remove_cui_hard_class_ref_cpp.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_resource_native_20260708_2050/`
- Validation after this pass:
  - Full editor target build passed after adding and adjusting native resource
    and crop classes.
  - Full editor target build passed after adding the native resource helper
    functions and the `CUI_BuildItem` native class-reference fix.
  - Re-read resource assets after migration:
    - `BP_Resource` parent is `/Script/CropoutSampleProject.CropoutResource`;
    - `BP_BaseCrop` parent is `/Script/CropoutSampleProject.CropoutBaseCrop`;
    - both graph lists are empty;
    - both compile with `BS_UP_TO_DATE`.
  - Fresh full Blueprint audit at local 2026-07-08 19:59:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 29 assets currently use project C++ parents.
    - 41 assets still use engine or Blueprint parents.
    - 46 assets still have at least one graph.
    - 6 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Remaining resource descendants `BP_Tree`, `BP_Stone`, `BP_Shrub`, and the
    four `BP_Crop_*` assets still have only `UserConstructionScript` and
    `EventGraph` shell graphs with zero implemented function graphs. They are
    the next resource cleanup candidates.
  - Remaining warning: `BP_Stone` has a stale `Nav Blocker` inherited-parent
    component reference to `Mesh`. It compiles, but should be fixed when the
    resource child Blueprint shells are cleaned.

### Resource Child Shell Graph Cleanup

- Targeted the resource and crop child Blueprint shell graphs:
  - `/Game/Blueprint/Interactable/Resources/BP_Tree`
  - `/Game/Blueprint/Interactable/Resources/BP_Stone`
  - `/Game/Blueprint/Interactable/Resources/BP_Shrub`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Corn`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Lettuce`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Pumpkin`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Wheat`
- Pre-cleanup read showed these graphs were shell-only:
  - `UserConstructionScript` only called parent construction script.
  - `EventGraph` only called parent `BeginPlay`, parent actor overlap, parent
    tick, or contained an empty tick event in `BP_Stone`.
- Removed local shell graphs from all seven assets:
  - `UserConstructionScript`
  - `EventGraph`
- Kept these assets parented through `BP_Resource` or `BP_BaseCrop` so their
  Blueprint data/default inheritance remains intact while the behavior stays in
  the native C++ parent chain.
- Migration scripts/results:
  - `Saved/CodexReadResourceChildShellGraphs.py`
  - `Saved/codex_read_resource_child_shell_graphs.json`
  - `Saved/CodexRemoveResourceChildShellGraphs.py`
  - `Saved/codex_remove_resource_child_shell_graphs.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_resource_child_shells_20260708_2115/`
- Validation after this pass:
  - All seven edited assets compiled with `BS_UP_TO_DATE` and saved.
  - Full Blueprint audit at local 2026-07-08 20:06:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 29 assets currently use project C++ parents.
    - 41 assets still use engine or Blueprint parents.
    - 39 assets still have at least one graph.
    - 6 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Resource/crop assets now all have `graph_count=0`:
    - `BP_Resource`
    - `BP_BaseCrop`
    - `BP_Tree`
    - `BP_Stone`
    - `BP_Shrub`
    - `BP_Crop_Corn`
    - `BP_Crop_Lettuce`
    - `BP_Crop_Pumpkin`
    - `BP_Crop_Wheat`
  - The previous `BP_Stone` stale `Nav Blocker` inherited-parent warning did
    not reappear in this fresh full audit commandlet summary.
  - Remaining compile warnings are the pre-existing `BP_Interactable` duplicate
    variable-name warnings for `BoundGap`, `RT_Draw`, and `OutlineDraw`.

### Pause And End Game UI Native Parents

- Migrated `/Game/UI/UI_Elements/UI_Pause` EventGraph behavior into
  `UCropoutPauseWidget`:
  - activation sets UI-only input mode, focuses the desired widget, pauses the
    game, and refreshes the embedded music slider through its existing
    `UpdateSlider` function;
  - `BTN_Resume` unpauses, restores game viewport focus, and deactivates the
    widget;
  - `BTN_Restart` clears save data with `ClearSeed=false`, unpauses, and opens
    `/Game/Village.Village` through `UCropoutGameInstance::HandleOpenLevel`;
  - `BTN_MainMenu` unpauses and opens `/Game/MainMenu.MainMenu`.
- Migrated `/Game/UI/UI_Elements/UI_EndGame` EventGraph behavior into
  `UCropoutEndGameWidget`:
  - activation sets UI-only input mode, disables controlled pawn input, focuses
    the desired widget, and pauses the game;
  - native `EndGame(bool)` preserves the reflection-called function name used
    by `UCropoutGameLayerWidget`, stores the legacy `WIN` BP variable when
    present, updates `MainText` from `WinText`/`LoseText`, sets the
    `Cropout_Music_WinLose` control bus value, and stops music through
    `UCropoutGameInstance`;
  - `BTN_Continue`, `BTN_MainMenu`, and `BTN_Retry` now run in C++.
- Removed `EventGraph` from both assets:
  - `/Game/UI/UI_Elements/UI_EndGame`
  - `/Game/UI/UI_Elements/UI_Pause`
- Migration scripts/results:
  - `Saved/CodexRemovePauseEndGameGraphs.py`
  - `Saved/codex_remove_pause_endgame_graphs.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_pause_endgame_native_20260708_2037/`
- Validation after this pass:
  - Full editor target build passed after adding the native UI behavior.
  - The graph-removal commandlet produced transient compile errors while the
    old graphs were still loaded, because `EndGame` now exists natively and
    `Slider_Music` is bound from native. The same commandlet removed the graphs,
    compiled both assets with `BS_UP_TO_DATE`, and saved them.
  - Fresh full Blueprint audit after removal passed with zero compile failures.
  - Fresh full editor target build passed.
  - Audit at local 2026-07-08 20:37:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 30 assets currently use project C++ parents.
    - 40 assets still use engine or Blueprint parents.
    - 25 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `UI_EndGame` and `UI_Pause` now both have `graph_count=0` and
    `implemented_function_count=0`.
  - Remaining project-native assets with graphs are:
    - `BP_Player`
    - `UI_MainMenu`

### Villager Native Parent

- Migrated `/Game/Blueprint/Villagers/BP_Villager` behavior into a native C++
  parent:
  - `ACropoutVillager`
- Added native villager bridge helpers in `UCropoutBlueprintLibrary` for the
  remaining Behavior Tree task callers:
  - `ReturnVillagerToDefaultBT`
  - `PlayVillagerDeliverAnim`
  - `PlayVillagerWorkAnim`
- Reparented `BP_Villager` from `/Script/Engine.Pawn` to
  `/Script/CropoutSampleProject.CropoutVillager`.
- Removed all local graphs from `BP_Villager`:
  - `UserConstructionScript`
  - `Eat`
  - `Reset Job State`
  - `Stop Job`
  - `Hair Pick`
  - `EventGraph`
  - interface implementation graphs removed while detaching `BPI_Villager`
    and `BPI_Resource`
- Replaced external villager interface message nodes in these Behavior Tree
  task Blueprints with native helper function calls:
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT`
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_DeliverRessource`
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_ProgressConstruction`
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_ProgressFarmingSeq`
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_Work`
- Migration scripts/results:
  - `Saved/CodexReadBPVillagerFull.py`
  - `Saved/codex_read_bp_villager_full.json`
  - `Saved/CodexMigrateBPVillagerNative.py`
  - `Saved/codex_migrate_bp_villager_native.json`
  - `Saved/CodexReplaceBttVillagerMessages.py`
  - `Saved/codex_replace_btt_villager_messages.json`
  - `Saved/CodexSearchVillagerDslTerms.py`
  - `Saved/codex_search_villager_dsl_terms.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_bp_villager_native_20260708_2025/`
- Validation after this pass:
  - Full editor target build passed after adding the native villager parent and
    helper functions.
  - All five Behavior Tree task assets with replaced villager calls compiled
    with `BS_UP_TO_DATE` and saved.
  - Search for old villager interface calls/events returned zero matches.
  - Fresh full Blueprint audit at local 2026-07-08 20:26:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 30 assets currently use project C++ parents.
    - 40 assets still use engine or Blueprint parents.
    - 38 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Villager` now has parent
    `/Script/CropoutSampleProject.CropoutVillager`, `graph_count=0`, and
    `implemented_function_count=0`.
  - `BPI_Villager` still exists as an interface asset with its 6 interface
    function graphs, but `BP_Villager` no longer implements it and no old
    villager interface message calls remain in the scanned Blueprints.
  - Remaining compile warnings are the pre-existing `BP_Interactable` duplicate
    variable-name warnings for `BoundGap`, `RT_Draw`, and `OutlineDraw`.

### Empty Shell Graph Cleanup

- Audited remaining Blueprint graphs through `BlueprintTools.read_graph_dsl`
  and removed only graphs proven to be empty/no-op shells.
- Removed empty `EventGraph` from:
  - `/Game/Blueprint/Core/Extras/BP_CamShakeIdle`
  - `/Game/UI/Common/CUI_BaseControllerData`
  - `/Game/UI/Common/CUI_InputData`
  - `/Game/UI/Common/CUI_Style_Border_Dark`
  - `/Game/UI/Common/CUI_Style_Border_Light`
  - `/Game/UI/Common/CUI_Style_Build`
  - `/Game/UI/Common/CUI_Style_Button`
  - `/Game/UI/Common/CUI_Style_Small`
  - `/Game/UI/Common/CUI_Style_Text`
  - `/Game/UI/Common/CUI_Style_Text2`
- Removed empty shell graphs from `/Game/Blueprint/Core/MainMenu/BP_MenuPawn`:
  - `UserConstructionScript` with DSL `(fn ConstructionScript ())`
  - empty `EventGraph`
- Deliberately did not remove `/Game/Blueprint/Core/Extras/BPC_Cheats`
  because its graph still contains input events for keys `1`, `2`, and `3`.
  They have no execution body, but removing them could still affect input
  consumption.
- Checked `/Game/Blueprint/Villagers/BPI_Villager` references before deletion:
  - no Blueprints currently implement it;
  - confirmed referencer remains `/Game/Blueprint/Core/Player/BP_Player`;
  - therefore the interface asset was kept for now and should be handled with
    the `BP_Player` migration.
- Migration scripts/results:
  - `Saved/CodexAuditBpiVillagerRefs.py`
  - `Saved/codex_audit_bpi_villager_refs.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
  - `Saved/CodexRemoveEmptyShellGraphs.py`
  - `Saved/codex_remove_empty_shell_graphs.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_empty_shell_graphs_20260708_2032/`
- Validation after this pass:
  - All 11 edited assets compiled with `BS_UP_TO_DATE` and saved.
  - Fresh full Blueprint audit at local 2026-07-08 20:32:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 30 assets currently use project C++ parents.
    - 40 assets still use engine or Blueprint parents.
    - 27 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Remaining compile warnings are the pre-existing `BP_Interactable` duplicate
    variable-name warnings for `BoundGap`, `RT_Draw`, and `OutlineDraw`.

### UI Build Native Widget

- Targeted `/Game/UI/UI_Elements/UI_Build`.
- Completed the remaining native implementation in `UCropoutBuildWidget`:
  - binds `BTN_Back`;
  - handles activation input mode, focus, pawn tick/input, and build-mode switch;
  - rebuilds the build-item list from
    `/Game/Blueprint/Interactable/Extras/DT_Buidables.DT_Buidables`;
  - creates `/Game/UI/Common/CUI_BuildItem.CUI_BuildItem_C` children with
    `/Game/UI/Common/CUI_Style_Build.CUI_Style_Build_C`.
- Added a native `UCropoutBuildItemButton::InitializeFromDataTableRow` helper so
  the C++ build list can copy each DataTable row into the existing Blueprint
  `TableData` variable before refreshing the button.
- Removed `UI_Build`'s `EventGraph` through
  `unreal.BlueprintEditorLibrary.remove_graph`.
- Migration scripts/results:
  - `Saved/CodexRemoveUiBuildGraph.py`
  - `Saved/codex_remove_ui_build_graph.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent changes:
  - `C:/Temp/Cropout_ui_build_native_20260708_204430/`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - `UI_Build` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh full Blueprint audit at local 2026-07-08 20:45:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 30 assets currently use project C++ parents.
    - 40 assets still use engine or Blueprint parents.
    - 24 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `UI_Build` now has parent
    `/Script/CropoutSampleProject.CropoutBuildWidget`, `graph_count=0`, and
    `implemented_function_count=0`.
  - Remaining project-native assets with graphs are:
    - `BP_Player`
    - `UI_MainMenu`

### UI Main Menu Native Widget

- Targeted `/Game/UI/MainMenu/UI_MainMenu`.
- Re-read the current asset through `BlueprintTools` before editing:
  - parent was already `/Script/CropoutSampleProject.CropoutMainMenuWidget`;
  - only `EventGraph` remained;
  - implemented events were `BP_OnActivated`, `Confirm New Game`,
    `Confirm Quit`, `Confirm Donate`, and `Call Donate`.
- Completed native implementation in `UCropoutMainMenuWidget`:
  - binds `BTN_Continue`, `BTN_NewGame`, `BTN_Quit`, and `BTN_Donate`;
  - activation refreshes save state, enables/disables Continue, updates Donate
    visibility by platform, and focuses the desired widget;
  - `ConfirmNewGame` clears save with `ClearSeed=true` and opens
    `/Game/Village.Village`;
  - `ConfirmQuit` mirrors the old `QuitGame` node;
  - `ConfirmDonate` uses the existing `OnlineSubsystem` purchase interface to
    start `demogame_donate`, finalizes successful purchases, and pushes
    `UI_Prompt` through the existing `StackRef` variable.
- Preserved the old callable entry names as native functions:
  - `OpenLevel`
  - `ConfirmNewGame`
  - `ConfirmQuit`
  - `CallDonate`
  - `ConfirmDonate`
- Removed `UI_MainMenu`'s `EventGraph` through
  `unreal.BlueprintEditorLibrary.remove_graph`.
- Migration scripts/results:
  - `Saved/CodexReadUiMainMenuFull.py`
  - `Saved/codex_read_ui_main_menu_full.json`
  - `Saved/CodexRemoveUiMainMenuGraph.py`
  - `Saved/codex_remove_ui_main_menu_graph.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent changes:
  - `C:/Temp/Cropout_ui_main_menu_native_20260708_205639/`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - `UI_MainMenu` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh full Blueprint audit at local 2026-07-08 20:57:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 30 assets currently use project C++ parents.
    - 40 assets still use engine or Blueprint parents.
    - 23 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `UI_MainMenu` now has parent
    `/Script/CropoutSampleProject.CropoutMainMenuWidget`, `graph_count=0`, and
    `implemented_function_count=0`.
  - Remaining project-native asset with graphs:
    - `BP_Player`

### UI Layer Menu Native Widget

- Targeted `/Game/UI/MainMenu/UI_Layer_Menu`.
- Re-read the current asset through `BlueprintTools` before editing:
  - parent was `/Script/CommonUI.CommonActivatableWidget`;
  - only `EventGraph` remained;
  - implemented events were `BP_OnActivated` and `Add Stack Item`;
  - the graph pushed `/Game/UI/MainMenu/UI_MainMenu.UI_MainMenu_C` into
    `MainStack`, then assigned the same stack to the pushed `UI_MainMenu`'s
    `StackRef`.
- Added `UCropoutMainMenuLayerWidget` as the native parent:
  - `NativeOnActivated` pushes `UI_MainMenu` into `MainStack`;
  - `NativeOnActivated` writes `MainStack` back to the pushed menu widget's
    `StackRef` property so prompt widgets still use the same stack;
  - native `AddStackItem` preserves the old callable custom-event surface.
- Removed `UI_Layer_Menu`'s `EventGraph` through
  `unreal.BlueprintEditorLibrary.remove_graph`.
- Reparented `UI_Layer_Menu` to
  `/Script/CropoutSampleProject.CropoutMainMenuLayerWidget`.
- Migration scripts/results:
  - `Saved/CodexReadUiLayerMenuFull.py`
  - `Saved/codex_read_ui_layer_menu_full.json`
  - `Saved/CodexMigrateUiLayerMenuNative.py`
  - `Saved/codex_migrate_ui_layer_menu_native.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent changes:
  - `C:/Temp/Cropout_ui_layer_menu_native_20260708_210624/`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - `UI_Layer_Menu` compiled with `BS_UP_TO_DATE` and saved after graph removal
    and reparent.
  - Fresh full Blueprint audit at local 2026-07-08 21:11:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `UI_Layer_Menu` now has parent
    `/Script/CropoutSampleProject.CropoutMainMenuLayerWidget`, `graph_count=0`,
    and `implemented_function_count=0`.
  - Remaining project-native asset with graphs:
    - `BP_Player`

### BP Player Small Native Slice

- Targeted `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current asset through `BlueprintTools` before editing:
  - parent was `/Script/CropoutSampleProject.CropoutPlayerPawn`;
  - `BP_Player` had 28 graphs before this slice;
  - the first direct delete attempt correctly failed to save because old
    call nodes still referenced BP-local function names.
- Added native implementations for the small build-helper slice in
  `ACropoutPlayerPawn`:
  - `DestroySpawn` destroys the `Spawn` actor and `SpawnOverlay` component
    stored on the Blueprint subclass;
  - `RotateSpawn` rotates `Spawn` by +90 degrees yaw;
  - `RemoveResources` calls the native game-mode resource wrapper, requests UI
    layer removal when the cost cannot be paid, otherwise calls
    `DestroySpawn`;
  - `GetPlayerController` remains native and is now marked `BlueprintPure` so
    old pure Blueprint call sites can be replaced cleanly.
  - `VillagerRelease` pauses the `Update Path` timer, destroys `NS_Path`, and
    clears the Blueprint subclass's `Selected` reference.
- Retargeted old BP-local call nodes before removing graphs:
  - `Spawn Build Target` now calls
    `Cropout|Player|Build|RemoveResources` instead of
    `BuildMode|RemoveResources`;
  - 10 old pure `|GetPlayerController` call nodes now call
    `Cropout|Player|GetPlayerController`.
  - `EventGraph` now calls `Cropout|Player|Villager|VillagerRelease` instead
    of `VillagerMode|VillagerRelease`.
- Removed five BP_Player function graphs:
  - `Destroy Spawn`
  - `Rotate Spawn`
  - `Remove Resources`
  - `Get Player Controller`
  - `Villager Release`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerSmallNativeV2.py`
  - `Saved/codex_migrate_bp_player_small_native_v2.json`
  - `Saved/CodexMigrateBPPlayerGetControllerNative.py`
  - `Saved/codex_migrate_bp_player_get_controller_native.json`
  - `Saved/CodexMigrateBPPlayerVillagerReleaseNative.py`
  - `Saved/codex_migrate_bp_player_villager_release_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_bp_player_small_native_v2_20260708_212408/BP_Player.uasset`
  - `C:/Temp/Cropout_bp_player_get_controller_native_20260708_212535/BP_Player.uasset`
  - `C:/Temp/Cropout_bp_player_villager_release_native_20260708_213029/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ metadata/body changes.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after each graph
    removal pass.
  - Fresh BP_Player full read at local 2026-07-08 21:32:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 23;
    - no remaining old call nodes for `|GetPlayerController`,
      `BuildMode|DestroySpawn`, `BuildMode|RemoveResources`, or
      `BuildMode|RotateSpawn`, or `VillagerMode|VillagerRelease`.
  - Fresh full Blueprint audit at local 2026-07-08 21:31:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=23`
    - `implemented_function_count=21`

### BP Player Villager And Empty Graph Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current asset through `BlueprintTools` before editing:
  - parent remained `/Script/CropoutSampleProject.CropoutPlayerPawn`;
  - `BP_Player` had 23 graphs at the start of this slice;
  - `Overlap Check` had no live old call sites and returned a bool in BP;
  - `Villager Select` had one old `VillagerMode|VillagerSelect` call in
    `EventGraph`;
  - `UserConstructionScript` was an empty graph:
    `(fn ConstructionScript ())`.
- Native C++ changes:
  - changed `ACropoutPlayerPawn::OverlapCheck` from `void` to `bool` and
    implemented the same `Collision.GetOverlappingActors(BP_Interactable)`,
    contains `Spawn` check;
  - changed `ACropoutPlayerPawn::VillagerSelect` to take `AActor* Selected`
    and moved the BP body into C++: set `Selected`, spawn `/Game/VFX/NS_Target`
    attached to `DefaultSceneRoot`, store `NS_Path`, and start the
    `Update Path` timer;
  - added the `Niagara` private module dependency required by the native
    `VillagerSelect` body.
- Retargeted old BP-local call nodes before removing graphs:
  - `EventGraph` now calls `Cropout|Player|Villager|VillagerSelect` instead
    of `VillagerMode|VillagerSelect`.
- Removed three more BP_Player graphs:
  - `Overlap Check`
  - `Villager Select`
  - empty `UserConstructionScript`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerOverlapCheckNative.py`
  - `Saved/codex_migrate_bp_player_overlap_check_native.json`
  - `Saved/CodexMigrateBPPlayerVillagerSelectNative.py`
  - `Saved/codex_migrate_bp_player_villager_select_native.json`
  - `Saved/CodexRemoveBPPlayerEmptyConstructionScript.py`
  - `Saved/codex_remove_bp_player_empty_construction_script.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_bp_player_overlap_check_native_20260708_213550/BP_Player.uasset`
  - `C:/Temp/Cropout_bp_player_villager_select_native_20260708_213816/BP_Player.uasset`
  - `C:/Temp/Cropout_bp_player_empty_construction_script_20260708_213926/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body and
    module dependency changes.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after each graph
    removal pass.
  - Fresh BP_Player full read at local 2026-07-08 21:41:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 20;
    - no remaining old call nodes for `|OverlapCheck`,
      `VillagerMode|VillagerSelect`, or `|VillagerSelect`.
  - Fresh full Blueprint audit at local 2026-07-08 21:40:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=20`
    - `implemented_function_count=19`

### BP Player Update Path Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Update Path` graph before editing:
  - it synchronously found a navigation path from `Collision` world location
    to `Selected` actor location;
  - it copied `PathPoints` onto the BP subclass variable;
  - it forced the first path point to `Collision` and the last path point to
    `Selected`;
  - it pushed the final vector array into `NS_Path` as Niagara array
    parameter `TargetPath`.
- Native C++ changes:
  - implemented `ACropoutPlayerPawn::UpdatePath`;
  - added a small reflected array setter for the BP-owned `PathPoints`
    variable, keeping BP subclass state ownership unchanged;
  - used `UNavigationSystemV1::FindPathToLocationSynchronously` and
    `UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector` to
    match the removed BP graph behavior.
- Removed one BP_Player function graph:
  - `Update Path`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerUpdatePathNative.py`
  - `Saved/codex_migrate_bp_player_update_path_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_update_path_native_20260708_214725/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed before and after the asset edit.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph`.
  - Fresh BP_Player full read at local 2026-07-08 21:47:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 19;
    - `Update Path` is no longer present.
  - Fresh full Blueprint audit at local 2026-07-08 21:48:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=19`
    - `implemented_function_count=18`

### BP Player Spawn Build Target Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Spawn Build Target` graph before editing:
  - it gated on BP-owned `CanDrop`;
  - it spawned `TargetSpawnClass` at the current `Spawn` actor transform;
  - it called `SetProgressionsState(0.0)` on the spawned interactable;
  - it called native `RemoveResources`;
  - it sent `UpdateAllInteractables` to the current game instance;
  - it finally called the still-BP-owned `Update Build Asset` graph.
- Native C++ changes:
  - implemented `ACropoutPlayerPawn::SpawnBuildTarget`;
  - added reflected helpers for BP-owned bool/class variables;
  - used `UWorld::SpawnActor` for the real build actor;
  - dispatched `UpdateAllInteractables` through
    `ICropoutGameInstanceInterface`;
  - called `UpdateBuildAsset` by `ProcessEvent` instead of directly calling
    the C++ stub, so this slice keeps using the existing BP graph until that
    graph is migrated.
- Removed one BP_Player function graph:
  - `Spawn Build Target`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerSpawnBuildTargetNative.py`
  - `Saved/codex_migrate_bp_player_spawn_build_target_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_spawn_build_target_native_20260708_215504/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ body changes.
  - Migration script found zero remaining old `SpawnBuildTarget` call nodes
    before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph`.
  - Fresh BP_Player full read at local 2026-07-08 21:56:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 18;
    - `Spawn Build Target` is no longer present.
  - Fresh full Blueprint audit at local 2026-07-08 21:56:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=18`
    - `implemented_function_count=17`

### BP Player Update Zoom Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Update Zoom` graph before editing:
  - it sampled BP-owned `ZoomCurve` at the current BP-owned `ZoomValue`;
  - it updated BP-owned `ZoomValue` with
    `Clamp(ZoomValue + ZoomDirection * 0.01, 0.0, 1.0)`;
  - it set `SpringArm.TargetArmLength` from `Lerp(800, 40000, CurveValue)`;
  - it set `SpringArm` relative pitch from
    `Lerp(-40, -55, CurveValue)`;
  - it set `FloatingPawnMovement.MaxSpeed` from
    `Lerp(1000, 6000, CurveValue)`;
  - it called native `Dof`;
  - it set `Camera.FieldOfView` from `Lerp(20, 15, CurveValue)`.
- Native C++ changes:
  - implemented `ACropoutPlayerPawn::UpdateZoom`;
  - added reflected float getter/setter helpers for BP-owned zoom state;
  - used native `UCurveFloat`, `USpringArmComponent`,
    `UFloatingPawnMovement`, and `UCameraComponent` APIs.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced two `Movement|UpdateZoom` call nodes with
    `Cropout|Player|Camera|UpdateZoom` in `EventGraph` and `Movement`.
- Removed one BP_Player function graph:
  - `Update Zoom`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerUpdateZoomNative.py`
  - `Saved/codex_migrate_bp_player_update_zoom_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_update_zoom_native_20260708_220105/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ body changes.
  - Migration script replaced 2 old `Movement|UpdateZoom` call nodes and
    confirmed zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph`.
  - Fresh BP_Player full read at local 2026-07-08 22:02:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 17;
    - `Update Zoom` is no longer present;
    - old `Movement|UpdateZoom` call nodes are gone;
    - native `Cropout|Player|Camera|UpdateZoom` call nodes are present.
  - Fresh full Blueprint audit at local 2026-07-08 22:03:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=17`
    - `implemented_function_count=16`

### BP Player Create Build Overlay Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Create Build Overlay` graph before editing:
  - it only created a new overlay when BP-owned `SpawnOverlay` was not valid;
  - it added a runtime static-mesh cube component with scale derived from
    `Spawn.GetActorBounds().BoxExtent / 50.0`;
  - it stored the component in BP-owned `SpawnOverlay`;
  - it attached the overlay to the current `Spawn` mesh with location
    `SnapToTarget`, rotation `KeepWorld`, and scale `KeepWorld`;
  - it finally called the still-BP-owned `Update Build Asset` graph.
- Native C++ changes:
  - implemented `ACropoutPlayerPawn::CreateBuildOverlay`;
  - added a small component lookup helper for the current `Spawn` mesh;
  - creates/registers a runtime `UStaticMeshComponent` using the engine cube
    mesh and stores it in BP-owned `SpawnOverlay`;
  - calls `UpdateBuildAsset` by `ProcessEvent` so this slice keeps using the
    existing BP graph until that graph is migrated.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced one `BuildMode|CreateBuildOverlay` call node with
    `Cropout|Player|Build|CreateBuildOverlay` in `EventGraph`.
- Removed one BP_Player function graph:
  - `Create Build Overlay`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerCreateBuildOverlayNative.py`
  - `Saved/codex_migrate_bp_player_create_build_overlay_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_create_build_overlay_native_20260708_221019/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ body changes.
  - Migration script replaced 1 old `BuildMode|CreateBuildOverlay` call node
    and confirmed zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph`.
  - Fresh BP_Player full read at local 2026-07-08 22:11:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 16;
    - `Create Build Overlay` is no longer present;
    - old `BuildMode|CreateBuildOverlay` call nodes are gone;
    - native `Cropout|Player|Build|CreateBuildOverlay` call node is present.
  - Fresh full Blueprint audit at local 2026-07-08 22:11:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=16`
    - `implemented_function_count=15`

### BP Player Switch Build Mode Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Switch Build Mode` graph before editing:
  - it took a bool parameter `Switch To Build Mode?`;
  - when true, it removed `IMC_Villager_Mode` and added `IMC_BuildMode`;
  - when false, it removed `IMC_BuildMode` and added
    `IMC_Villager_Mode`;
  - all mapping context operations used
    `bIgnoreAllPressedKeysUntilRelease=True`,
    `bForceImmediately=False`, and `bNotifyUserSettings=False`.
- Native C++ changes:
  - changed `ACropoutPlayerPawn::SwitchBuildMode` from `int32` to `bool`
    to match the BP graph parameter;
  - implemented the mapping-context swap through
    `UEnhancedInputLocalPlayerSubsystem`;
  - kept the existing UI reflection caller compatible because it already
    writes a bool-like first parameter.
- Removed one BP_Player function graph:
  - `Switch Build Mode`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerSwitchBuildModeNative.py`
  - `Saved/codex_migrate_bp_player_switch_build_mode_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_switch_build_mode_native_20260708_221605/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - Migration script confirmed zero old `SwitchBuildMode` BP-local call nodes
    before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph` plus
    `BlueprintEditorLibrary.remove_graph`.
  - Fresh BP_Player full read at local 2026-07-08 22:16:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 15;
    - `Switch Build Mode` is no longer present.
  - Fresh full Blueprint audit at local 2026-07-08 22:17:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=15`
    - `implemented_function_count=14`

### BP Player Closest Hover Check Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Closest Hover Check` graph before editing:
  - it read BP-owned `Collision`, `HoverActor`, and `NewHover`;
  - when `Collision.GetOverlappingActors(Actor)` returned no actors, it
    paused the `"Closest Hover Check"` timer and cleared `HoverActor`;
  - otherwise it selected the overlapping actor closest to the `Collision`
    component world location;
  - it updated `HoverActor` only when the chosen closest actor changed.
- Native C++ changes:
  - implemented `ACropoutPlayerPawn::ClosestHoverCheck`;
  - kept the existing BP-owned variable storage by reading/writing
    `Collision`, `NewHover`, and `HoverActor` through reflected properties;
  - kept the existing timer behavior by calling
    `UKismetSystemLibrary::K2_PauseTimer(this, TEXT("Closest Hover Check"))`
    on the no-overlap path.
- Removed one BP_Player function graph:
  - `Closest Hover Check`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerClosestHoverCheckNative.py`
  - `Saved/codex_migrate_bp_player_closest_hover_check_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_closest_hover_check_native_20260708_222435/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ body change.
  - Migration script confirmed zero old `ClosestHoverCheck` BP-local call
    nodes before graph deletion.
  - Migration script found one remaining function-name string reference in
    `EventGraph`, which is the existing timer setup
    `SetTimerbyFunctionName("Closest Hover Check")`.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph`.
  - Fresh BP_Player full read at local 2026-07-08 22:26:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 14;
    - `Closest Hover Check` is no longer present.
  - Fresh full Blueprint audit at local 2026-07-08 22:26:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=14`
    - `implemented_function_count=13`

### BP Player Input Switch Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Input Switch` graph before editing:
  - it took one `E_InputType` enum parameter named `New Input`;
  - it wrote BP-owned `InputType`;
  - enum value `NewEnumerator1` showed the BP-owned `Cursor` component;
  - enum value `NewEnumerator2` moved BP-owned `Collision` to relative
    location `0,0,10`;
  - enum value `NewEnumerator3` moved BP-owned `Collision` to relative
    location `0,0,-500`.
- Native C++ changes:
  - changed `ACropoutPlayerPawn::InputSwitch` from
    `const FInputActionValue&` to `uint8 NewInput` so it matches the native
    `BP_PC.KeySwitch` dispatcher contract;
  - removed the now-unused `InputActionValue.h` include from
    `CropoutPlayerPawn.h`;
  - added reflected `uint8`/enum property writing for BP-owned `InputType`;
  - implemented the same cursor visibility and collision relative-location
    switch cases in C++.
- Retargeted the EventGraph delegate binding before removing the graph:
  - changed the `K2Node_CreateDelegate` used by
    `Bind Event to Key Switch` from the BP function name
    `"Input Switch"` to the native function name `InputSwitch` using
    `BlueprintTools.set_create_event_function`.
- Removed one BP_Player function graph:
  - `Input Switch`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerInputSwitchNative.py`
  - `Saved/codex_migrate_bp_player_input_switch_native.json`
  - `Saved/CodexProbeBPPlayerCreateDelegate.py`
  - `Saved/codex_probe_bp_player_create_delegate.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_input_switch_native_20260708_223430/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - Initial graph deletion without retargeting produced a BP compiler
    signature error for the old `"Input Switch"` Create Event; that attempt
    was not saved.
  - After retargeting the Create Event to `InputSwitch`, `BP_Player`
    compiled with `BS_UP_TO_DATE` and saved.
  - Idempotent verification confirmed the KeySwitch Create Event is now bound
    to `InputSwitch`.
  - Fresh BP_Player full read at local 2026-07-08 22:37:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 13;
    - `Input Switch` is no longer present.
  - Fresh full Blueprint audit at local 2026-07-08 22:37:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=13`
    - `implemented_function_count=12`

### BP Player Corners in Nav Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Corners in Nav` graph before editing:
  - it read BP-owned `Spawn`;
  - it used the current spawn actor's `Box` collision component bounds;
  - it built four corner points using `BoxExtent.X/Y * 1.05`;
  - it ran four vertical `MultiLineTraceByChannel` checks from `Z=100`
    to `Z=-1` on `TraceTypeQuery1`, ignoring the spawn actor;
  - it wrote BP-owned `OnIsland` and returned the boolean result to
    `Update Build Asset`.
- Native C++ changes:
  - changed `ACropoutPlayerPawn::CornersInNav` from `void` to `bool`;
  - added reflected bool property writing for BP-owned `OnIsland`;
  - implemented the four-corner line trace checks in C++;
  - uses the current spawn actor's `Box` component and ignores the spawn
    actor during trace, matching the BP graph.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced one `BuildMode|CornersinNav` call node in
    `Update Build Asset` with native
    `Cropout|Player|Build|CornersInNav`;
  - preserved the exec chain and boolean return connection into `CanDrop`.
- Removed one BP_Player function graph:
  - `Corners in Nav`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerCornersInNavNative.py`
  - `Saved/codex_migrate_bp_player_corners_in_nav_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_corners_in_nav_native_20260708_224455/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - Migration script replaced 1 old `BuildMode|CornersinNav` call node and
    confirmed zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after
    `BlueprintTools.remove_function_graph`.
  - Fresh BP_Player full read at local 2026-07-08 22:46:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 12;
    - `Corners in Nav` is no longer present;
    - native `Cropout|Player|Build|CornersInNav` call node is present.
  - Fresh full Blueprint audit at local 2026-07-08 22:46:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=12`
    - `implemented_function_count=11`

### BP Player Villager Overlap Check Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Villager Overlap Check` graph before editing:
  - it read BP-owned `Collision`;
  - it called `GetOverlappingActors` with the `/Script/Engine.Pawn`
    class filter;
  - it used the first overlapping actor as `Output`;
  - it routed `Is Valid` when that first actor was valid, otherwise routed
    `Is Not Valid`.
- Native C++ changes:
  - added `ACropoutPlayerPawn::VillagerOverlapCheck`;
  - exposed it with `ExpandBoolAsExecs=ReturnValue`, so Blueprint gets
    `True`/`False` exec outputs matching the old valid/invalid branches;
  - returns the first overlapping pawn actor through the `Output` out
    parameter.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced one `Utilities|VillagerOverlapCheck` macro node in
    `EventGraph` with native
    `Cropout|Player|Villager|VillagerOverlapCheck`;
  - mapped old `Is Valid` to native `True`;
  - mapped old `Is Not Valid` to native `False`;
  - preserved the old `Output` actor connection.
- Removed one BP_Player function graph:
  - `Villager Overlap Check`
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerVillagerOverlapCheckNode.py`
  - `Saved/codex_probe_bp_player_villager_overlap_check_node.json`
  - `Saved/CodexMigrateBPPlayerVillagerOverlapCheckNative.py`
  - `Saved/codex_migrate_bp_player_villager_overlap_check_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_villager_overlap_check_native_20260708_225427/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - No-save probe confirmed the native node pins are `execute`, `True`,
    `False`, and `Output`.
  - Migration script replaced 1 old `Utilities|VillagerOverlapCheck` node
    and confirmed zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 22:55:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 11;
    - `Villager Overlap Check` is no longer present;
    - native `Cropout|Player|Villager|VillagerOverlapCheck` call node is
      present.
  - Fresh full Blueprint audit at local 2026-07-08 22:55:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=11`
    - `implemented_function_count=10`

### BP Player Single Touch Check Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Single Touch Check` graph before editing:
  - it read player controller touch state for `ETouchIndex::Touch2`;
  - it inverted `bIsCurrentlyPressed`;
  - not pressed routed to the `Single` exec output;
  - pressed routed to the `Double` exec output.
- Native C++ changes:
  - added `ACropoutPlayerPawn::SingleTouchCheck`;
  - exposed it with `ExpandBoolAsExecs=ReturnValue`;
  - returns `true` when Touch2 is not pressed and `false` when Touch2 is
    currently pressed.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced two `Utilities|SingleTouchCheck` macro nodes, one in
    `EventGraph` and one in `Movement`;
  - mapped old `Single` to native `True`;
  - mapped old `Double` to native `False`.
- Removed one BP_Player function graph:
  - `Single Touch Check`
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerSingleTouchCheckNode.py`
  - `Saved/codex_probe_bp_player_single_touch_check_node.json`
  - `Saved/CodexMigrateBPPlayerSingleTouchCheckNative.py`
  - `Saved/codex_migrate_bp_player_single_touch_check_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_single_touch_check_native_20260708_230156/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - No-save probe confirmed the native node pins are `execute`, `True`,
    and `False`.
  - Migration script replaced 2 old `Utilities|SingleTouchCheck` nodes and
    confirmed zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:03:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 10;
    - `Single Touch Check` is no longer present;
    - native `Cropout|Player|Input|SingleTouchCheck` call nodes are
      present.
  - Fresh full Blueprint audit at local 2026-07-08 23:03:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=10`
    - `implemented_function_count=9`

### BP Player Stepped Loc & Rot Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Stepped Loc & Rot` graph before editing:
  - it read BP-owned `Collision` and converted its world location through
    `Cropout|Grid|ConvertToSteppedPos`;
  - it read BP-owned `StepRotation`;
  - it quantized the pawn yaw using
    `floor((Yaw / 360) * StepRotation) / StepRotation * 360`;
  - it rotated the X axis by that quantized yaw and built the output rotator
    with `MakeRotFromXZ` using the pawn up vector.
- Native C++ changes:
  - added `ACropoutPlayerPawn::SteppedLocAndRot`;
  - returns the stepped `Location` and quantized `Rotation` through out
    parameters;
  - preserves BP-owned state access via reflection for `Collision` and
    `StepRotation`.
- Call-site audit before removing the graph:
  - scanned current BP_Player nodes and found zero external calls to
    `Stepped Loc & Rot`;
  - no Blueprint node replacement was needed.
- Removed one BP_Player function graph:
  - `Stepped Loc & Rot`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerSteppedLocAndRotNative.py`
  - `Saved/codex_migrate_bp_player_stepped_loc_and_rot_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_stepped_loc_and_rot_native_20260708_231118/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - Migration script confirmed zero matching old call nodes before removal.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:12:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 9;
    - `Stepped Loc & Rot` is no longer present.
  - Fresh full Blueprint audit at local 2026-07-08 23:12:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=9`
    - `implemented_function_count=8`

### BP Player Cursor Dist From Viewport Center Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Cursor dist from viewport center` graph before
  editing:
  - it took input `A` as a `Vector2D`;
  - it read viewport size from the player controller;
  - it read BP-owned `EdgeMoveDistance` and `InputType`;
  - it selected an input-type multiplier of `0`, `1`, `2`, or `2`;
  - it calculated how far the cursor was past the active edge threshold;
  - it output `Direction` as `(-sign(A.Y) * YAmount,
    sign(A.X) * XAmount, 0)`;
  - `Strength` was not connected internally and stayed at default `1.0`.
- Native C++ changes:
  - added reflected enum/byte property reading for BP-owned `InputType`;
  - added `ACropoutPlayerPawn::CursorDistFromViewportCenter`;
  - exposed it as a `BlueprintPure` native helper with matching `A`,
    `Direction`, and double-precision `Strength` pins.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced one `Utilities|CursorDistFromViewportCenter` macro node in
    `Edge Move` with native
    `Cropout|Player|Cursor|CursorDistFromViewportCenter`;
  - preserved the input `A`, output `Direction`, and output `Strength`
    connections.
- Removed one BP_Player function graph:
  - `Cursor dist from viewport center`
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerCursorDistNode.py`
  - `Saved/codex_probe_bp_player_cursor_dist_node.json`
  - `Saved/CodexMigrateBPPlayerCursorDistNative.py`
  - `Saved/codex_migrate_bp_player_cursor_dist_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_cursor_dist_native_20260708_231827/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - No-save probe confirmed the native node pins are `A`, `Direction`, and
    double-precision `Strength`.
  - Migration script replaced 1 old `Utilities|CursorDistFromViewportCenter`
    node and confirmed zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:19:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 8;
    - `Cursor dist from viewport center` is no longer present;
    - native `Cropout|Player|Cursor|CursorDistFromViewportCenter` call node
      is present in `Edge Move`.
  - Fresh full Blueprint audit at local 2026-07-08 23:19:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=8`
    - `implemented_function_count=7`

### BP Player Edge Move Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Edge Move` graph before editing:
  - it called `Project Mouse/Touch1 To Ground Plane` only for the
    `Screen Pos` output;
  - it read viewport size and built `ViewportSize / 2`;
  - it called native `CursorDistFromViewportCenter` with
    `Screen Pos - ViewportCenter`;
  - it transformed the local edge-move direction by the pawn actor transform;
  - it forwarded the cursor-distance `Strength` output unchanged.
- Native C++ changes:
  - added `ACropoutPlayerPawn::EdgeMove`;
  - reimplemented the required `Screen Pos` selection from
    `Project Mouse/Touch1 To Ground Plane`:
    - input type 0 uses viewport center;
    - input type 1 uses mouse position when available, otherwise viewport
      center;
    - input type 2 uses viewport center;
    - input type 3 uses Touch1 position while pressed, otherwise viewport
      center;
  - reused native `CursorDistFromViewportCenter`;
  - used `UKismetMathLibrary::TransformDirection` to match the BP
    `TransformDirection` node.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced one `Utilities|EdgeMove` macro node in `Move Tracking` with
    native `Cropout|Player|Movement|EdgeMove`;
  - preserved output `Direction` and output `Strength` connections.
- Removed one BP_Player function graph:
  - `Edge Move`
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerEdgeMoveNode.py`
  - `Saved/codex_probe_bp_player_edge_move_node.json`
  - `Saved/CodexMigrateBPPlayerEdgeMoveNative.py`
  - `Saved/codex_migrate_bp_player_edge_move_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_edge_move_native_20260708_232506/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - No-save probe confirmed the native node pins are `Direction` and
    double-precision `Strength`.
  - Migration script replaced 1 old `Utilities|EdgeMove` node and confirmed
    zero old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:27:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 7;
    - `Edge Move` is no longer present;
    - native `Cropout|Player|Movement|EdgeMove` call node is present in
      `Move Tracking`.
  - Fresh full Blueprint audit at local 2026-07-08 23:27:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=7`
    - `implemented_function_count=6`

### BP Player Project Mouse Touch1 Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Project Mouse/Touch1 To Ground Plane` behavior before
  editing:
  - it selected `Screen Pos` from viewport center, mouse position, or Touch1
    position based on BP-owned `InputType`;
  - it selected `ReturnValue` from mouse success, literal true for input type
    2, Touch1 pressed, or false for input type 0/default;
  - it deprojected the selected screen position with player controller 0;
  - it intersected the deprojected line with the world Z=0 plane;
  - it added a touch fallback Z offset of `-500` only for input type 3 when
    Touch1 was not pressed.
- Native C++ changes:
  - added `ACropoutPlayerPawn::ProjectMouseTouch1ToGroundPlane`;
  - exposed it as a `BlueprintPure` native helper with outputs
    `ScreenPos`, `Intersection`, and `ReturnValue`;
  - used `UGameplayStatics::DeprojectScreenToWorld` plus
    `FMath::LinePlaneIntersection` for the old BP deproject/plane trace path.
- Retargeted old BP-local call nodes before removing the graph:
  - replaced one `Utilities|ProjectMouse/Touch1toGroundPlane` node in
    `Move Tracking`;
  - replaced one old node in `Update Build Asset`;
  - replaced one old node in `Update Cursor Position`;
  - preserved the old `Screen Pos`, `Intersection`, and `ReturnValue`
    output links on all three replacements.
- Removed one BP_Player function graph:
  - `Project Mouse/Touch1 To Ground Plane`
  - `BlueprintTools.remove_function_graph` logged a warning for this slash
    graph name, then the script fallback removed it with
    `BlueprintEditorLibrary.remove_graph`.
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerProjectMouseNode.py`
  - `Saved/codex_probe_bp_player_project_mouse_node.json`
  - `Saved/CodexMigrateBPPlayerProjectMouseNative.py`
  - `Saved/codex_migrate_bp_player_project_mouse_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_project_mouse_native_20260708_233443/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ signature/body
    changes.
  - No-save probe confirmed the native node type
    `Cropout|Player|Cursor|ProjectMouseTouch1ToGroundPlane` can be created
    and has outputs `ScreenPos`, `Intersection`, and `ReturnValue`.
  - Migration script replaced 3 old
    `Utilities|ProjectMouse/Touch1toGroundPlane` nodes and confirmed zero
    old nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:35:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 6;
    - `Project Mouse/Touch1 To Ground Plane` is no longer present;
    - native `Cropout|Player|Cursor|ProjectMouseTouch1ToGroundPlane` call
      nodes are present in `Move Tracking`, `Update Build Asset`, and
      `Update Cursor Position`.
  - Fresh full Blueprint audit at local 2026-07-08 23:35:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=6`
    - `implemented_function_count=5`
  - Final `CropoutSampleProjectEditor Win64 Development` build passed.

### BP Player Nav Mesh Bound Check Residual Graph Removal

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Re-read the current `Nav Mesh Bound Check` graph before editing:
  - it was an uncalled helper/debug graph with `Inputs`/`Outputs` tunnel
    nodes;
  - it read `Spawn -> Box`, calculated four bounds-derived positions, and
    checked them with `IsValidAILocation`;
  - it also assembled debug position/success arrays and an integer count;
  - the current BP_Player export had no call/reference nodes pointing to this
    graph outside the graph itself.
- Runtime behavior note:
  - the live placement path already calls native
    `Cropout|Player|Build|CornersInNav` from `Update Build Asset`;
  - because `Nav Mesh Bound Check` had zero references, this pass removed the
    residual graph instead of adding another unused C++ entry point.
- Removed one BP_Player residual graph:
  - `Nav Mesh Bound Check`
  - `BlueprintTools.remove_function_graph` logged a warning for this graph
    name, then the script fallback removed it with
    `BlueprintEditorLibrary.remove_graph`.
- Migration scripts/results:
  - `Saved/CodexRemoveBPPlayerNavMeshBoundCheckGraph.py`
  - `Saved/codex_remove_bp_player_nav_mesh_bound_check_graph.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_nav_mesh_bound_check_remove_20260708_234048/BP_Player.uasset`
- Validation after this pass:
  - The removal script confirmed `reference_count=0` before deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:41:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 5;
    - remaining graphs are `Update Build Asset`, `Update Cursor Position`,
      `Move Tracking`, `EventGraph`, and `Movement`;
    - `Nav Mesh Bound Check` and its old `AI|IsValidAILocation` nodes are no
      longer present.
  - Fresh full Blueprint audit at local 2026-07-08 23:41:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=5`
    - `implemented_function_count=4`
  - Final `CropoutSampleProjectEditor Win64 Development` build passed.

### BP Player Update Cursor Position Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated `Update Cursor Position` from a BP function graph into native
  `ACropoutPlayerPawn::UpdateCursorPosition`.
- Native behavior implemented in this pass:
  - reads BP `InputType`, `Target`, `HoverActor`, `Collision`, and `Cursor`
    properties through the existing BP-property bridge pattern;
  - enum value 0 returns without changing cursor state;
  - enum values 1 and 2 follow the same validated `HoverActor` path as the BP
    graph:
    - valid hover actor uses `GetActorBounds(true, ..., false)`, moves the
      target to `(Origin.X, Origin.Y, 20)`, and applies the old pulse scale
      `max(abs(BoxExtent.X), abs(BoxExtent.Y)) / 50 + sin(Time * 5) * 0.25`;
    - invalid hover actor falls back to the `Collision` component world
      location/rotation with scale `(2, 2, 1)`;
  - enum value 3 uses native `ProjectMouseTouch1ToGroundPlane`, falls back to
    the old target X/Y at Z=-100 when projection fails, and uses scale
    `(2, 2, 1)`;
  - writes the BP `Target` transform property and applies
    `UKismetMathLibrary::TInterpTo(..., WorldDeltaSeconds, 12)` to the `Cursor`
    world transform.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed one BP_Player function graph:
  - `Update Cursor Position`
- Replaced one old BP-local call node:
  - from `Movement|UpdateCursorPosition`
  - to `Cropout|Player|Cursor|UpdateCursorPosition`
  - graph: `Move Tracking`
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerUpdateCursorPositionNode.py`
  - `Saved/codex_probe_bp_player_update_cursor_position_node.json`
  - `Saved/CodexMigrateBPPlayerUpdateCursorPositionNative.py`
  - `Saved/codex_migrate_bp_player_update_cursor_position_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_update_cursor_position_native_20260708_234841/BP_Player.uasset`
- Validation after this pass:
  - Initial C++ build caught the wrong helper name
    `UGameplayStatics::GetGameTimeInSeconds`; fixed to
    `UGameplayStatics::GetTimeSeconds`.
  - Full editor target build passed after the native C++ changes.
  - No-save probe confirmed the native node type
    `Cropout|Player|Cursor|UpdateCursorPosition` can be created with exec/then
    pins.
  - Migration script replaced 1 old `Movement|UpdateCursorPosition` call and
    confirmed zero old call nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:49:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 4;
    - remaining graphs are `Update Build Asset`, `Move Tracking`,
      `EventGraph`, and `Movement`;
    - `Update Cursor Position` is no longer present;
    - `Move Tracking` now calls native
      `Cropout|Player|Cursor|UpdateCursorPosition`.
  - Fresh full Blueprint audit at local 2026-07-08 23:49:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=4`
    - `implemented_function_count=3`
  - Final `CropoutSampleProjectEditor Win64 Development` build passed.

### BP Player Move Tracking Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated `Move Tracking` from a BP function graph into native
  `ACropoutPlayerPawn::MoveTracking`.
- Native behavior implemented in this pass:
  - adds the same center-return movement input:
    - direction `(-Normalize(GetActorLocation).X, -Normalize(GetActorLocation).Y, 0)`;
    - scale `max((VectorLength(GetActorLocation) - 9000) / 5000, 0)`;
  - calls the already-native `EdgeMove` once and applies its direction/strength
    through `AddMovementInput`;
  - calls the already-native `ProjectMouseTouch1ToGroundPlane` once;
  - for `InputType` 0, 1, and 2, moves `Collision` to
    `Intersection + (0, 0, 10)`;
  - for `InputType` 3, moves `Collision` to `Intersection` when projection
    succeeds, otherwise interpolates current collision location toward the
    same X/Y at Z=-500 with speed 12;
  - calls the already-native `UpdateCursorPosition` after collision movement.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed one BP_Player function graph:
  - `Move Tracking`
- Retargeted one timer delegate:
  - from BP graph function `Move Tracking`
  - to native C++ function `MoveTracking`
  - node: `EventGraph.K2Node_CreateDelegate_1`
- Migration scripts/results:
  - `Saved/CodexInspectBPPlayerCreateEventFunctions.py`
  - `Saved/codex_inspect_bp_player_create_event_functions.json`
  - `Saved/CodexMigrateBPPlayerMoveTrackingNative.py`
  - `Saved/codex_migrate_bp_player_move_tracking_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_move_tracking_native_20260708_235432/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Read-only delegate inspection confirmed the timer `CreateEvent` was bound
    to `Move Tracking` and that native `MoveTracking` was compatible.
  - Migration script retargeted 1 `CreateEvent` from `Move Tracking` to
    `MoveTracking`.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:55:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 3;
    - remaining graphs are `Update Build Asset`, `EventGraph`, and
      `Movement`;
    - function info count is now 1;
    - the only remaining ordinary function graph is `Update Build Asset`.
  - Fresh full Blueprint audit at local 2026-07-08 23:55:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=3`
    - `implemented_function_count=2`
  - Final `CropoutSampleProjectEditor Win64 Development` build passed.

### BP Player Update Build Asset Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated `Update Build Asset` from a BP function graph into native
  `ACropoutPlayerPawn::UpdateBuildAsset`.
- Native behavior implemented in this pass:
  - calls native `ProjectMouseTouch1ToGroundPlane` and returns immediately
    when projection fails;
  - uses the BP `Spawn` object reference and returns when the validated spawn
    path would have no executable `else`;
  - snaps the projected intersection through
    `UCropoutBlueprintLibrary::ConvertToSteppedPos`;
  - interpolates `Spawn` toward the stepped position with
    `UKismetMathLibrary::VInterpTo(..., WorldDeltaSeconds, 18)`;
  - checks `Spawn` overlaps against `ACropoutInteractable::StaticClass()`;
  - sets BP `CanDrop` to `CornersInNav()` only when no interactable overlap
    exists, otherwise false;
  - updates `/Game/Blueprint/Core/Extras/MPC_Cropout.MPC_Cropout`
    parameter `Target Position` with `(X, Y, Z, CanDropAsFloat)`, matching the
    original graph's fixed material parameter collection reference.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed one BP_Player function graph:
  - `Update Build Asset`
- Replaced one old BP-local call node:
  - from `BuildMode|UpdateBuildAsset`
  - to `Cropout|Player|Build|UpdateBuildAsset`
  - graph: `EventGraph`
- Migration scripts/results:
  - `Saved/CodexProbeBPPlayerUpdateBuildAssetNode.py`
  - `Saved/codex_probe_bp_player_update_build_asset_node.json`
  - `Saved/CodexMigrateBPPlayerUpdateBuildAssetNative.py`
  - `Saved/codex_migrate_bp_player_update_build_asset_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_update_build_asset_native_20260708_235900/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - No-save probe confirmed the native node type
    `Cropout|Player|Build|UpdateBuildAsset` can be created with exec/then
    pins.
  - Migration script replaced 1 old `BuildMode|UpdateBuildAsset` call and
    confirmed zero old call nodes remained before graph deletion.
  - `BP_Player` compiled with `BS_UP_TO_DATE` and saved after graph removal.
  - Fresh BP_Player full read at local 2026-07-08 23:59:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count is now 2;
    - remaining graphs are `EventGraph` and `Movement`;
    - function info count is now 0;
    - `Update Build Asset` is no longer present;
    - `EventGraph` now calls native
      `Cropout|Player|Build|UpdateBuildAsset`.
  - Fresh full Blueprint audit at local 2026-07-08 23:59:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=2`
    - `implemented_function_count=1`

### BP Player Enhanced Input Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated the remaining BP-side Enhanced Input event nodes into native
  `ACropoutPlayerPawn::SetupPlayerInputComponent`.
- Native behavior implemented in this pass:
  - binds `IA_Move` Triggered to forward/right `AddMovementInput`;
  - binds `IA_Spin` Triggered to yaw `AddActorLocalRotation`;
  - binds `IA_Zoom` Triggered to set BP property `ZoomDirection`, then call
    `UpdateZoom()` and `Dof()`;
  - binds `IA_Build_Move` Triggered to `UpdateBuildAsset()`;
  - binds `IA_Build_Move` Completed to snap the current `Spawn` actor to the
    stepped grid position;
  - binds `IA_Villager` Started to `SingleTouchCheck()`, `PositionCheck()`,
    `VillagerOverlapCheck()`, then either `VillagerSelect()` or add
    `IMC_DragMove`;
  - binds `IA_Villager` Canceled/Completed to remove `IMC_DragMove`, send the
    old `Action(NewParam=HoverActor)` interface message to the selected actor,
    then call `VillagerRelease()`;
  - binds `IA_DragMove` Triggered to either `TrackMove()` or remove
    `IMC_DragMove`, matching the old single-touch branch.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.h`
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed remaining BP_Player graph logic:
  - deleted 25 exact nodes from `EventGraph` for `IA_Build_Move` and
    `IA_Villager`;
  - deleted 18 exact nodes from `Movement` for `IA_Move`, `IA_Spin`,
    `IA_Zoom`, and `IA_DragMove`;
  - removed the now-empty `Movement` graph.
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerEnhancedInputNative.py`
  - `Saved/codex_migrate_bp_player_enhanced_input_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_enhanced_input_native_20260709_003202/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script deleted 43 exact Enhanced Input graph nodes, compiled
    `BP_Player`, removed `Movement`, saved the asset, and reported no errors.
  - Fresh BP_Player full read at local 2026-07-09 00:32:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - status is `BS_UP_TO_DATE`;
    - remaining graph list is only `EventGraph`;
    - `EventGraph` DSL is empty;
    - `Movement` no longer exists.
  - Fresh full Blueprint audit at local 2026-07-09 00:32:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains project-native and has no implemented function graphs:
    - `graph_count=1`
    - `implemented_function_count=0`

### BP Player BeginBuild Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated the `Event BeginBuild` build-placement chain out of the BP event
  graph into native `ACropoutPlayerPawn::BeginBuildNative`.
- Important implementation note:
  - a first same-name native `UFUNCTION BeginBuild(...)` build passed C++, but
    `BP_Player` compilation rejected it because the Blueprint interface
    `BPI_Player_C::BeginBuild` has a different parent-function signature;
  - the final implementation intentionally uses `BeginBuildNative(...)` and
    routes `UCropoutBuildItemButton` directly to it when the controlled pawn is
    `ACropoutPlayerPawn`;
  - the old reflection-based `BeginBuild` call path remains as a fallback for
    any non-native pawn.
- Native behavior implemented in this pass:
  - stores the selected build class into BP property `TargetSpawnClass`;
  - stores the build cost map into BP property `ResourceCost`, converting the
    numeric enum keys to the BP map storage;
  - destroys an existing placement `Spawn` actor when one is present;
  - spawns the selected build class at the player pawn location using
    `ESpawnActorCollisionHandlingMethod::Undefined`;
  - writes the new placement actor back to BP property `Spawn`;
  - calls `PlacementMode` on the spawned actor;
  - calls native `CreateBuildOverlay()`.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.h`
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
  - `Source/CropoutSampleProject/CropoutMenuWidgets.cpp`
- Removed one BP_Player event-chain from `EventGraph`:
  - `Event BeginBuild`
  - `Set Target Spawn Class`
  - `Set Resource Cost`
  - `Is Valid`
  - `Destroy Actor`
  - `SpawnActor`
  - `Set Spawn`
  - `Placement Mode`
  - `CreateBuildOverlay`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerBeginBuildNative.py`
  - `Saved/codex_migrate_bp_player_begin_build_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_begin_build_native_20260709_002619/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script deleted 13 exact BeginBuild-chain nodes, compiled
    `BP_Player`, saved the asset, and reported no errors.
  - Fresh BP_Player full read at local 2026-07-09 00:26:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - status is `BS_UP_TO_DATE`;
    - remaining graphs are `EventGraph` and `Movement`;
    - `Event BeginBuild` is no longer present in `EventGraph`;
    - remaining `EventGraph` entries are `EnhancedInputActionIA_Build_Move`
      and `EnhancedInputActionIA_Villager`.
  - Fresh full Blueprint audit at local 2026-07-09 00:27:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains a project-native asset with graphs:
    - `graph_count=2`
    - `implemented_function_count=1`

### BP Player Possessed Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated the `Event Possessed` chain from the BP event graph into native
  `ACropoutPlayerPawn::PossessedBy`.
- Native behavior implemented in this pass:
  - calls `Super::PossessedBy(NewController)`;
  - casts `NewController` to `ACropoutPlayerController`;
  - binds `ACropoutPlayerController::KeySwitch` to native
    `ACropoutPlayerPawn::InputSwitch` with `AddUniqueDynamic`.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.h`
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed one BP_Player event-chain from `EventGraph`:
  - `Event Possessed`
  - `Cast To BP_PC`
  - `Bind Event to Key Switch`
  - `Create Event`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerPossessedNative.py`
  - `Saved/codex_migrate_bp_player_possessed_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_possessed_native_20260709_001208/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script deleted 4 exact Possessed-chain nodes, compiled
    `BP_Player`, saved the asset, and reported no errors.
  - Fresh BP_Player full read confirmed `Event Possessed` is no longer present.
  - Fresh full Blueprint audit reported compile failures remain zero.

### BP Player ActorBeginOverlap Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated the `Event ActorBeginOverlap` hover bootstrap chain from the BP
  event graph into native `ACropoutPlayerPawn::NotifyActorBeginOverlap`.
- Native behavior implemented in this pass:
  - calls `Super::NotifyActorBeginOverlap(OtherActor)`;
  - checks the BP `HoverActor` property with Unreal `IsValid`;
  - returns immediately when `HoverActor` is already valid;
  - otherwise sets BP `HoverActor` to `OtherActor`;
  - starts the existing `Closest Hover Check` timer at `0.01` seconds,
    looping, matching the original `Set Timer by Function Name` node.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.h`
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed one BP_Player event-chain from `EventGraph`:
  - `Event ActorBeginOverlap`
  - validated `Get HoverActor`
  - `Set HoverActor`
  - `Set Timer by Function Name`
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerActorBeginOverlapNative.py`
  - `Saved/codex_migrate_bp_player_actor_begin_overlap_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_actor_begin_overlap_native_20260709_001450/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script deleted 4 exact ActorBeginOverlap-chain nodes, compiled
    `BP_Player`, saved the asset, and reported no errors.
  - Fresh BP_Player full read at local 2026-07-09 00:15:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count remains 2;
    - remaining graphs are `EventGraph` and `Movement`;
    - function info count remains 0;
    - `Event BeginPlay`, `Event Possessed`, and `Event ActorBeginOverlap`
      are no longer present in `EventGraph`;
    - remaining `EventGraph` entries are
      `EnhancedInputActionIA_Build_Move`,
      `EnhancedInputActionIA_Villager`, and `EventBeginBuild`;
    - `Movement` still contains the empty-looking
      `EnhancedInputActionIA_Spin`, `IA_Move`, `IA_Zoom`, and
      `IA_DragMove` event nodes.
  - Fresh full Blueprint audit at local 2026-07-09 00:15:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=2`
    - `implemented_function_count=1`

### BP Player BeginPlay Native Slice

- Continued `/Game/Blueprint/Core/Player/BP_Player`.
- Migrated the remaining `Event BeginPlay` startup chain from the BP event
  graph into native `ACropoutPlayerPawn::BeginPlay`.
- Native behavior implemented in this pass:
  - calls native `UpdateZoom()` once at BeginPlay;
  - starts a looping native `MoveTracking` timer at `0.016667` seconds with an
    initial delay of `0.0`;
  - adds `/Game/Blueprint/Core/Player/Input/IMC_BaseInput.IMC_BaseInput`;
  - adds `/Game/Blueprint/Core/Player/Input/IMC_Villager_Mode.IMC_Villager_Mode`;
  - preserves the BP `FModifyContextOptions` values:
    `bIgnoreAllPressedKeysUntilRelease=True`,
    `bForceImmediately=False`, `bNotifyUserSettings=False`;
  - clears the native `MoveTracking` timer in `EndPlay`.
- C++ files touched:
  - `Source/CropoutSampleProject/CropoutPlayerPawn.h`
  - `Source/CropoutSampleProject/CropoutPlayerPawn.cpp`
- Removed one BP_Player event-chain from `EventGraph`:
  - `Event BeginPlay`
  - `Sequence`
  - `UpdateZoom`
  - `Set Timer by Event`
  - `Create Event` for the timer delegate
  - `GetPlayerController`
  - `Get EnhancedInputLocalPlayerSubsystem`
  - two `AddMappingContext` nodes
- Migration scripts/results:
  - `Saved/CodexMigrateBPPlayerBeginPlayNative.py`
  - `Saved/codex_migrate_bp_player_begin_play_native.json`
  - `Saved/CodexReadBPPlayerFull.py`
  - `Saved/codex_read_bp_player_full.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_begin_play_native_20260709_000800/BP_Player.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script deleted 9 exact BeginPlay-chain nodes, compiled
    `BP_Player`, saved the asset, and reported no errors.
  - Fresh BP_Player full read at local 2026-07-09 00:08:
    - parent is `/Script/CropoutSampleProject.CropoutPlayerPawn`;
    - graph count remains 2;
    - remaining graphs are `EventGraph` and `Movement`;
    - function info count remains 0;
    - `Event BeginPlay` is no longer present in `EventGraph`;
    - remaining `EventGraph` entries are `EventPossessed`,
      `EventActorBeginOverlap`, `EnhancedInputActionIA_Build_Move`,
      `EnhancedInputActionIA_Villager`, and `EventBeginBuild`.
  - Fresh full Blueprint audit at local 2026-07-09 00:09:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 31 assets currently use project C++ parents.
    - 39 assets still use engine or Blueprint parents.
    - 22 assets still have at least one graph.
    - 5 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - `BP_Player` remains the only project-native asset with graphs:
    - `graph_count=2`
    - `implemented_function_count=1`

### BTT StuckRecover Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_StuckRecover`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT.BTT_DefaultBT_C`;
  - only graph was `EventGraph`;
  - only local BP variable was `Recovery Position`;
  - graph body got `Recovery Position` from the blackboard, added
    `(0,0,45)`, called `SetActorLocation` on `ControlledPawn`, then
    returned task success/failure from the bool result.
- Added native parent `UCropoutStuckRecoverTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - resolves `RecoveryPosition` from the behavior tree blackboard asset;
  - gets the controlled pawn from the task owner AI controller;
  - moves the pawn to `Blackboard[RecoveryPosition] + FVector(0,0,45)`;
  - returns `Succeeded` when `SetActorLocation` succeeds, otherwise `Failed`.
- Removed BP-side task implementation from `BTT_StuckRecover`:
  - `Recovery Position` BP member variable removed with
    `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutStuckRecoverTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttStuckRecover.py`
  - `Saved/codex_inspect_btt_stuck_recover.json`
  - `Saved/CodexMigrateBttStuckRecoverNative.py`
  - `Saved/codex_migrate_btt_stuck_recover_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_stuck_recover_native_20260709_004159/BTT_StuckRecover.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_StuckRecover`, saved the asset, and
    reported no errors.
  - Fresh full Blueprint audit at local 2026-07-09 00:42:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 32 assets currently use project C++ parents.
    - 38 assets still use engine or Blueprint parents.
    - 21 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=21`;
    - `BTT_StuckRecover` is no longer in the remaining graph list.

### BTT PlayNiagara Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_PlayNiagara`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT.BTT_DefaultBT_C`;
  - only graph was `EventGraph`;
  - only local BP variable was `System`;
  - graph body spawned `System` attached to `ControlledPawn.RootComponent`
    and then returned task success unconditionally.
- Added native parent `UCropoutPlayNiagaraTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable `UNiagaraSystem* System`;
  - if `System`, `ControlledPawn`, and root component are valid, calls
    `UNiagaraFunctionLibrary::SpawnSystemAttached`;
  - preserves the BP node defaults:
    `Location=Zero`, `Rotation=Zero`, `KeepRelativeOffset`,
    `bAutoDestroy=false`, `bAutoActivate=true`, `PoolingMethod=None`,
    `bPreCullCheck=true`;
  - returns `Succeeded`, matching the original `FinishExecute true`.
- Removed BP-side task implementation from `BTT_PlayNiagara`:
  - `System` BP member variable removed with `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutPlayNiagaraTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttPlayNiagara.py`
  - `Saved/codex_inspect_btt_play_niagara.json`
  - `Saved/CodexMigrateBttPlayNiagaraNative.py`
  - `Saved/codex_migrate_btt_play_niagara_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_play_niagara_native_20260709_004814/BTT_PlayNiagara.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_PlayNiagara`, saved the asset, and reported
    no errors.
  - Fresh full Blueprint audit at local 2026-07-09 00:49:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 33 assets currently use project C++ parents.
    - 37 assets still use engine or Blueprint parents.
    - 20 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=20`;
    - `BTT_PlayNiagara` and `BTT_StuckRecover` are no longer in the remaining
      graph list.

### BTT FindBounds Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_FindBounds`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT.BTT_DefaultBT_C`;
  - only graph was `EventGraph`;
  - local BP variables were `Target`, `Additional Bounds`, and `BB Bound`;
  - `Additional Bounds` default was `20.0`;
  - graph body read `Target` as an actor, called `GetActorBounds`, used
    `min(BoxExtent.X, BoxExtent.Y) + Additional Bounds`, stored that float in
    `BB Bound`, and returned task success; invalid target returned task
    failure.
- Added native parent `UCropoutFindBoundsTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable blackboard selectors `Target` and `BBBound`;
  - exposes editable `float AdditionalBounds = 20.0f`;
  - resolves selectors from the behavior tree blackboard asset;
  - gets the target actor from the blackboard, calls `GetActorBounds(false, ...)`,
    writes `min(BoxExtent.X, BoxExtent.Y) + AdditionalBounds` to `BBBound`, and
    returns `Succeeded`;
  - returns `Failed` if the blackboard or target actor is invalid.
- Removed BP-side task implementation from `BTT_FindBounds`:
  - `Target`, `Additional Bounds`, and `BB Bound` BP member variables removed
    with `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutFindBoundsTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttFindBounds.py`
  - `Saved/codex_inspect_btt_find_bounds.json`
  - `Saved/CodexMigrateBttFindBoundsNative.py`
  - `Saved/codex_migrate_btt_find_bounds_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_find_bounds_native_20260709_005235/BTT_FindBounds.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_FindBounds`, saved the asset, and reported
    no errors.
  - Fresh full Blueprint audit at local 2026-07-09 00:55:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 34 assets currently use project C++ parents.
    - 36 assets still use engine or Blueprint parents.
    - 19 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=19`;
    - `BTT_FindBounds`, `BTT_PlayNiagara`, and `BTT_StuckRecover` are no
      longer in the remaining graph list.

### BTT TransferResource Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_TransferResource`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Script/AIModule.BTTask_BlueprintBase`;
  - only graph was `EventGraph`;
  - local BP variables were `Key`, `Give To`, and `Take From`;
  - `Key` was not connected in the EventGraph;
  - graph body checked that blackboard actors `Take From` and `Give To` were
    valid, collected `(Resource, Value)` from `Take From`, added that resource
    to `Give To`, and returned task success; invalid actor returned task
    failure.
- Added native parent `UCropoutTransferResourceTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable blackboard selectors `TakeFrom` and `GiveTo` with BP
    display names `Take From` and `Give To`;
  - resolves selectors from the behavior tree blackboard asset;
  - reads both blackboard values as actors and returns `Failed` if either is
    invalid;
  - calls `UCropoutBlueprintLibrary::CollectActorResource` on `TakeFrom` and
    `UCropoutBlueprintLibrary::AddActorResource` on `GiveTo`;
  - returns `Succeeded`, matching the original `FinishExecute true` path.
- Removed BP-side task implementation from `BTT_TransferResource`:
  - `Key`, `Give To`, and `Take From` BP member variables removed with
    `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutTransferResourceTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttTransferResource.py`
  - `Saved/codex_inspect_btt_transfer_resource.json`
  - `Saved/CodexMigrateBttTransferResourceNative.py`
  - `Saved/codex_migrate_btt_transfer_resource_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_transfer_resource_native_20260709_010210/BTT_TransferResource.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_TransferResource`, saved the asset, and
    reported no errors.
  - Fresh full Blueprint audit at local 2026-07-09 01:02:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 35 assets currently use project C++ parents.
    - 35 assets still use engine or Blueprint parents.
    - 18 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=18`;
    - `BTT_TransferResource`, `BTT_FindBounds`, `BTT_PlayNiagara`, and
      `BTT_StuckRecover` are no longer in the remaining graph list.

### BTT DeliverRessource Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_DeliverRessource`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Script/AIModule.BTTask_BlueprintBase`;
  - only graph was `EventGraph`;
  - local BP variables were `Key`, `Give To`, and `Take From`;
  - `Key` was not connected in the EventGraph;
  - graph body checked that blackboard actors `Take From` and `Give To` were
    valid, called `PlayVillagerDeliverAnim(ControlledPawn)`, delayed by that
    return value, collected `(Resource, Value)` from `Take From`, added that
    resource to the GameMode, and returned task success; invalid actor returned
    task failure.
- Added native parent `UCropoutDeliverResourceTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable blackboard selectors `TakeFrom` and `GiveTo` with BP
    display names `Take From` and `Give To`;
  - sets `bCreateNodeInstance = true`, matching UE's latent BT task pattern for
    timer-backed task instances;
  - resolves selectors from the behavior tree blackboard asset;
  - reads both blackboard values as actors and returns `Failed` if either is
    invalid;
  - calls `UCropoutBlueprintLibrary::PlayVillagerDeliverAnim` on the controlled
    pawn and waits for the returned duration;
  - after the delay, re-reads `TakeFrom` from the blackboard, calls
    `UCropoutBlueprintLibrary::CollectActorResource`, then calls
    `UCropoutBlueprintLibrary::AddGameModeResource`;
  - abort clears the pending timer.
- Removed BP-side task implementation from `BTT_DeliverRessource`:
  - `Key`, `Give To`, and `Take From` BP member variables removed with
    `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutDeliverResourceTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttDeliverRessource.py`
  - `Saved/codex_inspect_btt_deliver_ressource.json`
  - `Saved/CodexMigrateBttDeliverRessourceNative.py`
  - `Saved/codex_migrate_btt_deliver_ressource_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_deliver_ressource_native_20260709_010934/BTT_DeliverRessource.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_DeliverRessource`, saved the asset, and
    reported no errors.
  - Follow-up inspect after removing the transient runtime owner pointer from
    UPROPERTY exposure confirmed:
    - parent is `/Script/CropoutSampleProject.CropoutDeliverResourceTask`;
    - local BP variables are empty;
    - graph list is empty;
    - inherited task variables are only the native `TakeFrom` and `GiveTo`
      selectors plus engine BT base properties.
  - Fresh full Blueprint audit at local 2026-07-09 01:10:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 36 assets currently use project C++ parents.
    - 34 assets still use engine or Blueprint parents.
    - 17 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=17`;
    - `BTT_DeliverRessource`, `BTT_TransferResource`, `BTT_FindBounds`,
      `BTT_PlayNiagara`, and `BTT_StuckRecover` are no longer in the remaining
      graph list.

### BTT ProgressConstruction Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_ProgressConstruction`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT.BTT_DefaultBT_C`;
  - only graph was `EventGraph`;
  - local BP variable was `TargetBuild`;
  - graph body called `PlayVillagerWorkAnim(ControlledPawn, 1.0)`, delayed for
    `1.0`, got the blackboard actor from `TargetBuild`, cast it to
    `BP_Interactable`, called `Interact`, and returned task success;
  - when `Interact` returned `NewParam <= 0`, the BP graph then called
    `SetBlackboardValueAsObject(TargetBuild)` with the value pin unconnected,
    effectively clearing that object key.
- Added native parent `UCropoutProgressConstructionTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable blackboard selector `TargetBuild`;
  - sets `bCreateNodeInstance = true` for timer-backed latent BT execution;
  - calls `UCropoutBlueprintLibrary::PlayVillagerWorkAnim` with delay `1.0`;
  - waits one second, then re-reads the `TargetBuild` actor from the blackboard;
  - casts to `ACropoutInteractable`, calls `Interact`, and completes success;
  - clears `TargetBuild` when the interaction result is `<= 0`, matching the
    unconnected-value clear in the original BP graph;
  - abort clears the pending timer.
- Removed BP-side task implementation from `BTT_ProgressConstruction`:
  - `TargetBuild` BP member variable removed with
    `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutProgressConstructionTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttProgressConstruction.py`
  - `Saved/codex_inspect_btt_progress_construction.json`
  - `Saved/CodexMigrateBttProgressConstructionNative.py`
  - `Saved/codex_migrate_btt_progress_construction_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_progress_construction_native_20260709_011703/BTT_ProgressConstruction.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_ProgressConstruction`, saved the asset, and
    reported no errors.
  - Migration result confirmed:
    - parent is `/Script/CropoutSampleProject.CropoutProgressConstructionTask`;
    - local BP variables are empty;
    - graph list is empty;
    - inherited task variables are only the native `TargetBuild` selector plus
      engine BT base properties.
  - Fresh full Blueprint audit at local 2026-07-09 01:17:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 37 assets currently use project C++ parents.
    - 33 assets still use engine or Blueprint parents.
    - 16 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=16`;
    - remaining BTT assets with graphs are `BTT_DefaultBT`,
      `BTT_InitialCollectResource`, `BTT_InitialFarmingTarget`,
      `BTT_ProgressFarmingSeq`, and `BTT_Work`.

### BTT Work Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_Work`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Script/AIModule.BTTask_BlueprintBase`;
  - only graph was `EventGraph`;
  - local BP variables were `DelayKey`, `Give To`, `Take From`,
    `Delay Multiplier`, and `As BP Interactable`;
  - `DelayKey` and `Delay Multiplier` were not connected in the exported
    EventGraph;
  - graph body checked that blackboard actors `Take From` and `Give To` were
    valid, saved a casted `BP_Interactable` from `Take From` into
    `As BP Interactable`, called `PlayWobble` with the controlled pawn
    location, called `Interact`, played the villager work animation with the
    returned delay, waited for that delay, re-read both blackboard actors,
    collected `(Resource, Value)` from `Take From`, added that resource to
    `Give To`, and returned task success;
  - Abort graph called `EndWobble(As BP Interactable)` and finished false.
- Added native parent `UCropoutWorkTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable blackboard selectors `TakeFrom` and `GiveTo` with BP
    display names `Take From` and `Give To`;
  - sets `bCreateNodeInstance = true` for timer-backed latent BT execution;
  - resolves selectors from the behavior tree blackboard asset;
  - returns `Failed` if either initial blackboard actor is invalid;
  - stores the casted `ACropoutInteractable` in an internal weak pointer,
    mirroring BP's `As BP Interactable` runtime variable without exposing it to
    the Blueprint asset;
  - calls `PlayWobble`, `Interact`, and
    `UCropoutBlueprintLibrary::PlayVillagerWorkAnim`;
  - waits for the `Interact` delay, then re-reads `TakeFrom` and `GiveTo` from
    the blackboard, calls `CollectActorResource`, and calls `AddActorResource`;
  - Abort clears the pending timer and calls `EndWobble` on the saved
    interactable.
- Removed BP-side task implementation from `BTT_Work`:
  - `DelayKey`, `Give To`, `Take From`, `Delay Multiplier`, and
    `As BP Interactable` BP member variables removed with
    `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutWorkTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttWork.py`
  - `Saved/codex_inspect_btt_work.json`
  - `Saved/CodexMigrateBttWorkNative.py`
  - `Saved/codex_migrate_btt_work_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_work_native_20260709_012405/BTT_Work.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_Work`, saved the asset, and reported no
    errors.
  - Migration result confirmed:
    - parent is `/Script/CropoutSampleProject.CropoutWorkTask`;
    - local BP variables are empty;
    - graph list is empty;
    - inherited task variables are only the native `TakeFrom` and `GiveTo`
      selectors plus engine BT base properties.
  - Fresh full Blueprint audit at local 2026-07-09 01:24:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 38 assets currently use project C++ parents.
    - 32 assets still use engine or Blueprint parents.
    - 15 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=15`;
    - remaining BTT assets with graphs are `BTT_DefaultBT`,
      `BTT_InitialCollectResource`, `BTT_InitialFarmingTarget`, and
      `BTT_ProgressFarmingSeq`.

### BTT ProgressFarmingSeq Native Slice

- Continued `/Game/Blueprint/Villagers/AI/Tasks/BTT_ProgressFarmingSeq`.
- Read the current BP task through the editor `BlueprintTools` surface before
  editing:
  - parent was `/Script/AIModule.BTTask_BlueprintBase`;
  - only graph was `EventGraph`;
  - local BP variables were `Crop` and `Tag State`;
  - graph body read the `Crop` blackboard actor, checked for `Ready` or
    `Harvest` tags, saved `Tags[1]` into `Tag State`, called `Interact` on the
    crop as `BP_Interactable`, played the villager work animation with the
    returned delay, delayed by that value, and then failed the task if
    `Tag State == Harvest`;
  - if the delayed branch did not fail, or if the crop did not have `Ready` or
    `Harvest`, the graph called `SetBlackboardValueAsObject(Crop)` with the
    value pin unconnected, effectively clearing that object key, and returned
    task success.
- Added native parent `UCropoutProgressFarmingSequenceTask` in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
- Native behavior implemented in this pass:
  - exposes editable blackboard selector `Crop`;
  - keeps BP's `Tag State` as internal native runtime state instead of exposing
    it back to the Blueprint asset;
  - sets `bCreateNodeInstance = true` for timer-backed latent BT execution;
  - resolves `Crop` from the behavior tree blackboard asset;
  - clears `Crop` and succeeds when the crop actor is missing or lacks both
    `Ready` and `Harvest` tags;
  - stores `Tags[1]` when available, calls `Interact`, calls
    `UCropoutBlueprintLibrary::PlayVillagerWorkAnim`, and waits for the
    returned delay;
  - after the delay, returns failure when the stored tag is `Harvest`;
  - otherwise clears `Crop` and succeeds;
  - Abort clears the pending timer.
- Removed BP-side task implementation from `BTT_ProgressFarmingSeq`:
  - `Crop` and `Tag State` BP member variables removed with
    `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutProgressFarmingSequenceTask`.
- Migration scripts/results:
  - `Saved/CodexInspectBttProgressFarmingSeq.py`
  - `Saved/codex_inspect_btt_progress_farming_seq.json`
  - `Saved/CodexMigrateBttProgressFarmingSeqNative.py`
  - `Saved/codex_migrate_btt_progress_farming_seq_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_btt_progress_farming_seq_native_20260709_012920/BTT_ProgressFarmingSeq.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled `BTT_ProgressFarmingSeq`, saved the asset, and
    reported no errors.
  - Migration result confirmed:
    - parent is `/Script/CropoutSampleProject.CropoutProgressFarmingSequenceTask`;
    - local BP variables are empty;
    - graph list is empty;
    - inherited task variables are only the native `Crop` selector plus engine
      BT base properties.
  - Fresh full Blueprint audit at local 2026-07-09 01:29:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 39 assets currently use project C++ parents.
    - 31 assets still use engine or Blueprint parents.
    - 14 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after migration:
    - `asset_count=14`;
    - remaining BTT assets with graphs are `BTT_DefaultBT`,
      `BTT_InitialCollectResource`, and `BTT_InitialFarmingTarget`.

### BTT Initial/Default Native Slice

- Continued the final remaining BTT graph assets:
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_InitialCollectResource`
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_InitialFarmingTarget`
  - `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT`
- Read the current BP tasks through the editor `BlueprintTools` surface before
  editing:
  - `BTT_InitialCollectResource` was still parented to
    `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT.BTT_DefaultBT_C`;
  - its local variables were `Resource Ref`, `Key_Resource`,
    `Key_ResourceClass`, `Key_ResourceTag`, `Key_CollectionClass`, and
    `Key_TownHall`;
  - the graph read `ControlledPawn.Tags[0]`, searched `BP_Resource` actors with
    that tag, set blackboard resource tag/class values, read the villager
    `Target Ref`, set the resource key, set `Key_TownHall` to the first
    `BPC_TownCenter`, and returned success; if no resource actors existed, it
    returned failure;
  - `BTT_InitialFarmingTarget` was still parented to
    `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT.BTT_DefaultBT_C`;
  - its local variables were `Key_Resource`, `Key_ResourceClass`,
    `Key_CollectionClass`, and `Key_TownHall`;
  - the graph reused a valid `Key_Resource` actor when present, otherwise
    searched ready `BP_BaseCrop` actors and wrote the nearest one into
    `Key_Resource`; after that it set resource/collection class keys,
    set `Key_TownHall` to the first `BPC_TownCenter`, and returned success;
  - `BTT_DefaultBT` had no local variables and only returned the controlled
    pawn to the default villager behavior before task success.
- Added native parents in:
  - `Source/CropoutSampleProject/CropoutBTTasks.h`
  - `Source/CropoutSampleProject/CropoutBTTasks.cpp`
  - `Source/CropoutSampleProject/CropoutVillager.h`
- Native behavior implemented in this pass:
  - `UCropoutDefaultBehaviorTask` calls
    `UCropoutBlueprintLibrary::ReturnVillagerToDefaultBT` and succeeds;
  - `UCropoutInitialCollectResourceTask` exposes the five blackboard key
    selectors used by the BP graph and mirrors the resource/tag/TownCenter
    setup;
  - `UCropoutInitialFarmingTargetTask` exposes the four blackboard key
    selectors used by the BP graph and mirrors the valid-resource/ready-crop
    selection flow;
  - `ACropoutVillager::GetTargetRefValue()` was made public so the initial
    collect task can read the same `Target Ref` value the BP graph read.
- Removed BP-side task implementations:
  - old local BP member variables removed with `BlueprintTools.remove_variable`;
  - `EventGraph` removed with `BlueprintEditorLibrary.remove_graph`;
  - assets reparented with `BlueprintTools.set_parent` to:
    - `/Script/CropoutSampleProject.CropoutInitialCollectResourceTask`
    - `/Script/CropoutSampleProject.CropoutInitialFarmingTargetTask`
    - `/Script/CropoutSampleProject.CropoutDefaultBehaviorTask`
- Migration scripts/results:
  - `Saved/CodexInspectBttInitialTasks.py`
  - `Saved/codex_inspect_btt_initial_tasks.json`
  - `Saved/CodexMigrateBttInitialTasksNative.py`
  - `Saved/codex_migrate_btt_initial_tasks_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup assets made before persistent change:
  - `C:/Temp/Cropout_btt_initial_tasks_native_20260709_013916/BTT_InitialCollectResource.uasset`
  - `C:/Temp/Cropout_btt_initial_tasks_native_20260709_013916/BTT_InitialFarmingTarget.uasset`
  - `C:/Temp/Cropout_btt_initial_tasks_native_20260709_013916/BTT_DefaultBT.uasset`
- Validation after this pass:
  - Full editor target build passed after the native C++ changes.
  - Migration script compiled all three BTT assets, saved them, and reported no
    errors.
  - Migration result confirmed all three target assets have project C++ parents,
    no local BP variables, and no graphs.
  - Fresh full Blueprint audit at local 2026-07-09 01:40:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 42 assets currently use project C++ parents.
    - 28 assets still use engine or Blueprint parents.
    - 11 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
- Re-read remaining graph DSL after migration:
  - `asset_count=11`;
  - remaining BTT assets with graphs are none.

### BP Player Empty EventGraph Cleanup

- Removed the final empty `EventGraph` from
  `/Game/Blueprint/Core/Player/BP_Player.BP_Player`.
- Safety check before removal:
  - `Saved/CodexRemoveBPPlayerEmptyEventGraph.py` re-read the graph through the
    editor `BlueprintTools` path and required the stripped DSL to be empty
    before deleting it.
- Removal path:
  - `BlueprintEditorLibrary.remove_graph(find_event_graph)` removed
    `EventGraph`;
  - the asset was compiled and saved after removal.
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bp_player_empty_event_graph_20260709_014344/BP_Player.uasset`
- Validation after this cleanup:
  - `Saved/codex_remove_bp_player_empty_event_graph.json` reports
    `compile_result=true`, `saved=true`, no errors, and no remaining
    `BP_Player` graphs.
  - Fresh full Blueprint audit at local 2026-07-09 01:45:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 42 assets currently use project C++ parents.
    - 28 assets still use engine or Blueprint parents.
    - 10 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after cleanup:
    - `asset_count=10`;
    - `/Game/Blueprint/Core/Player/BP_Player.BP_Player` is no longer listed.
    - Remaining graph assets are:
      - `/Game/Blueprint/Core/Extras/BPC_Cheats.BPC_Cheats`
      - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
      - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
      - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
      - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`
      - `/Game/UI/Common/CUI_Button.CUI_Button`
      - `/Game/UI/UI_Elements/UIE_Cost.UIE_Cost`
      - `/Game/UI/UI_Elements/UIE_Resource.UIE_Resource`
      - `/Game/UI/UI_Elements/UIE_Slider.UIE_Slider`
      - `/Game/UI/UI_Transition.UI_Transition`

### Remaining UI Widgets Native Slice

- Re-read the four remaining UI graph assets before editing:
  - `/Game/UI/Common/CUI_Button.CUI_Button`
    - parent was `/Script/CommonUI.CommonButtonBase`;
    - `EventPreConstruct` set `ButtonTitle` from `Button Text`, scaled
      `MinHeight` by `1.5` on Android/IOS, and passed
      `TriggeringInputAction` to `GamepadIcon`;
    - `MinHeight` and `TriggeringInputAction` are inherited CommonUI
      properties, so they were not redeclared in C++.
  - `/Game/UI/UI_Elements/UIE_Cost.UIE_Cost`
    - parent was `/Script/UMG.UserWidget`;
    - local variables were `Cost=0` and `Resource=None`;
    - `EventPreConstruct` updated `C_Cost` text and `Image_17` resource icon.
  - `/Game/UI/UI_Elements/UIE_Resource.UIE_Resource`
    - parent was `/Script/UMG.UserWidget`;
    - local variables were `Resource Type=None` and `Value=0`;
    - construct-time logic read the current game-mode resource value;
    - `UpdateValue` and `UpdateResources_Event` were no-op self-assignments.
  - `/Game/UI/UI_Elements/UIE_Slider.UIE_Slider`
    - parent was `/Script/UMG.UserWidget`;
    - local variables were `In Sound Class`, `Sound Class Title`,
      `In Sound Mix Modifier`, and `Index`;
    - `EventPreConstruct` set the label text;
    - `UpdateSlider` read `CropoutGI.SoundMixes[Index]`;
    - slider value changes wrote back to `SoundMixes[Index]` and called
      `SetSoundMixClassOverride`.
- Added native parents in the existing menu widget files:
  - `Source/CropoutSampleProject/CropoutMenuWidgets.h`
  - `Source/CropoutSampleProject/CropoutMenuWidgets.cpp`
  - `UCropoutCommonButton`
  - `UCropoutCostWidget`
  - `UCropoutResourceElementWidget`
  - `UCropoutAudioSliderWidget`
- Adjusted `Source/CropoutSampleProject/CropoutGameLayerWidget.cpp` so
  `SetResourceProperty` first tries the new native `ResourceType` property and
  then falls back to the old `Resource` name.
- Removed BP-side implementation:
  - local variables removed with `BlueprintTools.remove_variable`;
  - `EventGraph` removed with
    `BlueprintEditorLibrary.remove_graph(find_event_graph)`;
  - assets reparented with `BlueprintTools.set_parent`.
- Migrated assets:
  - `/Game/UI/Common/CUI_Button.CUI_Button` ->
    `/Script/CropoutSampleProject.CropoutCommonButton`
  - `/Game/UI/UI_Elements/UIE_Cost.UIE_Cost` ->
    `/Script/CropoutSampleProject.CropoutCostWidget`
  - `/Game/UI/UI_Elements/UIE_Resource.UIE_Resource` ->
    `/Script/CropoutSampleProject.CropoutResourceElementWidget`
  - `/Game/UI/UI_Elements/UIE_Slider.UIE_Slider` ->
    `/Script/CropoutSampleProject.CropoutAudioSliderWidget`
- Migration scripts/results:
  - `Saved/CodexReadRemainingUIWidgets.py`
  - `Saved/CodexReadRemainingUIVariables.py`
  - `Saved/CodexProbeUIPropertyNames.py`
  - `Saved/CodexMigrateRemainingUIWidgetsNative.py`
  - `Saved/codex_migrate_remaining_ui_widgets_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_remaining_ui_widgets_native_20260709_022125`
- Validation after this pass:
  - Full editor target build passed:
    `Build.bat CropoutSampleProjectEditor Win64 Development -Project="D:\ue\CropoutSampleProject\CropoutSampleProject.uproject" -WaitMutex -FromMsBuild`.
  - Migration script compiled and saved all four assets, reported
    `BS_UP_TO_DATE`, no errors, no local variables, and no remaining graphs.
  - Fresh full Blueprint audit at local 2026-07-09 02:21:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 48 assets currently use project C++ parents.
    - 22 assets still use engine or Blueprint parents.
    - 4 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Remaining graph assets are now only Blueprint interfaces:
    - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
    - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
    - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
    - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`

### UI Transition Native Slice

- Re-read `/Game/UI/UI_Transition.UI_Transition` before editing:
  - parent was `/Script/UMG.UserWidget`;
  - it had no local member variables;
  - `EventGraph` implemented two custom events:
    - `TransIn`: `PlayAnimation(NewAnimation)`;
    - `TransOut`: `PlayAnimation(NewAnimation, Reverse)`, delay by
      `NewAnimation.GetEndTime`, then `RemoveFromParent`.
  - `CropoutGameInstance` already calls these events by name through
    `ProcessEvent("TransIn")` and `ProcessEvent("TransOut")`.
- Added native parent:
  - `Source/CropoutSampleProject/CropoutTransitionWidget.h`
  - `Source/CropoutSampleProject/CropoutTransitionWidget.cpp`
  - `UCropoutTransitionWidget::TransIn()` mirrors the forward animation play.
  - `UCropoutTransitionWidget::TransOut()` mirrors the reverse animation play
    and delayed removal using a timer.
  - `NewAnimation` is now a native `BindWidgetAnimOptional` property.
- Removed BP-side implementation:
  - `EventGraph` removed with
    `BlueprintEditorLibrary.remove_graph(find_event_graph)`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutTransitionWidget`.
- Migration scripts/results:
  - `Saved/CodexReadUITransitionFull.py`
  - `Saved/codex_read_ui_transition_full.json`
  - `Saved/CodexMigrateUITransitionNative.py`
  - `Saved/codex_migrate_ui_transition_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexDumpRemainingGraphDsl.py`
  - `Saved/codex_dump_remaining_graph_dsl.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_ui_transition_native_20260709_015708/UI_Transition.uasset`
- Validation after this pass:
  - Full editor target build passed after adding
    `UCropoutTransitionWidget`.
  - Migration script compiled and saved `UI_Transition`, reported
    `BS_UP_TO_DATE`, no errors, no local variables, and no remaining graphs.
  - Fresh full Blueprint audit at local 2026-07-09 01:59:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 44 assets currently use project C++ parents.
    - 26 assets still use engine or Blueprint parents.
    - 8 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after cleanup:
    - `asset_count=8`;
    - `/Game/UI/UI_Transition.UI_Transition` is no longer listed.
    - Remaining graph assets are:
      - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
      - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
      - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
      - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`
      - `/Game/UI/Common/CUI_Button.CUI_Button`
      - `/Game/UI/UI_Elements/UIE_Cost.UIE_Cost`
      - `/Game/UI/UI_Elements/UIE_Resource.UIE_Resource`
      - `/Game/UI/UI_Elements/UIE_Slider.UIE_Slider`

### BPC Cheats Native Shell Cleanup

- Re-read `/Game/Blueprint/Core/Extras/BPC_Cheats.BPC_Cheats` before editing:
  - parent was `/Script/Engine.ActorComponent`;
  - it had no local member variables;
  - `EventGraph` DSL only contained key events for `1`, `2`, and `3`;
  - the graph also contained orphan `GetGameMode` and `AddResource` nodes, but
    pin-link audit confirmed every node pin had zero links.
- Checked asset references before editing:
  - confirmed referencer remains `/Game/Blueprint/Core/Player/BP_Player`;
  - therefore the `BPC_Cheats` asset was kept as a component shell rather than
    deleted.
- Added native parent:
  - `Source/CropoutSampleProject/CropoutCheatsComponent.h`
  - `Source/CropoutSampleProject/CropoutCheatsComponent.cpp`
  - `UCropoutCheatsComponent` is intentionally empty except for disabling
    component ticking, matching the current unconnected/no-op BP graph.
- Removed BP-side implementation:
  - `EventGraph` removed with
    `BlueprintEditorLibrary.remove_graph(find_event_graph)`;
  - asset reparented with `BlueprintTools.set_parent` to
    `/Script/CropoutSampleProject.CropoutCheatsComponent`.
- Migration scripts/results:
  - `Saved/CodexReadBPCCheatsFull.py`
  - `Saved/codex_read_bpc_cheats_full.json`
  - `Saved/CodexDumpBPCCheatsPinLinks.py`
  - `Saved/codex_dump_bpc_cheats_pin_links.json`
  - `Saved/CodexAuditBPCCheatsRefs.py`
  - `Saved/codex_audit_bpc_cheats_refs.json`
  - `Saved/CodexMigrateBPCCheatsNative.py`
  - `Saved/codex_migrate_bpc_cheats_native.json`
- Backup asset made before persistent change:
  - `C:/Temp/Cropout_bpc_cheats_native_20260709_015147/BPC_Cheats.uasset`
- Validation after this pass:
  - Full editor target build passed after adding `UCropoutCheatsComponent`.
  - Migration script compiled and saved `BPC_Cheats`, reported
    `BS_UP_TO_DATE`, no errors, no local variables, and no remaining graphs.
  - Fresh full Blueprint audit at local 2026-07-09 01:52:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 43 assets currently use project C++ parents.
    - 27 assets still use engine or Blueprint parents.
    - 9 assets still have at least one graph.
    - 4 assets still have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Re-read remaining graph DSL after cleanup:
    - `asset_count=9`;
    - `/Game/Blueprint/Core/Extras/BPC_Cheats.BPC_Cheats` is no longer listed.
    - Remaining graph assets are:
      - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
      - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
      - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
      - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`
      - `/Game/UI/Common/CUI_Button.CUI_Button`
      - `/Game/UI/UI_Elements/UIE_Cost.UIE_Cost`
      - `/Game/UI/UI_Elements/UIE_Resource.UIE_Resource`
      - `/Game/UI/UI_Elements/UIE_Slider.UIE_Slider`
      - `/Game/UI/UI_Transition.UI_Transition`

### Remaining BPI Empty Interface Cleanup

- Re-audited the remaining Blueprint Interface assets before removing their
  function graphs:
  - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
  - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
  - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
  - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`
- Native implementations already exist in the C++ interfaces and parents:
  - `Source/CropoutSampleProject/CropoutResourceInterface.h`
  - `Source/CropoutSampleProject/CropoutPlayerInterface.h`
  - `Source/CropoutSampleProject/CropoutGameInstanceInterface.h`
  - `Source/CropoutSampleProject/CropoutVillagerInterface.h`
  - corresponding implementations in `ACropoutGameMode`,
    `UCropoutGameInstance`, `ACropoutPlayerPawn`, `ACropoutResource`, and
    `ACropoutVillager`.
- Removed BP-side interface function graphs with
  `BlueprintTools.remove_function_graph`:
  - `BPI_Resource`: 5 graphs removed.
  - `BPI_Player`: 4 graphs removed.
  - `BPI_GI`: 12 graphs removed.
  - `BPI_Villager`: 6 graphs removed.
- Kept the BPI assets as empty interface shells instead of deleting assets:
  - after compile/save, all four have zero graphs;
  - `BPI_Player` still has package-level referencers from `BP_GM` and
    `BP_Player`, even after recompiling and saving those assets;
  - keeping an empty shell avoids breaking stale binary references while all
    function/event implementation has been removed from the interface assets.
- Added native bridge for the only plugin-side stale message call exposed by
  this cleanup:
  - `Source/CropoutSampleProject/CropoutBlueprintLibrary.h`
  - `Source/CropoutSampleProject/CropoutBlueprintLibrary.cpp`
  - `UCropoutBlueprintLibrary::NotifyUpdateAllInteractables` forwards to
    `ICropoutGameInstanceInterface::Execute_UpdateAllInteractables` on the
    active `UGameInstance`.
- Replaced the plugin Blueprint call site:
  - asset: `/IslandGenerator/Spawner/BP_Spawner.BP_Spawner`
  - old node: `EventGraph.K2Node_Message_0`, invalid
    `BPI_GI.Update All Interactables` message;
  - new node:
    `Cropout|GameInstance|Save|NotifyUpdateAllInteractables`;
  - old exec input/output links were preserved and the stale GameInstance
    target pin link was broken.
- Migration scripts/results:
  - `Saved/CodexAuditRemainingBPIRefs.py`
  - `Saved/codex_audit_remaining_bpi_refs.json`
  - `Saved/CodexCleanBPIPlayerStaleRefs.py`
  - `Saved/codex_clean_bpi_player_stale_refs.json`
  - `Saved/CodexRemoveRemainingBPIFunctionGraphs.py`
  - `Saved/codex_remove_remaining_bpi_function_graphs.json`
  - `Saved/CodexProbeNotifyUpdateAllInteractablesNodeNoSave.py`
  - `Saved/codex_probe_notify_update_all_interactables_node_no_save.json`
  - `Saved/CodexFixBPSpawnerUpdateAllInteractables.py`
  - `Saved/codex_fix_bp_spawner_update_all_interactables.json`
  - `Saved/CodexInspectBPSpawnerInvalidNodes.py`
  - `Saved/codex_inspect_bp_spawner_invalid_nodes.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_bpi_player_stale_refs_20260709_022849`
  - `C:/Temp/Cropout_remaining_bpi_empty_interfaces_20260709_023007`
  - `C:/Temp/Cropout_bp_spawner_update_all_interactables_20260709_023757/BP_Spawner.uasset`
- Validation after this pass:
  - Full editor target build passed after adding the native bridge:
    `Build.bat CropoutSampleProjectEditor Win64 Development -Project="D:\ue\CropoutSampleProject\CropoutSampleProject.uproject" -WaitMutex -FromMsBuild`.
  - `BP_Spawner` re-open/compile check passed:
    - commandlet exit code 0;
    - `invalid_count=0`;
    - `NotifyUpdateAllInteractables` native node count is 1;
    - compile status is `BS_UP_TO_DATE`.
  - Fresh full `/Game/Blueprint` + `/Game/UI` audit at local
    2026-07-09 02:39:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 48 assets currently use project C++ parents.
    - 22 assets still use engine or Blueprint parents.
    - 0 assets have graphs.
    - 0 assets have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - BPI reference audit after cleanup:
    - all four BPI assets have zero graphs;
    - node match count is 0;
    - node error count is 0;
    - only the `BPI_Player` empty shell has stale package referencers.

### Empty Main Menu Pawn And Camera Shake Native Shells

- Added native empty parent classes for two remaining non-logic Blueprint
  shells:
  - `ACropoutMainMenuPawn`, parented from `APawn`.
  - `UCropoutIdleCameraShake`, parented from `UCameraShakeBase`.
- Reparented and saved these assets:
  - `/Game/Blueprint/Core/MainMenu/BP_MenuPawn.BP_MenuPawn`
    - before: `/Script/Engine.Pawn`
    - after: `/Script/CropoutSampleProject.CropoutMainMenuPawn`
  - `/Game/Blueprint/Core/Extras/BP_CamShakeIdle.BP_CamShakeIdle`
    - before: `/Script/Engine.CameraShakeBase`
    - after: `/Script/CropoutSampleProject.CropoutIdleCameraShake`
- Both assets were graphless/data-only before this step and remain graphless
  after reparenting:
  - graph count: 0;
  - local variable count: 0;
  - compile result: true;
  - status: `BS_UP_TO_DATE`;
  - saved: true.
- Migration scripts/results:
  - `Saved/CodexInspectRemainingNonNativeAssets.py`
  - `Saved/codex_inspect_remaining_non_native_assets.json`
  - `Saved/CodexMigrateEmptyMainMenuPawnAndCamShakeNative.py`
  - `Saved/codex_migrate_empty_main_menu_pawn_and_cam_shake_native.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_empty_main_menu_pawn_camshake_native_20260709_024553`
- Project files were regenerated after adding the new C++ files:
  - command:
    `dotnet "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" -Mode=GenerateProjectFiles -Project="D:\ue\CropoutSampleProject\CropoutSampleProject.uproject"`
  - result: succeeded.
- Full editor target build was re-run:
  - command:
    `Build.bat CropoutSampleProjectEditor Win64 Development -Project="D:\ue\CropoutSampleProject\CropoutSampleProject.uproject" -WaitMutex -FromMsBuild`
  - result: succeeded; target was up to date.
- Fresh full `/Game/Blueprint` + `/Game/UI` audit at local
  2026-07-09 02:48:
  - 70 Blueprint/Widget Blueprint assets scanned.
  - 50 assets currently use project C++ parents.
  - 20 assets still use engine or Blueprint parents.
  - 0 assets have graphs.
  - 0 assets have non-trivial function/interface graphs.
  - Compile failures remain zero.
- Remaining non-project-native parent assets are still graphless:
  - 4 empty BPI interface shells:
    - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
    - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
    - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
    - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`
  - 7 resource/crop data child Blueprints under
    `/Game/Blueprint/Interactable/Resources/`.
  - 9 CommonUI/CommonInput style/input data assets under `/Game/UI/Common/`.
- The resource/crop child Blueprints were not flattened in this pass. They are
  data-only, but their inherited/default resource enum data needs a safer
  preservation pass before direct native reparenting.

### Resource And Crop Child Shell Native Flattening

- Re-audited the 7 remaining resource/crop child Blueprints before changing
  their parent class:
  - `/Game/Blueprint/Interactable/Resources/BP_Tree.BP_Tree`
  - `/Game/Blueprint/Interactable/Resources/BP_Shrub.BP_Shrub`
  - `/Game/Blueprint/Interactable/Resources/BP_Stone.BP_Stone`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Corn.BP_Crop_Corn`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Lettuce.BP_Crop_Lettuce`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Pumpkin.BP_Crop_Pumpkin`
  - `/Game/Blueprint/Interactable/Resources/BP_Crop_Wheat.BP_Crop_Wheat`
- Current C++ enum alignment was rechecked before this pass:
  - `ECropoutResourceType` is exposed to Python as
    `unreal.CropoutResourceType`;
  - members are `WOOD=0`, `NONE=1`, `STONE=5`, `FOOD=6`;
  - old BP enum values were mapped by enum name, not by Python's displayed
    numeric value.
- Reparented resource children directly to `ACropoutResource`:
  - `BP_Tree`: old `Resource Type=WOOD`, `Collection Time=3`,
    `Use Random Mesh=true` preserved into native properties.
  - `BP_Shrub`: old `Resource Type=FOOD`, `Collection Time=3`,
    `Use Random Mesh=true` preserved into native properties.
  - `BP_Stone`: old `Resource Type=STONE`, `Collection Time=10`,
    `Use Random Mesh=true` preserved into native properties.
- Reparented crop children directly to `ACropoutBaseCrop`:
  - `BP_Crop_Corn`
  - `BP_Crop_Lettuce`
  - `BP_Crop_Pumpkin`
  - `BP_Crop_Wheat`
- Default value preservation was done before saving:
  - `NativeResourceType`
  - `NativeResourceAmount`
  - `NativeCollectionTime`
  - `NativeCollectionValue`
  - `bNativeUseRandomMesh`
  - `NativeCooldownTime`
  - `ProgressionState`
  - `MeshList`
- No new C++ source was needed in this pass; existing native parents were
  already present:
  - `Source/CropoutSampleProject/CropoutResource.h`
  - `Source/CropoutSampleProject/CropoutResource.cpp`
- Migration scripts/results:
  - `Saved/CodexProbeResourceChildNativeProperties.py`
  - `Saved/codex_probe_resource_child_native_properties.json`
  - `Saved/CodexProbeResourceEnumWriteNoSave.py`
  - `Saved/codex_probe_resource_enum_write_no_save.json`
  - `Saved/CodexMigrateResourceChildShellsNative.py`
  - `Saved/codex_migrate_resource_child_shells_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexInspectRemainingNonNativeAssets.py`
  - `Saved/codex_inspect_remaining_non_native_assets.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_resource_child_shells_native_20260709_025636`
- Validation after this pass:
  - Migration commandlet exit code 0.
  - All 7 migrated assets compile to `BS_UP_TO_DATE`, save successfully, and
    have zero remaining graphs and zero local variables.
  - Fresh full `/Game/Blueprint` + `/Game/UI` audit at local
    2026-07-09 02:57:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 57 assets currently use project C++ parents.
    - 13 assets still use engine or Blueprint parents.
    - 0 assets have graphs.
    - 0 assets have non-trivial function/interface graphs.
    - Compile failures remain zero.
- Remaining non-project-native parent assets are now only graphless data/config
  shells:
  - 4 empty BPI interface shells.
  - 9 CommonUI/CommonInput style/input data assets under `/Game/UI/Common/`.

### CommonUI And CommonInput Data Shell Native Parents

- Added thin native parent classes for the remaining CommonUI/CommonInput
  data/style Blueprint shells:
  - `UCropoutCommonInputBaseControllerData`
  - `UCropoutCommonUIInputData`
  - `UCropoutCommonBorderStyle`
  - `UCropoutCommonButtonStyle`
  - `UCropoutCommonTextStyle`
- Source changes:
  - `Source/CropoutSampleProject/CropoutCommonStyleData.h`
  - `Source/CropoutSampleProject/CropoutCommonStyleData.cpp`
  - `Source/CropoutSampleProject/CropoutSampleProject.Build.cs`
- `CommonInput` was added as a public module dependency because the new native
  parents derive from `UCommonInputBaseControllerData` and
  `UCommonUIInputData`.
- Reparented and saved 9 CommonUI/CommonInput data/style assets:
  - `/Game/UI/Common/CUI_BaseControllerData.CUI_BaseControllerData`
  - `/Game/UI/Common/CUI_InputData.CUI_InputData`
  - `/Game/UI/Common/CUI_Style_Border_Dark.CUI_Style_Border_Dark`
  - `/Game/UI/Common/CUI_Style_Border_Light.CUI_Style_Border_Light`
  - `/Game/UI/Common/CUI_Style_Build.CUI_Style_Build`
  - `/Game/UI/Common/CUI_Style_Button.CUI_Style_Button`
  - `/Game/UI/Common/CUI_Style_Small.CUI_Style_Small`
  - `/Game/UI/Common/CUI_Style_Text.CUI_Style_Text`
  - `/Game/UI/Common/CUI_Style_Text2.CUI_Style_Text2`
- Migration scripts/results:
  - `Saved/CodexMigrateCommonStyleDataNative.py`
  - `Saved/codex_migrate_common_style_data_native.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
  - `Saved/CodexInspectRemainingNonNativeAssets.py`
  - `Saved/codex_inspect_remaining_non_native_assets.json`
- Backup assets made before persistent changes:
  - `C:/Temp/Cropout_common_style_data_native_20260709_030152`
- Validation after this pass:
  - Project files regenerated with UBT `-Mode=GenerateProjectFiles`.
  - `Build.bat CropoutSampleProjectEditor Win64 Development` succeeded.
  - CommonUI/CommonInput migration commandlet exit code 0.
  - All 9 migrated assets compile to `BS_UP_TO_DATE`, save successfully, and
    have zero remaining graphs and zero local variables.
  - Fresh full `/Game/Blueprint` + `/Game/UI` audit at local
    2026-07-09 03:02:
    - 70 Blueprint/Widget Blueprint assets scanned.
    - 66 assets currently use project C++ parents.
    - 4 assets still use engine interface parents.
    - 0 assets have graphs.
    - 0 assets have non-trivial function/interface graphs.
    - Compile failures remain zero.
- Remaining non-project-native assets are now only 4 empty Blueprint Interface
  shells:
  - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
  - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
  - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
  - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`

### Empty Blueprint Interface Asset Removal

- Re-audited the last 4 Blueprint Interface assets before deletion:
  - `/Game/Blueprint/Core/GameMode/BPI_Resource.BPI_Resource`
  - `/Game/Blueprint/Core/Player/BPI_Player.BPI_Player`
  - `/Game/Blueprint/Core/Save/BPI_GI.BPI_GI`
  - `/Game/Blueprint/Villagers/BPI_Villager.BPI_Villager`
- All 4 BPI assets had:
  - zero graphs;
  - zero package referencers;
  - zero implementers;
  - zero remaining `K2Node_Message` or old BPI call nodes.
- `BPI_Player` initially still had hard package references from `BP_GM` and
  `BP_Player`. Those were confirmed to be stale implemented-interface entries
  and removed with `CropoutBlueprintInterfaceRemove`.
- `CropoutBlueprintInterfaceRemove` was adjusted to save via
  `UEditorAssetSubsystem::SaveLoadedAsset` instead of direct
  `UPackage::SavePackage`; the direct save path can hit `PackagesDialog` in
  headless commandlets after interface removal.
- Deleted the 4 empty BPI `.uasset` files after backing them up:
  - `C:/Temp/Cropout_empty_bpi_delete_20260709_031236`
- Scripts/results:
  - `Saved/CodexProbeBPIPlayerDependencyDetails.py`
  - `Saved/codex_probe_bpi_player_dependency_details.json`
  - `Saved/codex_remove_bpi_player_interface.json`
  - `Saved/CodexAuditRemainingBPIRefs.py`
  - `Saved/codex_audit_remaining_bpi_refs.json`
  - `Saved/CodexDeleteEmptyBPIAssets.py`
  - `Saved/codex_delete_empty_bpi_assets.json`
  - `Saved/CodexBlueprintMigrationAudit.py`
  - `Saved/codex_blueprint_migration_audit.json`
- Validation after this pass:
  - `Build.bat CropoutSampleProjectEditor Win64 Development` succeeded after
    the commandlet save-path change.
  - Fresh full `/Game/Blueprint` + `/Game/UI` audit at local
    2026-07-09 03:13:
    - 66 Blueprint/Widget Blueprint assets scanned.
    - 66 assets currently use project C++ parents.
    - 0 assets use engine or Blueprint parents.
    - 0 assets have graphs.
    - 0 assets have non-trivial function/interface graphs.
    - Compile failures remain zero.
  - Fresh BPI-specific audit reports all 4 BPI packages as nonexistent, with
    zero referencers, zero implementers, zero old BPI call nodes, and
    `blueprint_count=66`.
