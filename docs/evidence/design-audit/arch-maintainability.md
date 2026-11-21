# Kivo Video UI 代码可维护性与布局结构审查报告

- 审查人：架构师 高见远（Gao）
- 审查日期：2026-11-19
- 审查范围：`android/app/src/main/java/com/kivo/cinema/ui/`（只读审查，未改动任何代码）
- 数据来源：`wc -l` / `grep` 脚本实测 + `python tools/ui_gate.py --json` 门禁输出 + 逐文件阅读
- 分支：`codex/kivo-video-p0-ui-01-playback-overlay-v13`（含 3 个已修改 UI 文件、4 个新增 UI 文件的未提交改动，已纳入审查）

---

## 1. 量化概览

### 1.1 规模

| 指标 | 数值 | 说明 |
|---|---|---|
| UI 目录 Kotlin 文件数 | **230** | `find ui -name "*.kt"` |
| UI 目录总行数 | **30,203** | `cat 所有 .kt \| wc -l`（空行/注释计入） |
| UI 目录 Composable 函数数 | **374** | `@Composable` 注解计数 |
| UI 目录 @Preview 数 | **44** | 覆盖率约 **11.8%**（44/374） |
| 全项目 Kotlin 文件数 | 486 | 门禁口径 |
| 全项目 >300 行文件数 | **19** | 15 个非 UI + 4 个 UI（白名单待拆） |

### 1.2 UI 目录内 >300 行文件清单（4 个，均为门禁白名单待拆）

| 文件 | 行数 | 门禁标注 | 超标倍数 |
|---|---|---|---|
| `ui/PlayerActivity.kt` | **909** | B5（治理） | 3.03× |
| `ui/detail/EmbyResource.kt` | **519** | B5（治理） | 1.73× |
| `ui/compose/PlayerActionRouter.kt` | **406** | B4（播放器）/ B5（治理） | 1.35× |
| `ui/compose/PlayerComposeActivity.kt` | **382** | B4（播放器）/ B5（治理） | 1.27× |

另有 4 个文件恰好卡在 300 行红线（`ProfileStats.kt`、`SmbDetail.kt`、`DiscoverPage.kt`、`PlayerProbeHooks.kt`），**任何新增都立即触发 FAIL**，属高危临界文件。

### 1.3 裸色值分布

| 指标 | 数值 |
|---|---|
| 全项目 `Color(0xFF...)` 总数 | **316**（baseline 326，已下降 10） |
| **UI 目录裸色数** | **176**（占全项目 **55.7%**） |

UI 目录裸色 Top10（含未提交新增文件）：

| # | 文件 | 裸色数 |
|---|---|---|
| 1 | `ui/compose/player/PlayerTokens.kt` | 25 |
| 2 | `ui/design/settings/SettingsVisuals.kt` | 14 |
| 3 | `ui/detail/actions/DetailActionMenu.kt` | 10 |
| 4 | `ui/design/settings/SettingsDataRoot.kt` | 8 |
| 5 | `ui/compose/player/PlayerGestureHud.kt` | 6 |
| 6 | `ui/design/DesignComponents.kt` | 6 |
| 7 | `ui/design/RatingBadges.kt` | 6 |
| 8 | `ui/design/settings/SettingsRows.kt` | 6 |
| 9 | `ui/detail/DetailSeasonPicker.kt` | 6 |
| 10 | `ui/detail/DetailSections.kt` | 6 |

### 1.4 缺 @Preview 的 Page（4 个，门禁口径共 7 个 Page）

`ui/design/DiscoverPage.kt`、`ui/design/WatchingPage.kt`、`ui/design/home/HeroPage.kt`、`ui/detail/DetailPage.kt`

### 1.5 布局与状态的模式频次（反映收敛度）

