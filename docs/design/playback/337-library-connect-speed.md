# 337 · 资源库连接提速（诊断 + 并发预取）

## 一、真机取证（K40 b6e950ee，NetDiag 埋点 + 录屏，非猜测）

用户：「连接资源库的时候有点慢，能优化吗」。先埋点测真实耗时，不猜。

埋点两次（网络有波动），第二次完整（点 emby.bangumi.ca 源）：

```
1794831667334 [emby-connect] login 1257ms
1794831668018 [emby-connect] items 684ms total 1941ms count=60
1794831668057 [emby-hero] 发起 recent(12字段/Limit8) + resume(12字段/Limit12)   ← 连接完成后 39ms 才发
1794831669451 [emby-hero] resume 完成 1392ms
1794831670169 [emby-hero] recent 完成 2111ms                                    ← hero 主角最后到
```

第一次（网络较好）：login 298ms + items 526ms total 824ms；hero recent 2111ms / resume 1392ms。

**端到端 ≈ login + items + hero = 1.26 + 0.68 + 2.11 ≈ 4.0s**（差的网络更久）。

录屏宫格 `connect_flow_grid.png`（每 0.2s）体感分解：
- 1.2s "正在连接资源库"转圈（login+items，串行）
- ~2.4s 首屏空白 + 小转圈（hero 请求中，无内容）
- 最后 hero 海报才出

⇒ 用户说的"连接慢"其实是**连接 1.9s + 连接后空白 2.4s**，空白更刺眼。

## 二、根因（结构性）

1. **串行依赖**：`connect()` 里 `login(POST) → items(GET)` 严格串行
   （items 需要 token，硬依赖，无法并行）。这一步 1.9s 里 login 1.26s 占大头（跨网认证）。
2. **hero 请求晚发**：`inLibrary=true` → EmbyBrowseScreen 组合 → `LaunchedEffect` 才发 hero recent/resume。
   **hero 只依赖 token（login 就有），却被排在 items 之后**，白等 items 的 684ms + Compose 首帧。
3. **hero 字段冗余**：recent 请求 12 个字段（Overview/Genres/Backdrop/…）导致 2.1s；
   而首屏 hero 大图只需要 id/name/ImageTags/BackdropImageTags/日期/评分 —— 一半字段用不上。

## 三、与 336 的关系（不重复造轮子）
- 336 已让 hero 与 resume **彼此并行**、TMDB 评分后台补 —— 保留。
- 337 解决的是**更早一层**：hero/resume 相对 items 的**发起时机**（提前到 login 后）。
- 两者正交，叠加生效。

## 四、方案：login 后并发预取 hero（preview-then-replace 的 Emby 版）

核心：**login 拿到 token 的那一刻，立刻并发发 items + hero recent + resume**（三者都只依赖 token），
hero 结果缓存进 model，首页 effect 直接读缓存（不再重复请求）。

- hero 的 2.1s 从「items 完成后」提前到「login 完成后」⇒ 与 items 的 0.68s 重叠，
  端到端 4.0s → ~3.3s（login + max(items, hero)）。
- 更关键：**连接转圈结束后 hero 数据已在手或接近完成**，首屏空白期大幅缩短。

## 五、文件家族树（先看树再写）
```
emby/
├── EmbySessionModel.kt   [改] connect() login 后并发预取 hero/resume 到缓存；
│                              heroRaw()/resumeItems() 优先返回缓存（带 generation 失效保护）
├── EmbyHeroSource.kt     [改] (诊断埋点已加，保留) 可考虑 recent 精简字段（本轮先不动，避免破坏展示）
└── ui/EmbyBrowseScreen.kt [不改] LaunchedEffect 仍调 model.heroRaw()/resumeItems()，
                                  现从缓存秒返，无需改动 UI
```
- **不新建平行数据源**：复用现有 `heroRaw()`/`resumeItems()`/`EmbyHeroSource`，只加缓存层与预取触发。
- **不改 UI**：EmbyBrowseScreen 零改动，视觉零变化。
- **缓存失效**：切服/重连时 `generation++`，预取结果带 generation 标记，过期不采用。

## 六、实施结果（真机埋点，2026-11-16 20:28）

最终版（recent + resume + items 三者全并行）埋点：
```
login 302ms
  ├─ recent 发起 +3ms   ┐
  ├─ resume 发起 +6ms   ├─ 三者并行
  └─ items 发起         ┘
items 完成 1127ms / recent 完成 1308ms / resume 完成 2348ms
```
- **理论总时长**：旧 = login + items + max(recent,resume) = 302+1127+2348 = **3777ms**；
  新 = login + max(items,recent,resume) = 302+2348 = **2650ms** ⇒ **省 1127ms**（items 全程被重叠）。
- **体验关键**：把 hero 等待合并进"正在连接资源库"转圈阶段，消除了旧版
  "转圈 → 空白小转圈 → 海报"的二段式割裂。录屏 `connect_final_grid.png`：
  转圈结束后**直接出 hero 海报**，中间空白格从旧版 ~12 格降到 0。
- hero 主角 recent 1308ms 完成（与 items 1127ms 几乎同时），首屏数据不再被 items 阻塞。

## 七、诚实边界（先写）
- **login 1.26s 是跨网硬成本**，禁止持久化 token（安全约束：SessionModel 注释明确
  `never persist passwords or tokens`）⇒ 本轮不碰，连接转圈至少还有 login 这一段时间。
- 转圈现在包含 hero 等待，**转圈本身变长**（约 +1s），但换来的是"转圈后直接出内容"、
  消除最刺眼的空白期 —— 这是产品权衡（连续进度优于二段式空白）。
- resume（继续观看，2348ms）也参与阻塞；它本可后台补（在下方），但为缓存一致性（避免重复请求）
  本轮让它一起等。若后续要再压转圈时长，可拆 resume 为异步缓存。
- hero recent 2.1s 里有多少是"12 字段冗余"未单独测；本轮不改字段（怕破坏 hero 展示），
  先靠并发重叠拿收益，字段精简留作后续可选项。
- 网络波动大（login 298ms vs 1257ms），提速比例以"重叠掉 items 时间"为准，不承诺固定秒数。

## 八、验证要求
- 埋点时间线：hero 发起时刻应提前到 login 完成附近（与 items 同一时刻），不再等 items 完成。
- 录屏宫格：首屏 hero 出现的格数应前移；空白格数减少。
- 无重复请求：同一次连接 hero recent 只发一次（缓存命中，日志无第二次"发起"）。
