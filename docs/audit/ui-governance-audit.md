# kivo Video · UI 架构治理审计

> 审计对象：`android/app/src/main/java/com/kivo/cinema`（46 个文件包，UI 相关 12 个目录）
> 对照基准：`F:\Files\开发\rex\rex\Rex_UI结构复刻说明书.md` §3.1 语义色板 / §8 复刻落地清单 / §9 待确认项
> 项目自有铁律：`.codex/memory/kivo-video-iron-gate.md`（**每个 `.kt` ≤ 300 行**、单文件单职责、禁止平铺）
> 本文只做审计，**未改动任何生产代码**。

---

## 0. 审计口径与复算方法

路径缩写：`kt/` = `android/app/src/main/java/com/kivo/cinema/`。
所有数字均可用下列命令在 `kt/` 下复算，**不引用印象值**：

```bash
# 裸 dp（数字字面量直接 .dp）
grep -rE '[0-9]+(\.[0-9]+)?\.dp' --include=*.kt . | wc -l
# 裸 sp
grep -rE '[0-9]+(\.[0-9]+)?\.sp' --include=*.kt . | wc -l
# 硬编码十六进制色
grep -roE 'Color\(0x[0-9A-Fa-f]+' --include=*.kt . | wc -l
# 命名色（同样绕过令牌）
grep -roE 'Color\.(White|Black|Transparent)' --include=*.kt . | wc -l
# 圆角裸值
grep -rnE 'RoundedCornerShape\([0-9]+(\.[0-9]+)?\.dp' --include=*.kt . | wc -l
# 可预览性
grep -rn '@Preview' --include=*.kt . | wc -l
```

实测（2026-11-16）：

| 指标 | 本报告实测 | 任务书初值 | 差异说明 |
| --- | --- | --- | --- |
| 裸 `dp`（数字字面量） | **771** | 932 | 初值疑为含 `DesignTokens.x.dp` 的宽口径；精确到"数字字面量直写"为 771 |
| 裸 `sp`（数字字面量） | **277** | 297 | 同上 |
| 硬编码 `Color(0x…)` | **227** | 225 | 差 2 处为审计窗口内的新增 |
| `Color.White/Black/Transparent` | **139** | 未计 | 同样绕过令牌，必须计入 |
| `RoundedCornerShape(n.dp)` | **57** | 未计 | 圆角是在令牌之外最普遍的一类裸值 |
| `@Preview` | **0** | 未计 | 全套 UI 无一处可预览 |

**统计结论**：令牌绕过的实际规模 ≈ 771 + 277 + 227 + 139 + 57 = **1471 处**，而三套令牌（`DesignTokens` 38 个成员 / `PlayerTokens` 110 个 / `SettingsVisuals` 27 个，共 175 个）在全仓只有 **526 个引用点**（DesignTokens 189 / PlayerTokens 249 / SettingsVisuals 88，复算：`grep -roE 'DesignTokens\.[A-Za-z_]+' --include=*.kt . | wc -l`）。

即**裸值 : 令牌引用 ≈ 2.8 : 1**。更关键的是这 526 个引用点里 **249 个集中在播放器侧**（`ui/compose/player/`，40 个文件 / 5733 行，是全仓唯一成体系的令牌用例），其裸值只有 **168 处**（109 dp + 6 sp + 29 色 + 21 命名色 + 3 圆角，复算：`grep -rE '[0-9]+(\.[0-9]+)?\.dp' --include=*.kt ui/compose/player | wc -l`）。
其余全部目录（首页 / 资源库 / 在看 / 我的 / 设置 / 详情 / 私密区 / 媒体库 / 聚合）合计：**裸值 1303 处，令牌引用只有 277 处**（DesignTokens 189 + SettingsVisuals 88）——比例 **4.7 : 1**。播放器侧 168 : 249（0.67 : 1），页面侧 1303 : 277（4.7 : 1），**两者差 7 倍**。

问题总数：**53 条**（P0 = 19 / P1 = 26 / P2 = 8）。

---

## 1. 结论摘要（先看这 6 条）

1. **语义色只有浅色一套。** 竞品的 5 色（`canvasFill` / `groupedFill` / `pageBackground` / `hairline` / `washFill`）是明暗双外观共 10 条资源；kivo `DesignTokens.kt:24-28` 只有 5 条浅色，深色仅剩导航 2 条（`navPillDark` / `navIdleDark`）。全 App 唯一"暗态"是发现页 hero 下方的**临时反色逻辑**（`DiscoverPage.kt:102-103`），不是主题。
2. **三套令牌互相不认识，且同名不同值。** `hairline` 有三个定义（黑 5% / `#E3E5EA` / 白 8%）；文本次色有三个值（`#8A8A8E` / `#6B7280` / `#8A8A8E`）；"可点蓝"有两个值（`#0A84FF` / `#007AFF`）。
3. **令牌定义了却没人用。** `washFill`、`glassFallback`、`navHeight`、`navBottomPad`、`fontNavLabel`、`fontHeroMeta`、`fontHeroSynopsis` 共 7 个**零引用**，其中 3 个的数值被**逐字硬编码回**调用点（`CapsuleNavBar.kt:38/52`、`MainActivityShell.kt:176`）—— 这是"令牌失效"最硬的证据。
4. **三个 UI 主文件违反项目自己的铁律 3 倍以上。** `PlayerActivity.kt` 901 行（上限 300）、`EmbyResource.kt` 454 行、`DiscoverPage.kt` 426 行。
5. **同类组件每种都有 3–6 套实现**：玻璃面 5 套、顶栏/返回条 5 套、空态 6 套、错误态 5 套、卡片 5 套、命中区 4 种尺寸（44 / 48 / 36 / 无约束）。
6. **可维护性的底线缺失**：0 个 `@Preview`、UI 侧纯函数 0 测试、`AnimatedGradientBg` 每 100 ms 全屏重组且没有 Reduce Motion 开关（竞品 Settings 699 键里明确有 Reduce Animation Effects）。

---

## 2. A 组 · 设计令牌分裂

