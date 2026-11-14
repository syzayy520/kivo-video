# 274 · 设计体系审查归档（**本轮不实施 UI 改造**）

> ## ⚠️ 本文档已降级为「审查归档」，不是待实施方案
>
> **2026-11-14 用户裁决**：**不允许写 UI，UI 已经设计好了。**
> 本轮只做 **273 前助手未完成的任务** ＋ **最后的 HDR**。
>
> ⇒ **本文档 §一~§七 描述的所有 UI 改造（token 收口 / 组件层 / a11y / 视觉收口 / 响应式）一律不实施。**
> ⇒ 保留它的唯一目的：**把上一轮三份审查报告的结论固化下来**，作为"将来若要动 UI 时的口径参考"，
> 避免这些结论随会话丢失、下一轮助手重复调研。
> ⇒ **唯一仍然有效并要实施的是 §七 的一部分**：`274-hdr-rendering.md`（HDR 渲染），
> 以及 §六 中与**播放器运行时行为**相关、但**不改视觉**的条目（见该文档 §〇 的边界说明）。

状态：**归档**（结论已固化，UI 改造不实施）
来源：2026-11-14 全库设计审查（三份报告：播放器 / 详情页+设置页 / 首页+导航+色彩）
上游：272 批次一（纯缺陷修复，已收口）
本文档原负责定义整个 App 的视觉语言、色彩材质、信息层级、组件规范、排版栅格与响应式口径；
**因用户裁决而冻结为归档**，其中与 HDR 相关的部分转到
[274-hdr-rendering.md](274-hdr-rendering.md)，实施顺序转到
[274-design-system-plan.md](274-design-system-plan.md)。

---

## 〇、本方案的定位与三条前置约束

### 0.1 为什么需要这份文档

> **注**：以下实测数据保留，作为**将来若动 UI 时的现状基线**（免得重新调研）。
> 本轮不据此改任何 UI 代码。

实测当前状态（2026-11-14，全 275 个 `.kt`）：

| 事实 | 实测值 | 出处 |
| --- | --- | --- |
| 字号种类 | **20 种**（9/10/11/12/13/14/15/16/17/18/19/20/21/22/24/26/28/30/32/34） | 全仓 `fontSize = X.sp` |
| 裸 `dp` 字面量 | **890 处** | 全仓 `X.dp` |
| 网格配置 | **6 套**（Adaptive 128/160/240 + Fixed 2/3 + 字面量） | `GridCells.*` 全仓 |
| 海报宽度 | **6 种**（92/103/104/128/161/230 dp） | 各页自写 |
| token 体系 | **两套并存**（`DesignTokens` / `SettingsVisuals`），字阶与 hairline 定义冲突 | 见 §二 |
| `Role.` / `onClick` 语义 | **0 处** ⇒ TalkBack 无法激活任何播放控件 | 全 `ui/` grep |
| 响应式 | 仅 **2 处** `BoxWithConstraints`，无 `WindowSizeClass` | 全仓 grep |

⇒ 结论：**没有统一的设计语言，只有各页各自的方言。**
本方案的目标不是"再补一批设计"，而是**建立唯一真相源**，让后续每个组件都有唯一可查的规格。

### 0.2 约束一：设计定稿必须先冻结在途改动基线

**问题**：272 批次二~五（视觉/a11y/交互/工程化）与 273 性能线（v25/v26/v29/v31/273-B）
**改动范围大面积重叠**——`PlayerOverlay.kt`、`PlayerSeekTrack.kt`、`MainActivity.kt`、
`mpv_options.cpp` 都在两边。

**若不先冻结**：先做视觉定稿 → 再做性能收口 → 性能线的真机证据因 UI 变化而失效，
必须重跑 57 个实验目录里的一部分。这是纯粹的返工。

**因此本方案的执行前置**（硬闸门）：

| 步 | 动作 | 判据 |
| --- | --- | --- |
| G0-1 | 把 104 项未提交改动**按卡分组入库**（272 批次一 / 273 内核线 / 273 实验工具） | `git status --porcelain \| wc -l` = 0 |
| G0-2 | 冻结 273 性能线的**当前证据基线**（v25/v26/v29/v31 收口完成） | 273 规格 §二十一 前全部有真机数字 |
| G0-3 | 本方案定稿为**文档基线**（只改文档，不改代码） | 文档提交，代码零改动 |

> **未过 G0-1/G0-2 之前，本方案只作为"待实施设计"，不得进入代码实现。**

### 0.3 约束二：HDR 只在本方案定"架构与降级策略"，不定最终参数

**理由**：HDR 是**渲染管线**问题，不是 UI 问题。它会反向影响视觉语言的两个决策：

1. 暗色模式下，HDR 内容是否要额外压亮度（避免 OLED 亮部刺眼）；
2. 高饱和渐变（类型墙 24 张粉紫卡）在 HDR 下会不会溢出色域。

