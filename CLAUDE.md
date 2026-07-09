# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

Unreal Engine 5.8 sample (`CropoutSampleProject.uproject`, EngineAssociation `5.8`) located at `D:\ue\CropoutSampleProject`. Detected UE install: `C:/Program Files/Epic Games/UE_5.8`. One primary game module (`CropoutSampleProject`, Runtime, Default load), all gameplay currently in Blueprints under `Content/Blueprint/...`.

**This project has a dedicated AI-context doc tree.** Read only the doc that matches your task — do not bulk-load it.

| If you need to… | Read |
| --- | --- |
| Overall project shape, plugin layout, data flow | [doc/ai-context/architecture.md](doc/ai-context/architecture.md) |
| Which module/plugin owns an area, dependency graph | [doc/ai-context/modules.md](doc/ai-context/modules.md) |
| Build, run, package, or launch the editor | [doc/ai-context/build-and-run.md](doc/ai-context/build-and-run.md) |
| Locate C++ / Blueprint / config entry points | [doc/ai-context/code-navigation.md](doc/ai-context/code-navigation.md) |
| UE / Blueprint / MCP / ModelContextProtocol questions | [doc/ai-context/unreal.md](doc/ai-context/unreal.md) |
| Pick a project-specific sub-agent (BP reader, migrator, build, etc.) | [.claude/agents/README.md](.claude/agents/README.md) |

Index: [doc/ai-context/README.md](doc/ai-context/README.md). Workspace-level CodeForge guidance: [.codeforge/codeforge.md](.codeforge/codeforge.md).

## Working rules in this repo

- **Current code wins.** The ai-context docs are a navigation map with a `Last verified 2026-07-02` stamp; re-verify each claim against the live source (`.uproject`, `Config/`, `Source/`, `Saved/Logs/`) before acting on it. Anything that has rotted is suspect.
- **Blueprint assets are binary.** Never claim `.uasset` / `.umap` graph content from filenames — use the editor, MCP tools (see `unreal.md`), or generated dumps.
- **Sub-agents live in `.claude/agents/`.** See [.claude/agents/README.md](.claude/agents/README.md) for the index. Every agent's hard rule: read `*.uasset` / `*.umap` **only** via the live `unreal-mcp` MCP toolset. If MCP is unreachable, the agent terminates the task — it does not fall back to grep / `cat` / filename inference.
- **No silent changes** to: `EngineAssociation=5.8` in `.uproject`; `BuildSettingsVersion.V7` in both `Source/*.Target.cs`; the `Plugins[]` enable list (especially `ModelContextProtocol`, `CommonUI`, `EnhancedInput`); `MapsToCook` in `Config/DefaultGame.ini`; `PublicDependencyModuleNames` in `CropoutSampleProject.Build.cs`.
- **MCP state changes between launches.** Transport (HTTP), default port (`8000`), default path (`/mcp`) are engine constants in `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`. Runtime port (`3094` as of the verified snapshot) lives in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` — re-check on every turn that depends on it.
- **BP → C++ workflow: read fully first, then act.** When migrating a Blueprint to C++, **finish reading the entire BP graph through MCP first** (variables, functions, components, event bindings, parent class, interfaces, defaults, references) **before** writing a single line of C++. Do not start sketching class skeletons, header includes, or `UFUNCTION` stubs until the MCP read is fully complete — partial reads lead to rework, lost nodes, and wrong parents. "Read completely, then act, don't rush" (蓝图转 C++ 时,一般都是 MCP 先读取完再动手,不要着急) is the standard pacing.
- **No external README, no Copilot/Cursor rules.** The only Markdown guidance is `doc/ai-context/` (AI navigation) and `.codeforge/` (CodeForge skills/conventions).

## Common build commands (Windows, this exact machine)

The engine install used by this workspace is `C:/Program Files/Epic Games/UE_5.8`. Build via UBT:

```bash
"/c/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" \
  CropoutSampleProjectEditor Win64 Development \
  -Project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject" \
  -WaitMutex -FromMsBuild