| 编号 | 问题 | 证据（文件:行） | 影响 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| TKN-01 | 5 个语义色只有浅色一套，缺 5 个 dark 变体；竞品是 10 条资源（5 色 × 明暗） | `kt/ui/design/DesignTokens.kt:24-28`（canvasFill/groupedFill/pageBackground/hairline/washFill 各 1 条）；仅 `:34-35` 有 `navPillDark/navIdleDark` | 视觉：暗环境（hero 暗底、播放器、私密区）无法用同一语义取色，只能各处 `Color.White.copy(alpha)` 临时反色；维护性：新增页面必再复制一套反色逻辑 | 拆 `DesignTokens.kt` → `ui/design/tokens/SemanticColors.kt`：`interface SemanticColors { val canvasFill; groupedFill; pageBackground; hairline; washFill; textMain; textSub }`，`object LightColors : SemanticColors`、`object DarkColors : SemanticColors`；`KivoTheme.kt` 用 `staticCompositionLocalOf<SemanticColors>` 提供；`DesignTokens` 保留为 `@Deprecated` 别名薄壳，逐个调用点改读 `KivoTheme.colors` | P0 |
| TKN-02 | `hairline` 三处定义、三处数值互斥（黑 5% / 浅灰实色 / 白 8%） | `DesignTokens.kt:27` `0x0D000000`；`ui/design/settings/SettingsVisuals.kt:32` `0xFFE3E5EA`；`ui/compose/player/PlayerTokens.kt:56` `0x14FFFFFF` | 视觉：设置页分隔线是不透明灰、全局是 5% 黑、播放器是 8% 白 —— 三种"分隔线"在同一次会话里出现；维护性：改一处不改另两处 | 语义拆分而非并值：`SemanticColors.hairline`（浅底上的 5%/10% 黑）、`SemanticColors.hairlineOnGlass`（玻璃白 8%）；`SemanticColors.hairlineStrong`（`#E3E5EA`，设置页实色档）；`SettingsVisuals.hairline` 删除，`SubPageScaffold:90` 改读 `hairlineStrong`；`PlayerTokens.panelBorder/panelDivider:56-57` 合并为 `hairlineOnGlass` | P0 |
| TKN-03 | 文本次色三值并存，其中两个完全同值、一个偏暗 | `DesignTokens.kt:17` `textSub=#8A8A8E`；`SettingsVisuals.kt:33` `sub=#6B7280`；`PlayerTokens.kt:20` `textSecondary=#8A8A8E`（与 textSub 同值重复定义） | 视觉：设置页副标题（`#6B7280`）比首页副标题（`#8A8A8E`）深约 12%，并排页面切换时可见灰度跳变 | 删 `SettingsVisuals.sub/sec` → 读 `KivoTheme.colors.textSub`；`SettingsVisuals.sec:34` 若无语义差异一并删除；`PlayerTokens.textSecondary:20` 改为引用 `SemanticColors.textSub`（播放器在视频上，改为 `textOnMediaSecondary = White 72%`，不共用文字灰） | P0 |
| TKN-04 | 品牌绿同值硬编码 6 处，仅 2 处走令牌 | 令牌：`DesignTokens.kt:20`、`SettingsVisuals.kt:21`、`PlayerTokens.kt:75,78`；硬编码：`kt/ui/detail/DetailResource.kt:93,97`、`kt/dlna/CastSheet.kt:50,77`、`kt/emby/ui/EmbyBrowseTopBar.kt:245`、`kt/ui/design/settings/SettingsDataRoot.kt:56,66` | 维护性：将来调整品牌绿需改 6 个文件；视觉：其中 2 处是"在线/成功"语义（DetailResource 源可用点）与"收藏/投屏"语义混用同一色，语义本就应分开 | 新增 `SemanticColors.success` / `SemanticColors.accent`；`DetailResource.kt:93,97` → `success`；`CastSheet.kt:50,77`、`EmbyBrowseTopBar.kt:245`、`SettingsDataRoot.kt:56,66` → `accent`；`SettingsVisuals.green:21` 删除 | P0 |
| TKN-05 | "可点蓝"两个值：`#0A84FF` 与 `#007AFF` 并存 | 令牌：`DesignTokens.kt:33` `navActive=0A84FF`、`SettingsVisuals.kt:20` `blue=0A84FF`；硬编码：`kt/ui/design/settings/SettingsRows.kt:149,213`、`kt/ui/design/settings/SettingsPicker.kt:95`、`SettingsDataRoot.kt:54,68` 均为 `007AFF` | 视觉：设置页开关/对勾/时装按钮的蓝与底部导航选中蓝肉眼可辨（Δ hue 约 3°），落在同一屏（设置页 + 胶囊导航同屏） | 统一为 `SemanticColors.accentLink`（取值保留 `0A84FF`，与 iOS 系统蓝对齐）；删除 `SettingsVisuals.blue`，4 个硬编码点改读令牌 | P1 |
| TKN-06 | 令牌被裸值逐字取代（"死令牌"），数值仍留在令牌里误导后来者 | `fontNavLabel=10f`（`DesignTokens.kt:56`）vs `kt/ui/design/CapsuleNavBar.kt:52` 写 `10.sp`；`navHeight=50f`（`:46`）vs `CapsuleNavBar.kt:38,57` 写 `50.dp`；`navBottomPad=22f`（`:47`）vs `MainActivityShell.kt:176` 写 `22.dp`；`fontHeroSynopsis=15f`/`fontHeroMeta=17f`（`:61-62`）vs `kt/ui/design/home/HeroPage.kt:139,141` 写 `14.sp`/`12.sp` | 维护性：**令牌值 = 谎言**。改令牌不生效，读令牌的人以为改一处就够；视觉：hero 简介实际 12sp，令牌声明 15sp，字号体系事实上不受控 | 三选一并写进注释：①调用点改读令牌（推荐，`CapsuleNavBar` 三处 + `MainActivityShell:176`）；②删死令牌；③若 hero 实测确实是 12/14，把令牌值改成 12/14 并注明"v3 复测"。**不允许令牌与实现长期不一致** | P0 |
| TKN-07 | `washFill`（洗色/高亮底）零引用 —— 竞品 5 色之一在 kivo 完全没落地 | 定义 `DesignTokens.kt:28`；全仓引用次数 = **0**（复算见 §0 命令扩展 `grep -r DesignTokens.washFill`） | 视觉：选中行 / chip / 高亮底只能用 `Color.White.copy(alpha)` 或 `navPill`，无法与竞品的"洗色"对齐（说明书 §3.1 明确 `washFill = #EDEDFB@50%`） | 在 `SemanticColors` 落 `washFill`（Light `#EDEDFB@50%`、Dark `#3D3D41`），先用到 `DiscoverPage.kt:46`（导航选中底）与 `SettingsRows.kt` 选中行；若决定不复刻，删除令牌并在 `docs/design` 写明放弃理由（禁止留无用令牌） | P1 |
| TKN-08 | `glassAlpha=0.70` 被渲染期篡改成 `0.92`，`glassFallback` 零引用 | `DesignTokens.kt:38-39`（`glassAlpha=0.70f`、`glassFallback=0xEEFFFFFF`）；`kt/ui/design/DesignComponents.kt:101` `.background(fill.copy(alpha = alpha + 0.22f))` | 视觉：所有白玻璃卡实际不透明度 92%，与设置页自定的"近白卡太糊亦否"（`SettingsVisuals.kt:9`）自相矛盾；卡片糊成一块、层级靠描边丢失 | `GlassSurface` 去掉 `+0.22f`；把 `alpha` 语义改为"底色不透明度"，缺省 `0.70`；API31 以下回退改为显式 `glassFallback`（当前是 0 引用的死常量）。顺便修 `MTB-10` | P1 |
| TKN-09 | 圆角四套体系 + 57 处裸圆角 | 令牌：`DesignTokens.kt:40-41`（13 / 10）、`SettingsVisuals.kt:43`（20）、`PlayerTokens.kt:194`（16）；裸值例：`kt/sources/ui/SourcesScreen.kt:103`（20）、`EmbyBrowseTopBar.kt:203-204`（16）、`kt/ui/design/WatchingPage.kt:103`（8）、`kt/emby/aggregate/ui/AggregateResultCard.kt`（多档） | 视觉：同一个 App 里"卡片"圆角有 8/10/13/16/20/24/50 七档，其中 8 与 10 在同一屏幕相邻出现（`WatchingPage.kt:95,103`）；维护性：改一个"卡圆角"要改 57 处 | 新增 `ui/design/tokens/RadiusTokens.kt`：`card=13`、`poster=10`、`thumb=8`、`pill=50`、`sheet=20`、`panel=16`（播放器专用命名空间）；57 处裸值按语义归位，**只允许"按语义选档"不允许"按数值相似合并"** | P1 |
| TKN-10 | 水平留白四档，其中最宽的一档是唯一在用的一档 | `DesignTokens.screenPad=13`（`:44`，9 处引用）；`SettingsVisuals.hInset=16`（`:45`，经 `kt/ui/design/settings/SettingsRows.kt:39` `settingsPad` 间接使用）；`DETAIL_PAD_DP=20`（`kt/ui/detail/DetailHeroContent.kt:116`）；`PlayerTokens.sidePad=19`（`:102`）；裸 `16.dp` 在 `DiscoverPage.kt:359,372` 与 `SettingsRows.kt:78,103,132,146,176,208,229` 反复出现 | 视觉：左边缘在四个主页面分别是 13 / 16 / 20 / 19 dp，页面切换时内容会"横向跳一下"——这是最容易被设计师一眼抓到的治理缺陷 | `SpacingTokens` 定 `pageInset=16`（统一目标值）、`playerInset=19`（播放器逐像素基准，单独命名空间）、`sectionGap=13`、`rowInset=16`；`DesignTokens.screenPad:44` 值由 13 改 16 并全量替换调用点（9 处），`DETAIL_PAD_DP` 改读 `pageInset` | P1 |
| TKN-11 | 布局令牌粒度混乱：`gap` 仅 1 处引用，`navHeight`/`navBottomPad` 0 引用 | `DesignTokens.kt:45-48`；引用统计见 §0（`gap=1`、`navHeight=0`、`navBottomPad=0`） | 维护性：无法回答"改间距要改哪里"——令牌既不全（大量裸值）又不准（死值） | 与 TKN-06/09/10 合并成一次 `SpacingTokens` 治理；`gap` 语义改名为 `cardGap` 并用于 `DiscoverPage.kt:167,233,258,322` 四处裸 `10.dp` | P2 |
| TKN-12 | Material 主题与自研令牌双轨，且 Material 侧自带 4 个新色值 | `kt/sources/ui/SourcesTheme.kt:16-25`：`Color.White.copy(alpha=.76f)`(:18,19)、`surfaceContainerLow=Color(0xFFFAF8FC)`(:21)、`surfaceContainerHigh=Color(0xFFF6F3F8)`(:21)、`outline=Color(0xFFD5D0DB)`(:23)、`error=Color(0xFFBA303B)`(:24)；另 `kt/privatezone/PrivateActivity.kt:109-110` 又一套 `darkColorScheme`；全仓 `MaterialTheme.colorScheme` 引用 **95 处**（26 个文件） | 维护性：**三套色彩来源**（DesignTokens / MaterialTheme / 局部硬编码）同时决定颜色，改主题不生效且难以排查；视觉：资源库页与发现页的白卡底不同（`.76` vs `.92` vs 纯白） | `SourcesTheme` 改为 `KivoTheme` 的薄封装：`MaterialTheme(colorScheme = KivoTheme.toColorScheme(LocalKivoColors.current))`，4 个本地新色并入 `SemanticColors`；`PrivateActivity.kt:109-110` 改为 `darkColorScheme` 从 `DarkColors` 派生；95 处 `MaterialTheme.colorScheme.*` 逐步替换为 `KivoTheme.colors.*` | P0 |
| TKN-13 | 背景三色渐变与"冷灰扁平"规范互斥，且渐变只在 1 处调用 | `DesignTokens.kt:11-13`（bgLavender/Pink/Cream 各 1 引用）；`kt/ui/design/DesignComponents.kt:60-65`；关闭逻辑 `MainActivityShell.kt:106-108` `flatColor = if (page == 1) null else pageBackground`；对立规范 `SettingsVisuals.kt:9`「拒粉紫脏底」 | 视觉：只有"发现"页开渐变，其余 4 页强制扁平；但发现页自己也 `drawBehind { drawRect(scrimColor) }`（`DiscoverPage.kt:124`）把渐变压掉 —— 即**渐变实际上没有任何页面可见**，却在持续消耗 CPU（见 MTB-01） | 决策二选一并删除另一边：①保留 `HomeAtmosphere` 色场，删 `AnimatedGradientBg` 的渐变分支；②保留渐变，去掉 `DiscoverPage.kt:124` 的不透明 `drawRect`。当前状态是"两套都开、互相抵消" | P1 |
| TKN-14 | 单位语义混用：令牌注释写 pt、代码写 dp、字号用 sp；`pt→dp→sp` 无统一换算 | `DesignTokens.kt:5-8` 注释「@3x÷3」、`:40-52` 全部 `.dp`；`PlayerTokens.kt:9` 注释「pt = px/3，误差 ±2pt」但字段是 `const val ... = 50f` + 调用点 `.dp`；`DiscoverPage.kt:406` `height(22.dp)` 与 `:408` `fontSize = 16.sp, lineHeight = 22.sp` 混排 | 视觉：系统字体放大时 `22.sp` 标题会溢出 `22.dp` 行高（无障碍大字模式必现）；维护性：设计师给 pt、实现写 dp、字号写 sp，三处换算靠人脑 | 新增 `ui/design/tokens/TypeScale.kt`：只暴露 `TextStyle`（含 `lineHeight`、`includeFontPadding=false`），**禁止在页面里直接写 `fontSize=`**；行高统一为 `TypeScale.X.lineHeight`；布局尺寸统一 `SpacingTokens`（dp），设计稿 pt 与 dp 视为 1:1 并在文件头写明 | P1 |
| TKN-15 | 缺两个竞品有、kivo 无的语义：`scrim`（遮罩/压暗）与 `groupedFill` 的实际使用 | `groupedFill` 定义 `DesignTokens.kt:25` 但仅 1 处引用（`SourcesTheme.kt:22`）；遮罩无令牌：`PlayerTokens.kt:42-47` `topScrimTop/Bottom` 全为 `0x00000000`（"刻意不加"，注释已声明），`DiscoverPage.kt:308-309` 手写 `Color.Black.copy(alpha=0.55f)`，`DetailPage.kt:120` 用 `luminance() < .40f` 自算亮度 | 视觉：压暗强度三处三个值（0 / 55% / 亮度阈值 40%），首页底部无压暗 → 白字压亮图时不可读；维护性：新页面必再手算一次 | `SemanticColors.scrimBottom`（浅 `Black@55%`、暗 `Black@70%`）+ `scrimHeight`；`DiscoverPage.kt:308-309`、`DetailPage.kt:120` 改读；`groupedFill` 用到设置分组底或删除 | P1 |

