# Code Navigation

## Purpose

Quick pointers to where the actual logic lives so you can jump to the
right file/dir without scanning the whole tree. **Blueprints are binary
(`.uasset`)** and cannot be read as text; for those, use the live editor
or the MCP server (see [`unreal.md`](unreal.md)).

## When to read

- You are about to grep the codebase for a symbol and want the likely
  owners first
- You need to know where a specific game feature lives (input, villagers,
  buildings, UI, audio, save game)
- You want to know which `.ini` key controls a runtime setting

## Source scope

- All paths under `Source/`, `Plugins/`, `Config/`, and `Content/` tree
  inspected via `list_dir` (see [`README.md`](README.md))
- Project C++ files (5 total under `Source/CropoutSampleProject/`)
- Project plugin `.uplugin` manifests (2: IconBaker, IslandGenerator)
- `Config/Default*.ini`

## C++ surface (only 5 files in the whole project)

`Source/CropoutSampleProject/`:

| File                                                                                            | Purpose                                                            |
|-------------------------------------------------------------------------------------------------|--------------------------------------------------------------------|
| [`CropoutSampleProject.Build.cs`](../../Source/CropoutSampleProject/CropoutSampleProject.Build.cs) | Module rules; deps `Core, CoreUObject, Engine, InputCore`         |
| [`CropoutSampleProject.h`](../../Source/CropoutSampleProject/CropoutSampleProject.h)            | Empty header (just `#include "CoreMinimal.h"`)                     |
| [`CropoutSampleProject.cpp`](../../Source/CropoutSampleProject/CropoutSampleProject.cpp)        | `IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ...)`        |
| [`MyGameMode.h`](../../Source/CropoutSampleProject/MyGameMode.h)                                 | Declares `AMyGameMode : public AGameMode` (empty body)            |
| [`MyGameMode.cpp`](../../Source/CropoutSampleProject/MyGameMode.cpp)                             | Empty `.cpp` (just `#include "MyGameMode.h"`)                       |

Conclusion: there is **no gameplay logic in C++** in this project. All
gameplay lives in Blueprints. The C++ side exists only to satisfy the
"project has code" build flag.

Target rule files (`Source/*.Target.cs`):

- [`CropoutSampleProject.Target.cs`](../../Source/CropoutSampleProject.Target.cs) — Game target
- [`CropoutSampleProjectEditor.Target.cs`](../../Source/CropoutSampleProjectEditor.Target.cs) — Editor target

## Blueprint surface (`Content/`)

Binary `.uasset` and `.umap` files. Treat asset names as authoritative but
**never claim graph behavior from filenames**. Use the editor or MCP tools.

### Top-level maps

- `Content/MainMenu.umap` — menu level (referenced as `GameDefaultMap` /
  `ServerDefaultMap` in `Config/DefaultEngine.ini`)
- `Content/Village.umap` — main village level (referenced as
  `EditorStartupMap`)
- `Content/Village_BuiltData.uasset` — editor built-data companion of `Village.umap`
- `Content/PackIgnore/` — `Demo.umap`, `IconBaker.umap`, `Overview.umap`,
  `ZoomIn.uasset`. **Never cooked** (`DirectoriesToNeverCook`)

### Gameplay classes (Blueprint, by directory)

| Area        | Path                                                | Key assets                                                                                                              |
|-------------|-----------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------|
| Core        | `Content/Blueprint/Core/GameMode/`                  | `BP_GI` (GameInstance), `BP_GM` (GameMode), `BPF_Cropout` (function library), `BPI_Resource` (interface)                 |
| Core        | `Content/Blueprint/Core/MainMenu/`                  | `BP_MainMenuGM`, `BP_MenuPawn`                                                                                          |
| Core        | `Content/Blueprint/Core/Player/`                    | `BP_PC` (PlayerController), `BP_Player` (Pawn/Character), `BPF_Shared`, `BPI_Player`                                      |
| Core        | `Content/Blueprint/Core/Player/Input/`              | `IA_*` (Input Actions), `IM_*` (Input Modifiers), `IMC_BaseInput`, `IMC_BuildMode`, `IMC_DragMove`, `IMC_Villager_Mode`, `CUI_InputTable`, `E_InputType` |
| Core        | `Content/Blueprint/Core/Save/`                      | `BP_SaveGM`, `BPI_GI`, `ST_SaveInteract` (string table), `ST_Villager` (string table)                                   |
| Core        | `Content/Blueprint/Core/Extras/`                    | `BP_CamShakeIdle`, `BPC_Cheats`, `C_Zoom`, `MPC_Cropout`, `PP_Highlight`, `RT_GrassMove`                                |
| Interactable| `Content/Blueprint/Interactable/`                   | `BP_Interactable`                                                                                                       |
| Interactable| `Content/Blueprint/Interactable/Building/`          | `BP_BuildingBase`, `BPC_EndGame`, `BPC_House`, `BPC_TownCenter`                                                         |
| Interactable| `Content/Blueprint/Interactable/Resources/`         | (resource-node Blueprints)                                                                                              |
| Villagers   | `Content/Blueprint/Villagers/`                      | `BP_Villager`, `BPI_Villager`, `DT_Jobs` (data table), `ST_Job` (string table)                                           |
| Villagers   | `Content/Blueprint/Villagers/AI/`                   | `BT_Build`, `BT_CollectResource`, `BT_Farming`, `BT_Idle` behavior trees                                                |

