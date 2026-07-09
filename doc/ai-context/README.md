# AI Context — Cropout (`D:\ue\CropoutSampleProject`)

> **This file is an index only.** It links to short, focused docs that help a
> future coding agent orient faster in this Unreal Engine 5.8 sample project.
> Nothing under `doc/ai-context/` is a source of truth. **Current code wins.**

## Rules for agents reading this folder

- This index and the linked docs are a **navigation map**, not documentation
  of behavior. Verify every claim against the current source tree, the
  `.uproject`, project `Config/`, `Saved/Logs/`, and (for UE / MCP questions)
  the UE engine source and the live CodeForge MCP server state.
- Read **only the docs that are relevant to the current task**. Do not bulk-load
  every file under `doc/ai-context/`.
- Code-specific conclusions (APIs, Blueprint graph logic, ports, transports,
  module ownership, call flow) **must be re-checked against the current
  source** before being acted on. Anything that has rotted since
  `Last verified 2026-07-02` is suspect.
- Do not invent files, APIs, ports, transports, or module relationships. If a
  detail is not visible in the inspected paths, treat it as **unknown** and
  write `unknown; verify in code/logs` instead of guessing.

## Quick orientation (workspace facts)

- **Workspace root:** `D:\ue\CropoutSampleProject`
- **Project file:** [`CropoutSampleProject.uproject`](../../CropoutSampleProject.uproject)
- **Engine association:** `5.8` (see `EngineAssociation` in the `.uproject`)
- **Detected UE install:** `C:/Program Files/Epic Games/UE_5.8` (confirmed via
  module load paths in `Saved/Logs/CropoutSampleProject.log`)
- **Target platforms (per `.uproject`):** Android, Windows, Mac, IOS, TVOS, Linux
- **Enabled plugin of interest:** `ModelContextProtocol` (engine plugin; MCP
  server inside the editor). See [`unreal.md`](unreal.md).
- **MCP transport/port (verified):** HTTP, current runtime listener
  `127.0.0.1:3094` — full evidence in [`unreal.md`](unreal.md). The
  `DefaultServerPort` constant is `8000` and `DefaultServerUrlPath` is `/mcp`
  in the engine plugin source.

## Doc index

Start with the doc that matches your task. Read only that one.

| If you need to…                                              | Read                                  |
|--------------------------------------------------------------|---------------------------------------|
| Understand overall project shape, plugin layout, data flow   | [`architecture.md`](architecture.md)  |
| Find which module/plugin owns an area, see dependency graph  | [`modules.md`](modules.md)            |
| Build, run, package, or launch the editor or game target     | [`build-and-run.md`](build-and-run.md)|
| Locate C++ / Blueprint / config entry points for a feature   | [`code-navigation.md`](code-navigation.md) |
| Answer UE / Blueprint / MCP / ModelContextProtocol questions  | [`unreal.md`](unreal.md)              |

## Inspection notes (what this init actually read)

- `CropoutSampleProject.uproject` (parsed for `EngineAssociation`, modules,
  enabled plugins, target platforms)
- Project C++: `Source/CropoutSampleProject/{Build.cs, h, cpp}`,
  `Source/CropoutSampleProject.{Target,Editor.Target}.cs`
- Project plugins: `Plugins/IconBaker/IconBaker.uplugin`,
  `Plugins/IslandGenerator/IslandGenerator.uplugin` (both **content-only**,
  no `Source/`)
- Project config: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`,
  `Config/DefaultEditor.ini`, `Config/DefaultInput.ini`,
  `Config/DefaultGameplayTags.ini`, `Config/Android/*`
- Project content tree (binary `*.uasset`/`.umap`):
  `Content/MainMenu.umap`, `Content/Village.umap`,
  `Content/Blueprint/...`, `Content/UI/...`, `Content/Characters/...`,
  `Content/Environment/...`, `Content/VFX/...`, `Content/Audio/...`
- Editor logs: `Saved/Logs/CropoutSampleProject*.log`,
  `Saved/Logs/UnrealVersionSelector-*.log`
- UE engine MCP plugin (read for transport/port verification):
  `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`
- Live CodeForge MCP server state via the `unreal-mcp` server
  (transport: `http`, url: `http://127.0.0.1:3094/mcp`)

## What this index does NOT cover

- Behavior of any specific Blueprint asset. `*.uasset` files are binary; use
  the live Unreal MCP tools, generated dumps, or the editor to inspect graph
  logic.
- Reverse-engineered behavior of closed binaries (`*.dll`, `Intermediate/`,
  `DerivedDataCache/`, `Binaries/`).
- Anything outside the inspected files above. If you need evidence for a
  claim that is not here, gather it directly from the source.

---

**Last verified:** 2026-07-02