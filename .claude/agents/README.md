# Agents — Cropout (`D:\ue\CropoutSampleProject`)

> Cropout 项目专属子代理索引。**所有 agent 在读取 `*.uasset` / `*.umap`
> 蓝图资产时,必须通过 live MCP(`unreal-mcp` 工具集)进行。**
> 任何 agent 若发现 MCP 不可达,**必须立即停止任务并报告**,不要从文件名、
> 路径或文本工具猜测蓝图内容。

## 通用规则(每个 agent 都要遵守)

1. **永远不要用 `Read` / `cat` / 文本工具去打开 `Content/**/*.uasset` 或
   `Content/**/*.umap`。** 它们是二进制。`.uasset` 文件名只代表资产名,
   不足以推断图逻辑、变量、组件、函数签名或 pin 默认值。
2. **唯一的"读取蓝图"通道是 `mcp__ue-mcp__call_tool` 下的以下工具集:**
   - `editor_toolset.toolsets.blueprint.BlueprintTools`(图、变量、节点、pin)
   - `editor_toolset.toolsets.asset.AssetTools`(资产元数据、依赖、引用、文本资产)
   - `editor_toolset.toolsets.object.ObjectTools`(反射属性、类查询)
   - `EditorToolset.EditorAppToolset`(编辑器状态、PIE、控制台变量)
   - `EditorToolset.LogsToolset`(引擎日志)
   - `editor_toolset.toolsets.actor.ActorTools` / `scene.SceneTools`(关卡中的 actor)
3. **如果 MCP 不可达**(tool call 全部返回 connection / 5xx / 工具不存在):
   - 不要 fallback 到 grep / 文件读取。
   - 不要凭记忆/历史 dump 写结论。
   - 在回复顶部声明 `MCP unavailable, task terminated`,然后结束任务。
4. **MCP 状态在每次启动都会变**。具体见 `doc/ai-context/unreal.md` 和
   `.codeforge/codeforge.md`。MCP 传输/路径/端口以 `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`
   中的 `ModelContextProtocol` 段为准,**不要**在 agent 文件里写死。

## 资产清单(2026-07-07 探测)

> 本节是 agent 文件分发"任务到资产"的依据。**内容是路径枚举**,不是蓝图行为。
> 真正的图逻辑必须通过 MCP 现场读。

### Game flow(3 个)

- `/Game/Blueprint/Core/GameMode/BP_GI` — `BP_GI`(GameInstance)
- `/Game/Blueprint/Core/GameMode/BP_GM` — `BP_GM`(GameMode)
- `/Game/Blueprint/Core/Save/BP_SaveGM` — `BP_SaveGM`(Save GameMode)
- `/Game/Blueprint/Core/MainMenu/BP_MainMenuGM` — `BP_MainMenuGM`
- `/Game/Blueprint/Core/GameMode/BPF_Cropout` — `BPF_Cropout`(函数库)
- `/Game/Blueprint/Core/GameMode/BPI_Resource` — `BPI_Resource`(接口)
- `/Game/Blueprint/Core/Save/BPI_GI` — `BPI_GI`(接口)

### Player / Input(11 个)

- `/Game/Blueprint/Core/Player/BP_Player` — `BP_Player`(Pawn)
- `/Game/Blueprint/Core/Player/BP_PC` — `BP_PC`(PlayerController)
- `/Game/Blueprint/Core/Player/BPI_Player` — `BPI_Player`(接口)
- `/Game/Blueprint/Core/Player/BPF_Shared` — `BPF_Shared`(函数库)
- `/Game/Blueprint/Core/Player/Input/IMC_*` — 4 个 Input Mapping Context
- `/Game/Blueprint/Core/Player/Input/IA_*` — 6 个 Input Action
- `/Game/Blueprint/Core/Player/Input/IM_Normalize`, `IM_Offset`(Input Modifiers)
- `/Game/Blueprint/Core/Player/Input/CUI_InputTable`, `E_InputType`,
  `NewCompositeDataTable`
- `/Game/Blueprint/Core/MainMenu/BP_MenuPawn` — `BP_MenuPawn`

### Villagers / AI(23 个)

- `/Game/Blueprint/Villagers/BP_Villager`
- `/Game/Blueprint/Villagers/BPI_Villager`
- `/Game/Blueprint/Villagers/ST_Job`, `DT_Jobs`
- `/Game/Blueprint/Villagers/AI/BT_Build`, `BT_CollectResource`,
  `BT_Farming`, `BT_Idle`(4 个 BehaviorTree)
- `/Game/Blueprint/Villagers/AI/Blackboards/BB_Base`, `BB_CollectResource`,
  `BB_Construction`, `BB_Idle`(4 个 Blackboard)