### UI (`Content/UI/`)

| Subfolder             | Notable assets                                                                                                |
|-----------------------|---------------------------------------------------------------------------------------------------------------|
| `Content/UI/Common/`  | CommonUI style assets: `CUI_BaseControllerData`, `CUI_Button`, `CUI_BuildItem`, `CUI_InputData`, `CUI_Style_Border_Dark/Light/Build`, `CUI_Style_Button/Small/Text/Text2` |
| `Content/UI/MainMenu/`| `UI_MainMenu`, `UI_Layer_Menu`                                                                                |
| `Content/UI/Game/`    | `UI_GameMain`, `UI_Layer_Game`                                                                                |
| `Content/UI/`         | `UI_Transition`                                                                                               |

These are referenced from `Config/DefaultEditor.ini` and
`Config/DefaultGame.ini` (`CUI_*` symbols).

### Content categories

- `Content/Characters/` — Animations / Materials / Meshes / Textures
- `Content/Environment/` — Environment / Materials / Meshes
- `Content/Audio/` — DATA / MUSIC / SFX
- `Content/VFX/` — Niagara systems (`NS_BirdFlying`, `NS_Impact`,
  `NS_Sleepy`, `NS_Target`, `NS_Waves`, `NS_WindSwish`) plus `Materials/`
  and `Meshes/`
- `Content/Splash/` — `EdSplash.png`, `gameicon.ico`, `Splash.png`
- `Content/Collections/` — empty (kept by UE)
- `Content/Developers/13436/` — per-developer content (currently empty
  in this checkout)

### Project plugin assets

- `Plugins/IconBaker/Content/` — `IconBaker.umap`, `BP_BakeMaster`,
  `BP_IconBaker`, and `Materials/{M_FixA, M_GroundPlane, m_IconPreview,
  CustomBlurSample}`
- `Plugins/IslandGenerator/Content/` — `BP_IslandGen`,
  `Spawner/BP_Spawner` + `ST_SpawnData` / `ST_SpawnInstance` string tables,
  `Misc/{BP_SpawnMarker, BPI_IslandPlugin, M_Landscape, MPC_Landscape,
  T_Useful}`

## Config keys cheat sheet

The most-frequently-asked config keys (read the file before editing any
of these):

| Concern             | Key                                                    | File                                                      |
|---------------------|--------------------------------------------------------|-----------------------------------------------------------|
| Startup map         | `EditorStartupMap`, `GameDefaultMap`, `ServerDefaultMap` | `Config/DefaultEngine.ini`                              |
| GameInstance class  | `GameInstanceClass`                                    | `Config/DefaultEngine.ini`                                |
| Global GameMode     | `GlobalDefaultGameMode`                                | `Config/DefaultEngine.ini`                                |
| CommonUI templates  | `TemplateTextStyle`, `TemplateButtonStyle`, `TemplateBorderStyle` | `Config/DefaultEditor.ini`         |
| Enhanced Input      | `DefaultPlayerInputClass`, `DefaultInputComponentClass`, `ActionMappings`, `AxisMappings` | `Config/DefaultInput.ini` |
| Packaging           | `MapsToCook`, `DirectoriesToNeverCook`, `PackageCompressionFormat`, `BuildConfiguration` | `Config/DefaultGame.ini` |
| Gameplay tags       | `+GameplayTagList=(Tag=…)`                             | `Config/DefaultGameplayTags.ini`                          |
| Android-only        | `PackageName`, `MinSDKVersion`, `bSupportsVulkan`      | `Config/DefaultEngine.ini`                                |

## Project settings the agent should NOT change silently

- `EngineAssociation=5.8` in the `.uproject`
- `BuildSettingsVersion.V7` in both target `.cs` files
- The `Plugins` enable/disable list (especially `ModelContextProtocol`,
  `CommonUI`, `EnhancedInput`)
- `MapsToCook` (do not drop the MainMenu or Village maps)
- The `PublicDependencyModuleNames` list in `CropoutSampleProject.Build.cs`

## Verification notes

- The C++ surface is small enough that any "the project has no C++"
  statement should be re-verified by listing `Source/` before relying on
  it. If a new `.cpp`/`.h` appears, update this doc.
- Blueprint asset names match the references in `Config/*.ini` (e.g.
  `BP_GI`, `CUI_*`, `IMC_*`), but the **graph content** is only visible
  through the editor or MCP tools.

---

**Last verified:** 2026-07-02