# Modules

## Purpose

Identify which module/plugin owns a given area so you know where to look
first and what dependency wiring to expect. This doc is **inventory**, not
implementation — verify each entry in the current source before changing it.

## When to read

- Adding a new `Source/` C++ class or module
- Touching a feature and wanting to know which plugin/module owns it
- Debugging a "module not found" / "plugin not enabled" / linker error
- Wiring a new dependency in a `Build.cs`

If your task is specifically about the engine `ModelContextProtocol` plugin,
skip to [`unreal.md`](unreal.md).

## Source scope

- [`CropoutSampleProject.uproject`](../../CropoutSampleProject.uproject)
  (modules + plugins arrays)
- `Source/CropoutSampleProject/*.cs`, `*.cpp`, `*.h`
- `Plugins/*/<Plugin>.uplugin`
- `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`
- `Saved/Logs/CropoutSampleProject*.log` (mount order of plugins)

## Project module

| Module                    | Type   | LoadingPhase | Source                                                                                                                                                |
|---------------------------|--------|--------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| `CropoutSampleProject`    | Runtime| Default      | [`Source/CropoutSampleProject/`](../../Source/CropoutSampleProject) — see [`CropoutSampleProject.cpp`](../../Source/CropoutSampleProject/CropoutSampleProject.cpp) and [`CropoutSampleProject.h`](../../Source/CropoutSampleProject/CropoutSampleProject.h). |

Public module dependencies (see
[`CropoutSampleProject.Build.cs`](../../Source/CropoutSampleProject/CropoutSampleProject.Build.cs)):

- `Core`, `CoreUObject`, `Engine`, `InputCore`

Private dependency list is currently empty. The `.Build.cs` has
commented-out hints for `Slate` / `SlateCore` / `OnlineSubsystem`, but
those are not active.

The only C++ class is `AMyGameMode` in
[`Source/CropoutSampleProject/MyGameMode.h`](../../Source/CropoutSampleProject/MyGameMode.h) —
an empty `AGameMode` subclass. All actual game logic is in Blueprints.

## Project plugins (`Plugins/`)

Both project plugins are **content-only** (no `Source/` subdirectory, so
they ship only `*.uasset` and resources).

| Plugin           | Author          | Type       | Purpose                                                      | `.uplugin`                                                                                                                                       |
|------------------|-----------------|------------|--------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------|
| `IconBaker`      | Epic Games, Inc. | Content    | Capture icons from an array of static meshes                 | [`Plugins/IconBaker/IconBaker.uplugin`](../../Plugins/IconBaker/IconBaker.uplugin)                                                                |
| `IslandGenerator`| Arran Langmead  | Content    | Landscape / spawner helpers (no description in `.uplugin`)   | [`Plugins/IslandGenerator/IslandGenerator.uplugin`](../../Plugins/IslandGenerator/IslandGenerator.uplugin)                                        |

Neither plugin's `Source/` exists, so they cannot host C++. They are
referenced indirectly by:

- `Config/DefaultGame.ini` — `DirectoriesToNeverCook += /IconBaker`
- `Content/` — Blueprints that may reference their assets

## Engine plugins enabled by the `.uproject` (subset relevant to this project)

The `.uproject` enables the following engine plugins (full list lives in
`CropoutSampleProject.uproject`; only the most relevant ones are listed
here):

