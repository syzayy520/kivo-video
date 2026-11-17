# 外部 AI 改动 · 接手质量审查总报告

时间：2026-11-18 02:05 GMT+8
对象：工作区未入库改动（32 已改 + 207 未跟踪），据用户说明由另一个 AI 完成
性质：**只审查 + 一处已执行的死代码清理**；未提交、未推送任何代码

---

## 0. 结论先行

**总体评价：工程质量中上，但尚不具备入库质量。** 分层清晰、注释勤奋、令牌纪律好（新增 profile/sections 目录零硬编码色值）、测试真实。但有 **1 个 P0（字幕翻译功能实质不可用）+ 7 个 P1（含 3 个用户可感知 bug）+ 1563 行死代码**。

我这次已实际处理：
- ✅ **删除 9 个死代码文件 / 1563 行**（备份在 `archive/p402-dead-code/`），删除后编译 BUILD SUCCESSFUL
- ✅ 全量编译 BUILD SUCCESSFUL（删前、删后各一次）
- ✅ K40 `b6e950ee` 真机装包，重拍首页/我的/设置三页（证据 `docs/evidence/p402-foreign-ai-review/`）
- ❌ 未提交、未推送（等你裁决）

**最严重的三件**：
1. **P0 字幕翻译是半成品**：引擎只有纯虚接口、静态库未链进任何 so、无 JNI——App 里完全不可用，但桌面单测全绿会造成"已完成"错觉。
2. **P1 横滑 seek 在片头失效**：`HandSeekState` 用 0 当哨兵，`begin(0)` 后整次横滑被吞——用户一进播放页就能撞到。
3. **P1 隐藏的控制栏仍吃横滑**：用 `alpha=0` 代替 `AnimatedVisibility`，透明进度条在上层继续抢事件，直接威胁横滑跟手。

---

## 1. 真机验证（K40 b6e950ee，用户指定机型）

| 页面 | 证据 | 结论 |
| --- | --- | --- |
| 首页 | `docs/evidence/p402-foreign-ai-review/home_k40.png` | TMDB 正常（「奥德赛」hero + 8 张评分点 + 正在热映海报墙）；底部导航胶囊正常；无白屏/无测试色块 |
| 我的 | `.../profile_k40.png` | 周日历/今日统计/历史热力图/待看空态/收藏在看全部正常渲染 |
| 设置 | `.../settings_k40.png` | 4 个分组、11 个设置项、无开发术语、无截断 |

**我收回两个初判（避免误报）**：
- 「我的」页标题"本地"不是 bug——它是设计契约里的环境名 `environmentName = "本地"`（`ProfileModels.kt:90`、`ProfilePagePreviewData.kt:26`），基准截图就是如此。
- 底部"本月观看 1.9小时"是 `MonthlyChart` 在 LazyColumn 底部被半透明导航胶囊盖住，属正常滚动内容，不是渲染错误。
- 首页 hero「奥 德 赛」的字距来自海报图片本身，不是代码 `letterSpacing`。

**真机未覆盖**（诚实边界）：播放器面板锚点、双指缩放、横滑手势、G1 动画降级——这些恰恰是本批改动重点，但无真机证据（见 §3）。

---

## 2. 已执行：死代码清理（用户指出 1405 行违背铁律后）

**用户原话**：「1405行代码？我的天啊，严重违背铁律啊」——正确。我上一轮给的"78 个未跟踪 Kotlin 全部应入库"清单是错的，会把这些死代码正式提交。

逐文件核实后删掉 9 个零调用文件（备份在 `archive/p402-dead-code/` 共 1563 行）：

| 文件 | 行数 | 判定依据 |
| --- | --- | --- |
| `ui/profile/ProfileHeader.kt` | 242 | 只在 `ProfilePage.kt:65` 注释里被提及 |
| `ui/profile/ContinueHero.kt` | 297 | 只在注释里 |
| `ui/profile/CollectionsRow.kt` | 297 | 只在注释里 |
| `ui/profile/WeeklyReport.kt` | 294 | 只在注释里 + 调用同为死代码的 `WeeklyReportCharts` |
| `ui/profile/WeeklyReportCharts.kt` | 272 | 只被上面那个死文件调用 |
| `ui/profile/ProfileViewModel.kt` | 20 | 全仓无调用，实际取数走 `MainShell` 的 produceState |
| `ui/components/GlassSurface.kt` | 41 | 新组件无人用；生产走旧包 `ui/design/DesignComponents.kt:90` |
| `ui/components/EmptyState.kt` | 26 | 只被同样无调用的 ErrorState 引用 |
| `ui/components/ErrorState.kt` | 74 | 全仓无外部调用 |

