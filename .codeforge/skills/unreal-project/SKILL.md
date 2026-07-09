---
name: unreal-project
description: Unreal Engine (UE, Unreal, 虚幻, Blueprint, 蓝图, C++, MCP, ModelContextProtocol) project navigation. Use this skill before answering anything about UE source, Blueprint assets, MCP transport/port, or the ModelContextProtocol plugin in this Cropout workspace.
triggers: [ue, unreal, blueprint, 蓝图, 虚幻, cpp, c++, mcp, ModelContextProtocol]
---

# Unreal Project — Cropout

## Purpose

A focused workflow for UE / Unreal / Blueprint / 蓝图 / C++ / MCP /
ModelContextProtocol questions in this workspace. It is **not** an
authoritative description of UE itself; it points at the evidence the
agent must gather before answering.

## When to use

Use this skill before acting on any request that mentions or implies:
UE, Unreal, 虚幻, Blueprint, 蓝图, C++, MCP, or ModelContextProtocol,
or before invoking the live MCP tool surface.

## Workspace facts (verified)

- Workspace root: `D:\ue\CropoutSampleProject`
- `.uproject`: [`CropoutSampleProject.uproject`](../../../CropoutSampleProject.uproject)
- `EngineAssociation`: `5.8`
- Detected UE install root: `C:/Program Files/Epic Games/UE_5.8/`
- Enabled engine plugins include `ModelContextProtocol` (and
  `EditorToolset`, `CommonUI`, `EnhancedInput`, `AudioModulation`,
  `OnlineSubsystem*`)
- CodeForge MCP server: `unreal-mcp`, transport `http`,
  url `http://127.0.0.1:3094/mcp` (re-check live state each turn)

## Required workflow

Follow these steps in order. Do not skip ahead.

If the request is about converting Blueprint logic to C++, reparenting a
Blueprint, or deciding what remains in Blueprint/assets versus C++, complete
this evidence chain first and then read
`.codeforge/skills/blueprint-to-cpp-migration/SKILL.md`.

1. **Read `.uproject`** at `CropoutSampleProject.uproject`.
   Confirm `EngineAssociation`, `Modules[]`, and `Plugins[]` enable
   flags. Note whether the question targets an engine plugin or a
   project plugin.

2. **Locate the UE install** from `EngineAssociation`. For `5.8`, the
   inspected install is `C:/Program Files/Epic Games/UE_5.8/`. Engine
   source: `Engine/Source/`. Engine plugins: `Engine/Plugins/.../Source/`.

3. **Read project config**: `Config/DefaultEngine.ini`,
   `Config/DefaultGame.ini`, `Config/DefaultEditor.ini`,
   `Config/DefaultInput.ini`, `Config/DefaultGameplayTags.ini`, and
   `Config/Android/*` when the question is platform-specific.

4. **Read project logs**: latest `Saved/Logs/CropoutSampleProject*.log`
   (sort by mtime). Look for `LogModelContextProtocol`,
   `LogPluginManager: Mounting …`, `LogHttpListener: Created new …`.

5. **Inspect project source**:
   - C++ under `Source/CropoutSampleProject/`
   - Project plugins under `Plugins/IconBaker/`,
     `Plugins/IslandGenerator/` (note: both are content-only, no
     `Source/` subdir)

6. **For MCP-specific questions**, read the engine plugin source at
   `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`
   and the CodeForge MCP list (live state). Cite the exact header /
   `.cpp` line numbers in your answer.

7. **Re-list connected MCP servers** (CodeForge MCP list tool) and any
   tools/toolsets before relying on transport, url, or tool surface.
   Do not cache across editor restarts.

8. **Treat any claim not grounded in the chain above as**
   `unknown; verify in code/logs`. Do not invent APIs, ports,
   transports, or module relationships.

## MCP transport / port (current understanding)

These come from the engine plugin source under
`C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`:

- Transport: HTTP (`FHttpServerModule::Get().GetHttpRouter(...)`, binds
  POST/GET/DELETE on the MCP URL path; SSE on a separate endpoint is
  not currently supported).
- Default URL path: `/mcp`
- Default port: `8000`
- Protocol version: `2025-11-25` (also accepts `2025-06-18`,
  `2024-11-05`)
- Override at runtime:
  `-ModelContextProtocolPort=N` and `-ModelContextProtocolStartServer`
- DeveloperSettings live at
  `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]`
  in `Saved/Config/<Platform>Editor/EditorPerProjectUserSettings.ini`.

Current runtime listener (re-derive on each launch):
`127.0.0.1:3094` (latest `Saved/Logs/CropoutSampleProject.log` lines
1805–1810). CodeForge MCP server reports the same URL
`http://127.0.0.1:3094/mcp`. The current saved source of the `3094`
value is `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`
section `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]`
with `ServerPortNumber=3094` and `bAutoStartServer=True`; re-check on
each live editor launch.

## Blueprint assets

- `*.uasset` and `*.umap` are **binary**. Do not pretend to read them
  as text. Use:
  - The Unreal editor's graph editor
  - MCP tools registered by the engine `ModelContextProtocol` /
    `EditorToolset` plugins (e.g. toolsets in
    `editor_toolset.toolsets.blueprint.BlueprintTools`,
    `editor_toolset.toolsets.asset.AssetTools`,
    `editor_toolset.toolsets.scene.SceneTools`)
  - Generated dumps (JSON exports of Blueprint graphs) when available

## Common pitfalls to avoid

- Confusing `CropoutSampleProject` (the project module) with
  `EditorToolset.EditorAppToolset` (an MCP toolset inside the engine
  `EditorToolset` plugin).
- Treating `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`
  as authoritative for MCP settings without checking the live state —
  the override may have come from a CLI flag, not the ini.
- Reading `.uasset` content as text or claiming you "see" a Blueprint
  graph from the filename alone.
- Inventing module dependency lists. The project module's actual public
  deps are in
  `Source/CropoutSampleProject/CropoutSampleProject.Build.cs`:
  `Core, CoreUObject, Engine, InputCore` (private deps currently
  empty).
- Conflating the deprecated `-StartModelContextProtocolServer` flag
  with `-ModelContextProtocolStartServer` (the engine source logs a
  deprecation warning for the former).

## Output expectations

When you answer, cite concrete evidence:
- Line numbers in the engine plugin source for MCP claims.
- Section/key names from the `.ini` files for config claims.
- Log timestamps and line numbers from
  `Saved/Logs/CropoutSampleProject*.log` for runtime claims.
- The CodeForge MCP server id and current `transport`/`url` for live
  state claims.

## Related docs

- [`doc/ai-context/unreal.md`](../../../doc/ai-context/unreal.md) — full
  evidence chain, port/transport details, log excerpts
- [`doc/ai-context/code-navigation.md`](../../../doc/ai-context/code-navigation.md) —
  where gameplay classes/assets live
- [`doc/ai-context/build-and-run.md`](../../../doc/ai-context/build-and-run.md) —
  build, launch, packaging
- [`.codeforge/codeforge.md`](../../codeforge.md) — top-level CodeForge
  guidance for this workspace
- [`.codeforge/skills/blueprint-to-cpp-migration/SKILL.md`](../blueprint-to-cpp-migration/SKILL.md) —
  staged Blueprint-to-C++ migration workflow

---

**Last verified:** 2026-07-02