---

## 3. B 组 · 单文件单职责

> 项目铁律：`.codex/memory/kivo-video-iron-gate.md`「**每个 `.kt` ≤ 300 行**；超了必须**按"变化原因"拆**」。

| 编号 | 问题 | 证据（文件:行） | 影响 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| SRP-01 | `PlayerActivity.kt` 901 行，承担 7 个变化原因：生命周期/视图装配、硬解会话编排、软解路由与会话、字幕接线、取证落盘、探针参数解析、屏幕调试叠层 | 视图装配 `:289-337`；硬解 `:416-569`；软解 `:588-643`；字幕 `:507-551`；取证 `:91-100,668-707,783-792`；探针解析 `:715-764`（21 个 `intent.getXExtra`）；调试叠层 `:298-303` 创建 `TextView` + `:645-659` 刷新 | 维护性：单文件超铁律 **3 倍**；任何"加一个探针参数"都要动生产播放宿主；视觉/体验：调试文字块（`:299-302` 白字 + `0x99000000` 底）与产品播放器共用同一 Activity，一次误触就会在真机露出取证叠层 | 按变化原因拆（见 §7 家族树）：`ui/player/PlayerHostActivity.kt`（≤120 行，只做生命周期 + 组装）、`ui/player/host/PlayerSurfaceHost.kt`（Surface 回调 → `rebind`）、`ui/player/probe/ProbeSpec.kt`（一个 data class + `from(intent)`，替代 21 个字段）、`ui/player/probe/ProbeMediaLocator.kt`（`:879-895`）、`ui/player/hardware/HardwareStartFlow.kt`（`:416-569`）、`ui/player/software/SoftwareStartFlow.kt`（`:588-643`）、`ui/player/subtitle/SubtitleWiring.kt`（`:507-551`）、`ui/player/debug/StatsOverlay.kt`（`:298-303,645-659`）。变化原因分别是"播放链路""取证链路""调试链路"，不再混住 | P0 |
| SRP-02 | `EmbyResource.kt` 454 行，承担 9 项职责，且 **UI 与网络/策略同文件** | Emby 取数 `:81-98`；TMDB 填充与图择优 `:178-201`；预热 `:134-139,171-175`；续播策略 `:147-169`；Intent 构造 `:285-338,400-420`；剧集装载（4 线程池）`:247-283`；标记乐观更新 `:378-398`；`@Composable actions()` `:357-376`；图片加载 `:340-351` | 维护性：网络契约、播放意图、Compose 状态三种变化原因绑死；`@Composable` 出现在一个 `internal class` 里（`:357`），使该文件必须依赖 Compose，纯 Kotlin 测试不可覆盖；体验：续播点逻辑（服务器 > 本地，5 s 阈值）无法被单测锁定 | 拆为 6 个文件（见 §7）：`emby/detail/EmbyDetailFetcher.kt`（取数+组装）、`emby/detail/EmbyArtworkRanker.kt`（`qualityFirst`/`pickHigher` 纯函数）、`emby/detail/EmbyEpisodeCatalog.kt`（`loadAllSeasons`/`stillsFirst`）、`playback/EmbyPlayLaunch.kt`（`playIntent`/`episodeIntent`）、`playback/EmbyResumePolicy.kt`（`serverResumeMs`/`resumeMsOf`，纯函数）、`playback/EmbyMarkSync.kt`（`serverToggle` 乐观更新，非 Composable）；`@Composable actions()` 移到 `ui/detail/actions/EmbyDetailActions.kt` | P1 |
| SRP-03 | `DiscoverPage.kt` 426 行，一个文件里有：滚动/氛围色计算、LazyColumn 编排、3 个私有 section 组件、2 个卡片、错误态、节头、纯函数 | 氛围色 `:84-104`；编排 `:123-215`；`PosterSection` `:221-241`；`WideSection` `:246-281`；`VarietySection` `:287-344`；`TmdbErrorCard` `:348-375`；`TmdbPosterCard` `:378-401`；`HomeSectionHeader` `:404-426`；`sourceOf` `:218` | 维护性：首页是迭代最频繁的页面，"改一个 section"要在 426 行里定位；视觉：三处 section 的间距写法各不相同（`:229` 13+16、`:254` 13+16、`:294` 13+13），实际渲染不一致；测试：氛围色决策无边界可测 | 拆为 `ui/design/page/discover/DiscoverPage.kt`（只保留 `:62-215` 的编排与 state 消费 ≤120 行）、`DiscoverAmbience.kt`（`:84-104` 提成纯函数 `ambienceOf(listState, fieldColor): Ambience`，可单测）、`sections/ContinueSection.kt`、`sections/PosterSection.kt`、`sections/WideSection.kt`、`sections/VarietySection.kt`、`sections/HomeSectionHeader.kt`；`TmdbPosterCard` 与 `TmdbErrorCard` 移入 `components/card`、`components/state`（与 DUP-01/03 合并去重） | P1 |
| SRP-04 | `WatchingPage.kt` 142 行，5 个职责同文件：返回条、标题区、空态、整行卡（含进度条）、日期格式化调用 | 返回条 `:48-57`；标题区 `:58-65`；空态 `:66-85`；行卡 `:87-140`；进度条 `:121-133` | 维护性：空态与行卡是两个完全独立的迭代节奏（空态改文案 vs 行卡改布局），却互相牵制；视觉：返回条是**内联复刻**的（与 DUP-02 同源） | `page/watching/WatchingPage.kt`（骨架 ≤60 行）+ `WatchingRow.kt`（行卡）+ `WatchingEmpty.kt`（空态，改用 DUP-03 的统一 `EmptyState`）+ 返回条改用 DUP-02 的统一 `AppNavHeader` | P1 |
| SRP-05 | `SourcesScreen.kt` 190 行，9 个职责：刷新触发、导航守卫、BackHandler+解锁联动、workspace 状态、网格布局、头部动作、入口卡、加载/错误/空态、服务器轮询 | 刷新 `:44`；导航守卫 `:49-54`；BackHandler/解锁 `:57-66`；网格 `:71-75`；头部 `:76-90`；入口卡 `:91-101`；聚合入口 `:102-116`；三态 `:128-144`；轮询 `:146-152` | 维护性：行数虽未超 300，但**变化原因有 9 个**（铁律要求按变化原因拆，不是按行数）；视觉：该页是唯一混用 Material 控件（`FilledTonalButton:87`、`IconButton:83`）与自研玻璃语言的页面 → 视觉语言断层 | `sources/ui/SourcesScreen.kt`（只编排 ≤80 行）+ `sources/ui/overview/SourcesGrid.kt`、`SourcesHeaderActions.kt`、`AggregateEntryCard.kt`、`SourcesStates.kt`（三态）；`SourcesTheme` 改为 `KivoTheme` 薄封装（TKN-12）；`FilledTonalButton`/`IconButton` 换为 `components` 里的自研按钮 | P1 |
| SRP-06 | `Pages.kt` 混装页面与时间格式化纯函数 | `ProfilePage` `:35-57`；`relDays` `:60-68`；`fmtSeen` `:70-74`；`formatDate` `:76-81`（后三者 `internal` 被 `DiscoverPage`/`WatchingPage` 调用） | 维护性：UI 迭代与时间语义迭代互相触发重新审查；测试：三个纯函数**零单测**（边界 86400 s / 30 天 / 跨年），而它们决定"今天/昨天/3 天前"的用户可见文案 | `ui/design/page/profile/ProfilePage.kt`；纯函数移 `ui/format/TimeText.kt`（`object TimeText { fun relDays; fun clock; fun date }`）+ `test/java/.../TimeTextTest.kt` 覆盖 0/1/29/30/31 天与 59/60/3599/3600 s | P1 |
| SRP-07 | `DetailPage.kt` 266 行，页面编排与 sheet 几何常量/亮度判定混住 | sheet 常量 `:265-267`（`DETAIL_SHEET_START_DP=0f`、`DETAIL_SHEET_RAMP_DP=390f`）；亮度判定 `:120-123`；加载态 `:241` | 维护性：几何常量应属令牌层；亮度阈值属"chrome 反色"策略，两页各写一份（`DiscoverPage.kt:101` 用 `HomeAtmosphere.lightForeground`） | `DETAIL_SHEET_*` 移入 `SpacingTokens`/`LayoutBaseline`；`:120-123` 改读 `Ambience` 策略（与 SRP-03 的 `DiscoverAmbience` 同一个纯函数），消除两套亮度阈值 | P2 |
| SRP-08 | `EmbyBrowseTopBar.kt` 249 行，文件自述 3 职责，实际 5 件可复用物被 `private` 锁死 | 自述 `:37-40`；返回胶囊 `:62-64`；切服胶囊 `:69-91`；`GlassPillSurface` `:119-136`；`BackArrow` `:148-162`；`BrandMark` `:171-177`；`SwitchServerPopup` `:187-231`；`CurrentCheck` `:235-248` | 维护性：`GlassPillSurface` 是**全 App 唯一带描边的胶囊玻璃**，被 `private` 锁在 emby 包内 → 其他页面只能各写一套（DUP-01/02 的直接成因） | `GlassPillSurface` 提升为 `ui/design/components/glass/GlassPill.kt`（`internal` 公开）；`BackArrow`/`CurrentCheck` 移入 `components/glyph/ChevronGlyph.kt`（与 `HomeSectionHeader:416-423` 的 chevron、`DetailResource` 的 `⌄` 合并）；`BrandMark` 移入 `ui/brand/`；本文件只剩"顶栏 + 切服弹层"≤120 行 | P2 |
| SRP-09 | 铁律违规清单（>300 行且属 UI 主链）未收口，且无门禁阻止继续增长 | `ui/PlayerActivity.kt:901`、`ui/detail/EmbyResource.kt:454`、`ui/design/DiscoverPage.kt:426`、`ui/compose/PlayerActionRouter.kt:406`、`ui/compose/PlayerComposeActivity.kt:382`、`kt/library/MediaLibraryActivity.kt:334` | 维护性：铁律写在 `.codex/memory` 但**没有任何自动检查**——后续每次改动都可能继续加长（`EmbyBrowseTopBar.kt:39` 的注释证明曾达 466 行）；团队协作：新助手无法靠规则自证合规 | 加 `tools/check_kt_size.py`（或 Gradle `check` task 里的 detekt `MaxLineLength`/`LargeClass`），阈值 300，白名单冻结上述 6 个文件并要求**只减不增**；CI/收口脚本接入 | P0 |
| SRP-10 | 同一"媒体网格 + 卡片 + 操作对话框"在两处几乎逐行复刻 | `kt/library/MediaLibraryActivity.kt:257-322`（网格/卡片/徽标/标题）vs `kt/privatezone/PrivateActivity.kt:142-215`（同结构）；对话框 `:232-244` vs `:157-166` | 维护性：新增一种徽标要改两处；视觉：两页卡片圆角/底色/字号各不相同（`8.dp`+`#E0E0E0` vs `8.dp`+`#2C2C2E`），同一 App 两个"媒体网格" | 抽 `ui/media/MediaGridCard.kt`（参数化 `dark: Boolean`）+ `ui/media/RemoveFromZoneDialog.kt`；两页各自只留数据源与标题 | P2 |

