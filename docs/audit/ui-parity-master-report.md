# kivo video × Rex 1:1 复刻 · UI 总审查报告

> 日期：2026-11-16 ｜ 角色：顶级设计师 + 前端架构审查
> 验收口径：**最低齐平竞品 Rex，1:1 复刻；做不到更好就必须一模一样**
> 工作目录：`C:\kivo video`（既有仓库；不新建项目包）
> 本报告只做审查与规划，**未改动任何生产代码**。

## 0. 证据链

| 证据 | 位置 | 说明 |
| --- | --- | --- |
| 竞品结构说明书 | `F:\Files\开发\rex\rex\Rex_UI结构复刻说明书.md`（352 行） | §2 外壳、§3.1 五语义色、§4 2:3/16:9、§5 首页 15 组件、§6 逐页 |
| 竞品动效拆解 | `F:\Files\开发\rex\rex\Rex_动效与交互全量拆解.md`（466 行） | G1–G6 全局治理、M03 播放器 S01–S07 |
| 竞品动效参数 | `F:\Files\开发\rex\rex\Rex_动效参数表.csv`（33 行） | duration/easing/位移逐项 |
| 竞品实拍（浏览/设置） | `C:\Users\Administrator\Pictures\REX 播放UI\2026_11_16_21_16–21:22`（41 张 iOS 竖屏） | 发现页全模块、详情、我的看板、资源库、设置、环境 |
| 竞品实拍（播放器） | 同目录 `2026_11_11_19_21–19:23`（15 张 2532×1170 横屏 + 1 段 MP4） | 干净态、右侧面板、选集态、音轨面板 |
| 子报告 A | `docs/audit/ui-governance-audit.md`（269 行） | 令牌/职责/裸值/重复/可维护性，53 条（P0 19） |
| 子报告 B | `docs/audit/ui-page-parity-audit.md`（417 行） | 7 个页面/区块，77 条（P0 11） |
| 子报告 C | `docs/audit/player-overlay-parity-audit.md`（368 行） | 顶栏/底栏/面板/手势/动效/职责，135 条（P0 2） |

**口径订正（实拍推翻文档推断，后续一律以实拍为准）**
1. 主 Tab = **发现 / 我的 / 资源库 / 设置**（IMG_0420 设置页显式列出「主 Tab」四项）。说明书 §2.1 的 Libraries/Discover/Watching/My 是 ◐ 推断，不作契约。当前 `MainActivityShell.kt:89` 顺序正确。
2. **「在看」不是 Tab，也不是发现子页**，而是「我的」页模块流 +「继续观看 ›」的下钻（IMG_0429/0430 环境浮层含 在看/待看/收藏/下载/日历）。
3. 发现页顶部**没有环境切换器**；环境能力归「我的」页。
4. 播放器实拍只在 `2026_11_11_*` 横屏批次，`2026_11_16_21_2x_*` 全是竖屏浏览页。

> 图像工具限制：`analyzeImage` 对原始 7–8 MB 截图与深色播放帧会返回空。本轮已建立可复算流水线：`.tmp/scale_rex.py` → `.tmp/rex_scaled`（1500px JPEG）→ `.tmp/rex_play`（1300px 提亮 1.25 + 对比 1.15）。

---

## 1. 结论摘要（六维度）

| 维度 | 结论 | 关键证据 | 条目 |
| --- | --- | --- | --- |
| 视觉设计 | 首页/播放器的“骨架”接近，**设计系统不成立**：只有浅色一套色，竞品是 5 语义色 × 明暗双外观；玻璃、描边、次色同名不同值 | `DesignTokens.kt:24-28`；A-TKN-01/03 | A 53 条 / C 135 条 |
| 用户体验 | **「我的」是占位页**，竞品是完整数据看板；「在看」入口归属错误；继续观看卡来源恒「本地」 | `Pages.kt:34-57`、`DiscoverPage.kt:217-218` | B-PROF-01/06、WATCH-01 |
| 交互逻辑 | 播放器面板**不锚定触发控件**；选集态整条进度条消失；双指缩放缺失；横滑手势状态是进程级 `var` | C-PNL-01、BOT-01、GST-06、BUG | C P0×2 |
| 布局结构 | 导航避让 84dp vs 资源库 94dp（跨页胶囊跳 10dp）；hero 整块不可点；分页点不变大 | `MainActivityShell.kt:116`、`HeroCarousel.kt:126-131` | B-NAV-06、DISC-08、DISC-02 |
| 设计一致性 | 玻璃面 5 套、顶栏 5 套、空态 6 套、错误态 5 套、卡片 5 套、命中区 4 种；7 个零引用死令牌 | A-DUP-01…10、A-TKN-02 | A P0 19 |
| 代码可维护性 | **41,471 处裸值 vs 526 处令牌引用**；`PlayerActivity.kt` 901 行（上限 300）；裸 int 路由 + `listOf(1,3,0,4)` 魔法数组；无 `@Preview`、无自动门禁 | A-§0、SRP-01、B-SHELL-01 | A 53 条 |