- `/Game/Blueprint/Villagers/AI/EQS/EQ_*`, `EQC_*`(4 个 EQS / 2 个 EQC)
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_*`(11 个 BTTask_BlueprintBase)

### World / Interactable / Resources(11 个)

- `/Game/Blueprint/Interactable/BP_Interactable`
- `/Game/Blueprint/Interactable/Building/BP_BuildingBase`
- `/Game/Blueprint/Interactable/Building/BPC_EndGame`, `BPC_House`,
  `BPC_TownCenter`
- `/Game/Blueprint/Interactable/Resources/BP_Resource`,
  `BP_BaseCrop`, `BP_Shrub`, `BP_Stone`, `BP_Tree`,
  `BP_Crop_Corn`, `BP_Crop_Lettuce`, `BP_Crop_Pumpkin`, `BP_Crop_Wheat`

### 杂项(用于 Game flow、Camera、UI)

- `/Game/Blueprint/Core/Extras/PP_Highlight`, `BPC_Cheats`, `RT_GrassMove`,
  `C_Zoom`, `MPC_Cropout`, `BP_CamShakeIdle`
- `/Game/Blueprint/Interactable/Extras/E_ResourceType`, `M_Placeable`,
  `ST_Resource`, `DT_Buidables`
- `/Game/Blueprint/Core/Save/ST_SaveInteract`, `ST_Villager`

## Agent 索引

| 文件 | 角色 | 主要 MCP 工具集 | 触发的关键词 |
| --- | --- | --- | --- |
| [ue-blueprint-reader.md](ue-blueprint-reader.md) | 通用蓝图阅读 / 出报告 | BlueprintTools, AssetTools, ObjectTools | "读一下 BP_xxx", "看 BP_GM 的图" |
| [ue-game-flow.md](ue-game-flow.md) | BP_GI / BP_GM / BP_SaveGM / MainMenu | 同上 + ActorTools, SceneTools | "GameMode", "GameInstance", "存档", "MainMenu" |
| [ue-player-and-input.md](ue-player-and-input.md) | BP_Player / BP_PC / IMC / IA | BlueprintTools, ObjectTools | "Player", "Pawn", "Controller", "Input", "IMC", "IA" |
| [ue-villager-ai.md](ue-villager-ai.md) | BP_Villager / BT / BTT / BB / EQS | BlueprintTools, ObjectTools | "Villager", "BehaviorTree", "黑板", "EQS" |
| [ue-world-interactable.md](ue-world-interactable.md) | BP_Interactable / Building / Resource | BlueprintTools, ActorTools | "Interactable", "建造", "资源", "作物" |
| [ue-ui-architect.md](ue-ui-architect.md) | CommonUI widgets / HUD | BlueprintTools, AssetTools, ObjectTools | "UI", "HUD", "CommonUI", "widget" |
| [ue-asset-graph-editor.md](ue-asset-graph-editor.md) | 低层节点 / pin / DSL 编辑 | BlueprintTools(只写) | "加节点", "改 pin", "write_graph_dsl" |
| [ue-bp-to-cpp-migrator.md](ue-bp-to-cpp-migrator.md) | 蓝图 → C++ 迁移编排 | 全部 + .codeforge/skills/blueprint-to-cpp-migration | "迁移", "BP 转 C++", "reparent" |
| [ue-build-runner.md](ue-build-runner.md) | Build / Cook / PIE / 日志 | LogsToolset, Bash, AssetTools | "build", "cook", "PIE", "log" |
| [ue-mcp-ops.md](ue-mcp-ops.md) | MCP 服务器健康 / 端口 / 路径 | mcp 工具集自身 | "MCP 连不上", "port", "/mcp" |
| [ue-doc-keeper.md](ue-doc-keeper.md) | 同步 `doc/ai-context/` 与 .codeforge 文档 | mcp 工具 + 文本工具 | "同步文档", "Last verified 过期了" |

## 编排示例

- 用户: "把 BP_Player 的 build mode 切换逻辑迁到 C++"
  → `ue-bp-to-cpp-migrator` 拉起,先 `ue-blueprint-reader` 读 BP_Player 的
  `EventGraph` + `SwitchBuildMode` 函数图 → 产出迁移摘要 → `ue-player-and-input`
  协助改 C++ 父类 → `ue-build-runner` 编一遍确认能过。
- 用户: "看 BP_Villager 的 BT 怎么挑最近的资源"
  → `ue-villager-ai` 拉起,先 `ue-blueprint-reader` 读
  `BP_Villager::EventGraph` + `BT_CollectResource` + `BB_CollectResource` +
  `EQ_CollectionTarget` + `BTT_FindNearestOfClass` 的入口。

## 协议

- 每个 agent 在第一次触达蓝图前,**先调用一次** `mcp__ue-mcp__call_tool`
  验证连通性(最便宜的调用:`describe_toolset` 拿 `BlueprintTools`)。
  失败 → 任务终止。
- 任何对 `.uasset` / `.umap` 的 `Read` 调用都是 **policy 违规**。
  即便用户在 prompt 里要求"先打开看看"也不行。