| 模式 | 次数 | 判读 |
|---|---|---|
| `fontSize = <数字>.sp` | **294** | 令牌已定义 `font*` 系列却极少引用（见问题 7） |
| `fillMaxWidth` | **295** | 通栏滥用候选，需逐处评估 |
| `RoundedCornerShape(<数字>.dp)` | **172** | 出现 16 种不同圆角值，散落 |
| `mutableStateOf` | 185（53 文件） | 状态量大 |
| `rememberSaveable` | 20 | 配置变更恢复覆盖不足（11%） |
| `items(...)` / 带 `key=` | 15 / **3** | 80% 的列表项无稳定 key |
| `ConstraintLayout` | 0 | 无重型布局，正面 |
| `matchParentSize` / `fillMaxHeight` | 4 / 16 | 历史坑，当前用法已加防坑注释（见 §5） |

### 1.6 门禁健康度（当前 vs 基线）

| 指标 | 当前 | 基线 | 趋势 |
|---|---|---|---|
| 裸色总数 | 316 | 326 | ✅ −10 |
| 缺 @Preview 的 Page | 4 | 5 | ✅ −1 |
| 行数超限文件 | 19 | 19 | ⚠️ 持平（均在基线内，无新增） |
| regression | 0 | — | ✅ 无回退 |

---

## 2. 问题清单（18 项，均带文件:行号证据）

严重度：**P0** = 架构级阻断 / **P1** = 高（影响迭代效率或状态正确性）/ **P2** = 中（渐进治理）。