---

## 4. C 组 · 裸值治理

> 判定原则：**"必须令牌化"= 该值承载语义（颜色/字号/圆角/页面留白/组件尺寸），会被第二个调用点复用且需要一起改**；
> **"可接受局部几何"= 只描述一个图形内部的比例或笔触，换屏不换值、不承载语义**（Canvas 比例、Stroke 宽度、阴影 elevation、动画毫秒）。

| 编号 | 问题 | 证据（文件:行） | 影响 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| RAW-01 | 单文件裸值密度最高：`DiscoverPage.kt` 43 处 dp + 13 处 sp + 5 处硬编码色 = **61 处**，且同一语义在同一文件有三种写法 | 节间距 `:163`(13) `:165`(12) `:229`(13) `:231`(16) `:254`(13) `:256`(16) `:294`(13) `:320`(8) `:340`(10)；横向 13 四处 `:168,234,259,322`；海报尺寸 `:387`(104) `:392`(156) `:264-269`(161×91) `:326-330`(103×58)；字号 12/11/15/13/16/20/14 八档 `:273,275,314,317,333,335,362,365,367,395,398,408`；色 `:102,103,309,315,317,368` | 视觉：**同一屏内 section 间距三种、16:9 缩略三种尺寸**（161×91 / 103×58 / 91×51），横滑时卡宽跳变；维护性：61 处无一处可被设计系统统一调整 | **必须令牌化**：`:163,229,254,294` → `SpacingTokens.sectionGap`；`:165,231,256` → `sectionTitleGap`；`:168,234,259,322` → `pageInset`；`:264-269`/`:326-330` → `CardMetrics.wideThumb`（16:9 单一来源）；`:387,392` → `CardMetrics.poster`（2:3 单一来源，**与 `DesignTokens.posterWidth=92` 冲突，取值需裁决**）；`:102,103,368` → `SemanticColors.textMain/textSub`；字号全走 `TypeScale`。**可接受**：`:416-422`（chevron Canvas 8×13dp、inset 1dp、stroke 1.7dp）保留 | P0 |
| RAW-02 | 整个页面把 4 个语义色数值逐字抄了一遍（`#F2F2F7`/`#8A8A8E`/`#1A1A1A`），共 9 处 | `kt/library/MediaLibraryActivity.kt:190`(bg `#F2F2F7` = `pageBackground`)、`:196,223,241,321`(`#8A8A8E` = `textSub`)、`:208,211,221,314`(`#1A1A1A` = `textMain`) | 视觉：该页在暗环境/主题改动后**不会跟随**（其余页面会）；维护性：这是"复制令牌值"而不是"引用令牌"，最危险的一类裸值——看起来一致、实际脱钩 | **必须令牌化**：9 处替换为 `KivoTheme.colors.pageBackground/textSub/textMain`；同时 `:283` `#E0E0E0`（卡片占位底）→ `SemanticColors.skeleton`（新增） | P0 |
| RAW-03 | 私密区自建深色主题，硬编码 4 个暗色值，其中 `#1C1C1E` 恰是竞品 `groupedFill` 的暗态值 | `kt/privatezone/PrivateActivity.kt:109-110`：`background=#1C1C1E`、`surface=#2C2C2E`、`onSurfaceVariant=#98989E`、`primary=#34C759`；`:187` `#2C2C2E`；`:138,150,165` `#98989E` | 视觉：暗色值是"对的"（与竞品一致）但**没有进令牌**，因此其他页面无法复用同一暗态 → App 内只有私密区是暗的；维护性：暗色体系事实上存在却不可发现 | **必须令牌化**：先按 TKN-01 建 `DarkColors`（`pageBackground=#000000`、`groupedFill=#1C1C1E`、`canvasFill=#222222`、`hairline=White@10%`、`washFill=#3D3D41` —— 全部取自说明书 §3.1 表），再把 `PrivateActivity` 的 6 处色值改为 `DarkColors` 引用 | P1 |
| RAW-04 | 设置行原语里 `16/12` 内边距重复 7 次、字号 15 与 4 个硬编码色 | 内边距：`kt/ui/design/settings/SettingsRows.kt:78,103,132,146,176,208,229`；字号：`:85,117,235`(15.sp)、`:63,151,161`(lineHeight 15/17)、`:213`(20.sp)；色：`:149,213`(`#007AFF`)、`:186,190`(`#E9E9EA`) | 视觉：行高在"设置"与"详情"两页不同（设置 12 上下内边距，详情 14），列表密度肉眼不一致；维护性：7 处同值内边距改一次要改 7 行 | **必须令牌化**：`SettingsRowTokens.rowPadH=16 / rowPadV=12`（或从 `SpacingTokens` 派生）、`TypeScale.rowTitle`；色按 TKN-05/TKN-12 归位。**可接受**：`:45,63` 的 4.dp/8.dp 微调（对齐字形 ink）保留但加注释 | P1 |
| RAW-05 | 字号事实上是 17 档，但令牌只有 8 个 —— 说明"字阶"不存在，只有散落数值 | 使用档位：10 / 10.5 / 11 / 11.5 / 12 / 13 / 14 / 15 / 16 / 17 / 18 / 20 / 22 / 24 / 26 / 28 / 32 / 34（sp）。同语义三值例：行标题 `DetailResource.kt:81`(17) vs `SettingsRows.kt:85`(15) vs `DetailResource.kt:59`(18)；节标题 `DesignTokens.fontSectionHeader=22f` vs `DiscoverPage.kt:408`(16) vs `SettingsVisuals.fontSection=13f` | 视觉：三级标题在三个页面是 22 / 16 / 13，层级关系不成立；维护性：新增文字无法回答"该用哪一档" | **必须令牌化**：`TypeScale` 只保留 9 档（`hero=34 / display=28 / title=22 / section=17 / row=16 / body=14 / meta=13 / caption=12 / micro=11`），带 `lineHeight` 与 `letterSpacing`；现有 34.sp（`HeroPage.kt:134`）、28.sp（`Pages.kt:38`、`WatchingPage.kt:58`、`SourcesScreen.kt:79`）、22.sp（`SettingsScaffold.kt:88`）逐一归位 | P1 |
| RAW-06 | 播放器"逐像素实测令牌"只覆盖主轨道，面板族仍大量裸值 —— 令牌覆盖边界不成立 | 裸值：`kt/ui/compose/player/PlayerResourcePanel.kt:60,71,94,97,118,120,131,136,143,149,150,157,164,169,178,183,191`（17 处）；`PlayerEpisodeStrip.kt:74,84,91,96,103,112,114,127,137,149,150,165,172,240,249,258`（16 处）；`PlayerGlassMenu.kt` 14 处 | 视觉：面板内 6/8/10/12/14/18/20 dp 七档间距，与 `PlayerTokens.panelPadding=16` 无对应关系 → 面板"看起来没对齐"；维护性：`PlayerTokens` 自己声明"唯一常量源"（`:6`）却不被面板族遵守，注释与事实相反 | **必须令牌化**：把面板族实际用到的间距归成 `panelGap` 四档（6/10/14/18）写进 `PlayerTokens` 并替换 47 处；`PlayerEpisodeStrip` 的 96dp 高度、`PlayerResourcePanel` 的 14dp 图标 → `episodeStripHeight` / `rowIconSize`。**可接受**：`:94` 1.5dp、`:118` 1dp 描边保留 | P1 |
| RAW-07 | 阴影 elevation 三值散落，无令牌 | `kt/ui/design/CapsuleNavBar.kt:39,58`（6.dp）；`kt/emby/ui/EmbyBrowseTopBar.kt:203`（16.dp）、`:130`（4.dp）；`kt/ui/compose/player/PlayerGlassSurface.kt:35`（18.dp 缺省）；`kt/ui/compose/player/PlayerToast.kt:42`（10.dp）、`PlayerLongPressHud.kt:53`（14.dp） | 视觉：悬浮层级不明（导航 6 / 弹层 16 / 播放面板 18 / toast 10），同层级不同高度 | **可接受局部几何但应集中**：`ElevationTokens { nav=6; popup=16; playerPanel=18; toast=10 }`，5 处引用，不新增抽象 | P2 |
| RAW-08 | 魔法数不是 dp 而是"390 基准"的裸数字，换设计基准即错 | `kt/ui/design/DiscoverPage.kt:97`（`390f`）、`:149`（`350 * density.density`）、`:88`（`180.dp` pull 阈值）；`kt/ui/design/home/HeroPage.kt:136`（`368` 直接当 dp 用 `offset(y=368.dp)`）；`kt/ui/detail/DetailPage.kt:267`（`390f`）；`kt/ui/design/HeroCarousel.kt:118-124`（`HERO_HEIGHT_DP=585`、`LOGO_CENTER_DP=402f`、`META_TOP_DP=472f`、`BADGE_TOP_DP=522f`、`DOTS_CENTER_DP=574.5f`、`DOT_SPACING_DP=17.7f`） | 视觉：hero 内容的位置全部按 390pt 宽硬编码；在 411/360 dp 设备上标题与 meta 与背景图错位（比例不随宽度缩放）；维护性：改一次 hero 结构要重算 6 个常量 | **必须令牌化（改为比例）**：`LayoutBaseline.kt` 提供 `DesignWidth = 390f` + `fun atWidth(frac: Float): Dp` → `585/390=1.5`、`368/390=.943`、`402/390=1.03`、`472/390=1.21`、`522/390=1.34`、`574.5/390=1.47` 全部改为相对比例；`DiscoverPage:97` 的 `390f` 与 `DetailPage:267` 复用同一常量 | P1 |

