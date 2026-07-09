# Architecture

## Purpose

A short, navigational map of how this Unreal Engine 5.8 sample is wired
together: what the project module is, what each project plugin contributes,
how the runtime config maps to maps and game modes, and where generated
artifacts live. This is **not** a behavior spec — verify every claim in the
current source.

## When to read

Read this when you need to orient before doing any of:

- Adding new C++ code under `Source/` or any `Plugins/*/Source/`
- Wiring up new maps, GameMode, GameInstance, PlayerController, or Pawn
- Adding or restructuring plugins
- Changing project packaging, rendering, or platform settings
- Touching the MCP tool surface or anything the editor exposes to AI agents

If your task is purely about UE/MCP plumbing or how the live editor talks to
the MCP server, prefer [`unreal.md`](unreal.md) instead.

## Source scope (what was actually read)

- [`CropoutSampleProject.uproject`](../../CropoutSampleProject.uproject)
- [`Source/CropoutSampleProject.Target.cs`](../../Source/CropoutSampleProject.Target.cs)
- [`Source/CropoutSampleProjectEditor.Target.cs`](../../Source/CropoutSampleProjectEditor.Target.cs)
- [`Source/CropoutSampleProject/CropoutSampleProject.Build.cs`](../../Source/CropoutSampleProject/CropoutSampleProject.Build.cs)
- [`Source/CropoutSampleProject/CropoutSampleProject.cpp`](../../Source/CropoutSampleProject/CropoutSampleProject.cpp)
- [`Source/CropoutSampleProject/CropoutSampleProject.h`](../../Source/CropoutSampleProject/CropoutSampleProject.h)
- [`Source/CropoutSampleProject/MyGameMode.h`](../../Source/CropoutSampleProject/MyGameMode.h)
- [`Plugins/IconBaker/IconBaker.uplugin`](../../Plugins/IconBaker/IconBaker.uplugin)
- [`Plugins/IslandGenerator/IslandGenerator.uplugin`](../../Plugins/IslandGenerator/IslandGenerator.uplugin)
- [`Config/DefaultEngine.ini`](../../Config/DefaultEngine.ini)
- [`Config/DefaultGame.ini`](../../Config/DefaultGame.ini)
- Top-level layout: see [`README.md`](README.md)

## Project shape (high level)

```
CropoutSampleProject/                       <-- workspace root
├── CropoutSampleProject.uproject           <-- one Runtime module, MCP/UI/EnhancedInput enabled
├── Source/
│   ├── CropoutSampleProject.Target.cs      <-- Game target, BuildSettingsVersion.V7
│   ├── CropoutSampleProjectEditor.Target.cs <-- Editor target, BuildSettingsVersion.V7
│   └── CropoutSampleProject/               <-- primary game module (Runtime, Default load)
│       ├── CropoutSampleProject.Build.cs   <-- deps: Core, CoreUObject, Engine, InputCore
│       ├── CropoutSampleProject.cpp/.h     <-- IMPLEMENT_PRIMARY_GAME_MODULE
│       └── MyGameMode.cpp/.h               <-- AGameMode subclass (currently empty)
├── Plugins/
│   ├── IconBaker/                          <-- content-only plugin (Epic)
│   └── IslandGenerator/                    <-- content-only plugin (Arran Langmead)
├── Config/                                 <-- DefaultEngine/Game/Editor/Input/GameplayTags + Android/
├── Content/                                <-- binary assets (Maps, BP, UI, Audio, VFX, Characters, Environment)
├── Saved/                                  <-- editor/UBT logs, derived editor configs, DDC
└── Binaries/Win64/                         <-- CropoutEditor.target (built editor binary)
```

## Key entry points and relationships

- **Primary game module:**
  `CropoutSampleProject` ([`Source/CropoutSampleProject/CropoutSampleProject.cpp`](../../Source/CropoutSampleProject/CropoutSampleProject.cpp))
  registered via `IMPLEMENT_PRIMARY_GAME_MODULE` with `FDefaultGameModuleImpl`.
- **C++ class:**
  `AMyGameMode` (subclass of `AGameMode`) at
  [`Source/CropoutSampleProject/MyGameMode.h`](../../Source/CropoutSampleProject/MyGameMode.h). Empty
  body — the actual game logic lives in Blueprints (see below).
