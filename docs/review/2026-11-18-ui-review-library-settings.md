# Apple 标准 UI 审查 · 资源库页 + 设置页

> 审查日期：2026-11-18 · 参照系：Apple HIG + Apple 设计工程哲学（taste / 看不见的细节 / 美感是杠杆）
> 范围：点「资源库」tab 进入的 `SourcesScreen`、点「设置」tab 进入的 `SettingsPage` 及其 8 个二级子页
> 方法：逐行读真实代码 + 对照项目既有 token（`DesignTokens` / `SettingsVisuals` / `MotionTokens`），不凭印象
> 边界：这是**审查报告**，不改代码（项目铁律第 4 条 UI 已定稿）。既有 `2026-11-18-full-audit.md` §3 已列的通用问题（返回键 4 种、48dp 命中、玻璃弹层、token 方言、导航选中态）此处标「已知」不重复，聚焦两页特有增量。

---

## 零、总体判断

这两页的**信息架构是成立的**：资源库把「本地/存储」与「媒体服务器」分区、源状态有在线点、聚合搜索独立成卡；设置页 12 个入口分 5 组、行式化后层级清晰。`SettingsRowItem` 的几何 chevron、`maxLines=1` 省略、`Role.Button` 语义、`heightIn(min)` 抗字体放大——这几处是有工艺水准的。

但用 Apple 的标准看，**最大问题不是单个控件，而是"三套视觉方言在同一屏并存"**：`DesignTokens`（全局）、`SettingsVisuals`（设置页私有）、`MaterialTheme.colorScheme`（资源库）、以及散落的硬编码色值/字号/圆角。Apple 的界面之所以"感觉对"，靠的是**一套尺度贯穿始终**——字号、间距、圆角、蓝色只有一种。当前两页是"每块都对、拼起来不对"。这是 P0 级的系统性问题，其余都是它的具体表现。

第二突出的问题是**符号当图标**：`›`、`↻`、`⋯`、`‹`、`✓` 全是文字字形，字号 22–24sp。它们随字体缩放变形、基线与文字不齐、粗细不可控——Apple 一律用矢量图标（SF Symbols 等价物）。

---

## 一、P0 · 必须修（系统性 / 明显破坏 Apple 质感）

### P0-1 三套视觉方言并存，同一屏内尺度冲突
**页面/元素**：两页全局 · `DesignTokens.kt` / `SettingsVisuals.kt` / `SourcesTheme.kt` + 硬编码

同一个"次要文字"有 3 个来源：`textSub=#8A8A8E`（DesignTokens）、`sub=#6B7280`/`sec=#7A8089`（SettingsVisuals）、`onSurfaceVariant`（MaterialTheme）。同一个蓝有 2 个值：`0xFF0A84FF`（SettingsVisuals.blue / navActive）与 `0xFF007AFF`（SettingsRows 的 LinkRow/RadioRow）。同一个圆角有 13/16/20/24dp 四种。Apple 的「语义色 + 单一尺度」在这里被切成三套。

| Before | After | Why |
| --- | --- | --- |
| 次要文字 3 源：`textSub` / `SettingsVisuals.sub` / `onSurfaceVariant` | 统一到 `DesignTokens.textSub` 一处，其余删除或改为别名 | 一处真相源，改一次全 App 一致；这正是 Apple 靠"看不见的一致性"建立信任的方式 |
| 蓝色 2 值：`0xFF0A84FF` 与 `0xFF007AFF` | 收敛为单一 `navActive` 语义色 | 两个 iOS 蓝差 2 个色阶，肉眼可辨但不协调，是"哪里不太对"的典型来源 |
| 行高/圆角/间距 13/16/20/24/28dp 混用 | 建 `design/tokens/` 唯一源，定 4 档圆角（小控件/卡/大卡/弹层）逐页收口 | 尺度离散 → 视觉噪音；Apple 的间距是有节奏的整数倍，不是随手取值 |

**影响**：每加一个控件都要在 3 套里选一套，是重复劳动和视觉漂移的根源；既有 full-audit P1-7 已诊断，但**从未落地**，本次升级为 P0。
**优先级**：P0（最高，其他项多是其下游）

### P0-2 符号字形当图标，随字体缩放变形、基线不齐
**页面/元素**：资源库 `SourcesScreen.kt:113/124`、`StorageEntryCard.kt:10`、`SourceCard.kt:56`；设置 `SettingsRows.kt:88/120/213`、`SettingsScaffold.kt:85`

`›`(22sp)、`↻`(22sp)、`⋯`(24sp)、`‹`(24sp)、`✓`(20sp) 全是文字。符号的字宽、基线、粗细由字体决定：系统字号放大时箭头会歪、和标题不对齐；`↻` 在不同字体渲染粗细不一。Apple 的 chevron/refresh 是矢量，几何恒定。