```

- **Editor target**: `CropoutSampleProjectEditor` (used for the editor — what you'll usually build).
- **Game target**: `CropoutSampleProject` Win64 Development (packaged builds only).
- **Generate VS files** — **REQUIRED after any new `.cpp` / `.h` is added under `Source/`** (including new C++ classes during Blueprint → C++ migration). UBT scans `Source/` on its own and will compile the new files, but Visual Studio's `.sln` / `.vcxproj` will NOT show them until this runs. Run it any time the source tree changes, not only "when `.sln` looks stale":
  ```bash
  dotnet "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" \
    -Mode=GenerateProjectFiles \
    -Project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject"
  ```
  Note: in UE 5.x the old `Engine/Build/BatchFiles/GenerateProjectFiles.bat` wrapper script was removed; the same functionality now lives inside UBT as the `-Mode=GenerateProjectFiles` mode (see `Engine/Source/Programs/UnrealBuildTool/Modes/GenerateProjectFilesMode.cs`).
- **VS startup**: open `CropoutSampleProject.sln`, configuration `Development Editor`, platform `Win64`, startup project `CropoutSampleProjectEditor`.

Output binary: `Binaries/Win64/UnrealEditor.exe` (launches via double-clicking `.uproject` or directly with the engine executable + project path). Full editor/launch flags and packaging settings: [doc/ai-context/build-and-run.md](doc/ai-context/build-and-run.md).

## High-level architecture (the 30-second version)

- **One module, mostly empty C++.** `Source/CropoutSampleProject/` has only `IMPLEMENT_PRIMARY_GAME_MODULE` glue and an empty `AMyGameMode : AGameMode`. The C++ side exists to satisfy the "project has code" build flag and is being intentionally grown — do not assume any new `.cpp`/`.h` is out of place.
- **All gameplay is currently in Blueprints** under `Content/Blueprint/Core/...`: `BP_GI` (GameInstance), `BP_GM` (GameMode), `BP_PC` (PlayerController), `BP_Player` (Pawn), `BP_Villager`, behavior trees in `Content/Blueprint/Villagers/AI/`, etc.
- **MCP server in-process.** Engine plugin `ModelContextProtocol` boots an HTTP MCP server inside the editor — confirmed listening on `127.0.0.1:3094` (path `/mcp`) in the latest log. CodeForge exposes it as `unreal-mcp`. Use it instead of grepping `.uasset` files for graph content.
- **Two content-only project plugins**: `Plugins/IconBaker` and `Plugins/IslandGenerator`. Neither has a `Source/` subdirectory, so they cannot host C++ without restructuring the `.uplugin` first.
- **Enhanced Input + CommonUI**: input bindings live in `Config/DefaultInput.ini` with `IMC_*` mapping contexts under `Content/Blueprint/Core/Player/Input/`; CommonUI template assets are under `Content/UI/Common/` and referenced from `Config/DefaultEditor.ini`.

## Project conventions worth knowing

- **Staged Blueprint → C++ migration.** The `.codeforge/skills/blueprint-to-cpp-migration/SKILL.md` skill describes how C++ should grow here. Convention from that skill: C++ owns gameplay logic / state / interaction / AI / input / lifecycle / reusable systems; Blueprints and assets keep resources, component defaults, art/audio/VFX references, animation links, UI layout, level placement, light tuning. If you see a `BP_*` class with a matching `ACropout*Pawn`-style C++ parent (e.g. `BP_Player` → `ACropoutPlayerPawn`), the C++ parent is the round-1 scaffolding for future migration — keep function-graph names 1:1 so BP graphs can `Call Parent:<FuncName>` later.
- **Empty-shell UFUNCTIONs are intentional.** Some `UFUNCTION(BlueprintCallable, ...)` declarations in C++ have empty bodies (see `CropoutPlayerPawn.h`). They exist so a future round can move BP graph bodies into C++ without renaming. Do not delete them as dead code.
- **Inheriting `BlueprintImplementableEvent` events**: `AActor::UserConstructionScript` (and similar) are `UFUNCTION(BlueprintImplementableEvent)` — not C++ virtuals. Never add `UFUNCTION()` above an "override", and never write `void Foo() override { ... }` for them. Implement them in the Blueprint's graph (e.g. the Construction Script node).

## Verification cross-checks (when in doubt)

- `EngineAssociation`: [CropoutSampleProject.uproject](CropoutSampleProject.uproject)
- Enabled plugins: same `.uproject` (`Plugins[]` array)
- Build settings: [Source/CropoutSampleProject.Target.cs](Source/CropoutSampleProject.Target.cs) and [Source/CropoutSampleProjectEditor.Target.cs](Source/CropoutSampleProjectEditor.Target.cs)
- Module deps: [Source/CropoutSampleProject/CropoutSampleProject.Build.cs](Source/CropoutSampleProject/CropoutSampleProject.Build.cs)
- Default maps / game-mode chain: [Config/DefaultEngine.ini](Config/DefaultEngine.ini)
- CommonUI / input config: [Config/DefaultEditor.ini](Config/DefaultEditor.ini), [Config/DefaultInput.ini](Config/DefaultInput.ini)
- Last launch's MCP listener + mounted plugins: `Saved/Logs/CropoutSampleProject.log`