| Plugin                          | Why it matters here                                                                                       |
|---------------------------------|----------------------------------------------------------------------------------------------------------|
| `EnhancedInput`                 | `Config/DefaultInput.ini` uses `EnhancedPlayerInput` / `EnhancedInputComponent`; `IMC_*.uasset` under `Content/Blueprint/Core/Player/Input/` |
| `CommonUI`                      | `Config/DefaultEditor.ini` sets CUI style/button/border templates; `Config/DefaultGame.ini` configures `CommonInputPlatformSettings_Windows` |
| `GeometryScripting`             | Enabled but no C++ use observed in project `Source/`                                                       |
| `OnlineSubsystemGoogle/Facebook/Apple` | Enabled; no `OnlineSubsystem` private dependency in `Build.cs` yet                                 |
| `AudioModulation`               | Enabled (matches the `Audio.*` gameplay tags in `Config/DefaultGameplayTags.ini`)                         |
| `ModelContextProtocol`          | **Engine MCP server plugin.** See [`unreal.md`](unreal.md) for transport/port evidence.                   |
| `EditorToolset`                 | Engine editor toolset — supplies the 19 toolsets visible in `Saved/Logs/CropoutSampleProject-backup-2026.07.02-02.19.39.log` (e.g. `editor_toolset.toolsets.actor.ActorTools`, `…blueprint.BlueprintTools`, `…material.MaterialTools`, `…texture.TextureTools`, `…static_mesh.StaticMeshTools`, `…skeletal_mesh.SkeletalMeshTools`, `…scene.SceneTools`, `…asset.AssetTools`, `…data_table.DataTableTools`, `…data_asset.DataAssetTools`, `…material_instance.MaterialInstanceTools`, `…object.ObjectTools`, `…primitive.PrimitiveTools`, `…string_table.StringTableTools`, `…curve_table.CurveTableTools`, `…programmatic.ProgrammaticToolset`, `EditorToolset.EditorAppToolset`, `EditorToolset.LogsToolset`, `ToolsetRegistry.AgentSkillToolset`). |

Disabled but listed in the `.uproject`: `Paper2D`, `AndroidMedia`,
`AndroidMoviePlayer`, `AppleMoviePlayer`, `WindowsMoviePlayer`,
`WmfMedia`, `KDevelopSourceCodeAccess`, `CableComponent`, `AvfMedia`,
`AudioCapture`, `ArchVisCharacter`, `Bridge`, `TraceUtilities`,
`ControlRigSpline`, `SequencerAnimTools`, `IKRig`, `AnimationSharing`,
`BlueprintHeaderView`, `GeometryMode`, `GLTFExporter`,
`SpeedTreeImporter`, `DatasmithContent`, `ChaosCloth`,
`ChaosClothEditor`, `MediaPlate`, `MediaCompositing`, `UVEditor`.

## Gameplay tags (`Config/DefaultGameplayTags.ini`)

```
Audio
Audio.AnimNotify
Audio.AnimNotify.Footstep
SubBehaviour
```

## Editor mount order (from latest log)

The `Saved/Logs/CropoutSampleProject.log` `LogPluginManager: Mounting …`
trace confirms that `AudioModulation`, `CommonUI`, `EnhancedInput`,
`OnlineSubsystem*`, `ModelContextProtocol`, and `EditorToolset` are all
mounted for this project. Module DLLs are loaded from
`C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-*.dll`,
e.g. `UnrealEditor-ModelContextProtocol.dll`,
`UnrealEditor-ModelContextProtocolEngine.dll`,
`UnrealEditor-ModelContextProtocolEditor.dll`.

## What this doc does NOT cover

- Plugin internal source (e.g. the engine `ModelContextProtocol` plugin's
  public/private C++). See [`unreal.md`](unreal.md) for that.
- Dependency versions / changelog. Verify against the engine source under
  `C:/Program Files/Epic Games/UE_5.8/` if needed.

## Verification notes

- The exact set of enabled plugins is the `Plugins` array of
  [`CropoutSampleProject.uproject`](../../CropoutSampleProject.uproject).
  If you change it, re-derive this doc from there.
- `Plugins/IconBaker` and `Plugins/IslandGenerator` have **no `Source/`
  subdir**, so adding C++ to them is a plugin-restructure task (move from
  content-only to mixed), not a code edit.
- The MCP server is provided by the **engine** plugin, not this project.
  Do not look for a `Plugins/ModelContextProtocol/` folder in this repo.

---

**Last verified:** 2026-07-02