这两条必须等 HDR **真机实测**（K40 `mSupportedHdrTypes=[1,2,3,4]`、`mMaxLuminance=420`）
才知道答案。所以：

- **本方案**只定：HDR 的能力探测分层、色彩空间链路架构、性能降级策略、UI 侧必须预留的开关位；
- **具体参数**（`target-peak` 取值、tonemap 曲线、EGL 属性顺序）留到
  [274-hdr-rendering.md](274-hdr-rendering.md) 的**实现卡**，用真机 A/B 定。

### 0.4 约束三：组件规范之前，先做 token 收口

**理由**：不先定唯一真相源，所有组件规范都会继续分叉。
本方案 §二 给出收口路径，§六 的组件规范全部建立在收口后的 token 之上。

---

## 一、整体视觉风格与设计语言

### 1.1 一句话定义

> **「冷灰画布 + 纯白扁平卡 + 蓝色选中 + 单一强调色」——
> 一个安静的、像系统设置一样克制的影视浏览器。**

不是"炫技的播放器"，不是"杂志式排版"，不是"毛玻璃特效集"。

### 1.2 设计原则（5 条，每条都带可判定的后果）

| # | 原则 | 判定方式 | 违反示例 |
| --- | --- | --- | --- |
| P1 | **靠明度分层，不靠描边和投影** | 全局 `border =` / `shadow` grep = 0 | `SourceTile.kt` 的 `BorderStroke(.5.dp, 0xFFDFDFE6)` |
| P2 | **单一强调色**：蓝 `#0A84FF` 是全 App 唯一"选中/主行动"色 | 非蓝的 primary/选中色 = 0 | 聚合页 `#238A59` 绿、Emby 页同绿 |
| P3 | **内容优先于装饰**：任何装饰元素的面积不得超过它承载内容的面积 | 渐变/动画底不得在完全被遮挡时仍运行 | 发现页背后跑着被盖死的粉紫渐变 |
| P4 | **诚实优先**：没接入的功能显示"未接入"且不可点，不显示假值 | `enabled=false` 行不得显示具体值 | 设置页灰行显示"自适应/自动"等假当前值 |
| P5 | **零静默降级**：任何失败/不支持必须用户可见且带非空原因 | 错误文案不得靠 `contains` 嗅探 | 播放器的缓冲波纹是死代码 |

### 1.3 视觉语言的"三句否定"（沿用并强化 268-v3 裁决）

| 否定 | 由来 | 当前残留（待 272 批次二清零） |
| --- | --- | --- |
| **否粉紫脏底** | 268-v3 用户裁决 | `bgLavender/bgPink/bgCream` 三色渐变仍在 `DesignTokens` 且「在看」页跑着 |
| **否描边框** | 用户明确否掉 | `SourceTile` 0.5dp 描边 + 90% 半透白 |
| **否近白卡** | "统一用首页底色" | 多处 `White.copy(alpha=.9/.24)` 洗色 |

### 1.4 材质（Material）分级表

**只用三种材质，全 App 不再出现第四种**：

| 材质 | 定义 | 用在哪 | 规则 |
| --- | --- | --- | --- |
| **M1 画布** | `#F2F2F7` 纯色 | 页面底 | 不叠加任何渐变/纹理 |
| **M2 卡片** | `#FFFFFF` 纯色 + 圆角 20dp | 所有内容卡 | **无描边、无投影**；与 M1 只靠明度差（ΔL ≈ 5%）分层 |
| **M3 半透明层** | 黑 75% `0xBF000000` | **仅播放器内**（HUD/玻璃菜单/浮层） | 播放器外一律不用半透明卡 |

> **M3 的边界是硬规则**：播放器在暗场里需要半透明才能读；页面在亮底上用半透明会"发糊"。
> 这条边界解释了为什么 272 批次二要清零 `GlassSurface`（页面侧）而**保留** `playerGlassSurface`（播放侧）。

---

## 二、色彩与材质体系

### 2.1 token 收口：唯一真相源

**现状冲突**（实测）：

| 语义 | `DesignTokens` | `SettingsVisuals` | 冲突 |
| --- | --- | --- | --- |
| 行标题字号 | `fontRowTitle = 17f` | `fontRow = 16f` | **1sp 差**，同页"清理缓存"比其他行大 |
| 分隔线 | `hairline = 0x0D000000`（黑 5%） | `hairline = 0xFFE3E5EA` | 两套 |
| 副标题色 | `textSub = 0xFF8A8A8E` | `sub = 0xFF6B7280` | 两套 |

**收口决策**：

> **以 `DesignTokens` 为唯一真相源**（它在 `ui/design/` 根、被最多页面引用、是"设计稿实测参数"的原始出处）；
> `SettingsVisuals` **降级为"设置页专属语义层"**，只保留设置页独有的量（chip 尺寸、卡片圆角、域色 tint），
> 其余全部转发 `DesignTokens`。