**总量：265 条编号差距（P0 32 / P1 58 / P2 62 / 其余 P3 与重复计数）**。三份子报告每条均为「编号｜问题｜证据(文件:行)｜影响｜改法(文件+符号)｜优先级」，可直接拆任务。

---

## 2. 视觉设计

### 2.1 设计系统只有一半
- **现象**：`DesignTokens.kt:24-28` 只定义浅色五色；暗色仅导航两枚（`navPillDark`/`navIdleDark`），注释写「深色态 v2」但不存在。竞品是 `canvasFill/groupedFill/pageBackground/hairline/washFill` × Light/Dark 共 10 条，且 `washFill` 是 **P3 + 50% alpha**，`pageBackground` 是 **extended sRGB**。
- **后果**：系统深色模式一开，页面仍是浅底黑字；继续观看/详情/资源库的深色环境（如 hero 暗场）靠局部硬编码硬扛，必然割裂。
- **改法**：拆 `tokens/DesignPalette.kt`（纯色值，明暗两套 object）+ `tokens/DesignTokens.kt`（语义映射，`@Composable` 读 `isSystemInDarkTheme()`），`washFill` 保留 alpha 与 P3 注释；`pageBackground` 标注 extended sRGB 原值。删除 7 个零引用死令牌（`washFill`/`glassFallback`/`navHeight`/`navBottomPad`/`fontNavLabel`/`fontHeroMeta`/`fontHeroSynopsis`），其中 3 个被逐字硬编码回调用点的要改回引用。

### 2.2 同名不同值
- `hairline` 三值、文本次色三值（`textSub`/`navIdle`/`chromeSub`）、可点蓝两值（`0A84FF` vs `007AFF`）。
- **改法**：语义唯一化——`hairline` 只留 `Color(0x0D000000)`；次色收敛为 `textSecondary`；可点蓝统一 `navActive`，`SourcesTheme` 不再自造。
- **验收**：`grep -rn "0x0D000000\|0A84FF\|007AFF"` 每个字面量只允许出现在 tokens 文件。

### 2.3 竞品实拍的视觉硬参数（必须逐项对齐）
| 项 | Rex 实拍/说明书 | 当前 | 差距 |
| --- | --- | --- | --- |
| 页面底 | 极浅灰紫渐变 `#F3F0F5 → #F7F6FA`（发现/我的） | `pageBackground #F2F2F7` 平涂 | B-DISC-01 |
| 卡片 | 纯白 `#FFFFFF`，圆角 16 | 白卡，圆角 13（`radiusCard`） | 圆角差 3 |
| 海报 | 2:3，圆角 12 | 104×156 = 2:3 ✓，圆角 10 | 圆角差 2 |
| 底部导航 | 毛玻璃半透明 + 选中「灰胶囊 + 原色图标」 | 实色 `#E8E9ED` + 蓝 tint + 白底 pill | B-NAV-02 |
| 开关绿 | `#34C759` | `accentGreen #34C759` ✓ | — |
| 播放器轨道 | 6pt、三段半透明 + 1px 描边、无滑块 | `PlayerTokens` ✓ | **已对齐** |

---

## 3. 用户体验