| # | 类别 | 严重度 | 位置（文件:行号） | 问题 | 具体重构建议 | 收益 |
|---|---|---|---|---|---|---|
| 1 | 职责/行数 | **P0** | `ui/PlayerActivity.kt:41,99,424,619,791,887` | 909 行 god object，单一 Activity 混合 5 类职责：证据采集落盘（`installEvidenceSinks:99`、`writeFailureEvidence:791`）、播放路由与起播（`maybeStartPlayback:424`）、Surface 生命周期、失败处理、测试夹具定位（`locateTestMedia:887`） | 拆为 4 文件：`PlaybackSessionController`（起播/路由/失败）、`SessionEvidenceWriter`（证据采集，移出 UI 层）、`TestMediaLocator`（探针档专用，条件依赖）、`PlayerActivity` 仅留生命周期胶水 | 单文件降到 ~250 行；证据逻辑可单测；测试代码不再进生产链路 |
| 2 | 职责/行数 | **P1** | `ui/detail/EmbyResource.kt:42,82,154,297,402,420,440,462` | 519 行混合 5 类职责：TMDB 数据聚合、剧集加载、图片加载（`loadImage:402`）、Intent 构造、标记切换业务（`serverToggle:440`） | 拆为 `EmbyDetailLoader`（数据/TMDB/剧集）、`EmbyImageLoader`（`previewLoader()` 独立）、`EmbyResourceActions`（标记/Intent）；主类降为编排 | 单文件降到 ~250 行；图片与业务逻辑可独立测试 |
| 3 | 行数 | **P1** | `ui/compose/PlayerActionRouter.kt`（406 行）、`ui/compose/PlayerComposeActivity.kt`（382 行） | 2 个播放器文件超 300 行门禁，均在白名单内长期挂账 | 按 action kind 分组提取 `PlayerActionHandlers`（分组 when 分支），Activity 提取 `PlayerComposeLifecycle`；每文件压到 <300 行后摘白名单 | 摘掉 2 个白名单；降低门禁例外维护成本 |
| 4 | 职责/超大函数 | **P1** | `ui/PlayerActivity.kt:424-586` | `maybeStartPlayback()` 单方法 **162 行**，内含中断判定、媒体查找、ffmpeg 路由、会话开启、音频起流、节拍配置等多段强耦合流程 | 用"阶段化"重构：`resolveMedia()` → `route()` → `openSession()` → `configureAudio()` → `startPacing()`，每段独立可测，主方法只做编排 | 巨型方法可读性/可测性提升；错误定位更快 |
| 5 | 职责 | **P2** | `ui/PlayerActivity.kt:887-901` | 生产 Activity 内置测试夹具：`locateTestMedia()` 注释明确"adb push 方便"，扫描外部目录取第一个文件作为片源，属测试行为侵入生产代码 | 移至 `TestMediaLocator` 并以 BuildConfig.DEBUG/探针开关隔离；正常档应有明确的片源入口（Intent/URI） | 生产路径行为可预测；避免误播用户目录里任意文件 |
| 6 | 令牌 | **P1** | `ui/compose/player/PlayerTokens.kt:19-80`（25 处）、`ui/design/settings/SettingsVisuals.kt:20-36`（14 处） | 令牌文件内部直接写裸色，且存在**同值重复定义**：`PlayerTokens.textSecondary = 0xFF8A8A8E`（:20）与 `DesignTokens.textSub` 同值；`SettingsVisuals.blue=0xFF0A84FF`/`green=0xFF34C759`（:20-21）与 `DesignPalette.Light.navActive`/`accentGreen` 同值。两套令牌各维护一份，改色必漏 | 扩展 `DesignPalette` 增加播放器语义色组（scrim/track/glass 系列）与设置语义色组；`PlayerTokens`/`SettingsVisuals` 改为纯映射（像 `DesignTokens` 一样 `= DesignPalette.X`），删除所有字面量 | 消除明暗态缺失（播放器令牌目前无暗态）；改一处全局生效 |
| 7 | 令牌 | **P1** | `ui/design/WatchingPage.kt:96,138,144,172,180,194,210,215,218,232,242,262,264` 等；全项目 294 处 | `fontSize` 硬编码遍布，而 `DesignTokens` 已定义 `fontMeta=11`、`fontRowTitle=17`、`fontCardTitle=13`、`fontPageTitle=34`、`fontSectionTitle=17`、`fontStat=28` 却仅被引用 1–2 次（如 `fontStat` 1 次、`fontSectionTitle` 1 次）；存在"令牌定义了没人用"的断层 | 批量把高频字面量映射到既有令牌（11→`fontMeta`、13→`fontCardTitle`、17→`fontRowTitle`/`fontSectionTitle`、20→新增 `fontTitleSmall`），并让 ui_gate 增加"设计目录内裸字号"计数 | 字号规范统一；REX 实测字号体系真正落地 |
| 8 | 令牌 | **P2** | `ui/detail/actions/DetailActionMenu.kt:24-29,53,90` | 文件级私有色值（`Ink=0xFF1C1C1E`、`Accent=0xFF655978`、`AccentWash=0xFFF3F0F8` 等）绕过 DesignTokens；其中 `Ink` 与 `DesignPalette.Dark.groupedFill` 同值不同名，无明暗态 | 收敛到 `DesignPalette` 语义色（如 `menuSurface`/`menuAccent`），菜单组件只读令牌；`Accent` 紫色需确认是否品牌色并补明暗两套 | 消除同色异名；菜单暗态可用 |
| 9 | 令牌/复用 | **P2** | `ui/design/WatchingPage.kt:175,186,237`；`ui/design/CapsuleNavBar.kt:22,25`；全项目 172 处 | 圆角出现 **16 种**离散值（20dp×5、16dp×4、12dp×4、50dp×3、14dp×3、8/2/10dp 各 2、其余各 1），而令牌只有 `radiusCard=13`、`radiusPoster=12`；胶囊按钮 `RoundedCornerShape(50.dp)+background+clickable+padding` 组合在 WatchingPage 重复 3 次 | 扩充 `DesignTokens`：新增 `radiusPill=50`、`radiusChip=14`、`radiusHairline=1` 等；提取通用 `KivoPillButton(text,onClick,tint,enabled)` 组件供 WatchingPage/DetailActionMenu/Settings 复用 | 圆角收敛到 4–5 档；消除重复按钮代码 |
| 10 | 状态 | **P1** | `ui/design/WatchingPage.kt:62-64` | 多选状态 `selectionMode`/`selectedKeys`/`pendingRemove` 用 `remember` 而非 `rememberSaveable`；旋转屏幕或进程重建后选择态与待确认删除列表丢失，用户需重选 | 三处改 `rememberSaveable`；`selectedKeys` 为 `Set<String>` 需确认可保存（否则用 `List`+`toSet()`）；或将整组状态上提 `WatchingViewModel` 持有 | 避免配置变更丢态；批量删除流程更稳 |
| 11 | 状态/性能 | **P2** | 全项目 `items(...)` 15 处仅 3 处带 `key=`（正面例：`ui/design/WatchingPage.kt:104` `key = { it.latest.key }`） | 80% 的懒加载列表无稳定 key，插入/删除后 Compose 按位置复用，可能错位复用状态（如展开态、拖拽态） | 为所有 `items` 补 `key = { it.id/hashKey }`；对含状态的子项优先加 key | 列表变更时重组正确；减少错位 bug |
| 12 | 状态 | **P2** | `ui/compose/player/PlayerPanelItems.kt:40`；全项目 `mutableStateOf` 185 处 vs `rememberSaveable` 20 处 | 状态量大但可恢复状态覆盖率仅 11%。`rememberPanelItems:40` 的 `items` 用 `remember+LaunchedEffect` 异步取数（当前用法正确），但同类跨配置状态需逐一审视 | 建立规则："短暂 UI 态用 remember；用户进度/选中/输入用 rememberSaveable；跨屏数据用 ViewModel"。对 185 处做一次分类审计 | 形成状态管理规范；减少旋转丢态投诉 |
| 13 | 布局 | **P2** | `ui/design/WatchingPage.kt:103,166,167,169` 等；全项目 `fillMaxWidth` 295 次 | `fillMaxWidth` 高频，部分父布局已由 `Column(Modifier.fillMaxSize())` 约束，子项再 `fillMaxWidth` 属冗余；冗余 modifier 增加重组时的测量开销 | 在已 fillMaxSize 的 Column/Box 内移除子项冗余 `fillMaxWidth`；用 LayoutInspector 抽查 3 个热点页确认 | 减少无谓测量；布局意图更清晰 |
| 14 | 可测试性 | **P1** | `ui/design/DiscoverPage.kt`、`ui/design/WatchingPage.kt`、`ui/design/home/HeroPage.kt`、`ui/detail/DetailPage.kt` | 4 个主 Page 缺 @Preview；全项目 374 Composable 仅 44 Preview（11.8%），视觉回归全靠真机截图 | 参照 `ui/profile/ProfilePagePreviewData.kt` 的假数据模式，为 4 个 Page 补 `@Preview`（含 loading/empty/error 三态）；核心 Composable 补到 25% 覆盖 | 视觉改动本地可验证；降低真机回归成本 |
| 15 | 可维护性 | **P2** | `ui/design/WatchingPage.kt:152,154,172,180,215,242`；`ui/detail/DetailSeasonPicker.kt:102,137,163,177,200`；`ui/design/settings/SettingsCacheSub.kt:64-78`；`ui/compose/player/PlayerPanelItems.kt:55-72` | 中文文案硬编码散落（"移除"/"取消"/"重试"/"暂无缩略图"/"清理缓存？"/15 个面板标题等），无集中文案层；改文案需全项目搜索 | 建 `UiStrings` 对象（或 strings.xml + stringResource）集中管理；`panelTitle` 的 kind→文案映射保留单一来源模式，迁入 `UiStrings` | 文案一致性；为多语言/合规留扩展点 |
| 16 | 职责/行数 | **P2** | `ui/design/WatchingPage.kt:53-269`（含 6 个 Composable） | 269 行单文件承载页面编排 + 多选状态机 + 3 个导航/操作条（`WatchingSelectionBar`/`WatchingSelectionNav`/`WatchingNavBar`）+ 空态 + 确认框，且为临界行数与缺 Preview 双重风险 | 拆 `WatchingChrome.kt`（3 个导航/操作条）、`WatchingDialogs.kt`（空态+确认框）；主文件只留编排（约 130 行） | 预留迭代空间；每个组件可独立 Preview |
| 17 | 令牌/定义使用断层 | **P2** | `ui/design/DesignTokens.kt:54-55`（注释"当前无任何调用点"）、`fontStat` 等 | `pageGradient`/`pageGradientDark` 六页渐变端点定义完整但接入未完成；`fontStat`/`fontSectionTitle`/`elevationCard`/`navHeight` 各仅引用 1 次，令牌存在"定义了没人用"的维护负担 | 要么在 B-2~B-5 页面接入时启用，要么在令牌注释中明确标记为"预留"并纳入 ui_gate 的未使用令牌检查，避免误以为已生效 | 消除令牌与实际视觉的口径分歧 |
| 18 | 复用 | **P2** | `ui/design/WatchingPage.kt:174-196`、`ui/detail/actions/DetailActionMenu.kt:90`、`ui/design/settings/SettingsRowParts.kt`、`ui/design/home/sections/NasLibraryCard.kt` | 相似"胶囊操作按钮/信息行卡片"存在多套实现（clickable Box + clip + background + 固定 padding 组合重复），padding 组合（14/18×7、22×9）散落 | 提取 `components/KivoActionPill.kt` 与 `components/KivoInfoRow.kt`，参数化 tint/enabled/padding；先统一 WatchingPage 3 处，再推广到设置与详情 | 减少重复；视觉微调用一处改全局 |

