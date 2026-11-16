# 337 · 资源库连接提速验证（2026-11-16 20:28，K40 b6e950ee）

## 用户诉求
「连接资源库的时候有点慢，这能优化吗」。

## 取证方法（非猜测）
NetDiag 落盘埋点（login / items / hero 各阶段毫秒时间戳）+ screenrecord 抽帧宫格（每 0.2s）。
两次真机重连取数，排除单次网络抖动。

## 根因数据（优化前，埋点铁证）
```
1794831667334 [emby-connect] login 1257ms
1794831668018 [emby-connect] items 684ms total 1941ms count=60
1794831668057 [emby-hero] 发起 recent + resume        ← 连接完成后才发
1794831669451 [emby-hero] resume 完成 1392ms
1794831670169 [emby-hero] recent 完成 2111ms
```
⇒ 端到端 ≈ login 1.26 + items 0.68 + hero 2.11 ≈ **4.0s**，三段严格串行。
录屏 `connect_flow_grid.png`：转圈 ~6 格 + **空白 ~12 格**（hero 请求中无内容）+ 海报。

## 修复
`EmbySessionModel.connect()`：login 拿到 token 后**立即并发**发起 items + hero recent + resume
（三者都只依赖 token），结果缓存 `heroPrefetch`（带 generation 防切服串数据）；
`heroRaw()`/`resumeItems()` 优先读缓存，秒返。`EmbyBrowseScreen` 零改动（UI 不碰）。
线程池 2→4（容纳三个并发请求）；诊断埋点保留（NetDiag 只记耗时/条数，不记 token）。

## 优化后铁证（埋点）
```
login 302ms
  ├─ recent 发起 +3ms   ┐
  ├─ resume 发起 +6ms   ├─ 三者并行
  └─ items              ┘
items 完成 1127ms (total 1429ms) / recent 1308ms / resume 2348ms
```
- 理论总时长 3777ms → **2650ms**（items 1127ms 全程被重叠，省 1.1s）。
- hero 主角 recent 1308ms 完成，与 items 几乎同时 → 首屏不再被 items 阻塞。
- 缓存命中：同一次连接 hero recent 只发一次（埋点仅一条"发起"）。

## 录屏体感对比
- 旧 `connect_flow_grid.png`：转圈 → **空白小转圈 ~12 格** → 海报（二段式割裂）。
- 新 `connect_final_grid.png`：转圈 → **直接出海报道具**（空白格 0），二段式消失。

## 诚实边界
- login（跨网认证）无法省：安全约束禁止持久化 token（SessionModel 注释明确），转圈含 login 时间。
- 转圈本身变长约 1s（含 hero 等待），但换来"转圈后直接出内容"，消除最刺眼空白；产品权衡。
- resume 2348ms 参与阻塞（本可后台，下方内容）；为缓存一致性本轮一起等，后续可拆异步。
- hero recent 的 12 字段冗余未精简（怕破坏展示），留作后续。
- 网络波动大（login 298ms~1257ms），不承诺固定秒数，收益以"重叠 items 时间"为准。
- `BUILD SUCCESSFUL`；改动 2 个 Kotlin 文件（EmbySessionModel 240 行、EmbyHeroSource 115 行）均 <300；UI 零改动。