### 3.1 「我的」页：占位 vs 完整看板（P0）
- **Rex 实拍（0357–0362）**：环境名 + 编辑 → 周历（日/一/…/六 + 当日播出态）→ 今日观看（0.0 小时 / 0 部电影 / 2 集剧 + 00:00–24:00 柱状图）→ 历史观看热力图 → 待看（空态「还没有待看的影片」+ 引导「在详情页加入待看，票根会出现在这里」）→ 收藏 / 在看 双数据卡 → 本月观看折线（11/1–11/30，↑0.1 小时）→ 刷剧 TOP3（条形）→ 类型分布（环形 + 剧情 46%/科幻 23%/恐怖 23%/其他 9%）→ 趋势卡 → 管理模块（12 个模块开关 + 拖动排序）。
- **当前**：`Pages.kt:34-57` 只有一个 👤 emoji +「媒体库 N 个视频 · 统计随功能接入」。
- **后果**：四个主 Tab 里有一个是空的；竞品最有辨识度的页面完全缺失。
- **改法（家族树）**：`ui/profile/ProfilePage.kt`（只做编排）+ `profile/calendar/WeekStrip.kt`、`TodaySummary.kt`、`HistoryHeatmap.kt`、`WatchlistSection.kt`、`CollectionSection.kt`、`MonthlyChart.kt`、`TopShowsCard.kt`、`GenreDonut.kt`、`TrendCard.kt`、`ModuleManagerSheet.kt` + `profile/ProfileViewModel.kt`。数据源复用现有 `stats/WatchStore`，缺失字段先返回显式 `null` 并渲染空态（禁止假数字）。
- **验收**：Rex 0357–0362 五张截图逐屏 1:1；空态文案原文一致。

### 3.2 「在看」入口归属错误（P0）
- 竞品：`我的 → 在看` 卡片 / 环境浮层；「继续观看」标题右侧 `›` 下钻。
- 当前：`WatchingPage.kt:48-57` 自造返回条「‹ 发现」，`MainActivityShell.kt:155-162` 作为 `page=2` 挂在发现下。
- **改法**：删除 `page=2` 路由；`WatchingPage` 迁到 `ui/profile/watching/`，由「我的」模块流与 `HomeSectionHeader("继续观看", onMore = { openWatching() })` 两个入口进入；返回条改为标准导航栏（左返回、中标题、右空）。

### 3.3 继续观看卡信息缺失（P0）
- 竞品：`清潭国际高中 / Wavve / 10小时前 · nyamedia / 第1季 集 1/10 · 剧集 / 4.5%`——**平台名 + 季集 + 类型 + 百分比**四段。
- 当前：`sourceOf()` 恒返回「本地」（`DiscoverPage.kt:217-218`），只显示「看到 01:28」+「N 天前 · 本地」。
- **改法**：`library/MediaItem` 增 `sourceName`/`season`/`episode`/`episodesTotal`/`kind`；`sourceOf` 移到 `library/SourceLabel.kt`（单一职责），按真实来源渲染；缺字段显示 `—` 而不是「本地」。

### 3.4 hero 整块不可点（P0）
- `DiscoverPage.kt:141-157` 的 `HeroCarousel` 未传 `onClick`；用户点海报/标题/评分无反应，只能滑动。
- **改法**：`HeroCard` 增 `onOpen: () -> Unit`，整卡 + 「播放」按钮分别路由到详情/直接播放。

### 3.5 资源库与添加流程
- 竞品（0366–0370）：网格卡（Logo + 状态点 + 名称 + 主线路 + `电影:2470 剧集:1921`）、「编辑」胶囊、分组设置（网盘/媒体服务器/模块 + 拖动 + 开关 + 新分组）、添加资源底部弹层（文件系统 WebDAV/SMB/当前设备；媒体服务器 Drivemby/Emby/Jellyfin/Plex；网盘 115/光鸭/百度/OneDrive）、添加表单五段（连接/信息/线路/iCloud/高级）。
- 当前：`sources/ui/*` 已拆得较清楚（15 个文件 958 行），但缺「电影/剧集计数」「分组设置」「五段式表单」，添加弹层分类维度不同。
- **改法**：`sources/ui/cards/SourceCard.kt` 增计数行；新增 `sources/ui/grouping/SourceGroupSheet.kt`、`sources/ui/add/AddSourceSheet.kt`（三段分类）、`sources/ui/form/SourceFormTabs.kt`（五段）；现有 `SourceWorkspace` 复用为「连接」段。

---

## 4. 交互逻辑