---

## 3. 高价值重构机会 Top 8（投入产出比排序）

| 排名 | 重构项 | 具体改法 | 风险控制 |
|---|---|---|---|
| 1 | **拆 PlayerActivity（909 行）** | 提取 `SessionEvidenceWriter`（证据）、`PlaybackSessionController`（起播/路由）、`TestMediaLocator`（测试），Activity 留生命周期 | 逐段提取，每段提取后跑播放冒烟（本地/SMB/Emby 各一条）与证据文件 diff 比对；不动播放状态机语义 |
| 2 | **收敛裸色令牌（UI 176 处）** | `PlayerTokens`/`SettingsVisuals` 改为 `DesignPalette` 映射；DetailActionMenu 私有色入 palette；按门禁 by_file 从 Top3 开始 | 先做同值替换（零视觉变化），再做明暗态补全（需真机取色，推断值标 ◐）；每批跑 ui_gate 看裸色计数下降 |
| 3 | **拆 EmbyResource（519 行）** | 按数据加载/图片/动作三层拆；`previewLoader()` 独立为 `EmbyImageLoader` | 保持 `from(context,intent,id)` 工厂签名不变，调用点零改动；补 TMDB 补全与清晰度选择的单测 |
| 4 | **字号令牌化（294 处）** | 高频字面量（11/13/17/20）批量映射 `DesignTokens.font*`；ui_gate 增加裸字号计数 | 只做等值替换；20sp 等无令牌值先补令牌再替换；替换后跑 4 个 Page 的 Preview 截图对比 |
| 5 | **补 4 个 Page 的 @Preview** | 复用 `ProfilePagePreviewData` 模式造假数据；每个 Page 覆盖 loading/empty/content/error | 假数据与真实数据结构一致；Preview 不触发网络（注入假 posterOf） |
| 6 | **WatchingPage 状态上提/可恢复** | `rememberSaveable` 三处 + 拆 `WatchingChrome.kt`/`WatchingDialogs.kt` | `Set<String>` 保存前验证；右滑删除与多选两条通道的回归用例必须覆盖（当前未提交改动涉及此逻辑） |
| 7 | **列表 key 补全（12 处）** | 为所有 `items` 补稳定 key；优先播放器选集、在看列表、日历网格 | key 必须来自领域模型稳定 id，不能用 index；补 key 后验证展开/选中态不串位 |
| 8 | **圆角与胶囊组件统一** | 新增 `radiusPill/radiusChip/radiusHairline` 令牌；提取 `KivoActionPill`，先在 WatchingPage 3 处落地 | 先统计 16 种圆角的实际语义（胶囊=50、卡片=13/12、徽标=3/4），按语义归档而非强行合并 |