**执行方式（可逆）**：先 `cp` 到 `archive/p402-dead-code/`，再移出源码树，`compileDebugKotlin` 验证 BUILD SUCCESSFUL 后清理临时副本。未跟踪文件删了无法从 git 恢复，所以备份是必须的。

**遗留**：`ui/components/GlassSurface` vs `ui/design/DesignComponents.GlassSurface` 两套同名组件仍在，新的无人用、旧的在生产——需要二选一统一（见 §4 P2）。

---

## 3. P0 / P1 问题清单（按严重度）

### P0

**P0-1 字幕翻译功能实质不可用**
- 证据：`subtitle_translation_engine.hpp:45-83` 纯虚接口，全仓无派生实现；`cpp/CMakeLists.txt:100-143` 静态库未链进任何主 so；全仓无 JNI 入口。
- 真相：prompt 契约 + SRT 写出做完了（这两个质量很高、测试真实），但**真正调用推理内核的那层没写**，App 里 AI 字幕完全不可用。桌面单测全绿会误导。
- 改法：要么在 PR 文档明确标注"仅契约层、不可用于 App"；要么补 llama.cpp 派生类 + JNI + 主 so 链接才算完成。

### P1（用户可感知 / 高风险）

**P1-1 片头横滑被整次吞掉** — `HandSeekState.kt:29` 用 `startMs=0` 当哨兵，`PlayerGestureRouter.kt:228` 判 `startMs <= 0L return`。`begin(0)` 后整次横滑无效。讽刺的是该文件注释正是批评旧实现用 0 哨兵。改：加独立 `active: Boolean`，附单测 `begin(0)`。

**P1-2 隐藏控制栏仍吃点击/横滑** — `PlayerBarsHost.kt:155-187` 改 `graphicsLayer{alpha=0}` 常驻，`PlayerOverlay.kt:142-166` 手势层在底。alpha=0 不影响命中测试，透明进度条仍抢事件。改：hidden 态吞事件，或恢复 `AnimatedVisibility` 并把 progress thickness 提升父级（消除双进度条根因）。

**P1-3 双指缩放的手势互斥假设不成立** — `PlayerPinch.kt:90-104` 在 Initial pass consume，但四个单指检测器用默认 Main pass，consume 不会跨 pass 传播；可能一边缩放一边跳进度。改：单指回调里显式排除多指（`changes.count{it.pressed}>=2`）；真机验。

**P1-4 G1 动画门语义与文档相反** — `MotionGate.kt:16` 注释说系统+App 取"或"，但 `:43` `false -> false` 让 App 开关关时无视系统无障碍设置；播放器还叠了第二套门（`PlayerPanelHost.kt:117-124` 只查一个 scale）。改：`reduce = appEnabled == true || systemReduced`，删第二套门。

**P1-5 管理模块是假开关** — `ModuleManagerSheet.kt:155` 文案承诺"关闭的模块不在我的页显示"，但 `ProfilePage.kt:85,184-186` 的 `enabledModules` 只传回弹层自己、不控制显隐、不持久化，离开页面即恢复。改：真正驱动 item 显隐 + 落盘，或撤掉开关改文案。

**P1-6 首页冷启动 ~26 次 TMDB 请求无去重** — `TmdbStudios` 12 次 + `TmdbProviders` 5 + `TmdbGenres` 6 + `MainActivityShell.kt:171-198` 6 个并行 produceState；`TmdbHome.get` 无 in-flight 去重/限流。跨境高延迟下部分模块数秒才出。改：同 URL 合并 + 最小间隔，discover 剧照延后。

**P1-7 主线程磁盘遍历/图片解码 4 处** — `SettingsPage.kt:59-60`（缓存目录遍历）、`EntryCover.kt:34`、`ContinueHero.kt:74`（Bitmap 解码）在组合期同步执行。项目内已有正确范式（`HeroPage.kt:72`、`DetailRelatedVideos.kt:131-132` 用 produceState + IO）。改：统一切 IO。