### 4.1 播放器面板不锚定触发控件（P0）
- 竞品：倍速面板出现在底栏「倍速」钮**上方**；顶栏图标面板从该图标**下方**展开（IMG_0160/0165/0171/0173）。
- 当前：`PlayerOverlayPanel.kt:63-69` 一律 `align(TopEnd)` + 固定 `topPad`——所有面板都从右上角冒出来。
- **后果**：点底栏「倍速」，面板在屏幕另一端出现，视线断裂；与竞品一眼可辨。
- **改法**：`PlayerPanelHost` 接收 `anchor: PanelAnchor`（`TopIcon(index)` / `BottomTextButton(id)` / `Center`），面板用 `Popup` + `LayoutDirection`/`offset` 计算锚点；`PlayerPanelRouter` 负责把触发源映射成 anchor。

### 4.2 顶栏第 4/5 图标语义互换（P1）
- 竞品：第 4 = 场记板（填充模式 + 画面比例），第 5 = 圈✓（跳过片段）。
- 当前：`PlayerTopBar` 的 `TOP_ACTIONS` 两者颠倒（C-TOP-01，IMG_0173/0165 证实）。
- **改法**：交换 `TOP_ACTIONS` 两项及对应 glyph；补一条 screenshot 回归（golden）。

### 4.3 选集态整条进度条消失（P1）
- 竞品 IMG_0170：选集横条展开时，轨道仍在。
- 当前：`PlayerBottomBar.kt:97-108` 提前 `return`，整条 `SeekTrack` 被摘掉。
- **改法**：改为「轨道保留、按钮行被选集条替换」，`SeekTrack` 常驻。

### 4.4 双指缩放缺失（P1）
- 竞品 `settings.playback.gestures.pinch`。全仓无 `detectTransformGestures`/pinch。
- **改法**：`PlayerGestureRouter` 增 `detectTransformGestures`，缩放范围 1.0–2.0，锁定态禁用；与 `PlayerLockLayer` 矩阵保持一致（长按/双击在锁定态仍可用）。

### 4.5 手势状态是进程级 `var`（真实缺陷）
- `PlayerGestureRouter.kt:198-203` 横滑状态写在 Kotlin 顶层 `private var`。
- **后果**：第二次进播放器，第一次横滑无反应或跳到上次位置。
- **改法**：改为 `remember { GestureState() }` 或 ViewModel 持有，随 Composition 生命周期。

### 4.6 减弱动画未接 App 内开关（P0）
- 竞品 G1：`settings.interface.page.reduceMotion` 显式开关 + 响应系统 `ReduceMotion`；每个动效必须有降级分支。
- 当前：`SettingsAppearanceSub.kt:51-53` 是灰行（「未接全局动效门」），播放器只认系统 `ANIMATOR_DURATION_SCALE == 0`。
- **改法**：`ui/motion/MotionGate.kt`（单一职责）：`reduceMotion = appPref || systemPref`；`PlayerMotion`/`HomeMotion` 统一读它；位移/缩放/旋转归零，仅保留 α 淡入。

---

## 5. 布局结构

| 编号 | 问题 | 证据 | 改法 |
| --- | --- | --- | --- |
| B-NAV-06 / SHELL-04 | 导航避让：发现/我的 84dp，资源库 94dp（另一处 84），切页胶囊**跳 10dp** | `MainActivityShell.kt:116`、`SourcesScreen.kt` | 统一抽 `ShellInsets.kt`，一个 `navReservedHeight = 84.dp` + `WindowInsets` 策略按页声明 |
| B-DISC-02 | hero 分页点选中态不变大（非尾页都 7.3） | `HeroCarousel.kt:126-131` | 选中点 7.3→9.0，动画 150ms；用 `animateDpAsState` |
| B-DISC-08 | hero 整块不可点 | `DiscoverPage.kt:141-157` | 见 §3.4 |
| B-DISC-11 | 首页缺 6 个模块：热门人物、播出平台、分类浏览、5 列海报网格、高分剧集、电影公司 | `DiscoverPage.kt:188-208` | 拆 `ui/design/home/sections/{PeopleRow,PlatformCards,GenreCards,PosterGrid,TopRatedGrid,StudioCards}.kt`，每文件一个 section |
| B-DISC-12 | 各 section 标题字号 16sp；竞品一级 18–20pt、二级 16–18pt | `DiscoverPage.kt:405-410` | `HomeSectionHeader(level: Primary/Secondary)`，字号走令牌 |
| C-BOT-01 | 选集态摘掉进度条 | 见 §4.3 | 同左 |
| A-SRP-01 | `PlayerActivity.kt` 901 行 | 实测 | 拆 `PlayerActivity`（仅生命周期/窗口）→ `PlayerStartCoordinator`、`PlayerEvidenceRecorder`、`PlayerSoakRunner`、`PlayerIntentRouter` |