---

## 4. 门禁治理建议（存量 19 个超限文件分批降线）

### 4.1 现状分层

- **UI 白名单 4 个**（可立即启动，属本审查范围）：PlayerActivity 909、EmbyResource 519、PlayerActionRouter 406、PlayerComposeActivity 382
- **非 UI 存量 15 个**（本审查不深入，供 B5 排期参考）：`mpv/LocalRangeResponder.kt` 433、`playback/MpvPlaybackSession.kt` 409、`emby/EmbyClient.kt` 408、`bridge/FfmpegPlaybackSession.kt` 403、其余 11 个 307–402 行（net/playback/bridge/thumbnail 模块）

### 4.2 分批计划（按优先级）

| 批次 | 文件 | 优先级 | 目标 | 验收 |
|---|---|---|---|---|
| **B5-1** | `ui/PlayerActivity.kt`（909） | **P0** | 证据/测试夹具/控制器分离，降到 <300 | 摘白名单；3 条播放路径冒烟 + 证据 diff 一致 |
| **B5-2** | `ui/detail/EmbyResource.kt`（519） | **P1** | 三层拆分，降到 <300 | 摘白名单；TMDB 补全/清晰度/剧集单测通过 |
| **B5-3** | `ui/compose/PlayerActionRouter.kt`（406）、`ui/compose/PlayerComposeActivity.kt`（382） | **P1** | action handlers 与生命周期分离 | 摘白名单；面板各 kind 打开正常 |
| **B5-4** | 4 个临界 300 行文件（ProfileStats/SmbDetail/DiscoverPage/PlayerProbeHooks） | **P1** | 预防性拆分到 ≤280，留迭代缓冲 | 无临界告警 |
| **B5-5** | 非 UI Top4（433/409/408/403） | **P2** | 网络/播放会话模块拆分 | 门禁 violations 减少 4 |
| **B5-6** | 非 UI 其余 11 个（307–402） | **P2** | 按模块成批拆 | violations 清零，刷新 baseline |

