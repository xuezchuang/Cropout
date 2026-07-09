# CodeForge — `Cropout` (`D:\ue\CropoutSampleProject`)

This file is the top-level CodeForge guidance for this workspace. It
points agents at the right context, evidence chain, and skills.

## Workspace facts (verified)

- **Workspace name:** Cropout
- **Workspace root:** `D:\ue\CropoutSampleProject`
- **Project type:** Unreal Engine 5.8 sample (`CropoutSampleProject.uproject`)
- **`EngineAssociation`:** `5.8`
- **Detected UE install root:** `C:/Program Files/Epic Games/UE_5.8`
- **Primary game module:** `CropoutSampleProject` (Runtime, default load)
- **Enabled engine plugin of interest:** `ModelContextProtocol`
- **Live MCP server:** CodeForge `unreal-mcp` — transport `http`,
  url `http://127.0.0.1:3094/mcp`

## General rules

1. Read [`doc/ai-context/README.md`](../doc/ai-context/README.md) first if
   it exists. It is an **index only**, not a source of truth. Read only
   the focused docs that match the current task.
2. Any code-specific conclusion must be re-verified against the current
   source, `.uproject`, project `Config/`, `Saved/Logs/`, and (for UE/MCP
   questions) the UE engine source and the live MCP server state.
3. Never claim `*.uasset` Blueprint assets can be read as text. Use the
   editor, MCP tools, or generated dumps.
4. Do not invent files, APIs, ports, transports, module relationships,
   or build commands. When uncertain, write `unknown; verify in code/logs`.

## Clarification and execution autonomy

Ask the user only when missing information would likely cause a wrong edit,
irreversible action, broad architecture decision, or unsafe operation.

When the user replies with a clear approval or choice such as `改吧`,
`开始吧`, `做第一轮`, `A/B/C`, `1/2/3`, or `X弄吧`, treat it as execution
authorization. Do not ask for the same confirmation again.

For non-blocking uncertainty such as tool order, token cost, whether to inspect
one graph before another, or choosing the most conservative reversible path,
make the decision yourself and continue. State the assumption briefly in
progress updates instead of turning every execution detail into a user choice.

If clarification is needed, batch the questions once and keep them minimal.

## Blueprint-to-C++ migration goal

This workspace should move from Blueprint-first gameplay toward C++-first
gameplay in staged, reviewable steps. The goal is not to remove every
Blueprint. C++ should become the authority for gameplay logic, state,
interaction, AI, input, lifecycle, and reusable systems. Blueprints and assets
should remain the authority for resources, component defaults, art/audio/VFX
references, animation links, UI layout, level placement, and light tuning.

For any request to convert Blueprint logic to C++, reparent a Blueprint, or
decide what belongs in code versus assets, read:
`.codeforge/skills/blueprint-to-cpp-migration/SKILL.md`.

## Unreal Engine evidence chain (use before any UE/MCP conclusion)

The order matters. Every answer that touches UE, Blueprint, 蓝图, C++,
MCP, or ModelContextProtocol must be grounded in this chain:

1. **`.uproject`** — [`CropoutSampleProject.uproject`](../CropoutSampleProject.uproject).
   Confirm `EngineAssociation`, `Modules[]`, and the `Plugins[]` enable
   flags. `ModelContextProtocol` is `Enabled: true` here.
2. **UE install root from `EngineAssociation`** — for `5.8`, the
   inspected install is `C:/Program Files/Epic Games/UE_5.8/`. Engine
   source is at `Engine/Source/`, engine plugins at
   `Engine/Plugins/...`.
3. **Project config** — `Config/DefaultEngine.ini`,
   `Config/DefaultGame.ini`, `Config/DefaultEditor.ini`,
   `Config/DefaultInput.ini`, `Config/DefaultGameplayTags.ini`,
   `Config/Android/*`.
4. **Project logs** — `Saved/Logs/CropoutSampleProject*.log` (latest
   `Saved/Logs/CropoutSampleProject.log`), plus
   `Saved/Logs/UnrealVersionSelector-*.log` for launcher events.
5. **Project C++ / plugin source** — `Source/CropoutSampleProject/` and
   `Plugins/{IconBaker,IslandGenerator}/`. Note: the project plugins are
   **content-only** (no `Source/` subdir).
6. **Engine MCP plugin source** (when the question is MCP-specific) —
   `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`.
   Transport (HTTP), default port (`8000`), default URL path (`/mcp`),
   protocol version (`2025-11-25`), and command-line flags
   (`-ModelContextProtocolPort=N`, `-ModelContextProtocolStartServer`)
   are all defined there.
7. **CodeForge MCP server / tool state** — re-list connected servers and
   tools before relying on transport, url, or registered toolsets.

## Concrete paths in this workspace