---

## 6. 设计一致性

### 6.1 重复实现（必须收敛到唯一实现）
| 类别 | 套数 | 应保留 | 删除 |
| --- | --- | --- | --- |
| 玻璃面 | 5 | `PlayerGlassSurface.kt`（播放器）+ 新增 `GlassSurface.kt`（竖屏） | 其余 3 处 |
| 顶栏 | 5 | `PlayerTopBar.kt` + `ShellTopBar.kt` | Emby/Sources 各自的自绘顶栏 |
| 空态 | 6 | `components/EmptyState.kt`（图标+标题+说明+可选按钮） | 5 处内联 |
| 错误态 | 5 | `components/ErrorState.kt`（kind → 文案映射单一来源） | 4 处 |
| 卡片 | 5 | `components/MediaCard.kt`（2:3/16:9/宽卡三 variant） | 4 处 |
| 命中区 | 4 种（44/48/36/无） | `components/HitBox.kt`（44dp 下限） | 3 种 |

### 6.2 导航选中态（P0）
- 竞品（0366）：选中「资源库」= **灰底胶囊 + 原色图标 + 黑字**；未选中 = 黑灰图标。
- 当前：选中 = 蓝 tint `#0A84FF` + 白底 pill（`CapsuleNavBar.kt:43-53`）。
- **改法**：`selected` 时背景 `Color.White.copy(alpha=.60f)`（暗态 `.14f`），前景统一 `textMain`；`NavGlyph` 增加「发现」显式分支（当前落 `else` 兜底成指南针，B-NAV-01）。

### 6.3 Shell 路由
- `MainActivityShell.kt:89-93`：`listOf(1,3,0,4)` 魔法数组 + 裸 int `page`。
- **改法**：`sealed interface ShellTab { Discover, Profile, Library, Settings }` + `ShellRoute` 状态机；删除数组映射；`when(page)` 改 `when(tab)`。

### 6.4 安全/副作用
- `MainActivityShell.kt:80-84` 在 `SideEffect` 里按「是否有隐藏资源库且已解锁」给**整窗**加 `FLAG_SECURE`。
- **后果**：解锁后全 App 截图全黑（含正常的发现/设置页）；退出解锁后 flag 可能残留。
- **改法**：只在私有区 Activity 的 `onCreate` 设置、`onDestroy` 清理；Shell 不碰窗口 flag。

---

## 7. 代码可维护性

### 7.1 硬数据
| 指标 | 实测 | 门槛 | 判定 |
| --- | --- | --- | --- |
| UI 代码量 | 19,786 行 / `ui`；45,667 行 / Kotlin 全量（364 文件） | — | — |
| 裸 `dp` | 771 | 0（令牌外） | ❌ |
| 裸 `sp` | 277 | 0 | ❌ |
| `Color(0x…)` | 227 | 0（tokens 外） | ❌ |
| `Color.White/Black` | 139 | 0（用语义别名） | ❌ |
| 裸圆角 | 57 | 0 | ❌ |
| 令牌引用点 | 526 | — | 1471 : 526 |
| `@Preview` | 0 | 每个页面组件 ≥1 | ❌ |
| 超 300 行文件 | `PlayerActivity` 901、`EmbyResource` 454、`DiscoverPage` 426、`PlayerActionRouter` 407、`PlayerComposeActivity` 383 等 | 300 | ❌ |
| 自动门禁 | 无 | 行数/裸值 grep | ❌ |

### 7.2 可落地治理
1. `tokens/` 目录：`DesignPalette`（明暗原始值）、`DesignTokens`（语义）、`PlayerTokens`（播放器实测）、`SettingsVisuals`（设置页）——四者引用单向：Palette ← Tokens ← 组件。
2. `components/` 目录：GlassSurface、HitBox、MediaCard、SectionHeader、EmptyState、ErrorState、SwitchRow、SheetScaffold——页面只允许组合。
3. 门禁脚本 `tools/ui_gate.py`：① `PlayerActivity`/`DiscoverPage`/`EmbyResource` 行数 ≤300；② tokens 目录外 `Color(0x` 与裸圆角为 0；③ 每个 `*Page.kt` 至少有 1 个 `@Preview`。
4. 拆分顺序：先拆 `PlayerActivity`（901）→ `EmbyResource`（454）→ `DiscoverPage`（426）；每个拆分单独 PR，先补 `@Preview` 与 golden 截图再搬代码。