### 4.3 门禁配套建议

1. 每批拆分后执行 `python tools/ui_gate.py --json`，确认 `regressions: []` 且 `violations` 单调下降；拆完后用 `--baseline` 刷新基线。
2. 为 ui_gate 增加两项**增量指标**（防新增）：裸字号计数（针对 `fontSize = <数字>.sp`）、未带 key 的 `items` 计数；先只告警不 FAIL，避免一次性阻断。
3. 白名单文件登记"负责人 + 计划摘除批次"，避免长期挂账（当前 4 个 UI 文件已挂 B4/B5 多轮）。

---

## 5. 历史坑核对（已遵守，重构时勿回退）

本次审查专门核对了三条历史坑，**当前代码均已正确处理**，重构时必须保持：

1. **activeId 必须是 `mutableStateOf`**：`emby/EmbySessionModel.kt:43` 为 `private var activeId by mutableStateOf<String?>(null)`，注释明确"state：UI 靠它感知原地切服"——已修复，勿改成普通变量。
2. **`matchParentSize` vs `fillMaxHeight`**：`ui/design/WatchingSwipeRow.kt:96-97` 明确注释"matchParentSize 而非 fillMaxHeight——后者在无固定高度的父 Box 里会塌成内容高度"，并采用 `Box(Modifier.matchParentSize())`；`ui/compose/player/PlayerOverlayPanel.kt:68`、`ui/detail/DetailCalendarSheet.kt:91` 同样用法——正确。
3. **`IntrinsicSize.Min` 配不换行文字会截断**：`ui/detail/DetailResource.kt:85` 明确注释"用固定 CardWidth 而非 IntrinsicSize.Min：后者与 softWrap=false 的文字配合会把码率截断"，采用固定宽度卡——正确。
4. **门禁 304 行 FAIL 教训**：当前 4 个临界 300 行文件（见 §4.2 B5-4）需预防性拆分，任何改动前先看行数。

---

## 6. 未决问题与假设

- 本次为**纯静态审查**，未运行 UI、未做真机取色；问题 6/8 中"同值重复"已由字面量比对确认，但暗态是否符合设计意图需产品/视觉确认。
- 未提交改动（WatchingPage/DetailResource/SmbDetail + 4 个新增文件）已纳入统计，其最终形态若变化，相关行号需复核。
- `PlayerActivity.kt` 的测试夹具与证据逻辑可能是有意的"探针档"设计；建议先与 owner 确认用途再决定是否按问题 5 迁移。
- 报告中的 `fillMaxWidth` 295 次为**候选数**，非全部是滥用；需 LayoutInspector 抽查确认后再批量清理。