### 2.2 目标色板（浅色）

```kotlin
// ── 画布与卡 ──
canvasFill     = #FFFFFF   // M2 卡片
pageBackground = #F2F2F7   // M1 画布
// ── 文本（4 级，全部 ≥4.5:1 on M1/M2）──
textPrimary    = #1A1A1A   // 标题/正文        对 M1 对比度 14.8:1 ✓
textSecondary  = #6B7280   // 副标题/说明      对 M1 对比度  4.8:1 ✓
textTertiary   = #9AA0A8   // 弱信息/时间戳    仅用于 ≥13sp 装饰位
textDisabled   = #B9BDC4   // 禁用态（纯色，不用 alpha 稀释）——见 §六 6.3
// ── 交互 ──
accent         = #0A84FF   // 唯一强调色：选中 / 主行动 / 链接
accentPressed  = #0064D8   // 按下（-20% 明度）
// ── 语义 ──
danger         = #FF3B30   // 破坏性操作
warning        = #FF9500   // 注意
success        = #34C759   // 仅 toggle/成功态，不作 primary
// ── 分隔 ──
hairline       = #E3E5EA   // 唯一分隔线色（4.3% 对比，装饰级）
// ── 领域色（仅 chip glyph 着色，禁作 primary/禁作大面积底）──
domainBlue/Orange/Purple/Indigo/Pink/Teal/Gray  #0A84FF/#FF9500/#AF52DE/#5856D6/#FF2D55/#30B0C7/#8E8E93
```

### 2.3 配色规则（可判定）

| 规则 | 内容 |
| --- | --- |
| C1 | **主色只有蓝**。`MaterialTheme.colorScheme.primary` 全局 = `accent`；聚合/Emby/私密区各自的绿 primary **全部删除** |
| C2 | **绿只做 toggle**。`success`/`accentGreen` 仅允许出现在开关的 on 态与"已保存"瞬时反馈 |
| C3 | **领域色只染 glyph**，面积 ≤ 30dp，且其底色是同色 14% tint（`chipTintAlpha = 0.14f`） |
| C4 | **占位色唯一**：`#E5E5EA`（中性）。当前有 5 套（`0xFFE9E4EE` 淡紫 / `0xFFE3DEEA` / `0xFFEFE9F2` / `0xFFE8E6EF` / `0xFFE0E0E0`）→ 全部收敛 |
| C5 | **评分徽章**：TMDB/豆瓣两色统一为 `accent` + 白字，识别靠文字不靠颜色（色盲可达） |
| C6 | **类型墙 24 张粉紫渐变卡**：本轮**明确豁免**（256 复刻规格授权），但必须① 改低饱和（峰值亮度 ≤ 65%）② 改深色字 ③ 在设计文档写死豁免理由，防止下轮被当残留清掉 |

### 2.4 深色模式：明确延后，但预留

**现状**：App **没有深色模式**。`themes.xml` 的 parent 是 `Theme.AppCompat.NoActionBar`，
而代码大量用 M3 组件（`Switch`/`ModalBottomSheet`/`AlertDialog`/`Snackbar`）——
**根主题与实际组件库不匹配**。设置页已如实标注"深色色板 v2 未实现，当前固定浅色"。

**本方案决策**：

| 项 | 决策 |
| --- | --- |
| 根主题 | **迁到 `Theme.Material3.DayNight.NoActionBar`**（本方案范围内，因为不迁就无法定义"浅/深"两套 token） |
| `values-night/` | **本轮只建骨架**（`colors.xml` 与 `themes.xml` 的 night 变体），**不迁移页面** |
| 设置页那行 | **保持现状**（诚实标注"深色色板 v2 未实现"）——不改成假的开关 |
| token 结构 | `DesignTokens` 改为**双层**：`DesignTokens`（语义接口）+ `DesignTokens.Light` / `DesignTokens.Dark`（值） |

> **理由**：迁根主题是低风险、纯收益（消除"用 AppCompat 却用 M3 组件"的隐患）；
> 而深色页面的**真机验收成本极高**（每个页面两套截图 + 对比度复算），
> 在主视觉线未收口前做深色，等于把 2 倍返工提前。**先建骨架，后填内容。**

---

## 三、界面布局与信息层级

### 3.1 三层信息架构（全局统一）

```
L0  背景层   M1 画布（#F2F2F7）
L1  内容层   M2 卡片（#FFFFFF）/ 媒体内容（图片/视频）
L2  控件层   导航胶囊 / 顶栏 / 底栏 / 浮层
```

**硬规则**：层级靠**明度**（L1 比 L0 亮）与**位置**（L2 浮于上层）表达，
**不靠阴影**。唯一允许的阴影是播放器浮层（M3 半透明层的可读性兜底）。

