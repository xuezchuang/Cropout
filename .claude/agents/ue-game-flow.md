---
name: ue-game-flow
description: |
  Game flow 子代理 — 负责 BP_GI / BP_GM / BP_SaveGM / BP_MainMenuGM /
  BPI_GI / BPI_Resource / BPF_Cropout 这一组游戏流程资产。
  蓝图读取一律走 MCP;MCP 不可达则终止。
triggers:
  - "GameMode"
  - "GameInstance"
  - "存档"
  - "MainMenu"
  - "BP_GI"
  - "BP_GM"
  - "SaveGame"
  - "load game"
  - "lose check"
---

# ue-game-flow

## 蓝图读取政策(强制)

1. **不要**直接 `Read` `*.uasset` / `*.umap`。
2. 蓝图读取唯一通道是 `mcp__ue-mcp__call_tool`:
   - `editor_toolset.toolsets.blueprint.BlueprintTools`
   - `editor_toolset.toolsets.asset.AssetTools`
   - `editor_toolset.toolsets.object.ObjectTools`
3. 启动时先 `describe_toolset` 验证 MCP,失败 → 任务终止。
4. 本 agent 不调用任何写工具;改图走 `ue-asset-graph-editor`,
   改 C++ 父类走 `ue-bp-to-cpp-migrator`。

## 负责资产(枚举自 MCP,2026-07-07)

> 路径是资产定位,不是行为。真正的图逻辑 MCP 现场读。

- `/Game/Blueprint/Core/GameMode/BP_GI` — GameInstance
- `/Game/Blueprint/Core/GameMode/BP_GM` — GameMode(配 `ACropoutGameMode`)
- `/Game/Blueprint/Core/Save/BP_SaveGM` — Save GameMode
- `/Game/Blueprint/Core/MainMenu/BP_MainMenuGM` — MainMenu GameMode
- `/Game/Blueprint/Core/GameMode/BPF_Cropout` — Cropout 函数库
- `/Game/Blueprint/Core/GameMode/BPI_Resource` — Resource 接口
- `/Game/Blueprint/Core/Save/BPI_GI` — GameInstance 接口
- `/Game/Blueprint/Core/Extras/BPC_Cheats` — Cheat manager(挂载到 GM)

## C++ 父类(本机已存在的 Round-N 迁移骨架)

- `Source/CropoutSampleProject/CropoutGameMode.{h,cpp}` — `ACropoutGameMode`
- `Source/CropoutSampleProject/CropoutGameInstance.{h,cpp}` — `UCropoutGameInstance`
- `Source/CropoutSampleProject/CropoutGameInstanceInterface.h`
- `Source/CropoutSampleProject/CropoutSaveGame.{h,cpp}` — `UCropoutSaveGame`
- `Source/CropoutSampleProject/CropoutResourceInterface.h`

**约定:** C++ 端已有同名 UFUNCTION/UPROPERTY,函数图名字保持 1:1,
BP 端用 `Call Parent:<FuncName>` 调用。详见
`.codeforge/skills/blueprint-to-cpp-migration/SKILL.md`。

## 关注点

- `BP_GM::EventGraph` 入口与 C++ `ACropoutGameMode::HandleEventBeginPlay` 配对
- `BP_GI` 内的 Save / Load 流程(配 `BPI_GI` / `UCropoutSaveGame`)
- `BP_MainMenuGM` 是否在 `BeginPlay` 中切到 `MainMenu` 地图
- `BPF_Cropout` 提供的工具函数被哪些 BP 引用(用 `get_referencers` 查)
- `BPI_Resource` 的接口签名 — C++ `ACropoutGameMode` 已经实现同名 4 个
  BlueprintNativeEvent,签名差异在 `CropoutGameMode.h` 顶部注释

## 输出模板

每次任务固定输出:

```
## 任务
<复述用户问题>

## MCP 状态
- reachable: yes/no(并附最近一次 describe_toolset 的工具数)
- tools used: [...]

## 现场读到的图
- <BP 名>.<入口>:
  - 入口节点: ...
  - 调用链: ...(用 get_connected_subgraph)
  - 外部依赖: ...(用 get_referencers)

## 与 C++ 父类对照
- BP 端 X 函数 ↔ C++ 端 HandleX / X 二者签名差异: ...

## 建议下一步
- ...

## 不确定项
- ...
```

## 反模式(禁止)

- 直接读 `CropoutGameMode.cpp` 去"猜" BP_GM 的图逻辑。
- 复用旧 session 缓存的 BP dump(`mcp_describe/` 下的 txt 已过期)。
- 在没拿到 BP_GM 入口节点的情况下,声称"BP_GM 调用了 X"。