| Before | After | Why |
| --- | --- | --- |
| `Text("›", fontSize=22.sp)` 作箭头（资源库卡、设置子页行） | 复用 `SettingsRowItem` 已有的 Canvas 几何 chevron，抽成 `KivoChevron` 全 App 共用 | 项目里**已经有一个正确的几何实现**（12dp 圆头折线），却没有复用——这是最可惜的浪费 |
| `Text("↻", fontSize=22.sp)` 作刷新 | 用 `Icons.Default.Refresh` 矢量图 | 符号随字体缩放且无点击反馈形变，矢量稳定 |
| `Text("⋯"/"‹"/"✓")` | 分别用 `Icons.MoreVert` / 返回 chevron / `Icons.Check` | 一致性 + 可访问性（图标可带 contentDescription） |

**影响**：视觉精致度最大的扣分项，一眼就能看出"不是原生质感"。**优先级**：P0

### P0-3 资源库大标题字号与设置页不一致（28 vs 34）
**页面/元素**：资源库 `SourcesScreen.kt:79` vs 设置 `SettingsPage.kt:124`

同为 tab 顶级页大标题，资源库硬编码 `28.sp`，设置用 `DesignTokens.fontPageTitle=34.sp`。两页相邻切换时标题字重/尺寸跳变。Apple 的 tab 级页面标题是全 App 统一的 Large Title。

| Before | After | Why |
| --- | --- | --- |
| 资源库 `Text("资源库", fontSize=28.sp, Bold)` | `fontSize = DesignTokens.fontPageTitle.sp`（34） | 同层级标题一致；Apple Large Title 是全局尺寸，不按页微调 |
| 设置 `SettingsScaffold.kt:46` 又定义 `fontRootTitle=32/ExtraBold`（死代码，未被 SettingsPage 使用） | 删除死常量，`SettingsScaffold` 要么接入要么删 | 三处定义同一个"设置大标题"= 迟早用错 |

**影响**：tab 间切换的连贯性被打破。**优先级**：P0（成本低、收益直观）

---

## 二、P1 · 应该修（明显偏离 Apple 规范）

### P1-4 设置子页转场 350ms 且整页用 EaseOut，违反 Apple 动效铁律
**页面/元素**：`SettingsNav.kt:38-44`

`tween(350, easing = EaseOut)` 做整页水平平移。三个问题：①350ms > 300ms UI 上限；②屏上位移应用 `ease-in-out`（FastOutSlowIn），`EaseOut` 是进场专用，整页移动会显得"起步猛、刹车飘"；③与 `MotionTokens.pageTransitionMs=300` 直接冲突，且 MotionTokens 根本没定义缓动曲线。

| Before | After | Why |
| --- | --- | --- |
| `tween<IntOffset>(350, easing = EaseOut)` 整页平移 | `tween(300, easing = FastOutSlowInEasing)`（或 260ms） | 屏上移动用对称缓动；300ms 内更跟手，符合 Apple Navigation 推入手感 |
| 时长硬编码 350，与 `MotionTokens.pageTransitionMs=300` 冲突 | 改读 `MotionTokens.pageTransitionMs` | 扩展而非分叉 token；时长只留一个真相源 |
| 无 `prefers-reduced-motion` 分支 | 系统减少动效时降为纯 fade（不归零） | Apple 要求 reduced-motion 下"更轻柔"而非"完全没反馈" |

**影响**：每次进设置子页都能感知到的"不够顺"。**优先级**：P1

### P1-5 在线状态灯只有色点，色觉障碍不可辨
**页面/元素**：`SourceCard.kt:43-53`

6dp 圆点用绿/红/灰表示在线/失败/待检测，`semantics` 里虽有中文描述（这点做得对），但**视觉上**只有颜色一个维度。Apple 要求状态用「颜色 + 形状/文字」双编码。

| Before | After | Why |
| --- | --- | --- |
| 仅 6dp 色点区分三态 | 绿点+「在线」/ 红点+「离线」文字 chip，或在线用实心、离线用空心+斜杠 | 色觉障碍用户（约 8% 男性）无法区分红绿；双编码是无障碍基本要求 |
| 状态色硬编码 `0xFF76C442`/`0xFFE65361`/`0xFFB8BBC4` | 收进 token 并复用 `accentGreen`/`dangerRed` 语义 | 与全局语义色统一，避免第 N 种绿 |

**影响**：可用性 + 无障碍。**优先级**：P1

### P1-6 触摸目标不达标：返回钮 36dp、源卡"管理"文字区
**页面/元素**：`SettingsScaffold.kt:80`（返回 36dp）、已有 full-audit P1-5 覆盖全局

设置子页返回钮 `size(36.dp)`，低于 44/48dp 下限。虽点击有 ripple，但热区太小。既有报告已列 P1-5（全局 <48dp），此处是**设置页具体实例**，需一并纳入修复清单。

| Before | After | Why |
| --- | --- | --- |
| 返回钮 `Modifier.size(36.dp)` | 视觉 36dp 保留，透明命中区撑到 48dp（`.size(48).wrapContentSize { ...size(36) }`） | Apple 44pt 最小热区；视觉可小、命中不可小 |
| chevron 用 `Text("‹", 24.sp)` | 矢量 chevron + 48dp 命中 | 同 P0-2 |

