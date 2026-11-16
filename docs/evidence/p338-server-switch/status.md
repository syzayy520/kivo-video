# 338 · Emby 原地切服（2026-11-16 20:46，K40 b6e950ee）

## 用户诉求
「emby里面切服最好不要回到资源库，而是直接切换到目标服，你这样就多此一举了」。

## 根因（代码实证）
旧 `EmbyActivity.onSwitchSource` 是
`startActivity(Intent(this, EmbyActivity::class.java).putExtra("kivo.sourceId", newId)) + finish()`。
虽然带了新的 sourceId、名义上"原地换服"，但：
1. 新 Activity 创建**全新 ViewModel**，client/auth/hero/views/图片缓存全部丢失；
2. `checked=false` 首帧显示「正在读取资源库」+「返回资源库」；
3. `openSaved()` 再显示「正在连接资源库」+「返回资源库」，全量重走 login+items+hero+resume。

用户肉眼看到的就是"又回资源库走一遍"——不是错觉，是真重启。

## 家族树（先规划后实现）
```
emby/
├─ EmbyActivity.kt                   [改] 去掉重启；sourceId 可变；gate 绑初始服；onSwitchSource → model.switchTo()
├─ EmbySessionModel.kt               [改] 抽 connectCore()；switchTo() 原地换会话；connect 成功登记快照
├─ session/EmbySessionRegistry.kt    [新] 会话快照 + 按 sourceId 的 LRU(4) 注册表（只驻留内存）
└─ library/EmbyLibraryModel.kt       [改] snapshot()/restore()，保住列表/滚动/查询态
ui/EmbyBrowseScreen.kt               [改] hero 取数监听 sourceId（原地切服 model 实例不变）
```

## 实现要点
- **命中缓存**：目标服之前连过 ⇒ 原样恢复 client/auth/列表/hero/评分/服名，零 loading、零网络。
- **未命中**：旧页**完全不动**，后台 login+items+hero+resume；成功后在 `main.post` 一次性原子替换
  （`activeId`/savedName/client/auth/library/hero/views），失败只留日志并保留当前服。
- **关键坑（真机暴露）**：
  1. `EmbyBrowseScreen` 原本 `LaunchedEffect(model)` 只监听实例；原地切服实例不变 ⇒ hero 一直是旧服内容。
     改为 `LaunchedEffect(model, model.currentSourceId)`，但不用 sourceId 作 remember key（避免切服瞬间闪空位）。
  2. `activeId` 必须是 `mutableStateOf`，否则 Compose 观察不到切服（服名 savedName 是 state 会变，hero 却不变）。
  3. 提交时清 `views/viewsLoaded`，否则旧服分类串到新服页面。
- 安全：注册表不含密码；凭据恢复时不缓存；token 随 auth 驻留，生命周期同 ViewModel（back/onCleared 即清）。

## 真机证据
### 埋点（NetDiag，落盘原文）
```
1794833126784 [emby-switch] 4631a814… 未缓存，后台连接（旧页保留）
1794833128591 [emby-switch] 4631a814… 原子切换完成          ⇒ 1807ms
1794833139847 [emby-switch] 856c46dc… 命中缓存，秒切          ⇒ 无需网络
```
（PowerShell 控制台把中文显示为乱码，文件内为 UTF-8 原文；ID 为 sourceId。）

### 录屏逐秒宫格
`switch_flow_grid.png`（6×5，30 秒，原始 `switch_flow.mp4`）：
- 第 1–5 格 A 服（nyamedia / 谷雨 / 贫贱·读书会）→ 第 6 格打开菜单；
- 第 7–8 格仍在 A 服内容（后台连接中，**旧页保留**）；
- 第 9 格起 B 服（iLoveHD / 财神崬占… / 空枪·孤立无援）；
- 第 21 格再次打开菜单 → 第 22 格起 A 服内容**1 秒内完整恢复**（缓存秒切）。

**全程 30 格：0 格出现「正在读取资源库 / 正在连接资源库 / 返回资源库」，0 格白屏或全屏转圈。**

单帧：`switched_server.png`（B 服内容）、`switched_back_cached.png`（秒切回 A）、
`back_to_library.png`（点返回键仍正常回资源库列表）。

## 门禁
- `BUILD SUCCESSFUL in 14s`（`:app:assembleDebug` 全量）。
- Kotlin 行数：EmbySessionModel 299、EmbyLibraryModel 120、EmbyBrowseScreen 227、
  EmbyActivity 161、EmbySessionRegistry 39 —— 全部 ≤300。
- 旧切服重启代码已彻底移除（`Intent(EmbyActivity::class.java)` 无残留）。
- UI 视觉零改动（仅数据订阅 key 一行）。

## 诚实边界
- B 服（iLoveHD）首屏 hero 显示「封面加载失败，点击重试」：该服图片 URL 本身拉取失败，
  非切服缓存串台（同帧 Emby 元数据/继续观看均正确、A 服切回后图片正常）；图片链路未在本轮修改。
- 会话缓存只在内存、上限 4 服；进程被杀后首次切仍要网络重连（安全约束：不持久化 token）。
- 仅 K40（b6e950ee / A13 / kona）验证；MatePad 未跑，无跨 SoC/跨版本证据。