- **Default game-mode chain (from `Config/DefaultEngine.ini`):**
  - `GameInstanceClass = /Game/Blueprint/Core/GameMode/BP_GI.BP_GI_C`
  - `GlobalDefaultGameMode = /Game/Blueprint/Core/GameMode/BP_GM.BP_GM_C`
  - `GameDefaultMap = /Game/MainMenu.MainMenu`
  - `EditorStartupMap = /Game/Village.Village`
- **Maps that are cooked:** `/Game/MainMenu`, `/Game/Village` (see
  `MapsToCook` in `Config/DefaultGame.ini`).
- **Packaged-blocked directories:** `/Game/PackIgnore` and `/IconBaker`
  (per `DirectoriesToNeverCook`).
- **UI style chain:** CommonUI is enabled. `DefaultEditor.ini` points CommonUI
  editor templates at `/Game/UI/Common/CUI_Style_*` and `CUI_BaseControllerData`,
  `CUI_InputData`. `Config/DefaultGame.ini` enables CommonInput on Windows with
  MouseAndKeyboard + Gamepad.
- **Enhanced Input:** enabled in `.uproject`; `Config/DefaultInput.ini` uses
  `EnhancedPlayerInput` / `EnhancedInputComponent` and `IMC_*` assets live at
  `Content/Blueprint/Core/Player/Input/IMC_BaseInput.uasset` etc.
- **Rendering defaults (`Config/DefaultEngine.ini`):** Lumen GI on,
  `r.Shadow.Virtual.Enable=1`, `r.Lumen.HardwareRayTracing=True`, Mobile
  shading path 1, DX12 default for Windows, `bUseFixedFrameRate=True` /
  `FixedFrameRate=60`.

## Project plugins (see [`modules.md`](modules.md) for detail)

- **IconBaker** (`Plugins/IconBaker/`) — Epic-authored, content-only
  (`CanContainContent=true`, no `Source/`). "Capture icons from an array of
  static meshes." Assets: `BP_BakeMaster.uasset`, `BP_IconBaker.uasset`,
  `IconBaker.umap`, plus `Materials/`.
- **IslandGenerator** (`Plugins/IslandGenerator/`) — Arran Langmead,
  content-only. Assets include `BP_IslandGen.uasset`,
  `Spawner/BP_Spawner.uasset`, `Misc/M_Landscape.uasset` /
  `MPC_Landscape.uasset`, and the `BPI_IslandPlugin.uasset` interface.

Both project plugins are referenced from cooked-blocked paths
(`/IconBaker`) and only contribute assets — they have no C++ surface in this
repo.

## Generated / per-machine dirs to ignore (mostly)

- `Binaries/Win64/` — built editor/game binaries (`CropoutEditor.target`)
- `DerivedDataCache/` — engine DDC
- `Intermediate/` — UBT/UHT output; project file generation under
  `Intermediate/ProjectFiles/` (Visual Studio + slnx) and a generated
  `Saved/UnrealBuildTool/BuildConfiguration.xml`
- `Saved/Logs/` — editor + UBT logs (see [`unreal.md`](unreal.md))
- `Saved/Config/` — per-user editor settings, e.g.
  `WindowsEditor/EditorPerProjectUserSettings.ini` (where `ModelContextProtocol`
  DeveloperSettings get persisted)

## Search keywords that show up often

- `BP_GI`, `BP_GM`, `BP_PC`, `BP_Player` — core game instance / mode /
  controller / pawn Blueprint classes
- `BP_Villager`, `BPI_Villager` — villager actor + interface
- `IMC_BaseInput`, `IMC_BuildMode`, `IMC_DragMove`, `IMC_Villager_Mode` —
  Enhanced Input mapping contexts
- `ST_SaveInteract`, `ST_Villager`, `ST_Job`, `ST_SpawnData`,
  `ST_SpawnInstance` — string tables / data tables
- `DT_Jobs` — DataTable of jobs

## Verification notes

- Map names referenced by `Config/DefaultEngine.ini` resolve under
  `Content/<MapName>.umap` (`MainMenu.umap`, `Village.umap` are present).
- The `Saved/Logs/CropoutSampleProject.log` shows the editor mounting both
  the `ModelContextProtocol` and `EditorToolset` engine plugins, plus
  `OnlineSubsystem*` and `CommonUI`. That is consistent with the `.uproject`.
- Blueprint asset graph logic is **not** readable from this repo — use the
  MCP server (see [`unreal.md`](unreal.md)) or the editor.
- This doc is informational only. It does not describe behavior; re-read the
  referenced files before changing anything.

---

**Last verified:** 2026-07-02