### 3.2 页面骨架（5 个一级页统一）

```
┌─────────────────────────────┐
│ [statusBar inset 区域]      │  ← 所有页顶部必须留出，不许内容穿过
├─────────────────────────────┤
│                             │
│   页面内容（LazyColumn）    │  ← contentPadding.top = statusBars
│                             │
├─────────────────────────────┤
│  导航胶囊（float）           │  ← navigationBarsPadding + 22dp
└─────────────────────────────┘
```

**当前违反**（实测，待 272 批次二清零）：

| 位置 | 问题 | 目标 |
| --- | --- | --- |
| `MainActivity.kt:264` | 显式传 `WindowInsets(0,0,0,0)`，发现页内容穿状态栏 | 移除显式 0，交给各页 `contentPadding(top = statusBars)` |
| `MainActivity.kt:323` | 导航胶囊无 `navigationBarsPadding()` | 补上（资源库页 `SourceWorkspace.kt:27` 已有，对齐它） |
| `MediaLibraryActivity.kt` | 顶/底浮钮全无 inset（`targetSdk=37` 强制 edge-to-edge） | 根 Box 加 `safeDrawingPadding()` |

### 3.3 信息层级：每屏只有一个主角

**规则**：一屏内 **主行动（CTA）有且只有一个**，且必须满足：

| # | 规则 | 判定 |
| --- | --- | --- |
| H1 | 每屏 ≥1 个 CTA，且视觉权重最高的可点元素就是它 | 首页 hero **当前无 CTA**（`HeroPage.kt` 全文件仅"封面加载失败点击重试"一处可点）→ 必须补 |
| H2 | CTA 位置：首屏可见（hero 区内或固定底部条） | 详情页 CTA 在 LazyColumn 第 1 项内，**滚走即消失** → 必须改常驻底部条 |
| H3 | CTA 文案与副标题必须**同源**（同一 episode 对象） | 详情页副标题报"第2季·第5集"、实际播第 1 季第 1 集（`EmbyResource.kt:118-121`）→ 真 bug，已在 272 批次一修 |
| H4 | 信息不重复：同一信息在一屏只出现一处 | 季集信息在详情页有 **4 种形态**重复（CTA 副标题/日历条/集数 chip/浮层头）→ 收敛为 1 处 |
| H5 | 关键决策信号前置：评分应在首屏 meta 行，不该排在正文之后 | 详情页评分在"简介"之后 → 前移 |

### 3.4 导航（底部胶囊）

**问题（实测）**：`MainActivity.kt:238` `selected = listOf(1,3,0,4).indexOf(page)`，
page 语义 `0=资源库/1=发现/2=在看/3=我的/4=设置` ⇒ `page==2`（在看）时 `indexOf(2) == -1`
⇒ **四个 tab 全灰**，用户完全不知道自己身处哪页。

**本方案决策**（沿用 272 批次一已实现的方案，此处记录理由）：

| 项 | 决策 | 理由 |
| --- | --- | --- |
| 「在看」定位 | **「发现」的子页**，不是第 5 个 tab | 胶囊 4 项时每项 ~64dp；加到 5 项会压到 <48dp（违反触区下限） |
| 选中态 | `page==2` 时胶囊高亮落在「发现」 | 表达归属关系 |
| 「在看」页 | 顶部加返回行 + 系统返回键 + 空态「去发现」按钮 | 解决"无返回路径"（272 批次一已实现） |

**胶囊本身的规范**（269 v3 已定，本方案只补两条）：

| 规范 | 值 | 理由 |
| --- | --- | --- |
| 选中态冗余信号 | **选中项加 2dp 顶部指示条**（除蓝字外） | 当前选中态**只有颜色一个信号**（`NavGlyph` 是单色 path，无填充差异）⇒ 色盲/低对比不可辨 |
| 「资源库」图标白点 | 改 `Color.Black.copy(alpha=.35f)` 或删除 | 白点画在 `#E8E9ED` 胶囊/白 pill 上 = 隐形 |

---

## 四、排版与间距栅格系统

### 4.1 字阶（收敛 20 → 9 级）

**目标字阶**（全部使用 `sp`，随系统字号缩放）：

| Token | 值 | 用途 | 字重 |
| --- | --- | --- | --- |
| `displayLarge` | 34sp | 详情页大标题 | Bold |
| `displayMedium` | 30sp | 设置页根页标题 | Bold |
| `titleLarge` | 22sp | 页面节标题 | SemiBold |
| `titleMedium` | 18sp | 卡标题 / 详情页节标题 | SemiBold |
| `titleSmall` | 17sp | 行标题（设置/表单） | Medium |
| `bodyLarge` | 16sp | 正文 | Regular |
| `bodyMedium` | 15sp | 海报标题 / 简介 | Regular |
| `bodySmall` | 13sp | 副标题 / 元信息 | Regular |
| `footnote` | 11sp | 时间戳 / 角标 | Regular |

