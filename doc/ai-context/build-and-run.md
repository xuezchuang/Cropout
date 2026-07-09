# Build and Run

## Purpose

How to build, launch, and package this Unreal Engine 5.8 sample on
Windows. The exact commands depend on the local UE install and the
agent's environment — this doc only records what the inspected files
actually show.

## When to read

- You need to compile the project, run the editor, or cook/package a build
- You want to know what `*.Target.cs` files exist and which target to pick
- You want to know where build outputs land
- You want to know which MCP / Engine flags affect editor startup

For MCP transport/port and the editor's HTTP server, see
[`unreal.md`](unreal.md) instead.

## Source scope

- [`CropoutSampleProject.uproject`](../../CropoutSampleProject.uproject) —
  target platforms + enabled plugins
- [`Source/CropoutSampleProject.Target.cs`](../../Source/CropoutSampleProject.Target.cs) — `TargetType.Game`, `BuildSettingsVersion.V7`
- [`Source/CropoutSampleProjectEditor.Target.cs`](../../Source/CropoutSampleProjectEditor.Target.cs) — `TargetType.Editor`, `BuildSettingsVersion.V7`
- [`Source/CropoutSampleProject/CropoutSampleProject.Build.cs`](../../Source/CropoutSampleProject/CropoutSampleProject.Build.cs) — public deps `Core, CoreUObject, Engine, InputCore`
- [`.vsconfig`](../../.vsconfig) — required VS workloads
- [`Config/DefaultEngine.ini`](../../Config/DefaultEngine.ini),
  [`Config/DefaultGame.ini`](../../Config/DefaultGame.ini),
  [`Config/DefaultInput.ini`](../../Config/DefaultInput.ini)
- `Saved/UnrealBuildTool/BuildConfiguration.xml` — UBT build config dump
- `Saved/Logs/CropoutSampleProject*.log` — last successful editor launches

## Targets

There are two UBT targets in `Source/`:

| Target file                                              | Type    | BuildSettings | Notes                                                  |
|----------------------------------------------------------|---------|---------------|--------------------------------------------------------|
| `Source/CropoutSampleProject.Target.cs`                  | Game    | V7            | Extra module: `CropoutSampleProject`. No editor.       |
| `Source/CropoutSampleProjectEditor.Target.cs`            | Editor  | V7            | Extra module: `CropoutSampleProject`. Used for editor. |

The SLN files at the repo root (`CropoutSampleProject.sln`,
`CropoutSampleProject.slnx`, plus `Automation_CropoutSampleProject.sln*`)
are the Visual Studio entry points. The current `Saved/Logs` shows
`D:/ue/CropoutSampleProject/Binaries/Win64/CropoutEditor.target` is already
built, i.e. the editor target has been compiled at least once on this
machine.

## Build commands (Windows, high level)

The exact command shape is whatever the agent's environment supports. The
following are the canonical UE patterns that match this project shape:

- **Generate VS project files (if needed):**
  `Engine/Build/BatchFiles/GenerateProjectFiles.bat -project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject"`
  Result: `Intermediate/ProjectFiles/UE5.vcxproj` (already present in
  `Intermediate/ProjectFiles/`).
- **Build the editor target with UBT:**
  `Engine/Build/BatchFiles/Build.bat CropoutSampleProjectEditor Win64 Development -project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject" -waitmutex`
- **Build the game target:**
  `Engine/Build/BatchFiles/Build.bat CropoutSampleProject Win64 Development -project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject" -waitmutex`
- **Build via Visual Studio:** open `CropoutSampleProject.sln`, set
  `CropoutSampleProjectEditor` (or `CropoutSampleProject`) as the startup
  project, configuration `Development Editor` (or `Development`), platform
  `Win64`, then Build.

Required Visual Studio components (from
[`.vsconfig`](../../.vsconfig)):