---

## 5. D 组 · 重复实现

> 本表"可落地改法"列统一回答"**这批重复里应保留哪一个唯一实现**"，其余全部删除或降为它的参数。

| 编号 | 问题 | 证据（文件:行） | 影响 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| DUP-01 | **玻璃面板 5 套**：白玻璃、黑玻璃、带描边胶囊玻璃、行卡"白 92%"、Material `primaryContainer .76` | ①`DesignComponents.kt:90-103`（白 + `alpha+.22`）②`PlayerGlassSurface.kt:33-67`（黑 75% + 高光 + 描边）③`EmbyBrowseTopBar.kt:119-136`（`Surface` + 白 .95 + 4dp 阴影 + 0.5dp 描边）④`WatchingPage.kt:96`（`Color.White.copy(alpha=.92f)` 直接铺）⑤`SourcesTheme.kt:18`（`Color.White.copy(alpha=.76f)`） | 视觉：**五种"玻璃"**，不透明度 76/92/95/100%，描边有/无、阴影有/无混搭 —— 这是全 App 最直观的"拼装感"来源 | 保留 **一个** `ui/design/components/glass/GlassSurface.kt`：参数 `role: Light/OnMedia`、`shape`、`elevation`，内部按 `role` 取 `panelFill`/`panelFillOnMedia`、`hairline`/`hairlineOnGlass`、`sheen`；`DesignComponents:90` 改为它的 `Light` 分支；`PlayerGlassSurface.kt` 删除（逻辑并入 `OnMedia`）；③→ `GlassPill.kt`（见 SRP-08）；④⑤ 全部替换 | P0 |
| DUP-02 | **顶栏/返回条 5 套**，命中盒与字号各不相同 | ①`SettingsScaffold.kt:72-91`（36dp 圆钮 + 22sp 标题 + 0.5dp 分隔）②`WatchingPage.kt:48-57`（`‹` 20sp + 15sp 文字，`clickable` 无最小尺寸）③`EmbyBrowseTopBar.kt:57-64`（44dp 圆 + 0.5dp 描边）④`kt/ui/compose/player/PlayerTopBar.kt:51`（38 个 `PlayerTokens` 支撑，横屏）⑤`kt/library/MediaLibraryActivity.kt:203-211`（`‹` 22sp + 17sp 标题 + `padding(13.dp)`） | 视觉：返回键在四页是 36 / 无 / 44 / 44 / 44dp 四档，标题 22/15/13/17sp；**用户在不同页按返回时手指落点不同**；维护性：五个文件各改一次 | 保留 `ui/design/components/nav/AppNavHeader.kt`：`AppNavHeader(title, onBack, actions: @Composable RowScope.() -> Unit, dense: Boolean)`，内部统一 44dp 命中盒 + `TypeScale.title`、`vertical` 由 `dense` 切换；①②⑤ 替换；③ 保留为 `GlassPill` 组合（播放器 ④ 因横屏逐像素基准独立，允许保留但必须复用 `hitTarget()`） | P0 |
| DUP-03 | **空态 6 套**：文案句式、字号、CTA 规格全不同 | ①`WatchingPage.kt:66-85`（`14.sp` + 22×9 内边距按钮）②`DetailResource.kt:143-180`（`EmptyResourceBlock`：Canvas TV 图 + 15sp + 12sp）③`DetailSeasonPicker.kt:104`（`SectionNote("本季暂无剧集")`）④`SourcesScreen.kt:136-144`（Material `Button` + `titleLarge`)⑤`EmbyPosterCard.kt:36`（`13.sp` "暂无海报"）⑥`DetailCalendarSheet.kt:98`（`13.sp` + `CalMuted`） | 视觉：6 种空态，其中 2 种有 CTA、4 种没有；字号 12/13/14/15 四档 → "空态"不是一个概念；维护性：新增页面必写第 7 套 | 保留 `ui/design/components/state/EmptyState.kt`：`EmptyState(art: @Composable ()->Unit?, title, body, action: EmptyAction?)`，字号走 `TypeScale.section/body/caption`；①②⑤⑥ 替换；③④ 用 `compact = true` 变体 | P0 |
| DUP-04 | **错误态 5 套**，重试语义三样：Snackbar / 全宽卡 / 行内文字 | ①`DiscoverPage.kt:348-375`（`TmdbErrorCard`：五类文案 + 全宽玻璃卡 + 绿色重试）②`SourcesScreen.kt:128-134`（`Surface` + Material `TextButton`）③`DetailLoadFeedback.kt:24-60`（**同时**有 Snackbar 与全宽卡两条路径）④`kt/emby/ui/EmbyBrowseRows.kt:107` 附近内联 ⑤`DetailLoadFeedback.kt:52-60` 全屏分支 | 体验：同一类"网络失败"在不同页面分别是"可以关掉的条"和"占满屏的卡"，用户不知道哪种会丢内容；视觉：重试按钮颜色/圆角/文案（"重试"/"重新读取"/"重试加载"）三种 | 保留 `ui/design/components/state/ErrorState.kt`（`inline`/`blocking` 两态，一个 `retryLabel`）；`DetailLoadFeedback` 的 Snackbar 分支保留但改为调 `ErrorState.inline`；文案集中到 `ui/format/ErrorText.kt`（`timeout/offline/secure/server/unknown` 五类，`DiscoverPage.kt:349-355` 的映射搬过去） | P1 |
| DUP-05 | **加载态 10 处使用 `CircularProgressIndicator`（另 3 个文件仅 import），尺寸五档 + 无统一说明文案** | `kt/emby/aggregate/ui/AggregateSearchScreen.kt:107`(24) `kt/emby/ui/EmbyBrowseRows.kt:107`(22) `EmbyBrowseScreen.kt:199`(26) `EmbyLibraryScreen.kt:79`(24) `kt/sources/ui/workspace/SourceConnectionContent.kt:108`(18) `kt/ui/detail/DetailSeasonPicker.kt:97`(28) `DetailPage.kt:241` `kt/sources/ui/SourcesScreen.kt:135`(无尺寸) `kt/emby/EmbyActivity.kt:150`(无尺寸) `AggregateActivity.kt:67`(无尺寸) | 体验：加载指示器大小随机、部分无文案（用户不知道在等什么），违反项目"不许静默降级"精神；视觉：`strokeWidth=2.dp` 只在部分处设置 | 保留 `components/state/LoadingState.kt`：`LoadingState(label: String?, size: LoadingSize = Small)` 统一 24dp + 2dp stroke + 必填 label；10 处替换，`label=null` 不允许通过 lint 白名单 | P1 |
| DUP-06 | **卡片 5 套**，同为"2:3 或 16:9 + 标题 + 副标题"，宽度四值 | ①`DesignComponents.kt:140-171`（`DesignPosterCard` 92×138）②`DiscoverPage.kt:379-401`（`TmdbPosterCard` 104×156）③`kt/sources/ui/cards/SourceTile.kt` + `SourceCard.kt`（列表行）④`kt/sources/ui/cards/StorageEntryCard.kt`⑤`kt/emby/ui/EmbyPosterCard.kt`；宽度：92 / 104 / 103（`WatchingPage.kt:102`、`DiscoverPage.kt:326`）/ 161（`DiscoverPage.kt:264`） | 视觉：海报卡 92 与 104 在同一 App 的"首页 vs 详情推荐"出现，2:3 比例精度不同（138 vs 156）；维护性：改一处圆角/占位色要改 5 处（占位色已有 3 值：`#E3DEEA`、`#24242A`、`#E0E0E0`） | 保留 `components/card/PosterCard.kt`（`width: PosterWidth = Small/Medium`、`aspect = 2/3`、`placeholder` 取自 `SemanticColors.skeleton`）：①②⑤ 替换；16:9 保留一个 `WideCard.kt`：③④ 替换；**占位色收敛到 1 个令牌** | P0 |
| DUP-07 | **进度条 3 套**（2dp 白轨 / 3dp 黑轨 / 6dp 三段玻璃轨） | `DesignComponents.kt:206-210`（`ContinueCard`：2dp、白 35% 轨）；`WatchingPage.kt:121-133`（3dp、`#1A000000` 轨 + 纯黑填充）；`PlayerSeekTrack.kt` + `PlayerTokens.kt:28-36`（6dp，五段） | 视觉：同为"看到哪儿"，三处颜色/厚度/圆角都不同（首页白、在看黑、播放器玻璃）；体验：首页与在看的进度条**同一部片显示同一比例但看起来不同粗细** | 保留 `components/progress/ProgressTrack.kt`（`thickness: 2/3/6dp` 作为档位参数、`onMedia: Boolean`），① ② 替换；播放器保留自实现但引用同一个 `thickness` 档位常量 | P1 |
| DUP-08 | **命中区 4 种**：44dp / 48dp / 36dp / 无约束 | 44：`PlayerTokens.kt:108,258`（9 处使用，仅播放器）；48：`kt/sources/ui/SourceCard.kt:54`；36：`SettingsScaffold.kt:80`；无约束：`WatchingPage.kt:74-78`（约 35dp 高）、`DetailResource.kt:108`（`clickable` 无高度）、`EmbyBrowseTopBar.kt:133`（44dp 但靠 `size` 而非令牌） | 体验：**无障碍下限 48dp 在 5 个页面里 4 个不达标**（Material 指南 48、项目 `PlayerTokens.minTouchTarget=44` 也只在播放器生效）；用户"按不到返回"就是这里的直接后果 | 保留 `components/hit/TouchTarget.kt`：`fun Modifier.hitTarget(size: Dp = 48.dp): Modifier`（`sizeIn` + `minimumInteractiveComponentSize`），全 App 交互点统一调用；播放器 44dp 作为 `hitTargetOnMedia(44.dp)` 显式例外并在 `PlayerTokens` 注明理由（屏幕物理尺寸） | P0 |
| DUP-09 | **导航/页头 3 套语义**：底部胶囊、设置 push/pop 头、聚合页 tab | `CapsuleNavBar.kt:23-64`（灰 pill + 蓝选中 + 阴影 6dp）；`SettingsNav.kt:28`（`SettingsNavHost` + `SubPageScaffold` 蓝 tint 圆钮）；`kt/emby/aggregate/ui/AggregateActivity.kt:87`（`ModeTab`） | 视觉：同一 App 里"选中态"有三种表达（白底 + 蓝字 / 蓝 tint 圆 / 自绘 tab）；维护性：`ModeTab` 只有 1 个调用方却自成一套 | 保留 `CapsuleNavBar` 为底部唯一实现；`ModeTab` 改为 `components/nav/SegmentedTabs.kt`（若聚合页需要二级切换）；设置子页头并入 DUP-02 的 `AppNavHeader` | P2 |
| DUP-10 | **Material 主题 2 套 + 无主题页面**，导致同屏颜色来源不同 | `SourcesTheme.kt:14-32`（light）+ `PrivateActivity.kt:109-110`（dark）+ 其余页面无 `MaterialTheme` 包裹（用默认） | 维护性：95 处 `MaterialTheme.colorScheme` 的解析结果取决于"当前在哪个页面树里"；视觉：Material 组件（`Button`/`Snackbar`/`Switch`）与自研组件底色不一致 | 只保留 `KivoTheme.kt` 一处 `MaterialTheme(...)` 包裹（在 `MainShell` 与每个 Compose Activity 的 `setContent` 根部），`SourcesTheme` 删除、`PrivateActivity` 的 dark `MaterialTheme` 删除 | P1 |