**收敛规则**：

| 旧值 | 归入 | 说明 |
| --- | --- | --- |
| 19/21/26/28/32sp | 就近归入上表 | 全部删除 |
| 9/10/11sp | `footnote` 11sp | 10sp 归 11 |
| 12/13/14sp | `bodySmall` 13sp | 12/14 归 13 |
| 15/16sp | `bodyLarge` 16 / `bodyMedium` 15 | 15 保留、16 保留 |
| 17/18sp | `titleSmall` 17 / `titleMedium` 18 | 17 保留、18 归 18 |
| 22sp | `titleLarge` | 保留 |
| 34sp | `displayLarge` | 保留 |

> **注意**：13sp 是当前出现最多的字号（59 处）⇒ 定为 `bodySmall` 主力，是最省改动的一档。
> **执行方式**：先加 token，**不批量替换**；每个组件改造时按 §六 顺带归位。
> （890 处 dp / 20 种字号全量替换是高风险大改，收益低于风险。）

### 4.2 间距栅格（4dp 基）

| Token | 值 | 用途 |
| --- | --- | --- |
| `spaceXs` | 4dp | 图标与文字、chip 内边距 |
| `spaceSm` | 8dp | 元素内间距、同组元素 |
| `spaceMd` | 12dp | 卡片内容间距 |
| `spaceLg` | 16dp | 卡片间距、页面左右留白 |
| `spaceXl` | 24dp | 区块间距 |
| `spaceXxl` | 32dp | 大区块分隔 |

**页面左右留白统一 16dp**（当前 8/12/13/16/20/24 并存 ⇒ 取 `SettingsVisuals.hInset = 16f`，
`DesignTokens.screenPad = 13f` 作废）。

### 4.3 圆角

| Token | 值 | 用途 |
| --- | --- | --- |
| `radiusCard` | 20dp | 内容卡（设置页已用 20） |
| `radiusPoster` | 12dp | 海报卡（当前 10） |
| `radiusChip` | 9dp | chip（设置页已用 9） |
| `radiusPill` | 999dp | 胶囊 / 按钮（圆形用 50% 而非固定值） |

### 4.4 网格系统（收敛 6 套 → 2 套）

| 场景 | 规则 | 替代当前 |
| --- | --- | --- |
| **海报网格**（可滚动网格） | `GridCells.Adaptive(minSize = 128.dp)`，**不乘 fontScale** | `Adaptive(128)` / `Adaptive(160*fontScale)` / `Fixed(3)` |
| **类型墙**（固定 2:3 比例大卡） | `GridCells.Adaptive(240.dp)` | 现有 240 ✓ 保留 |

**海报宽度由列宽决定，不写死**——这是收敛 6 种海报宽度（92/103/104/128/161/230）的唯一办法。

> **`SourcesScreen.kt:69` 的 `160.dp * fontScale` 必须改**：fontScale=2.0 时最小卡宽 320dp
> ⇒ 手机上只剩 1 列，卡片巨大。正确做法是**字号用 sp 缩放、列宽不缩放**。

---

## 五、响应式与多分辨率适配

### 5.1 断点定义（引入 Material3 WindowSizeClass）

| 断点 | 宽度 | 布局形态 |
| --- | --- | --- |
| **Compact** | < 600dp | 单栏（当前全部页面） |
| **Medium** | 600–840dp | 单栏加宽 / 双栏可启动 |
| **Expanded** | ≥ 840dp | 双栏 master-detail |

**当前实现**：仅 `SourceWorkspace.kt:21` 一处 `maxWidth >= 960.dp` 判断（资源库页），
其余全仓 **0 处 `WindowSizeClass`**。

### 5.2 分辨率适配的三个具体动作

| # | 动作 | 现状问题 | 目标 |
| --- | --- | --- | --- |
| R1 | **hero 高度按宽度推导** | `HeroCarousel.kt:85` `HERO_HEIGHT_DP = 585`（= 390×1.5 写死）⇒ 360pt 机型海报被裁、412pt 机型比例失真 | `height = width × 1.5`，内部坐标用 `BoxScope` 的 fraction 而非绝对 dp |
| R2 | **网格列宽不乘 fontScale** | `SourcesScreen.kt:69` | 见 §4.4 |
| R3 | **详情页 hero 锚点改为内容推导** | `DetailHeroContent.kt:110` `DETAIL_TITLE_TOP_DP = 404.5f` 写死 ⇒ 超长标题会把 CTA 顶出首屏 | 改由内容实测推导；标题 `maxLines=2` + Ellipsis |

### 5.3 大字号（fontScale ≥ 1.3）降级

**规则**：