**优先级**：P1（与 full-audit P1-5 合并执行）

### P1-7 资源库副标题对比度依赖 MaterialTheme，与设置页灰色不一致
**页面/元素**：`SourcesScreen.kt:80/92/122`、`SourceTile.kt:35`

资源库副标题用 `MaterialTheme.colorScheme.onSurfaceVariant`（在 `SourcesTheme` 里映射到 `textSub=#8A8A8E`）。该灰值在 full-audit P1-8 已被认定"压在 75% 黑上 ≈3.1:1 不达标"。资源库是浅底，对比度勉强，但与设置页 `SettingsVisuals.sub=#6B7280`（更深）明显不同——同一"副标题"语义两页一个深一个浅。

| Before | After | Why |
| --- | --- | --- |
| 资源库副标题 `onSurfaceVariant`(#8A8A8E) vs 设置 `sub`(#6B7280) | 统一到对比度 ≥4.5:1 的 `textSub`，两页同值 | Apple 正文次级文字也要 4.5:1；跨页一致性 |

**优先级**：P1

---

## 三、P2 · 锦上添花（精致度 / 长期）

### P2-8 资源库分区标题字号不齐
**页面/元素**：`SourcesScreen.kt:92`（「本地与存储」13sp）、`:119`（「媒体服务器」16sp）

同层级的两个分区标题一个 13sp 常规、一个 16sp Medium。Apple 的 section header 是统一字阶。

| Before | After | Why |
| --- | --- | --- |
| 13sp vs 16sp 两个分区标题 | 统一 `fontSectionTitle`（17sp Medium 或定一档 13sp） | 同级信息同权重，避免用户误判层级 |

**优先级**：P2

### P2-9 空状态插画缺失
**页面/元素**：`SourcesScreen.kt:136-144`

无服务器时用纯文字 + Surface 实心卡（「把你的电影库带进来」）。Apple 的空状态通常配一副轻量插画/符号，且按钮位置固定。项目已有 `ui/components/EmptyState.kt`（full-audit P2-10 提到）却未在此复用。

| Before | After | Why |
| --- | --- | --- |
| 手写 Surface + 三段文字 | 复用 `EmptyState` 组件 | 复用既有，避免又一套空状态样式 |

**优先级**：P2

### P2-10 死代码文件未清理
**页面/元素**：`SettingsEntryGrid.kt` / `SettingsStatusTiles.kt` / `SettingsStorageHero.kt`

`SettingsPage.kt:45-47` 自注这三个文件"不再被调用，保留不删"。项目铁律第 3 条要求"新代码落地时彻底移除被替代的旧代码"。三份死文件是后来者误用的陷阱。

| Before | After | Why |
| --- | --- | --- |
| 3 个死文件保留 | 删除（git 历史可追溯） | 消除歧义；符合铁律第 3 条 |

**优先级**：P2（涉及铁律，可提到 P1 执行）

---

## 四、已做得好的（保持，别改）

| 元素 | 好在哪 |
| --- | --- |
| `SettingsRowItem` 几何 chevron | Canvas 圆头折线、不随字体缩放变形——应抽出来全 App 复用（见 P0-2） |
| `heightIn(min=46)` + `maxLines=1` | 抗系统字体放大溢出、长文案省略，Apple 式鲁棒 |
| `Role.Button` / `Role.Switch` / `contentDescription` | 语义完整，`SourceCard` 在线点甚至有中文状态描述 |
| `SwitchRow` 用 `toggleable` 整行可点 | iOS 设置开关的标准交互 |
| `SwitchDefaults.colors` 自定义 track/thumb | iOS 绿开关语义到位 |
| 资源库 `minimumCardWidth = 160.dp * fontScale` | 响应式栅格随字号自适应，是难得的正确实现 |

---

## 五、修复优先级路线（给落地者）

| 波次 | 项 | 理由 |
| --- | --- | --- |
| 第 1 波 | P0-1 + P0-3 | 单一 token 源 + 标题一致，一次改完全面止血，其他项多是下游 |
| 第 2 波 | P0-2 + P1-4 | 矢量图标 + 转场曲线，质感提升最直观 |
| 第 3 波 | P1-5/6/7 + P2-10 | 无障碍 + 命中区 + 死代码，规范与卫生 |
| 第 4 波 | P2-8/9 | 精致度微调 |

> 体感需真机验证项：P1-4 转场时长（改后 2–5× 慢放看推入是否顺）、P0-2 矢量图标基线是否与文字对齐（真机大字体档复看）。这些无法从代码判定，需解锁设备实测。

---

## 六、与既有审查的关系

- `full-audit.md` §3 的 P1-4/5/6/7、P2-10 是**通用诊断**；本报告的价值是把它们**落到资源库/设置页的具体文件行号**，并新增两页特有发现：P0-3 标题字号冲突、P0-2 符号当图标的具体清单、P1-5 在线点色觉问题、P2-8 分区标题不齐、P2-10 死代码清理。
- 既有报告未对 `SourcesScreen`（tab 本体）和设置页当前行式化版本做专项审查——本报告填补这一空白。