---

## 8. 统一 P0 清单（按修复依赖排序）

| # | 编号 | 问题 | 文件 |
| --- | --- | --- | --- |
| 1 | A-TKN-01 | 语义色仅浅色一套，竞品明暗双外观 | `DesignTokens.kt:24-28` |
| 2 | A-DUP-01…06 | 玻璃/顶栏/空态/错误/卡片/命中区各多套 | 见 §6.1 |
| 3 | A-SRP-01 | `PlayerActivity.kt` 901 行 | `ui/PlayerActivity.kt` |
| 4 | B-NAV-01/02 | 「发现」图标兜底成指南针；选中态蓝 tint 应为白底原色 | `NavGlyph.kt:63-69`、`CapsuleNavBar.kt:43-53` |
| 5 | B-NAV-06 | 导航避让 84 vs 94，跨页跳 10dp | `MainActivityShell.kt:116` |
| 6 | B-SHELL-01 | 裸 int 路由 + `listOf(1,3,0,4)` | `MainActivityShell.kt:89-93` |
| 7 | B-SHELL-03 | `FLAG_SECURE` 全窗 SideEffect | `MainActivityShell.kt:80-84` |
| 8 | B-PROF-01/06 | 「我的」占位、无模块流 | `Pages.kt:34-57` |
| 9 | B-WATCH-01/SHELL-02 | 「在看」入口归属错误 | `WatchingPage.kt:48-57`、`MainActivityShell.kt:155-162` |
| 10 | B-DISC-02/03/08 | 分页点不变大、继续卡缺季集与平台、hero 不可点 | `HeroCarousel.kt:126-131`、`DiscoverPage.kt:217-218/141-157` |
| 11 | B-DISC-11 | 首页缺 6 个模块 | `DiscoverPage.kt:188-208` |
| 12 | C-PNL-01 | 面板不锚定触发控件 | `PlayerOverlayPanel.kt:63-69` |
| 13 | C-MOT-01 | 减弱动画开关是灰行 | `SettingsAppearanceSub.kt:51-53` |
| 14 | C-GST-06 | 双指缩放缺失 | 全仓无 pinch |
| 15 | C-BOT-01 | 选集态进度条消失 | `PlayerBottomBar.kt:97-108` |
| 16 | C-BUG | 手势状态进程级 `var` | `PlayerGestureRouter.kt:198-203` |
| 17 | A-RAW | 1471 处绕过令牌 | 见 §7.1 |

---

## 9. 目标文件家族树（UI，落地后）