| 场景 | 规则 |
| --- | --- |
| 网格 | 列宽**不缩放**（R2）⇒ 大字号用户得到更多行而不是巨大卡片 |
| 平板双栏 | `SourceWorkspace.kt:21` 已有 `fontScale <= 1.3f` 条件 ⇒ 保持 |
| 固定高度容器 | 任何 `height(固定dp)` 且内含文字的容器，在 fontScale ≥ 1.5 时必须改 `heightIn(min=)` |

**审计点**：`DetailMorePopup.kt:82` 高度写死 `166dp`（= 3×50+2×8）⇒ 加一行就裁切 ⇒ 改 `IntrinsicSize`。

### 5.4 横屏 / 折叠屏

**当前**：全仓无横屏专项处理。播放器是唯一天然横屏的场景（非全屏播放时仍是竖屏容器）。

**本方案**：不新增横屏布局，但**所有自绘浮层**必须能收敛到屏宽内：
`DetailSeasonMenu.kt:182` 固定 `width(250.dp)`、`DetailMorePopup.kt:82` 固定 `width(210.dp)`，
x 只做 `coerceAtLeast` 未按 `constraints.maxWidth` 收敛 ⇒ 改 `widthIn(max = constraints.maxWidth - 32.dp)`。

---

## 六、组件与控件规范（含状态与交互反馈）

### 6.1 触控目标（硬下限 48dp）

**现状实测违规**（7 类）：

| 控件 | 当前 | 证据 |
| --- | --- | --- |
| 进度条命中区 | 24dp | `PlayerTokens.kt:149` |
| 玻璃菜单行 | 42dp | `PlayerTokens.kt:200` |
| stepper −/+ | 30dp 高 | `PlayerTokens.kt:223` |
| 开关 pill | 46×28dp | `PlayerStepperRow.kt:138` |
| 微调卡 ✕ | 38dp | `PlayerTokens.kt:218` |
| 底栏文字钮 | ≈27dp 高 | `PlayerBarText.kt:89` |
| 资源 chips | 30dp 高 | `PlayerResourcePanel.kt:90` |

**规范**：视觉尺寸可小于 48dp，**但命中区必须 ≥48dp**（`Modifier.minimumInteractiveComponentSize()`
或外层 `Box` 撑开），且**不得**用 `alpha` 缩小命中区。

### 6.2 无障碍语义（本方案最高优先级技术债）

**实测**：`Role.` / `stateDescription` / `liveRegion` / `heading()` 在全 `ui/` **0 命中**。
所有按钮都是手写 `pointerInput + detectTapGestures` ⇒ **语义树里没有 button 角色，
TalkBack 双击无法激活任何控件**（`PlayerTopBar.kt:171` 只有 `contentDescription`，只可朗读不可操作）。

**规范**：

| # | 规则 | 落地方式 |
| --- | --- | --- |
| A1 | 所有可点元素必须有 `role = Role.Button` | 封装 `KivoIconButton(onClick, label, stateDescription)` |
| A2 | 按钮必须有**可激发的语义动作** `onClick { … ; true }` | 保留 `pointerInput`（无涟漪视觉）但补 `semantics` |
| A3 | 开关/选中态必须有 `stateDescription` | "已开启"/"已关闭"、当前选中项名称 |
| A4 | 状态变化必须有 `liveRegion = LiveRegionMode.Polite` | 缓冲开始/失败/播放结束/写回失败 |
| A5 | 触摸目标 ≥48dp（见 §6.1） | — |
| A6 | 文本对比度 ≥4.5:1（正文）/ ≥3:1（≥18sp 或图形） | 播放页 `#8A8A8E` on 75% 黑 ≈ **3.1:1 不达标** ⇒ 提到 `#B4B4B8` |
| A7 | Canvas 自绘内容必须有语义镜像 | 日历网格整月是 Canvas（`DetailCalendarGrid.kt:84-124`）**屏幕上零 Text 节点** ⇒ TalkBack 读不出"6月15日 第1-4集" ⇒ 改为 `Text` 网格或补 `contentDescription` |
| A8 | 禁用态用**实色**不用 alpha | `alpha=0.4f` 灰字对比度塌陷 ⇒ 改 `textDisabled` 实色 |
| A9 | 被遮挡元素移出焦点序 | `PlayerPanelOcclusion.kt` 只做绘制裁剪，TalkBack 仍聚焦"视觉上被挖空"的按钮 ⇒ `clearAndSetSemantics {}` |

### 6.3 状态模型（四态必备）

每个异步区域必须有 **加载 / 成功 / 空 / 错误** 四态，且错误态必须带**可操作出口**：

| 状态 | 规范 | 当前违规 |
| --- | --- | --- |
| 加载 | spinner + 文案 + **可取消**（长任务） | 首页只有一行"正在加载推荐影片…"（`DiscoverPage.kt:120`），无 spinner、无取消 |
| 空 | 图标/占位 + **一句说明 + 一个行动按钮** | 「在看」空态纯文案无按钮；本地库空态纯文案；Emby 浏览页空态纯文案 |
| 错误 | **标题 + 原因 + 重试按钮**（按钮 ≥48dp） | 首页错误被丢到 LazyColumn **最后一项**（要滚到底才看得到）；「重试」是绿色纯文本、高 26dp |
| 成功 | — | — |

