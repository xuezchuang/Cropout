# Unreal Engine / Blueprint / MCP

## Purpose

The evidence chain that every UE / Blueprint / MCP answer in this
workspace must reference. **Do not skip steps.** Treat any conclusion
that is not backed by items in this chain as
`unknown; verify in code/logs`.

## When to read

- The user asks about UE, Unreal, Blueprint, 蓝图, C++, MCP, or
  ModelContextProtocol
- You need to inspect Blueprint graph logic
- You need to know whether MCP / how MCP is wired into this project
- You need the current MCP transport, port, URL path, or protocol version
- You are about to invoke an MCP tool from the editor

For general project layout, see [`architecture.md`](architecture.md).
For build/launch, see [`build-and-run.md`](build-and-run.md).

## Required evidence chain (use in this order)

1. **`.uproject`** — [`CropoutSampleProject.uproject`](../../CropoutSampleProject.uproject)
   - `EngineAssociation: "5.8"`
   - `Modules[0].Name: "CropoutSampleProject"`, `Type: Runtime`,
     `LoadingPhase: Default`
   - `Plugins[].Name` with `Enabled: true` — confirm
     `ModelContextProtocol` is enabled here (it is)
   - `TargetPlatforms`: Android, Windows, Mac, IOS, TVOS, Linux
2. **UE install root** — discovered from `Saved/Logs/CropoutSampleProject.log`:
   `C:/Program Files/Epic Games/UE_5.8` (also `Engine/Source/`,
   `Engine/Plugins/...` are under that root).
3. **Project config** — `Config/DefaultEngine.ini`,
   `Config/DefaultGame.ini`, `Config/DefaultEditor.ini`,
   `Config/DefaultInput.ini`, `Config/DefaultGameplayTags.ini`,
   `Config/Android/*`
4. **Project logs** — `Saved/Logs/CropoutSampleProject*.log` (latest is
   `Saved/Logs/CropoutSampleProject.log`) plus
   `Saved/Logs/UnrealVersionSelector-*.log`
5. **Project C++** — `Source/CropoutSampleProject/{*.h,*.cpp}` and
   `Source/*.Target.cs`
6. **Project plugins** — `Plugins/{IconBaker,IslandGenerator}/*.uplugin`
   (content-only)
7. **Engine MCP plugin source** —
   `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`
   (this is where transport/port/protocol version live as code)
8. **Live MCP server state** — CodeForge MCP server `unreal-mcp`
   (transport/http, url `http://127.0.0.1:3094/mcp`)

## Key facts (with evidence)

### Engine

- `EngineAssociation = 5.8` (`.uproject`)
- Engine binaries referenced in logs:
  `C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-*.dll`
  (e.g. `UnrealEditor-ModelContextProtocol.dll`).

### Modules

- Project module `CropoutSampleProject` is the only game module; it has
  zero gameplay logic in C++ (only an empty `AGameMode` subclass — see
  [`code-navigation.md`](code-navigation.md)).
- All game behavior is in Blueprints under `Content/Blueprint/...`,
  `Content/UI/...`, etc.

### Blueprint assets are binary

- `*.uasset` and `*.umap` are **binary**. Do not pretend to read them as
  text. Use:
  - The Unreal editor's graph editor
  - The MCP tools exposed by the engine `ModelContextProtocol` plugin
    (e.g. `editor_toolset.toolsets.blueprint.BlueprintTools` — see logs)
  - Generated dumps (JSON exports of Blueprint graphs) when available
  - The original Epic-provided source if you have the sample's source
    drop elsewhere
- Map asset names (`MainMenu.umap`, `Village.umap`) are usable as
  identifiers and are referenced from `Config/DefaultEngine.ini`.

### Model Context Protocol — transport / port / URL

All claims below are backed by code in the engine plugin source under
`C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/`:

- **Transport: HTTP** (not stdio, not Unix sockets). Verified by:
  - [`Source/ModelContextProtocol/Private/ModelContextProtocolServer.cpp`](C:/Program%20Files/Epic%20Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocol/Private/ModelContextProtocolServer.cpp:432)
    — `HttpRouter = FHttpServerModule::Get().GetHttpRouter(ActiveServerPort);`
  - Same file binds `EHttpServerRequestVerbs::VERB_POST` (line 435),
    `VERB_GET` (line 438), and `VERB_DELETE` (line 441) on the MCP path.
  - The runtime plugin **does not currently support SSE on a separate
    endpoint** (line 1073 comment: `// We do not currently support sse
    on a separate endpoint`).
- **Default URL path:** `/mcp` (constant
  `UE::ModelContextProtocol::DefaultServerUrlPath = TEXT("/mcp")` in
  [`Source/ModelContextProtocol/Public/ModelContextProtocol.h`](C:/Program%20Files/Epic%20Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocol/Public/ModelContextProtocol.h:46)).
- **Default port:** `8000` (constant
  `UE::ModelContextProtocol::DefaultServerPort = 8000` in the same
  header, line 45).