- Project C++: `Source/CropoutSampleProject/CropoutSampleProject.{cpp,h,Build.cs}`,
  `Source/CropoutSampleProject/MyGameMode.{cpp,h}`
- Targets: `Source/CropoutSampleProject.Target.cs`,
  `Source/CropoutSampleProjectEditor.Target.cs`
- Project plugins: `Plugins/IconBaker/IconBaker.uplugin`,
  `Plugins/IslandGenerator/IslandGenerator.uplugin` (both content-only)
- Config: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`,
  `Config/DefaultEditor.ini`, `Config/DefaultInput.ini`,
  `Config/DefaultGameplayTags.ini`, `Config/Android/{AndroidDeviceProfiles.ini,AndroidEngine.ini}`
- Content (binary): `Content/MainMenu.umap`, `Content/Village.umap`,
  `Content/Blueprint/...`, `Content/UI/...`, `Content/Characters/...`,
  `Content/Environment/...`, `Content/Audio/...`, `Content/VFX/...`,
  `Content/PackIgnore/...` (cooked-excluded)
- Per-machine / generated: `Binaries/Win64/`, `DerivedDataCache/`,
  `Intermediate/`, `Saved/Logs/`, `Saved/Config/WindowsEditor/`
- Engine MCP plugin (read-only):
  `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`

## MCP transport / port (known, with re-verification required)

- Transport: **HTTP** (verified in engine source — `FHttpServerModule`
  HTTP router bound on POST/GET/DELETE).
- Default URL path: `/mcp` (engine constant).
- Default port: `8000` (engine constant).
- Runtime listener: `127.0.0.1:3094` (latest
  `Saved/Logs/CropoutSampleProject.log` lines 1805–1810).
- CodeForge server name: `unreal-mcp`, url
  `http://127.0.0.1:3094/mcp`, transport `http`.
- Current saved source for the `3094` override:
  `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` section
  `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]`,
  `ServerPortNumber=3094`, `bAutoStartServer=True`. Re-derive on each
  live editor launch.

## AI context index

Pointed at [`doc/ai-context/README.md`](../doc/ai-context/README.md) plus
its linked docs:

- [`doc/ai-context/architecture.md`](../doc/ai-context/architecture.md)
- [`doc/ai-context/modules.md`](../doc/ai-context/modules.md)
- [`doc/ai-context/build-and-run.md`](../doc/ai-context/build-and-run.md)
- [`doc/ai-context/code-navigation.md`](../doc/ai-context/code-navigation.md)
- [`doc/ai-context/unreal.md`](../doc/ai-context/unreal.md)

## Skills

This workspace ships a CodeForge skill for Unreal Engine projects:

- `.codeforge/skills/unreal-project/SKILL.md` — read first when the
  request mentions UE, Unreal, Blueprint, 蓝图, C++, MCP, or
  ModelContextProtocol. Trigger keywords are listed in the skill's
  frontmatter.
- `.codeforge/skills/blueprint-to-cpp-migration/SKILL.md` — read after
  `unreal-project` when the request is about migrating Blueprint logic to
  C++, reparenting Blueprints, or deciding what stays in assets versus code.

## Sub-agents (`.claude/agents/`)

Project-specific sub-agent index lives at
[`.claude/agents/README.md`](../.claude/agents/README.md). Hard rule that
every agent inherits:

> **Read `*.uasset` / `*.umap` only through the live `unreal-mcp` MCP
> toolset. If MCP is unreachable, the agent terminates the task — no
> fallback to `cat` / `Read` / filename inference.**

Roster (cross-references for the orchestrator):

- `ue-blueprint-reader` — generic BP inspector (read-only)
- `ue-game-flow` — BP_GI / BP_GM / BP_SaveGM / BP_MainMenuGM
- `ue-player-and-input` — BP_Player / BP_PC / IMC / IA
- `ue-villager-ai` — BP_Villager / BT / BTT / BB / EQS
- `ue-world-interactable` — BP_Interactable / Building / Resource
- `ue-ui-architect` — CommonUI widgets / HUD
- `ue-asset-graph-editor` — low-level node / pin / DSL editing
- `ue-bp-to-cpp-migrator` — Blueprint → C++ migration driver
- `ue-build-runner` — Build / Cook / PIE / log
- `ue-mcp-ops` — MCP server health / port / path
- `ue-doc-keeper` — sync `doc/ai-context/` and `.codeforge/`

## Verification notes

- This guidance is informational. The current source, `.uproject`,
  project `Config/`, `Saved/Logs/`, the engine MCP plugin source, and
  the live CodeForge MCP server state always take precedence.
- Re-run the evidence chain at the start of each task that depends on
  any of: ports, transports, module/plugin enable flags, Blueprint
  asset behavior, gameplay tag definitions, or build configuration.

**Last verified:** 2026-07-07