---

## 6. E 组 · 可维护性（副作用 / 状态提升 / 预览 / 纯函数测试）

| 编号 | 问题 | 证据（文件:行） | 影响 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| MTB-01 | 全屏渐变每 100 ms 触发一次整页重组，且无停止条件、无 Reduce Motion | `DesignComponents.kt:47-54`（`mutableFloatStateOf` + `LaunchedEffect(Unit) { while (true) { t.floatValue = ...; delay(100) } }`）；`:56-65` 在组合期读 `t.floatValue` 并算 3 次 `lerp3` + `Brush.linearGradient` | 体验：发现页每秒 10 次全屏重绘（`Brush` 每帧重建），在 K40 上滑动掉帧；无障碍：竞品有 Reduce Animation Effects（说明书 §6.7，Settings 699 键），kivo `AppearancePrefs` 无此开关（`AppearancePrefs.kt:19-27` 只有 4 项）；且该渐变实际被 `DiscoverPage.kt:124` 的 `drawRect` 全部遮住（TKN-13）→ **纯耗电、零收益** | ①按 TKN-13 决定是否保留；②若保留：`LaunchedEffect(Unit)` 改为 `withInfiniteAnimationFrameNanos`（Compose 自带、随动画时钟、可被系统缩放关闭），只把渐变**画在 Canvas**（`Canvas {}` + `drawRect(brush)`）而不是背景层重组；③`AppearancePrefs` 增 `reduceMotion`（`updateReduceMotion`），`AnimatedGradientBg(flatColor)` 在 reduceMotion 时直接扁平，动画时长读 `PlayerMotion`/`MotionTokens`（竞品动效文档 §1） | P0 |
| MTB-02 | 氛围色是页面内私有 state，被 3 条链路消费，且直接读 `LazyListState` → 无法单测、无法预览、无法外部驱动 | `DiscoverPage.kt:84-104`（`listState` → `density` → `pull` → `heroFieldColor by remember` → `scrimColor = remember { derivedStateOf }` → `chromeDark` → `chromeFg/chromeSub`）；`:214` 外写；`kt/ui/design/home/HeroCarousel.kt` 的 `onAmbient` 回调 | 维护性：**颜色决策是纯函数，却写在 Composable 体内**；体验：`heroFieldColor` 变更时 `derivedStateOf` 依赖 snapshot 读，行为正确但不可验证；改一次 hero 需要手工回归整页配色 | 提取纯函数（可单测）：`ui/design/theme/Ambience.kt` `data class Ambience(field: Color, dark: Boolean, fg: Color, sub: Color)` + `fun ambienceOf(fieldColor: Color, scrollDp: Float, pullDp: Float): Ambience`；`DiscoverPage` 只 `val amb = ambienceOf(...)`；`DetailPage.kt:120-123` 改调同一 `ambienceOf`，删掉自算 `luminance() < .40f` | P0 |
| MTB-03 | UI 状态外泄到 Activity：发现页把"我变暗了"写进 shell state，破坏单向数据流 | `DiscoverPage.kt:81,214`（`onAmbientDark: (Boolean) -> Unit` + `LaunchedEffect(chromeDark)`）；`MainActivityShell.kt:56,102,154,191-192`（`ambientDark` 被导航与状态栏读） | 维护性：任何页面都能改全局 chrome，新增页面会重复"上报"；体验：`LaunchedEffect` 在组合后才生效，暗态切换比画面晚一帧 → 胶囊导航闪一下 | 改成派生而非上报：`ambientDark` 由 shell 自己算（`page == 1 && ambienceOf(state.heroField, 0f, 0f).dark`），删除 `onAmbientDark` 回调；状态栏/导航色从同一 `Ambience` 派生 | P1 |
| MTB-04 | **`@Preview` = 0**；且组件在组合期 `bind(context)` 使它们无法脱离 Activity 预览 | `grep -rn '@Preview'` = 0；`CapsuleNavBar.kt:30-31`（`LocalContext` + `LaunchedEffect(Unit) { AppearancePrefs.bind(context) }`） | 维护性：设计系统没有"组件陈列室"，每次视觉改动只能靠真机截图回归（项目流程成本极高）；体验：无法在改代码时看到组件状态 | 新增 `android/app/src/debug/java/.../ui/preview/DesignSystemPreview.kt`（1 个文件，`@Preview` 网格覆盖 GlassSurface/PosterCard/EmptyState/ErrorState/AppNavHeader/CapsuleNavBar 的明暗 × 常态）；`AppearancePrefs` 改为"值 + setter"，`bind(context)` 只在 `MainActivity.onCreate` 调一次 | P1 |
| MTB-05 | 决定用户可见文案的纯函数零单测 | `Pages.kt:60-68`（`relDays`：0/1/30 天边界）、`:70-74`（`fmtSeen`：3599/3600 s 跨小时）、`:76-81`（`formatDate`）；`DiscoverPage.kt:218`（`sourceOf` 恒返回"本地"，`MediaItem` 有来源字段却不读）；`EmbyResource.kt:196-201`（`pickHigher` 图择优）、`:165-169`（`serverResumeMs` 5 s 阈值）；`DetailResource.kt:86,88`（`formatSize`/`formatBitrate`） | 体验：时间/续播/图质是"用户会在截图里挑错"的三类文案，目前靠人工看；维护性：`test/` 下唯一 UI 侧测试是 `brand/BrandTimelineTest.kt`（1 个），JVM 单测对 UI 语义零覆盖 | 移 `ui/format/TimeText.kt`（SRP-06）后写 `test/java/.../ui/format/TimeTextTest.kt`（≥8 例：-1/0/1/29/30/31 天、59/60/3599/3600 s、跨年）；`EmbyArtworkRanker`（SRP-02 拆出）配 `EmbyArtworkRankerTest`（相等宽度取库、缺图取对方、双缺）；`EmbyResumePolicyTest`（4999/5000 ms）；`sourceOf` 改为读 `MediaItem` 真实来源并配测试 | P0 |
| MTB-06 | `PlayerActivity` 同一段 KDoc 粘贴两遍（文件已难维护的征兆） | `PlayerActivity.kt:766-771` 与 `:773-782`（"失败也要留证据"整段重复，第二次多了 P14-R 三行） | 维护性：注释重复说明该文件已被多次增量追加而无人整体回读；后续改动者会读到两份"规则" | 随 SRP-01 拆分，注释各归其文件（`FailureEvidence` 的说明留在 `ui/player/probe/`） | P1 |
| MTB-07 | 起播链路里一处 `?: return` 会**静默跳过**探针的 pause/seek/soak 调度（取证型缺陷） | `PlayerActivity.kt:531-551`：`val controller = SubtitleOverlayController(... view = overlayView ?: return, ...)`；`:553-568` 的 `pauseRunnable`/`seekRunnable`/`soakRunnable` 全部在其后 | 体验/流程：`overlayView` 为 null（视图被提前移除或生命周期竞态）时，`maybeStartPlayback()` 直接返回 → 本轮 `kivo.pause.ms` / `kivo.seek.ms` / `kivo.soak` **一个都不执行，也没写入任何理由**；这正是项目铁律"不许静默降级"禁止的形状 | 改为局部守卫而非函数返回：`if (subtitleSibling != null && overlayView != null) { ... } else if (subtitleSibling != null) { Log.w(...); statusView.append("字幕不可挂: overlay 未就绪") }`；把 pause/seek/soak 调度提到字幕块**之前**，使调度不受字幕影响 | P0 |
| MTB-08 | `SourcesScreen` 每 60 s 对**所有**服务器强制刷新，且 `while(true)` 无显式退出 | `SourcesScreen.kt:146-152`：`dispatch source → LaunchedEffect(source.id, source.revision, lifecycle) → repeatOnLifecycle(STARTED) → while (true) { model.summaries.request(source, force = true); delay(60_000) }` | 体验：进页面即全量强制刷新（忽略缓存），多服务器时首屏抖动；电池/网络：后台切回立刻再打一轮 N 个 RTT；维护性：`while(true)` 仅靠 lifecycle 取消，违反"循环要有显式退出" | 改为 `flow { emitAll(ticker(60_000)) }` + `model.summaries.request(source, force = false)`（只在 `revision` 变化时 `force`）；刷新频率与一次性触发由 `SourceSummaries` 内部策略决定（把策略从 UI 移到模型，符合 SRP-05） | P1 |
| MTB-09 | `remember` 的 key 选择隐含高成本计算：数据映射在组合期执行 | `DiscoverPage.kt:107-121`（`remember(heroCandidates) { heroCandidates.map { HeroCard(...) } }`，含 `rating.toDouble()` 与 7 个字段拷贝）、`:94-100`（`remember { derivedStateOf }` 依赖隐式 snapshot） | 体验：TMDB 刷新时（5 个 list 同时到）会在一帧内做 6 次 list 映射 → 首屏卡顿；维护性：`map` 属于领域转换，不属于 UI | 映射移到数据层（`TmdbHome.toHeroCards()`，可单测），`DiscoverPage` 只接 `List<HeroCard>`；`remember` 改 `remember(heroCandidates)` → 直接消费已转换的不可变列表 | P2 |
| MTB-10 | `GlassSurface` 的 `alpha` 参数语义反直觉：传 `0f` 得到 22% 白底 | `DesignComponents.kt:91-101`（`alpha: Float = glassAlpha`、`.copy(alpha = alpha + 0.22f)`）；调用点 `:149`（`alpha = 0f` 用于海报卡） | 维护性：调用方无法从参数推断渲染结果；该隐式规则与 TKN-08 同为"玻璃不透明度不可控"的根因 | 删 `alpha` 参数，改为 `role` 枚举（见 DUP-01）；`:149` 用 `role = Plain`（无底色） | P2 |