**P1-8 两套详情页并存，新能力只接了一套** — 新增 `DetailInfoCards`/`DetailRelatedVideos`/`DetailTechRows`/`TechBadges` 只接进 `DetailActivity`；主用的 `MovieDetailActivity + DetailPage` 没有这些能力。改：统一入口（与全项目审查 P1-8 同一问题）。

**P1-9 整批播放器改动零真机证据** — `PlayerPanelAnchor.kt:27,33` 自述推断；`PlayerPinch.kt:40-44` 自述静态无法确定；`docs/evidence/p318-gesture-audit/status.md` 仍是"待复验"。铁律要求证据关闭。改：K40 逐项截图证据落 `docs/evidence/p340-player/`。

---

## 4. P2 精选（完整 60+ 条见三份分报告）

- **行数铁律**：`ProfileStats.kt` 实际 301 行（`wc -l` 因末尾无换行少报），6 个文件 295–300 贴线。
- **本周口径不一致**：`ProfileStats.kt:207-212` 周一是起点，`:143-155` 日历周日是起点，同页两个"本周"。
- **重复读盘**：`ProfileStats.kt:88` 和 `:118` 两次 `dailyMs`，注释却声称复用。
- **死状态**：`MainActivityShell.kt:107` `selectedTitle` 永远 null，`HomeTitleSheet` 不可达。
- **组合期副作用**：`MainActivityShell.kt:295-299` 直接写 Window insets，未包 LaunchedEffect。
- **裸线程**：`MainActivityHomeFeed.kt:59-64` `refreshContinue` 用 Thread + runOnUiThread，其余用 lifecycleScope。
- **同名组件跨包**：3 组（GlassSurface/SectionHeader/MediaCard）新旧共存，IDE 易 import 错包。
- **错误文案两份**：`ErrorState.kt` 与 `TmdbErrorCard.kt:27-33` 各维护一份五类中文文案。
- **缩略图每帧 seek**：`PlayerGestureRouter.kt:242` 在 150ms 节流外，重演 v29 已修的带宽互踩（一次拖动掉 5 MiB）。
- **拖到边界发黏**：clamp 到 0/duration 后 `accumMs` 继续累加，反向拖回要等量还债。
- **CMake 硬编码本机路径**：`C:/re-tools/...`，他机静默跳过（构建层静默降级）。
- **CJK 守卫未验 UTF-8 续字节**：`subtitle_translation_prompt.cpp:226-229` 只查越界不查 0x80..0xBF。

---

## 5. 做得好的地方（不能只挑毛病）

1. **分层与令牌化扎实**：编排—数据—视图—令牌四层清晰；`DesignTokens` 平滑代理 `DesignPalette`，公开成员零删除。
2. **不造假数据**：`ProfileStats.kt:66-74` 无数据源的项返回空集合并渲染真实空态；错误态五类原文保留。
3. **复用而非重造**：4 个 TMDB 拉取器全走既有 `TmdbHome.get`（鉴权/超时/缓存/错误分类复用），没有另起一套。
4. **C++ 测试是真测试**：FNV 摘要我独立复算三个全部匹配；`MediaTechProbe` 有 `finally release` 和类型容错。
5. **状态管理规范**：produceState key、rememberSaveable、LaunchedEffect 大多正确；横滑状态从进程级顶层 var 改成随组合生死的 `HandSeekState`（思路对，只是哨兵实现错了）。

---

## 6. 下一步（按优先级，需你裁决）

1. **P0 字幕翻译**：标注"仅契约层"还是补齐实现？
2. **P1 三个播放器 bug**（0 哨兵 / 透明命中 / 双指互斥）——我可以直接修，改完真机复验横滑与双指。
3. **G1 门统一 + 假开关**：改 `MotionGate.reduce` 取或、删第二套门；模块开关二选一。
4. **入库清单修正**：排除已删的 9 个死代码；`backend/` 字幕翻译建议**暂缓入库**或标注阶段成果。
5. **播放器真机证据**：面板锚点 / 双指 / 横竖屏 / 拖到头尾逐项截图。

> 分报告全文：`docs/review/audit-home-profile.md`（21K）、`audit-player.md`（20K）、`audit-data-detail.md`（20K），每条都带文件:行号证据。
