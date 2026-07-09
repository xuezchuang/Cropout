---
name: blueprint-to-cpp-migration
description: Staged migration workflow for turning Blueprint-heavy Cropout gameplay into C++-first gameplay while keeping assets, presentation, and tuning in Unreal assets/Blueprints. Use when converting Blueprint logic to C++, reparenting Blueprints, or deciding what belongs in C++ versus Blueprint.
triggers: [blueprint-to-cpp, blueprint migration, 蓝图转c++, 蓝图转C++, bp to c++, reparent, c++ parent, gameplay c++, 资源放蓝图, 逻辑放c++]
---

# Blueprint To C++ Migration - Cropout

## Purpose

Use this workflow to migrate Cropout from a Blueprint-first project to a
C++-first project without destroying the value of Unreal assets.

The goal is not "no Blueprints." The goal is:

- C++ owns gameplay logic, state transitions, interaction rules, AI rules,
  input handling, lifecycle code, and reusable systems.
- Blueprints/assets own resources, art/audio/VFX references, component
  defaults, animation links, UI layout, level placement, and light tuning.
- Existing Blueprint assets remain as thin C++-backed shells when they carry
  useful resources or designer-facing defaults.

## Required first step

Read `.codeforge/skills/unreal-project/SKILL.md` first. Follow its evidence
chain before making any claim about UE version, project source, Blueprint
assets, MCP state, or build commands.

## Evidence before migration

Before editing C++ or assets, gather a small migration dossier for the exact
Blueprint or feature being moved:

1. Identify the target asset path, current parent class, and where it is
   referenced by maps, GameMode settings, widgets, AI, or other Blueprints.
2. Inspect Blueprint behavior through the live Unreal MCP tools, EditorToolset,
   generated dumps, or the editor. Never read `*.uasset` as text.
3. Separate the Blueprint into:
   - logic to move to C++
   - resource/default data to keep in the Blueprint
   - editor-only wiring to preserve
   - unknown behavior that needs another inspection pass
4. Check existing C++ under `Source/CropoutSampleProject/` before creating
   new classes. Prefer extending the existing module over adding plugins.
5. Decide the smallest migration unit: one class, one feature, or one graph
   cluster. Do not bulk-convert unrelated Blueprints in the same change.

## MCP usage

Use CodeForge MCP server `unreal-mcp` as a live editor bridge when available.
Its current configured URL is `http://127.0.0.1:3094/mcp`, but re-check live
server state because the editor can restart or change ports.

The UE MCP server exposes meta-tools. Use this order:

1. `list_toolsets`
2. `describe_toolset` for the relevant toolset
3. `call_tool` with the exact toolset and tool name from the description

Do not guess direct tool names. If MCP is unavailable, use logs, generated
Blueprint dumps, or ask for editor access before claiming graph behavior.

## Migration pattern

For a Blueprint with substantial behavior:

1. Create or extend a C++ parent class with the narrowest useful UE base class.
2. Move behavior into C++ using clear Unreal APIs and module dependencies.
3. Expose asset-facing seams with `UPROPERTY(EditDefaultsOnly)` /
   `UPROPERTY(EditAnywhere)` and `BlueprintReadOnly` / `BlueprintReadWrite`
   only where Blueprint defaults really need them.
4. Use `UFUNCTION(BlueprintCallable)` only for intentional designer hooks.
   Prefer `BlueprintImplementableEvent` / `BlueprintNativeEvent` for
   presentation callbacks such as playing VFX, sounds, animation montages, or
   widget transitions.
5. Reparent the existing Blueprint to the new C++ class and preserve its asset
   references, components, and default values.
6. Remove or bypass the migrated Blueprint graph logic only after the C++
   path is verified.

## What belongs in C++

Prefer C++ for:

- rules, state machines, interaction decisions, cooldowns, timers, and Tick
  behavior
- input action handling and pawn/controller coordination
- AI task/service/decorator logic and target selection
- inventory, scoring, health, resources, save/load, and replicated state
- reusable gameplay utilities and validation helpers
- code that needs tests, refactors, or reliable review

## What can stay in Blueprint/assets

Keep in Blueprint/assets unless there is a concrete reason to move:

- meshes, materials, Niagara systems, sounds, animations, widgets, data assets,
  and data tables
- component hierarchy and designer-visible default values
- level-specific placement and references
- one-off presentation events called from C++
- asset-only child classes that exist to choose art, defaults, or UI layout

## Safety rules

- Do not edit binary assets casually. Say which assets need editor-side changes
  before changing them.
- Do not hard-code content paths in C++ unless the project already uses that
  pattern or the path is intentionally stable.
- Do not add broad module dependencies to `CropoutSampleProject.Build.cs`
  without checking the exact API owner.
- Do not delete existing Blueprint graphs, variables, or components until the
  replacement C++ path is compiled and the Blueprint still loads.
- Do not mix migration with unrelated cleanup, renames, or style refactors.

## Verification

After a migration change, verify as far as the current environment allows:

0. **If a new `.cpp` / `.h` was added under `Source/`**, regenerate the Visual Studio solution so VS users see the new files (UBT compiles regardless):
   `dotnet "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" -Mode=GenerateProjectFiles -Project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject"`
   (In UE 5.x the old `Engine/Build/BatchFiles/GenerateProjectFiles.bat` wrapper was removed; the functionality now lives inside UBT as the `-Mode=GenerateProjectFiles` mode.)
1. Build the editor target:
   `C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat CropoutSampleProjectEditor Win64 Development -project="D:/ue/CropoutSampleProject/CropoutSampleProject.uproject" -waitmutex`
2. Check latest `Saved/Logs/CropoutSampleProject*.log` for compile, load, or
   Blueprint reparent errors.
3. If assets were changed, use the editor/MCP to compile or load the affected
   Blueprint and confirm references/defaults survived.
4. Report which behavior was moved, which asset responsibilities remain in
   Blueprint, and what still needs manual editor verification.

## Output expectations

For each migration step, report:

- Target Blueprint / feature
- Evidence used
- C++ class or files changed
- Blueprint/assets expected to remain resource owners
- Verification command and result
- Remaining risk or manual editor check