---

## 7. 目标文件家族树

> 原则：目录语义优先（`tokens → components → page → format`），**每个文件一句话能说清职责**、≤300 行、不为"只有一个调用方"造空壳。
> 播放器侧 `ui/compose/player/`（40 文件 / 5733 行）已符合该范式，**作为迁移范本**，只做令牌引用替换，不重构。

```
kt/ui/design/
├── tokens/                                ← 唯一数值来源（新增，替代 DesignTokens/SettingsVisuals）
│   ├── SemanticColors.kt                  interface + LightColors / DarkColors（5 色 × 明暗 + success/accent/skeleton/scrim）
│   ├── KivoTheme.kt                       提供 LocalKivoColors + MaterialTheme 唯一包裹点
│   ├── TypeScale.kt                       9 档 TextStyle（含 lineHeight/字体缩放策略）
│   ├── RadiusTokens.kt                    card/poster/thumb/pill/sheet/panel
│   ├── SpacingTokens.kt                   pageInset/playerInset/sectionGap/rowInset/rowPadH/V
│   ├── ElevationTokens.kt                 nav/popup/playerPanel/toast
│   └── LayoutBaseline.kt                  DesignWidth=390 + atWidth(frac)
├── components/
│   ├── glass/GlassSurface.kt              唯一玻璃（role: Light | OnMedia）
│   ├── glass/GlassPill.kt                 带描边胶囊（从 EmbyBrowseTopBar 提升）
│   ├── glass/GlassCircleButton.kt         圆形玻璃钮
│   ├── nav/AppNavHeader.kt                唯一顶栏/返回条（普通 + dense）
│   ├── nav/CapsuleNavBar.kt               唯一底部导航
│   ├── nav/SegmentedTabs.kt               聚合页二级切换（替代 ModeTab）
│   ├── glyph/ChevronGlyph.kt              › / ‹ / ⌄ / ✓ 统一字形
│   ├── card/PosterCard.kt                 2:3 海报卡（Small/Medium）
│   ├── card/WideCard.kt                   16:9 宽卡
│   ├── card/ContinueCard.kt               继续观看卡
│   ├── card/SourceTileCard.kt             资源条目/存储条目
│   ├── state/EmptyState.kt                唯一空态（normal / compact）
│   ├── state/ErrorState.kt                唯一错误态（inline / blocking）
│   ├── state/LoadingState.kt              唯一加载态（必带 label）
│   ├── state/SectionNote.kt               行内说明（"本季暂无剧集"）
│   ├── progress/ProgressTrack.kt          唯一进度条（2/3/6dp 档）
│   └── hit/TouchTarget.kt                 Modifier.hitTarget(48dp) / hitTargetOnMedia(44dp)
├── theme/Ambience.kt                      ambienceOf() 纯函数 + Ambience 数据类（首页/详情共用）
├── page/
│   ├── discover/DiscoverPage.kt           只编排 + 消费 Ambience
│   ├── discover/DiscoverAmbience.kt       滚动→氛围色（调 theme/Ambience）
│   ├── discover/sections/ContinueSection.kt
│   ├── discover/sections/PosterSection.kt
│   ├── discover/sections/WideSection.kt
│   ├── discover/sections/VarietySection.kt
│   ├── discover/sections/LibrarySectionHeader.kt
│   ├── watching/WatchingPage.kt           骨架
│   ├── watching/WatchingRow.kt            整行卡 + 进度
│   ├── watching/WatchingEmpty.kt          空态（调 EmptyState）
│   ├── profile/ProfilePage.kt             我的（占位）
│   └── shell/MainShell.kt                 4 页编排（从 MainActivityShell 抽出）
└── format/
    ├── TimeText.kt                        relDays / clock / date（纯函数 + 单测）
    └── ErrorText.kt                       五类网络错误文案映射

kt/ui/player/                              ← 从 PlayerActivity.kt 拆出（901 → 8 文件）
├── PlayerHostActivity.kt                  生命周期 + 视图组装（≤120 行）
├── host/PlayerSurfaceHost.kt              SurfaceHolder 回调 → rebind
├── probe/ProbeSpec.kt                     一个 data class + from(intent)（替代 21 个字段）
├── probe/ProbeMediaLocator.kt             locateTestMedia / externalMediaDir
├── hardware/HardwareStartFlow.kt          openVideo→audio→pacing→pipeline→start→failStart
├── software/SoftwareStartFlow.kt          ffmpeg 路由与会话
├── subtitle/SubtitleWiring.kt             字幕挂载 + 图像轨提示（修 MTB-07）
└── debug/StatsOverlay.kt                  调试文字叠层（仅 debug 变体构建注入）

kt/emby/detail/                            ← 从 EmbyResource.kt 拆出（454 → 6 文件）
├── EmbyDetailFetcher.kt                   Emby 真值 + TMDB 填充 + DetailContent 组装
├── EmbyArtworkRanker.kt                   qualityFirst / pickHigher（纯函数 + 单测）
├── EmbyEpisodeCatalog.kt                  loadAllSeasons / stillsFirst（4 并发池）
├── EmbyPreheat.kt                         prewarm / discardPrewarm
├── EmbyMarkSync.kt                        serverToggle 乐观更新（非 Composable）
└── kt/playback/
    ├── EmbyPlayLaunch.kt                  playIntent / episodeIntent
    └── EmbyResumePolicy.kt                serverResumeMs / resumeMsOf（纯函数 + 单测）

kt/ui/detail/actions/EmbyDetailActions.kt  @Composable actions()（从 EmbyResource 迁出）

kt/sources/ui/
├── SourcesScreen.kt                       只编排（≤80 行）
├── overview/SourcesGrid.kt                网格 + 分组
├── overview/SourcesHeaderActions.kt       锁/添加/刷新
├── overview/AggregateEntryCard.kt
└── overview/SourcesStates.kt              加载/错误/空态（调 components/state）

kt/ui/media/
├── MediaGridCard.kt                       本地影片/私密区共用卡
└── RemoveFromZoneDialog.kt                共用确认对话框
```