**错误必须就近呈现**：错误态位置应与触发它的区域同层（hero 槽位/顶部状态条），不是页尾。

### 6.4 反馈通道（每个动作至少一条）

| 动作类型 | 视觉 | 触觉 | 语义播报 |
| --- | --- | --- | --- |
| 播放/暂停 | 图标切换 | `CONFIRM` | `stateDescription` |
| 拖动进度 | 轨道跟随 + 时间浮窗 | 进入/离开各 `CLOCK_TICK` | 拖动中 `liveRegion` 报时间 |
| 双击快进/快退 | HUD 标签淡入 | `CONFIRM` | — |
| 面板行选中 | 高亮 | `CLOCK_TICK` | `stateDescription` |
| 长按快进开始/锁定 | 标签 + 锁定图标 | 已有 ✓ | — |
| 写盘失败 | Toast | — | `liveRegion` |

> **触觉现状**：全项目播放页仅 3 处 haptic（长按开始/快进锁定/锁屏）⇒ 拖动、双击、面板选中全无。

### 6.5 播放器专属规范（不适用通用组件）

| # | 规则 | 现状问题 |
| --- | --- | --- |
| P-1 | 暂停时控制条**保可见** | 四条链路都没有 `isPlaying` 判断 ⇒ 暂停 3 秒后控制条消失（与主流播放器相反） |
| P-2 | 缓冲时控制条保可见 + 中央波纹 | mpv 链路**完全没有 BUFFERING 状态**；波纹是死代码（`contains("缓冲")` 全项目 0 命中） |
| P-3 | 阶段用**显式枚举**，不用文案子串 | `PlayerComposeRoot.kt:68-69` 靠 `contains` ⇒ 272 批次一已修为 `PlayerPhase` ✓ |
| P-4 | 失败/结束态**继续轮询** | `PlayerSessionBridge.kt:119-128` 的 `return` 跳过了 `postDelayed` ⇒ 位置/时长冻结（272 批次一已修 ✓） |
| P-5 | 拖动取消必须复位 `scrubbing` | `onDragCancel` 不复位 ⇒ 弹幕永久消失（272 批次一已修 ✓） |
| P-6 | seek 灵敏度按片长分档 | 长片源 1pt ≈ 11.4 秒，一拖到底 ⇒ 分档（<10min 全宽 / <60min 半宽 / >60min 1/4 宽） |
| P-7 | 浮层面板**锚定触发按钮**，不遮挡它 | 倍速面板盖住自己的触发按钮，且 `PlayerPanelOcclusion` 把按钮文字**挖空** ⇒ 用户点不到也看不见 |
| P-8 | 面板返回走**栈**，不直写 `activePanel=""` | 6 处直写；钻入「字幕→内嵌字幕」按 BACK 直接退出整个面板 |
| P-9 | 长按快进不支持时**必须 toast 说明** | 软解/硬解链路长按被 `consume()` 吃掉但无任何反馈（违反 P5 不静默） |
| P-10 | 自动隐藏时长收口为 token | `3000ms` 硬编码 4 处 |

---

## 七、技术架构、模块划分与数据流

### 7.1 token 层（新增）

```
ui/design/tokens/                       ← 新目录（token 唯一真相源）
├─ DesignTokens.kt          语义接口 + Light/Dark 两套值
├─ DesignColors.kt          色板（§2.2）
├─ DesignType.kt            字阶（§4.1）
├─ DesignSpacing.kt         间距 + 圆角（§4.2/4.3）
├─ DesignGrid.kt            网格/海报宽度规则（§4.4）
├─ DesignBreakpoints.kt     断点 + WindowSizeClass 接入（§5.1）
└─ DesignMotion.kt          动效时长 + 曲线
```

**边界**：`ui/design/tokens/` 只放**常量**，不含任何 Composable、不含业务语义。
`SettingsVisuals` 降级为转发层（见 §2.1）。

### 7.2 组件层（新增/改造）

```
ui/design/components/                    ← 通用组件（页面级与播放级分开）
├─ KivoButton.kt            按钮（role/onClick/48dp/haptic）
├─ KivoIconButton.kt        图标钮
├─ KivoChip.kt              chip（领域色 tint 14%）
├─ KivoPosterCard.kt        统一海报卡（收敛 6 套 → 1）
├─ KivoSectionHeader.kt     节标题（收敛同名两份 → 1）
├─ KivoStateView.kt         加载/空/错误三态统一组件
├─ KivoImage.kt             图片（加载/失败/渐入三态）
└─ KivoRow.kt               行原语（收敛设置页 6 份 → 1）
```