- **Current runtime listener:** `127.0.0.1:3094`. Verified by
  `Saved/Logs/CropoutSampleProject.log` lines 1805–1810:
  ```
  LogModelContextProtocol: Starting MCP server on port 3094 (override with -ModelContextProtocolPort=N).
  LogHttpListener: Created new HttpListener on 127.0.0.1:3094
  LogHttpServerModule: All listeners started
  ```
  The override is not present in `Config/Default*.ini` or
  `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`; treat
  the **origin of the 3094 value as unknown; verify against the next
  live launch (e.g. via the MCP server's current state)**.
- **CodeForge MCP server state:** `unreal-mcp` is connected, transport
  `http`, url `http://127.0.0.1:3094/mcp`. Re-check via the CodeForge MCP
  list tool before relying on this in any given turn.
- **DeveloperSettings (`UModelContextProtocolSettings`):**
  [`Source/ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h`](C:/Program%20Files/Epic%20Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h)
  - `UCLASS(... config=EditorPerProjectUserSettings, ...)` — settings
    are stored under `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]`
    in `Saved/Config/<Platform>Editor/EditorPerProjectUserSettings.ini`.
  - Properties: `ServerUrlPath` (default `/mcp`), `ServerPortNumber`
    (default `8000`), `bAutoStartServer` (default `false`),
    `bEnableToolSearch` (default `true`).
- **Command-line overrides:** parsed in
  [`Source/ModelContextProtocolEngine/Private/ModelContextProtocolSettings.cpp`](C:/Program%20Files/Epic%20Games/UE_5.8/Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocolEngine/Private/ModelContextProtocolSettings.cpp):
  - `-ModelContextProtocolPort=N` (validated 1–65535, falls back to
    `ServerPortNumber` if invalid)
  - `-ModelContextProtocolStartServer` (deprecated alias:
    `-StartModelContextProtocolServer`)
- **MCP JSON-RPC protocol version:**
  `UE::ModelContextProtocol::ProtocolVersion = TEXT("2025-11-25")`
  (same `ModelContextProtocol.h`, line 19). Supported versions
  (newest-first): `2025-11-25`, `2025-06-18`, `2024-11-05`.
- **MCP tool surface:** `tool search enabled: registered 3 meta-tools
  (N toolsets discoverable via list_toolsets)`, logged at startup. The
  `Saved/Logs/CropoutSampleProject-backup-2026.07.02-02.19.39.log`
  counts up to **19** toolsets before shutdown (e.g. `ActorTools`,
  `AssetTools`, `BlueprintTools`, `CurveTableTools`, `DataAssetTools`,
  `DataTableTools`, `MaterialTools`, `MaterialInstanceTools`,
  `ObjectTools`, `PrimitiveTools`, `SceneTools`, `SkeletalMeshTools`,
  `StaticMeshTools`, `StringTableTools`, `ProgrammaticToolset`,
  `TextureTools`, plus `EditorToolset.EditorAppToolset`,
  `EditorToolset.LogsToolset`, `ToolsetRegistry.AgentSkillToolset`).

### MCP runtime hints

- **MCP analytics warning** appears at startup after the "Starting MCP
  server" log:
  `LogModelContextProtocol: Warning: Data transmitted via this plugin
  to your connected LLM service is Licensed Technology under the UE
  EULA…`
  If you plan to send data to a third-party LLM, verify with the user
  that this is acceptable per Section 6(e) of the UE EULA.
- **Module load order (from logs):** `HTTPServer` is loaded just
  before the MCP server starts.

### `.uproject` plugin checklist

When the user asks "is plugin X enabled?", check
`CropoutSampleProject.uproject` directly. Confirmed-enabled in this
project (subset):

- `GeometryScripting` — true
- `EnhancedInput` — true
- `CommonUI` — true
- `OnlineSubsystemGoogle`, `OnlineSubsystemFacebook`,
  `OnlineSubsystemApple` — true (Apple has `SupportedTargetPlatforms:
  ["Mac", "IOS", "TVOS"]`)
- `AudioModulation` — true
- `ModelContextProtocol` — true (engine MCP server)
- `EditorToolset` — true (supplies the MCP toolset tools)

## What this doc does NOT cover

- Behavior of any individual Blueprint asset (read through the editor or
  MCP tools).
- Engine source for non-MCP plugins. Refer to `C:/Program Files/Epic
  Games/UE_5.8/Engine/Plugins/` on disk for those.
- Anything about the `Buildable` / `TP_BlankBP` legacy redirect in
  `Config/DefaultEngine.ini` (`+ActiveGameNameRedirects=(OldGameName="TP_BlankBP",NewGameName="/Script/Buildable")`)
  beyond noting it exists — it is engine-side and not part of this
  project's gameplay.

## Verification notes

- "Default" port / URL / protocol version come from the engine plugin's
  header constants — re-read the file at the cited line if the engine
  version changes.
- The **runtime** port and **runtime** MCP server URL come from logs
  and the live CodeForge MCP server state — re-derive them on every
  turn if the editor has been restarted or the port was overridden.
- Do not rely on `EditorPerProjectUserSettings.ini` containing MCP
  values; in this checkout it does not, even though the runtime port is
  `3094`. The 3094 value is therefore **origin unknown in inspected
  files**; verify with the next live launch.
- Blueprint graph behavior must be re-read from the editor or MCP each
  turn; do not cache conclusions across editor restarts.

---

**Last verified:** 2026-07-02