**删除清单（迁移完成后）**：`ui/design/DesignTokens.kt`（并入 tokens/）、`ui/design/settings/SettingsVisuals.kt`（并入 tokens/）、`kt/sources/ui/SourcesTheme.kt`（并入 KivoTheme）、`ui/design/Pages.kt`（拆 page/profile + format/TimeText）、`ui/compose/player/PlayerGlassSurface.kt`（并入 components/glass）。

---

## 8. 建议修复顺序（按"阻断后续工作"排序）

| 批次 | 内容 | 理由 | 验收方式 |
| --- | --- | --- | --- |
| 第 1 批（P0 结构） | TKN-01/02/03/04/06/12 → `tokens/` 落地；DUP-01/02/08 → 三个唯一组件落地 | 令牌与基础组件是所有页面改动的上游；不先做，后面每个页面都要改两遍 | 迁移前后真机截图逐页比对（K40 `b6e950ee`），像素差仅允许出现在"刻意的统一"处；`grep -c 'Color(0x'` 下降 ≥ 60% |
| 第 2 批（P0 拆分） | SRP-01（PlayerActivity 901→8）+ SRP-09（size 门禁脚本） | 铁律违规最重、且带一个静默失败缺陷（MTB-07） | 每个新文件 ≤300 行；`tools/check_kt_size.py` 退出码 0；真机回归播放/seek/字幕/软解四条路径 |
| 第 3 批（P0 裸值/重复） | RAW-01/02、DUP-03/06、MTB-01/05 | 首页裸值最密、空态/卡片最多、纯函数无测试 | `DiscoverPage` 裸 dp 从 43 → ≤8（余下为 Canvas 几何）；新增 5 个 JVM 单测全绿 |
| 第 4 批（P1） | TKN-05/07/08/09/10/14/15、SRP-02/03/04/05/06、RAW-03/04/05/06/08、DUP-04/05/07/10、MTB-02/03/04/06/08 | 语义与体验级差异 | 每项单独一条 commit，附真机截图 |
| 第 5 批（P2） | SRP-07/08/10、TKN-11、RAW-07、DUP-09、MTB-09/10 | 打磨与去重，不阻断 | — |

---

## 9. 与竞品口径的差距对照（只列治理相关）

| 维度 | Rex（说明书） | kivo（本审计） | 差距性质 |
| --- | --- | --- | --- |
| 语义色 | 5 色 × 明暗双外观 = 10 条资源（§3.1，含 P3/extended sRGB 注意项） | 5 色仅浅色一套；另 2 个暗色值是导航 chrome（`navPillDark`/`navIdleDark`），不构成语义色对 | 结构性缺失（TKN-01） |
| 色域 | `washFill` 浅色态是 **Display P3 @50%** | `washFill` 定义为 sRGB `0x80EDEDFB` 且**零引用** | 双重缺失（TKN-07） |
| 分隔线 | `hairline` 唯一（5% 黑 / 10% 白） | 三套三值 | 语义未收口（TKN-02） |
| 圆角/阴影/间距 | 说明书 §9 标为 ❌ 加密不可得 → 复刻方必须自己定一套 | 现有 4 套圆角、4 档留白、5 种阴影 | kivo 本应是"定义者"，却成了"分裂者" |
| Reduce Motion | Settings 699 键含 Reduce Animation Effects / Haptics 低电自动关 | `AppearancePrefs` 4 项，无 reduce motion | 缺失（MTB-01） |
| 设计基准 | 393×852 pt，2:3 / 16:9 | 390 基准硬编码为裸数字（6 个 hero 常量） | 基准存在但不可参数化（RAW-08） |

---

*本报告的所有行号基于审计时工作区快照（2026-11-16）。数字均可按 §0 命令复算；未修改任何生产代码。*