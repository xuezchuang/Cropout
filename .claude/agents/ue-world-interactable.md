---
name: ue-world-interactable
description: |
  World / Interactable 子代理 — 负责 BP_Interactable / BP_BuildingBase /
  3 个 BPC_(EndGame|House|TownCenter) / BP_Resource / BP_BaseCrop /
  4 个 BP_Crop_* / BP_Shrub / BP_Stone / BP_Tree 以及它们配套的
  E_ResourceType / ST_Resource / DT_Buidables / M_Placeable。
  蓝图读取一律走 MCP;MCP 不可达则终止。
triggers:
  - "Interactable"
  - "建造"
  - "资源"
  - "作物"
  - "BP_Resource"
  - "BP_BuildingBase"
  - "BP_Crop"
  - "BP_Stone"
  - "BP_Tree"
  - "BP_Shrub"
  - "BPC_House"
  - "BPC_TownCenter"
  - "BPC_EndGame"
  - "M_Placeable"
  - "DT_Buidables"
---

# ue-world-interactable

## 蓝图读取政策(强制)

1. **不要**用 `Read` 工具打开 `*.uasset` / `*.umap`。
2. 唯一通道是 `mcp__ue-mcp__call_tool`:
   - `editor_toolset.toolsets.blueprint.BlueprintTools`
   - `editor_toolset.toolsets.asset.AssetTools`
   - `editor_toolset.toolsets.actor.ActorTools`(关卡中已放置的实例)
   - `editor_toolset.toolsets.scene.SceneTools`(关卡)
   - `editor_toolset.toolsets.static_mesh.StaticMeshTools`
   - `editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools`
3. 启动时先 `describe_toolset` 验证 MCP,失败 → 任务终止。
4. 本 agent 只读。改图 / 加变量 / 改 pin 默认值走 `ue-asset-graph-editor`。

## 负责资产

### 基础 / 通用

- `/Game/Blueprint/Interactable/BP_Interactable`
- `/Game/Blueprint/Interactable/Extras/E_ResourceType`
- `/Game/Blueprint/Interactable/Extras/M_Placeable`
- `/Game/Blueprint/Interactable/Extras/ST_Resource`
- `/Game/Blueprint/Interactable/Extras/DT_Buidables`

### Building(配 `BPC_*` 组件)

- `/Game/Blueprint/Interactable/Building/BP_BuildingBase`
- `/Game/Blueprint/Interactable/Building/BPC_EndGame`
- `/Game/Blueprint/Interactable/Building/BPC_House`
- `/Game/Blueprint/Interactable/Building/BPC_TownCenter`

### Resources

- `/Game/Blueprint/Interactable/Resources/BP_Resource`
- `/Game/Blueprint/Interactable/Resources/BP_BaseCrop`
- `/Game/Blueprint/Interactable/Resources/BP_Crop_Corn`
- `/Game/Blueprint/Interactable/Resources/BP_Crop_Lettuce`
- `/Game/Blueprint/Interactable/Resources/BP_Crop_Pumpkin`
- `/Game/Blueprint/Interactable/Resources/BP_Crop_Wheat`
- `/Game/Blueprint/Interactable/Resources/BP_Shrub`
- `/Game/Blueprint/Interactable/Resources/BP_Stone`
- `/Game/Blueprint/Interactable/Resources/BP_Tree`

## 关注点

- `BP_BuildingBase` 与 `BP_Interactable` 的继承关系(`get_parent` 验证)
- `BPC_House` / `BPC_TownCenter` / `BPC_EndGame` 是不是
  `ActorComponent` 蓝图(用 `get_asset_class` 验证)
- `BP_Resource` 的 `ECropoutResourceType` 枚举绑定:用
  `ObjectTools` 查 `bpi_resource` 上的 enum key,与 C++
  `Source/CropoutSampleProject/CropoutResourceType.h` 对照
- `M_Placeable` / `DT_Buidables` 是资源表,不是图逻辑 — 用
  `AssetTools.DataTableTools` / `AssetTools.read_file`(M_Placeable 是
  DataTable)读取 JSON/CSV,不要走 `BlueprintTools`
- `BP_Crop_*` 的"作物阶段"字段(看 `list_variables`),决定收获时机

## 反模式(禁止)

- 改 `BPC_*` 的组件类时改父类;`BPC_*` 是 **ActorComponent 蓝图组件**,
  改父类会破坏所有 `BP_BuildingBase` 子类对组件字段的引用。
- 把 `M_Placeable` 当成 MaterialInstance;用 `get_asset_class` 先验证。
- 直接 `grep` 蓝图资源路径去推断 `BP_Crop_Corn` 比 `BP_Crop_Wheat`
  快 N 天 — 这两个资产用 `find_assets(name="BP_Crop_")` 一行就列全。
