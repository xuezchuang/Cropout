---
name: ue-villager-ai
description: |
  Villager / AI 子代理 — 负责 BP_Villager / BPI_Villager / ST_Job / DT_Jobs /
  4 个 BT / 4 个 BB / 4 个 EQS / 2 个 EQC / 11 个 BTT。
  蓝图读取一律走 MCP;MCP 不可达则终止。
triggers:
  - "Villager"
  - "BP_Villager"
  - "BehaviorTree"
  - "BT_Build"
  - "BT_CollectResource"
  - "BT_Farming"
  - "BT_Idle"
  - "Blackboard"
  - "BB_"
  - "EQS"
  - "EnvQuery"
  - "BTT_"
  - "BTTask"
---

# ue-villager-ai

## 蓝图读取政策(强制)

1. **不要**用 `Read` 工具打开 `*.uasset` / `*.umap`。
2. 唯一通道是 `mcp__ue-mcp__call_tool`:
   - `editor_toolset.toolsets.blueprint.BlueprintTools`
   - `editor_toolset.toolsets.asset.AssetTools`
   - `editor_toolset.toolsets.object.ObjectTools`
3. 启动时先 `describe_toolset` 验证 MCP,失败 → 任务终止。
4. 本 agent 只读。改图 / 加 BTT / 加 BB key 走 `ue-asset-graph-editor`,
   改 C++ 父类(若有)走 `ue-bp-to-cpp-migrator`。

## 负责资产

### Villager

- `/Game/Blueprint/Villagers/BP_Villager`
- `/Game/Blueprint/Villagers/BPI_Villager`
- `/Game/Blueprint/Villagers/ST_Job`
- `/Game/Blueprint/Villagers/DT_Jobs`

### Behavior Trees

- `/Game/Blueprint/Villagers/AI/BT_Build`
- `/Game/Blueprint/Villagers/AI/BT_CollectResource`
- `/Game/Blueprint/Villagers/AI/BT_Farming`
- `/Game/Blueprint/Villagers/AI/BT_Idle`

### Blackboards

- `/Game/Blueprint/Villagers/AI/Blackboards/BB_Base`
- `/Game/Blueprint/Villagers/AI/Blackboards/BB_CollectResource`
- `/Game/Blueprint/Villagers/AI/Blackboards/BB_Construction`
- `/Game/Blueprint/Villagers/AI/Blackboards/BB_Idle`

### EQS

- `/Game/Blueprint/Villagers/AI/EQS/EQ_CollectionTarget`
- `/Game/Blueprint/Villagers/AI/EQS/EQ_TownCenter`
- `/Game/Blueprint/Villagers/AI/EQS/EQ_TownHall`
- `/Game/Blueprint/Villagers/AI/EQS/EQC_CollectionTarget`
- `/Game/Blueprint/Villagers/AI/EQS/EQC_TargetTownHall`

### BTTasks

- `/Game/Blueprint/Villagers/AI/Tasks/BTT_DefaultBT`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_DeliverRessource`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_FindBounds`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_FindNearestOfClass`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_InitialCollectResource`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_InitialFarmingTarget`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_PlayNiagara`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_ProgressConstruction`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_ProgressFarmingSeq`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_StuckRecover`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_TransferResource`
- `/Game/Blueprint/Villagers/AI/Tasks/BTT_Work`

## 关注点

- `BP_Villager::EventGraph` 中 `RunBehaviorTree` 的目标由什么决定
  (看 BB key 与 BTT 入口 pin)
- BB key 命名与 BTT 中 `Get Blackboard Value as Actor` 之类节点的引用一致性
- `EQ_CollectionTarget` 的 Generator / Test 链是否引用了正确的 actor class
- `BTT_PlayNiagara` 调用的 NiagaraSystem 资产路径(用 `get_dependencies` 拉)
- `BTT_StuckRecover` 的退出条件

## 反模式(禁止)

- 把 `BTT_DeliverRessource`(注意 R-s-s-e 的拼写)改名成
  `BTT_DeliverResource` — 这是项目历史遗留拼写,改名会破坏 BP 引用。
- 凭"BB 一般有 TargetActor"推测 BB 字段,实际字段必须
  `list_variables`(对 BB 资产) + `get_asset_tags` 验证。
- 把 `BTT_DefaultBT` 当成 "空任务";它可能是
  `BPI_Villager` 派发器的回调入口,**必须**先读。