- `Component.Unreal.Debugger`, `Component.Unreal.Ide`
- `Microsoft.Net.Component.4.6.2.TargetingPack`
- `Microsoft.VisualStudio.Component.VC.14.50.18.0.ATL`,
  `Microsoft.VisualStudio.Component.VC.14.50.18.0.x86.x64`,
  `Microsoft.VisualStudio.Component.VC.Llvm.Clang`,
  `Microsoft.VisualStudio.Component.VC.Tools.x86.x64`
- `Microsoft.VisualStudio.Component.Windows11SDK.22621`
- `Microsoft.VisualStudio.Workload.{CoreEditor,ManagedDesktop,NativeDesktop,NativeGame}`

## Launch the editor

- **Double-click `CropoutSampleProject.uproject`** — Windows asks whether
  to associate with the registered UE 5.8 install (Epic Games Launcher /
  UnrealVersionSelector). Logs land under `Saved/Logs/`.
- **Direct from the engine install:**
  `"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "D:/ue/CropoutSampleProject/CropoutSampleProject.uproject"`
- The startup map (per `Config/DefaultEngine.ini` →
  `EditorStartupMap`) is `/Game/Village.Village` (`Content/Village.umap`).
  The default game map (`GameDefaultMap`) is `/Game/MainMenu.MainMenu`.

## Cook / package

- **`Config/DefaultGame.ini`** packaging settings: `Build=IfProjectHasCode`,
  `BuildConfiguration=PPBC_Development`, `UsePakFile=True`,
  `PackageCompressionFormat=Oodle`, `PackageCompressionMethod=Kraken`.
- **`MapsToCook`**: `/Game/MainMenu`, `/Game/Village`.
- **`DirectoriesToNeverCook`**: `/Game/PackIgnore`, `/IconBaker`.
- **`bCookMapsOnly=True`, `bSkipEditorContent=True`**,
  `InternationalizationPreset=English`, `CulturesToStage=en`.
- Use **File → Package Project** in the editor, or
  `RunUAT.bat BuildCookRun -project=… -platform=Win64 -build -cook -stage -pak -package`.

## Useful command-line flags (observed / supported by this build)

The engine `ModelContextProtocol` plugin supports these (see
[`unreal.md`](unreal.md) for evidence):

- `-ModelContextProtocolStartServer` — autostart the MCP HTTP server
- `-ModelContextProtocolPort=N` — override the MCP listener port (default
  `8000`, see [`unreal.md`](unreal.md))
- `-StartModelContextProtocolServer` — deprecated alias

Other notable flags mentioned in logs (PIE / editor defaults):

- `-BUILDMACHINE` (no effect for local development)
- `ServerPort=17777` is the PIE server port (from
  `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`,
  not the MCP port)

## Output directories

- **Editor binary:** `Binaries/Win64/CropoutEditor.target` (already exists)
- **Build receipts:** `Binaries/Win64/UnrealEditor.modules`,
  `Binaries/Win64/UnrealEditor.target`, etc.
- **UHT generated headers:** under `Intermediate/Build/.../Inc/`
- **VS project files:** `Intermediate/ProjectFiles/UE5.vcxproj` plus the
  root `.sln`/`.slnx`
- **Editor logs:** `Saved/Logs/CropoutSampleProject*.log` plus dated
  backups (`-backup-YYYY.MM.DD-HH.MM.SS.log`)
- **UBT logs:** `Saved/UnrealBuildTool/BuildConfiguration.xml`

## Verification notes

- Build settings version is `V7` for both targets. If you upgrade UE,
  re-check the targets.
- The `Saved/Logs` already shows successful editor launches that mounted
  `ModelContextProtocol`, `EditorToolset`, and `CommonUI`. If a future
  launch is missing one of these, the `.uproject` `Plugins` array or the
  engine install is the first place to look.
- If you change `MapsToCook` or `DirectoriesToNeverCook`, re-derive this
  doc from `Config/DefaultGame.ini`.

---

**Last verified:** 2026-07-02