**边界**：组件层**不含业务数据**（不碰 Emby/TMDB/SMB），只吃参数。
播放页的 `player/` 家族**保持独立**（暗场材质、玻璃、HUD 语义与页面层不同）。

### 7.3 数据流

```
数据源(Emby/TMDB/SMB/本地)
   ↓
Repository / EmbyResource          ← IO 与状态分离（当前 EmbyResource 在非 Compose 类里持有 mutableStateOf → 拆）
   ↓
StateHolder（按域聚合）            ← 当前 Activity 持 50 个独立 mutableStateOf → 聚合成 3 个 holder
   ↓
UiState（不可变 data class）
   ↓
Composable（只读 state + 调 actions）
```

**本方案只定"状态该在哪"，不改具体代码**（属 272 批次四/五的实施范围）。

### 7.4 a11y 层

```
ui/design/a11y/
├─ KivoSemantics.kt          统一语义封装（role/stateDescription/liveRegion）
└─ KivoTouchTarget.kt         48dp 命中区扩展 Modifier
```

### 7.5 实施顺序（**已作废 —— 本轮不实施 UI 改造**）

> **2026-11-14 用户裁决：不允许写 UI。**
> 下列 S1–S6 全部**不实施**，保留仅为记录"如果将来要动 UI，顺序应该是这样"。
> 本轮实际执行顺序见 [274-design-system-plan.md](274-design-system-plan.md) §三。

```
S0  冻结基线（入库 104 项 + 273 收口）        ← G0 闸门
S1  token 层落地（§2/§4）+ 双向别名          ← 不改视觉，只加真相源
S2  组件层新建（KivoButton/Chip/PosterCard/StateView…）
S3  播放器页 a11y + 交互规范（P-1~P-10）
S4  详情页 + 设置页 a11y + 组件替换
S5  首页/导航/色彩收口（渐变清零、占位色统一、网格统一）
S6  响应式（R1~R3 + 断点）
S7  HDR 实现卡（[274-hdr-rendering.md](274-hdr-rendering.md)）  ← 本轮唯一实施的 UI 相关项
```

#### 7.5.1 例外：S3 中"不改视觉"的部分仍可能需要

用户裁决是"**不允许写 UI**"，指的是**视觉与交互改造**。
以下三条属**运行时行为缺陷**，与"UI 已设计好"不冲突，**若在 273 收口时被真机测到，则按缺陷修**：

| 条目 | 性质 | 判据 |
| --- | --- | --- |
| P-2 缓冲时控制条保可见 + 中央波纹 | 状态缺失（波纹是死代码） | 真机测到卡缓冲时画面无任何提示 |
| P-4 失败/结束态继续轮询 | 状态冻结（位置/时长停住） | 真机测到失败后进度条不动 |
| P-5 拖动取消复位 `scrubbing` | 状态不复位（弹幕永久消失） | 真机测到弹幕不回来 |

> 注：P-2 / P-4 / P-5 **已在 272 批次一修掉**（`PlayerPhase` 枚举 + `postDelayed` 移入 `finally` + `onScrubEnd`）。
> 此处保留是作为**回归检查项**，不是待办。

---

## 八、验收判据

### 8.1 静态

- [ ] `ui/design/tokens/` 存在，`SettingsVisuals` 不再重复定义字阶与色
- [ ] 全仓字号种类 ≤ 10（当前 20）
- [ ] `GridCells.*` 全仓 ≤ 2 种（当前 6）
- [ ] 全仓 `border =` / `shadow` 仅出现在播放器浮层
- [ ] `MaterialTheme.colorScheme.primary` 全局唯一（无页面级绿 primary）
- [ ] 全 `ui/` `Role.` / `onClick` 语义命中 > 0（当前 0）
- [ ] 所有 `ui/` 下 `.kt` ≤ 300 行（当前 `DiscoverPage.kt` 398 / `MediaLibraryActivity.kt` 334 超限）

### 8.2 真机（K40 `b6e950ee`）

| # | 场景 | 期望 |
| --- | --- | --- |
| 1 | TalkBack 遍历播放页 | 每个控件可朗读**且可双击激活** |
| 2 | 360×640 与 412×915 两档 | hero 构图不失真、CTA 在首屏可见 |
| 3 | fontScale=1.5 | 网格不塌成单列、浮层不裁切 |
| 4 | 首页断网 | 错误**在首屏**可见、重试按钮 ≥48dp |
| 5 | 五个一级页 | 胶囊选中态唯一且有形状冗余、底部不陷入手势区 |
| 6 | 详情页滚到底 | CTA 常驻可见、返回键先关浮层 |

### 8.3 诚实披露（必填）

未达成项必须写"未达成 + 原因 + 差距"，不得写"基本达成"。
模拟器数字无效；对比度、坐标、帧率必须真机像素实测。
