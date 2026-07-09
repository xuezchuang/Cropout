# Round 10 — BP graph persistence restored (R6 + R7 + R8 disk landing)

## Scope
After two sessions of in-memory routing that the broken MCP harness
could not persist, this round successfully wrote the R6 (BP_GM
8-event routing), R7 (BP_GI 9-event routing), and R8
(`set_parent(BP_SaveGM, UCropoutSaveGame)`) changes to disk by
bypassing the broken ZCode MCP client and talking directly to the
UE ModelContextProtocol HTTP server at `127.0.0.1:3094/mcp`.

## Path taken

1. **Restarted the editor** that held the original in-memory edits.
   The edits were lost (expected), but the restart was necessary to
   break ZCode's stale-session state on the MCP server.
2. **Started a fresh editor** (PID 47620) via
   `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" D:\ue\CropoutSampleProject\CropoutSampleProject.uproject`.
3. **Discovered direct MCP** is reachable at
   `http://127.0.0.1:3094/mcp` (Streamable HTTP, server returns
   `Mcp-Session-Id` header on `initialize`, accepts JSON-RPC 2.0
   `tools/call` envelopes).
4. **Wrote an SSE-aware Python MCP client** (`C:/Temp/mcp_util.py`)
   that reads the first `data: {json}` frame of an SSE response and
   closes the connection (the MCP server keeps the connection open
   indefinitely otherwise, which hung `urllib.request.urlopen`).
5. **Probed every `K2Node_Event_N` and `K2Node_CustomEvent_N`** for
   BP_GM and BP_GI via `get_connected_subgraph` to map each entry to
   its `type_id` (e.g. `AddEvent|Custom|OpenLevel`). Confirmed 8+9
   events = matches the C++ handler set 1:1.
6. **Re-ran the R6 routing** (8 `create_node` + 8 `connect_pins`)
   followed by **the R7 routing** (9 `create_node` + 9 `connect_pins`),
   then a single `save_assets` for both.
7. **`set_parent(BP_SaveGM.BP_SaveGM, /Script/CropoutSampleProject.CropoutSaveGame)`** + `save_assets`.

## create_node schema fix (vs prior session)

The earlier session used `node_class: { refPath: "...K2Node_CallFunction" }`,
which is rejected by the current MCP. The correct schema is:

```json
{
  "graph":         {"refPath": "<asset>.EventGraph"},
  "type_id":       "Cropout|GameMode|Event|<UFUNCTION>",   // or "|GameInstance"
  "pos":           {"x": <int>, "y": <int>},
  "declaring_class": {"refPath": "/Script/CropoutSampleProject.<parent>"}
}
```

The `type_id` string is `Category|Subcategory|<FuncName>` and
mirrors the UFUNCTION's `Category=` annotation in the C++ header
(`Category = "Cropout|GameMode|Event"` →
`type_id = "Cropout|GameMode|Event|<HandleX>"`).

Successful type_ids issued:
- 8 on `BP_GM`: `Cropout|GameMode|Event|HandleEventAddUI`,
  `…|HandleEventAddResource`, `…|HandleEventRemoveCurrentUILayer`,
  `…|HandleEventBeginPlay`, `…|HandleSpawnVillagers`,
  `…|HandleEndGame`, `…|HandleEventIslandGenComplete`,
  `…|HandleBeginASyncSpawning`.
- 9 on `BP_GI`: `Cropout|GameInstance|Event|HandleEventInit`,
  `…|HandleEventSaveGame`, `…|HandleEventUpdateAllInteractables`,
  `…|HandleEventLoadLevel`, `…|HandleEventUpdateAllResources`,
  `…|HandleEventUpdateAllVillagers`, `…|HandleEventClearSave`,
  `…|HandleAddLoadingUI`, `…|HandleOpenLevel`.

`declaring_class` path is `/Script/CropoutSampleProject.CropoutGameMode`
or `.CropoutGameInstance` (NOT the `A`/`U` prefix — this is the
`UClass*` of the BP-style GeneratedClass, not the C++ class).
`set_parent` accepted `/Game/.../BP_SaveGM.BP_SaveGM` (no `_C` suffix).

## Outcomes

| Step | Result |
|---|---|
| BP_GM `create_node × 8` | 8 K2Node_CallFunction_61..68 returned |
| BP_GM `connect_pins × 8` | all `{returnValue: null}` |
| BP_GI `create_node × 9` | 9 K2Node_CallFunction_14..26 returned |
| BP_GI `connect_pins × 9` | all `{returnValue: null}` |
| `save_assets(BP_GM, BP_GI)` | `{returnValue: true}` |
| `is_dirty(BP_GM)` / `is_dirty(BP_GI)` | both `false` |
| `set_parent(BP_SaveGM, CropoutSaveGame)` | `{returnValue: null}` |
| `save_assets(BP_SaveGM)` | `{returnValue: true}` |
| `is_dirty(BP_SaveGM)` | `false` |

### Disk MD5 shift (vs. pre-r10 state)
```
BP_GM.uasset     07c7f714d146c1e14a4516b7f762689b   (was Auto1 pre-R6)
BP_GI.uasset     ef8fdc366b8ac12af4a5c2799145f23d   (was pre-R7)
BP_SaveGM.uasset caf6cfd3299d98a90cc3d4469a21f98d   (was BP-defined, now reparented)
```
File sizes grew by ~17 KB (BP_GM, BP_GI from routing) and BP_SaveGM
was rebuilt (10233 bytes — still small, no body nodes added).

### UBT compile sanity
- `Build.bat CropoutSampleProjectEditor Win64 Development ...`
  → `Target is up to date` (no .cpp/.h changed since last compile, DLL -0034 still valid).
- The BP_SaveGM reprocessing happens at editor load time, not at UBT
  compile time, so a reparent does not trigger a C++ recompile. The
  BP-side class recompile will happen on next editor load; the
  C++ `UCropoutSaveGame` symbols are already linked in `-0034.dll`.

## Side artefacts (not in repo)
- `C:/Temp/mcp_util.py` — reusable SSE-aware MCP HTTP client
  (Python 3 stdlib only).
- `C:/Temp/reroute3.py` — R6+R7 reroute script (single-variant
  `Cropout|<class>|Event|<handler>` type_ids).
- `C:/Temp/r8_save.py` — `set_parent` + `save_assets` for BP_SaveGM.
- `C:/Temp/set_parent2.py` — schema-shape probe used to discover
  that `BP_SaveGM.BP_SaveGM` (no `_C`) is the correct path.

## Open items (not in this session)
> RESOLVED in Round 11. BP_SaveGM is data-only; no BP-side function
> overrides exist. `Cast<UCropoutSaveGame>(SaveRef)` already
> dispatches directly to the native UFUNCTIONs at runtime.
> Details in [`round-11-bp-save-gm-data-only.md`](round-11-bp-save-gm-data-only.md).

## Files touched
- `Content/Blueprint/Core/GameMode/BP_GM.uasset` (R6 routing saved)
- `Content/Blueprint/Core/GameMode/BP_GI.uasset` (R7 routing saved)
- `Content/Blueprint/Core/Save/BP_SaveGM.uasset` (R8 reparented)
- `doc/round-reports/round-10-persistence-restored.md` (this file)