```
ui/
├─ tokens/            DesignPalette.kt  DesignTokens.kt  PlayerTokens.kt  SettingsVisuals.kt  MotionTokens.kt
├─ motion/            MotionGate.kt  HomeMotion.kt  PlayerMotion.kt
├─ components/        GlassSurface.kt  HitBox.kt  MediaCard.kt  SectionHeader.kt
│                     EmptyState.kt  ErrorState.kt  SwitchRow.kt  SheetScaffold.kt
├─ shell/             ShellTab.kt  ShellRoute.kt  ShellScaffold.kt  CapsuleNavBar.kt  NavGlyph.kt
├─ discover/          DiscoverPage.kt（编排）
│   ├─ hero/          HeroCarousel.kt  HeroPage.kt  HeroCard.kt  HeroDots.kt
│   └─ sections/      ContinueRow.kt  PosterRow.kt  WideRow.kt  VarietyCard.kt
│                     PeopleRow.kt  PlatformCards.kt  GenreCards.kt  PosterGrid.kt
│                     TopRatedGrid.kt  StudioCards.kt
├─ profile/           ProfilePage.kt  ProfileViewModel.kt
│   ├─ calendar/      WeekStrip.kt
│   ├─ overview/      TodaySummary.kt  HistoryHeatmap.kt
│   ├─ charts/        MonthlyChart.kt  TopShowsCard.kt  GenreDonut.kt  TrendCard.kt
│   ├─ lists/         WatchlistSection.kt  CollectionSection.kt
│   ├─ modules/       ModuleManagerSheet.kt
│   └─ watching/      WatchingPage.kt  ContinueListCard.kt
├─ library/           LibraryPage.kt  LibraryGrid.kt  SourceCard.kt
│                     AddSourceSheet.kt  SourceFormTabs.kt  SourceGroupSheet.kt
├─ settings/          SettingsPage.kt  SettingsGroup.kt  SettingsRow.kt  SettingsAppearanceSub.kt
├─ detail/            DetailPage.kt  DetailHero.kt  DetailActions.kt  DetailCast.kt
│                     DetailParentsGuide.kt  DetailSimilar.kt  DetailSeasonPicker.kt
└─ player/
    ├─ overlay/       PlayerOverlay.kt  PlayerTopBar.kt  PlayerBottomBar.kt  PlayerSeekTrack.kt
    │                 PlayerLockLayer.kt  PlayerStatusLayer.kt  PlayerToast.kt
    ├─ panels/        PlayerPanelHost.kt  PlayerPanelAnchor.kt  PlayerPanelRouter.kt
    │                 PlayerPanel.kt  PanelItems.kt  PlayerResourcePanel.kt  PlayerEpisodeStrip.kt
    ├─ gesture/       PlayerGestureRouter.kt  PlayerGestureState.kt  PlayerGestureHud.kt
    └─ session/       PlayerStartCoordinator.kt  PlayerSessionBridge.kt  PlayerEvidenceRecorder.kt
```

---

## 10. 修复批次（每批都可独立验收）

| 批次 | 内容 | 验收 |
| --- | --- | --- |
| **B0 基线** | 建 `tokens/`、`components/`、`motion/` 骨架；`tools/ui_gate.py`；补 golden 截图（Rex 116 张缩略 + kivo 同机同页） | 门禁能对现状打红；基线截图逐页可 diff |
| **B1 外壳** | 导航选中态、发现图标、避让统一、ShellTab 路由、FLAG_SECURE 下沉 | 四页切换胶囊不跳；选中态与 0366 一致 |
| **B2 我的+在看** | ProfilePage 看板（10 个 section）、ModuleManagerSheet、WatchingPage 迁入 | 0357–0362 五屏 1:1；空态文案一致 |
| **B3 发现** | 补 6 模块、继续卡数据、hero 可点、分页点 | 0330–0335 六屏 1:1 |
| **B4 播放器** | 面板锚定、图标语义、选集轨道、双指缩放、MotionGate、手势状态 | 0159/0162/0165/0170/0171/0173 六帧 1:1 |
| **B5 治理** | 令牌收敛、去重、裸值清零、拆三大文件 | `ui_gate.py` 全绿；行数 ≤300 |

---

## 11. 已对齐、不得回退（避免过度改动）

- 播放器轨道：6pt、三段半透明 + 1px 描边、**无滑块**、缓冲段可见。
- 底栏几何：`pitch 65`、`bottomSidePad 16`、`playGroupInset 7`、按钮行 24、距底 33。
- 面板几何：宽 248、圆角 16、行高 42/60、滚动条 3pt、内边距 16。
- 锁定态手势矩阵：长按/双击仍可用，上下滑/双指/横滑禁用。
- 触觉三态 + 低电量自动关闭（G2）；HUD 150/1000/300。
- 顶栏无遮罩（`topScrimTop/Bottom = 0x00000000` 是刻意，不是漏做）。
- 主 Tab 顺序 发现/我的/资源库/设置 与搜索圆钮。

---

## 12. 待真机确认（8 项，已给验证方法）

亮度分区归属（左音量/右亮度）、锁定入口位置、长按 HUD 精确位置、亮画面下未播段对比度、顶栏第 4/5 图标在真实帧的最终形状、iPhone 灵动岛/刘海对状态行的影响、选集态轨道保留的具体层级、环境浮层（在看/待看/收藏/下载/日历）的入口手势。

---

## 13. 下一步

按用户偏好：**先确认本报告与三份子报告，再进入编码**。确认后从 **B0 → B1** 开始，每一批先补家族树与 golden 截图，再动生产代码；严禁新建项目包，严禁平铺堆砌。