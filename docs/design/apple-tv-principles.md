# Kivo Play · Apple TV（tvOS）设计原则（第二设计基准）

> 2026-11-17 ｜ 用途：为 Kivo 的**大屏 / 遥控 / 剧场化**场景提炼 tvOS 设计原则，并给出它与 REX 冲突时的裁决规则。
> 本文**只做分析，不改任何生产代码**（符合审计阶段只交付文档的约定）。
> 全文原则必须能落到一个具体页面或控件；落不到的原则一律不收。

---

## 0. 地位与效力

### 0.1 两个基准的优先级

| 基准 | 覆盖范围 | 效力 |
| --- | --- | --- |
| **第一基准：REX** | 手机竖屏信息架构、播放器 overlay 的逐像素几何（轨道 6pt / 按钮 pitch 65 / 面板 248-16-42-60 / 锁定矩阵 / HUD 150-1000-300） | 已验收部分**只增不改**（`docs/design/originality-matrix.md:30`：「结构与像素不必再改，只修 bug」） |
| **第二基准：Apple TV** | 大屏观看距离、焦点模型、遥控步数、内容优先、动效克制 | **只在 REX 无确定项、或 REX 项被本仓审计判为缺陷时生效**；不得用来推翻已验收像素 |

### 0.2 裁决总则（冲突时按 R1→R4 依次判）

| 规则 | 条件 | 处置 |
| --- | --- | --- |
| **R1** | REX 有实测确定项（截图可数 + 代码已有 token） | **保留 REX**，Apple 原则只作解释性注脚 |
| **R2** | REX 无此项，或该项在 `docs/audit/*-parity-audit.md` 已被判 P0/P1 缺陷 | **吸收 Apple**（因为它补的是 REX 的空白/错误，不是风格偏好） |
| **R3** | 两者都无（新增能力：电视端、遥控、外接屏、10-foot） | **吸收 Apple**，并按 §3 映射到 Android |
| **R4** | 两者都有且互相矛盾，但 REX 项**已验收** | **融合**：保留 REX 的几何与密度，只吸收 Apple 的"关系"（层级/时序/退场），不改尺寸 |

> 一句话：**Apple 管"为什么这样看"，REX 管"具体多宽多高"。** 二者交叠处，尺寸归 REX。

### 0.3 证据（本次已取到官方原文，非二手转述）

抓取日期 2026-11-17，以下四篇均为 Apple 官方 HIG 的 Markdown 版原文：

| # | 文档 | URL | 本文用到的关键原文（保留英文，便于复算） |
| --- | --- | --- | --- |
| E1 | Designing for tvOS | `developer.apple.com/design/human-interface-guidelines/designing-for-tvos.md` | "people generally remain many feet away … often 8 feet or more"；"Embrace the tvOS focus system, letting it gently highlight and expand onscreen items"；"Deliver beautiful, edge-to-edge artwork, subtle and fluid animations … legible … from across the room" |
| E2 | Focus and selection | `…/focus-and-selection.md` | "Focus helps people visually confirm the object that their interaction targets"；"tvOS generally uses the parallax effect"；"Avoid changing focus without people's interaction"；"Avoid displaying a pointer"；"In a full-screen experience, let people use gestures to interact with the content, not to move focus"；五态表（unfocused / focused / highlighted / selected / unavailable） |
| E3 | Playing video | `…/playing-video.md` | "your actions don't take more than a step or two and your content is succinct"；"restrict this content so that it doesn't obscure media playback"；"implement a minimum delay of 0.5 seconds to pause playing media, and display an interactive overlay"；"Avoid asking people if they want to resume playback"；"loading takes more than two seconds → black loading screen with a centered activity spinner" |
| E4 | Layout（tvOS 段） | `…/layout.md` | "Inset primary content 60 points from the top and bottom of the screen, and 80 points from the sides"；"Include appropriate padding between focusable elements"；"Minimum vertical spacing 100 pt / Horizontal spacing 40 pt"（网格）；"Use consistent spacing"；"Use progressive disclosure" |
| E5 | Typography | `…/typography.md` | tvOS 字号：**Default 29 pt / Minimum 23 pt**；tvOS text styles：Body 29/36、Caption 2 23/30、Title 1 76/96 |

> ⚠️ **未取到官方原文的一项**：文字对比度阈值（4.5:1 / 3:1）。该数值来自 WCAG 2.1（AA），**不是本次抓到的 HIG 原文**，本文凡引用处均标注「WCAG 2.1」。

### 0.4 使用边界（硬约束）

1. **禁止把 Apple 的 pt 值当 Kivo 规格**。60/80、29pt、40/100pt 全部是"关系与下限"，不是 dp。
2. 换算口径（唯一允许的用法）：`Kivo 值 = Apple 值 × (目标场景视距系数)`；
   - 手机竖屏（≈35cm）：**不适用**，沿用 REX 实测帧；
   - 横屏播放（≈50cm 手持）：只取「最小字号比例」；
   - 客厅电视 / 投屏（≥2.4m）：取 Apple 比例（29/23 → 标题约为正文 2.6 倍）。
3. 任何吸收项落地前必须给出「目标页面 + 目标文件 + 验收判据」三件套，缺一不进批次。

---

## 1. 可吸收原则（12 条）

> 每条格式：**一句话** → HIG 依据 → Kivo 落地（页面/控件/文件）→ **不做什么**（防过度吸收）。

### P1 空间与留白：大屏的第一件事是"别把内容靠到边"（E4）

- **原则**：内容必须落在安全区以内，且安全区是**下限**，不是目标值。
- **落地**：横屏播放器的状态行/顶栏图标行与底栏，全部以 `ShellInsets` + `PlayerTokens.bottomPad` 起算（`ui/shell/ShellInsets.kt`、`ui/compose/player/PlayerTokens.kt`）；将来做投屏/电视布局时新增 `TvInsets`（上下 10% 屏高、左右 7.5% 屏宽），不复用手机 13dp 页边。
- **不做什么**：不把 60/80pt 直接写进手机布局；不带屏幕密度参数的安全区常量禁止入 tokens。

### P2 层级与景深：焦点靠"放大 + 投影 + 光"三件事同时表达（E2 parallax）

- **原则**：层级不靠描边，靠"离观众更近"的物理暗示（放大 + 阴影加深 + 轻微提亮）。
- **落地**：
  - 首页卡片按压已有 `scale 0.98 + 180ms`（`docs/design/visual-polish-spec.md:19`）——那是"按下去"；**焦点态**另设 `scale 1.05 + elevation 2→6dp + 白 6% 提亮`，只用在遥控/键盘可达的组件上（`ui/components/MediaCard.kt`）。
  - 焦点放大必须落在预留 padding 内：卡片间距当前 10dp（`DesignTokens.gap`），5% 放大在一张 92dp 海报上约 +4.6dp，**两侧合计 9.2dp < 10dp**，刚好不重叠；因此**放大上限锁 1.05**，超过就必须同时加 gutter。
- **不做什么**：不做视差倾斜（parallax tilt）——需要陀螺仪且在高刷屏上易晕，收益与风险不成比例。

### P3 内容优先、控件退场：播放中控件是"临时访客"（E3 "restrict this content so that it doesn't obscure media playback"）

- **原则**：视频永远在最高视觉优先级；控件出现必须有时限、退场必须比入场快。
- **落地**：`PlayerBarsHost` 的三秒自动隐退（`ui/compose/player/PlayerBarsHost.kt`）保持；顶栏**继续无遮罩**（`PlayerTopBar.kt` 无 background，审计 `TOP-02` 判定 ✅ 对齐 REX `IMG_0160/0170`）。控件隐藏后**不保留**任何常驻元素（现 `PlayerStatusLayer` 三点循环是例外，见 §1-P12 与 MOT-02 待修项）。
- **不做什么**：不为"看起来高级"给顶栏加黑色渐变带——REX 五张截图证据显示竞品顶部无黑带，且 v2 曾踩过。

### P4 Focus 语义：五态必须是五个可区分的视觉，不是五种临时样式（E2 五态表）

- **原则**：`unfocused / focused / highlighted(按下瞬间) / selected(持久) / unavailable(不可用)` 五态在**同一屏上互相可辨**。
- **落地**（映射到 Compose 焦点 API，详见 §3.1）：

| tvOS 态 | Kivo 语义 | 视觉 | 现有落点 |
| --- | --- | --- | --- |
| unfocused | 普通 | 现状 | 全部页面 |
| focused | 遥控/键盘选中 | `scale 1.05 + elevation 6 + 白 6%` | 新增 |
| highlighted | 按下瞬间 | 现有 `scale 0.98` + 本体变色（无涟漪） | 已有（`TopIconButton` 用 `detectTapGestures`） |
| selected | 持久选中 | 白胶囊 / 白勾 / `accentGreen` | 已有（`CapsuleNavBar`、`WeekStrip`） |
| unavailable | 能力缺失 | `alpha 0.4` + **非空 reason 副行** | 已有（`PlayerPanelForms.disabled()`） |

- **不做什么**：焦点**不得**自动跳变——`E2`: "Avoid changing focus without people's interaction"。唯一例外是焦点项消失时移到"一步之内"的邻项（同条款后半句）。

### P5 10-foot 可读性：大屏的正文是"手机标题"那么大（E5）

- **原则**：观看距离每翻一倍，最小字号约 ×1.6–1.8；tvOS 的下限是 **23pt**、默认 **29pt**。
- **落地**（仅在 ≥2m 场景启用，写进 `TvTypeScale`）：
  - 正文/卡片标题 ≥ 29 等效（`DesignTokens.fontPosterTitle = 15` 在手机成立，在电视上须 ≥ 29 等效）；
  - 副标题 ≥ 23 等效；
  - 字重不低于 Medium（`E5`: "avoid light font weights"）；
  - 字号层级仍只有三层（`docs/design/visual-polish-spec.md:11`），不因大屏加第四层。
- **不做什么**：不在手机上把 15sp 改成 29sp；不用 `autoSize` 自适应把字号变成不可预期。

### P6 遥控操作距离：到播放不超过 2 步（E3 "no more than a step or two"）

- **原则**：D-pad 下"从一个屏幕到开始播放"的按键次数是硬指标。
- **落地**（可量化、可验收）：启动 → 播放 ≤ **6 次方向键 + 1 次确认**；播放中任意面板 → 回到画面 ≤ **1 次返回**。当前页面结构下初判：首页 hero「整卡可点」（`HeroCarousel`）直接满足；播放器面板点外部关闭（`PlayerOverlay.kt:144-150`）满足。
- **不做什么**：不做需要"长按 + 二次确认"才能播放的流程。

### P7 页面转场连续性：转场是"同一个空间的移动"，不是"两个页面的替换"（E4 progressive disclosure / E3 exiting playback）

- **原则**：入/出方向相反、时长一致、背景不闪白；返回必须回到**上一个眼神落点**（列表滚动位置、当前选中项）。
- **落地**：
  - 现有设置页 `slide 350ms EaseOut + fade 350ms`（`ui/design/settings/SettingsNav.kt:37-46`）保留为「推入/弹出」标准；
  - 播放器退出按 `E3` 「显示与刚看内容相关的详情视图」——当前实现是从播放返回来源页，需核对是否回到该片详情（`ui/detail/DetailActivity.kt`，本轮未验，标 ⚠️ 待真机确认）；
  - 大屏下把"推入"改为"同层淡入 + 焦点落点保持"，避免整屏平移在大屏幕上过度运动。
- **不做什么**：不做"翻页/3D 转场"；转场期间禁止加载网络图（避免中途重排）。

### P8 背景处理：内容可以延伸到控件之下，但**文字不能**（E4 "extend it underneath sidebars, toolbars, and tab bars"）

- **原则**：控件背景可以让内容透过来；控件的**文字**必须有稳定对比度。
- **落地**：
  - 竖屏已按此做：页面底为 `pageBackground + bgLavender` 渐变（`style-unification.md:10`）；
  - 播放器面板背景现为黑 75% 实色（`PlayerGlassSurface.kt`，审计 PNL-09 判"亮画面上可读性下降"）→ 按 R2 **吸收**：面板底提到黑 85%（`panelSurface = 0xD9000000`），不做真模糊；
  - 导航胶囊保持 92% 白 + 2dp 阴影（`visual-polish-spec.md:17`），**不改成高透玻璃**——审计 `b1-verification.md:145` 已判 D-1 为 P2，改为高透会让 10dp 小图标在亮图上失读。
- **不做什么**：不做"玻璃叠玻璃"（面板内再套 GlassSurface）。

### P9 减少视觉噪音：一屏只有一个视觉焦点（E1 "cinematic experience that's clear, legible"）

- **原则**：同屏最多一个"最亮/最大/最饱和"元素；强调色只用于**交互控件与能力**，不用于装饰。
- **落地**：
  - 发现页 hero 是全屏唯一大图焦点，下方横滑区只允许 1 个 `accentGreen`（进度条或徽章）；
  - 播放器同屏只允许 1 个 `activeAccent`（现 `PlayerTokens.activeAccent` 被判"绿色激活与 REX 克制体系冲突"，`TOP-07`）→ 按 R2 收敛为仅"听视频白圆 + 源卡绿点"。
- **不做什么**：不给每个 section 标题配彩色装饰线/emoji；不做背景粒子。

### P10 可访问性·对比度：文字 4.5:1、非文字 3:1（WCAG 2.1 AA，非本次 HIG 原文）

- **原则**：对比度是**下限**，不是"看起来够"。
- **落地**：
  - `textSub` 在白卡上必须 ≥ 4.5:1；播放器半透明白字必须叠在 ≥ 60% 黑底或画面亮度受控（`PlayerBottomBar` 三段渐变 `0x00→0xC8` 已有，需实测亮场景）；
  - 非文字（轨道、勾、焦点框）≥ 3:1：`trackRemaining = 0x1F`（白 12%）在亮画面不达标（审计 `BOT-05`）→ 给轨道加黑 20% 底衬。
- **不做什么**：不用"加阴影"去代替提高对比度（`E5` 同结论：不要靠阴影提对比）。

### P11 可访问性·动态字体：字号变大时布局要重排，不是裁切（E5 "Make sure your app's layout adapts to all font sizes"）

- **原则**：字号放大 → 容器长高、横排改竖排；**禁止** ellipsis 吞掉关键信息。
- **落地**：设置页行高 `rowHeight = 59`（`DesignTokens.kt:53`）需随 `fontScale` 用 `heightIn(min=)` 而非固定 `height`；玩家面板行高 42/60 同理（`PlayerTokens.panelRowHeight/panelRowTallHeight`）。验收：系统字号 1.3× 下，设置页与详情页无截断（真机一项）。
- **不做什么**：不做"全局固定 dp 缩放"——那等于关掉无障碍。

### P12 可访问性·减弱动效：门在，降级必须覆盖到底（E2 "Avoid changing focus without people's interaction" + 工程铁律）

- **原则**：减弱动效时，**位移/缩放/旋转归零，只保留透明度**；开关必须在设置页可点（不能是灰行）。
- **落地**：复用现成门 `ui/motion/MotionGate.kt`：
  - `MotionGate.rememberAppReduceMotion()` 已合并「App 开关 ∨ 系统 animator scale」（`MotionGate.kt:42-64`）；
  - 待补的降级点（审计 MOT-02）：`PlayerLongPressHud` 的 `scaleIn 0.9`、`PlayerSeekPreview` 的 `scaleIn 0.92`、`PlayerStatusLayer` 的三点循环；
  - 大屏新增的焦点放大动效**必须**读同一个门（`reduceMotion → scale 恒 1.0`）。
- **不做什么**：不做"减弱动效 = 全部动画 0ms"（那会让状态切换失去可感知反馈）；保留 α 淡入淡出 150ms 是合规下限。

### P13 渐进披露：详情页只给"要不要看"，其余收进折叠（E4 "Use progressive disclosure"）

- **原则**：一屏只回答一个问题；次要信息用折叠/钻入。
- **落地**：详情页首屏 = hero + 播放/收藏 + 元信息一行 + 技术徽章一行；「文件信息 / 演职员 / 同目录」下沉（`ui/detail/DetailInfoCards.kt` 已有文件信息卡，扩为可折叠）；发现页 6 模块**不再增加**到 8 个。
- **不做什么**：不把"折叠"做成默认收起关键信息（播放按钮永远不收）。

---

## 2. 与 REX 的冲突裁决表

> 列含义：**REX 做法**（带审计编号，便于回溯）｜**Apple TV 原则**｜**裁决**｜**理由**｜**落地边界**（谁不许动）。

| # | 场景 | REX 做法 | Apple TV 原则 | 裁决 | 理由 | 落地边界 |
| --- | --- | --- | --- | --- | --- | --- |
| A1 | **顶栏遮罩**：控件区是否加黑带 | **无遮罩**，白图标直压画面（`IMG_0160/0168/0170`；审计 TOP-02 ✅） | E3「不许遮挡媒体」+ E1「edge-to-edge artwork」 | **保留 REX** | 两者结论一致（Apple 也要求控件不压内容），REX 用"纯白高对比图标"已达成目的 | 谁都不许加 `topScrim` 渐变；`PlayerTokens.topScrimTop/Bottom` 应删或显式 0%（审计 TOK-01） |
| A2 | **底栏透明度** | 三段半透明 + 1dp 提亮描边 + 黑渐变 200/210 高（审计 BOT-01/02 ✅） | E4「内容可延展到控件下」 | **保留 REX** | 已是"内容透出 + 文字可读"的平衡，且几何是实测值 | 只在**选集态**补压暗（BOT-02），不改三段结构 |
| A3 | **面板位置**：面板从哪冒出来 | 从**触发控件**展开（顶栏图标下方 / 底栏按钮上方），`IMG_0165/0171/0173` | E2「焦点跟随操作对象」+ E3「回到观看不超过一二步」 | **吸收 Apple**（本质是修复 REX 未落实的部分） | 审计 PNL-01 判 P0：现状统一右上角，视线跨屏。Apple 原则与 REX 契约**同向**，冲突只是"实现没做到" | 面板宽度 248 / 圆角 16 / 行高 42-60 **不动**（R1）；只改锚点计算 |
| A4 | **面板是否需要遮罩（scrim）** | 无遮罩，点外部关闭（`PlayerOverlay.kt:144-150`） | E3「内容不被遮挡」；Apple 视频播放器浮层亦不压暗全屏 | **保留 REX** | 全屏压暗会让"调字幕时想看一眼画面"变难 | 禁止为面板加全屏 scrim；只允许面板自身底 85% 黑（见 P8） |
| A5 | **字幕样式**：字幕是否允许用户改到"很花" | 面板给 字号/垂直位置/描边/延迟 四项 + 「标准统一样式」开关（`sub-ass-override`，`PlayerPanelData.kt:96-119`）；样式由 libass 合成进帧 | E3「内容优先，附加信息不得遮挡播放」+ E5「避免细字重」 | **融合** | 保住 REX 的四项微调（那是实测契约），但把"标准统一样式"**默认开**，并在开启时强制最小描边 ≥1 + 字号下限（23 等效 × 屏幕系数） | 默认值变更需与 REX 面板行文案一致；「关闭后保留 ASS 源特效」的既有说明不改写 |
| A6 | **进度条形态**：拖拽时要不要变粗 / 加滑块 | 6pt 恒定、**无滑块**、半透明白三段（`PlayerTokens.trackHeight=6`，五张截图一致）；kivo 自创"按下 6→10"（`TOK-07`） | Apple 内置播放器有 scrubber；但 E2「不要指针」指向"不要自由指针"，不是"必须有滑块" | **保留 REX**（6pt + 无滑块）；**吸收** Apple 的**预览**语义 | REX 实测是最强证据；Apple 的可借鉴点是"拖动前先给缩略帧预览"，kivo 已有 `PlayerSeekPreview`，只需把 `tap=立即跳` 改为"点=预览定位、拖=提交"（审计 BOT-04） | `trackHeight/rim/三段色` 不动；`trackActiveHeight=10` 降为可选开关并标"自创" |
| A7 | **顶部大面积标题** | 播放器顶部只有片名（12–13pt 级）+ 状态行，无大标题 | tvOS Title 1 = 76pt，剧场式大标题 | **不吸收** | 手机/横屏播放器没有"8 英尺观看"的视距前提；大标题会压掉画面 | 大标题只允许出现在**电视布局**（`TvTypeScale`）；手机不加 |
| A8 | **返回/转场动效** | 设置页 `slide+fade 350ms EaseOut`（`SettingsNav.kt:37-46`）；页面级 `pageTransitionMs = 300`（`MotionTokens.kt:27`） | E1「subtle and fluid」；Apple 无规定时长，原则是"快于观感阈值、方向可解释" | **融合** | 保留 REX 时长（已验收），但补 Apple 的"方向与落点"：返回必须回滚到原滚动位置与选中项 | 时长不改；新增 `rememberScrollState` 恢复项 |
| A9 | **按压反馈** | 本体变色、无涟漪、`alpha 0.6`（`TopIconButton.kt:172-186` ✅） | E2「highlighted 态即时反馈，可短暂反色」 | **保留 REX** | REX 的"无涟漪"正是 Apple 描述的 highlighted 态 | 不引入 Material ripple |
| A10 | **GlassSurface（毛玻璃）** | 半透明白 + 1dp 白 8% 描边，无真模糊（`GlassSurface.kt:16-41` 自注取舍） | E4 提到 Liquid Glass / 背景延展模糊 | **融合（降级优先）** | API 31 前无稳定模糊；Apple 允许"降级材质"，但要求控件与内容可区分 | `GlassSurface` 加可选 `blurRadius` 参数（仅 API31+ 生效，否则走现路径）；禁止在视频层上叠模糊面板（性能/发热） |
| A11 | **强调色/激活色** | 播放器激活态闪 1.5s 绿（`TOP-07` 判"与克制体系冲突"） | E2「focus 用系统效果，自定焦点效果非必要不要做」 | **吸收 Apple**（收敛） | 绿点=成功提示，不是焦点/激活；混用会让"选中"失去唯一语义 | `activeAccent` 仅留"听视频白圆 + 源卡绿点"；其余激活用白/α |
| A12 | **不可用控件** | `disabled()` + 非空 reason 副行（`PlayerPanelForms.kt:120-132`，铁律） | E2 `unavailable` 态：更暗、无阴影、对比低 | **保留 REX**（并加成 Apple 的形态） | kivo 的"非空 reason"比 Apple 更强（可解释 > 仅变暗） | 视觉上补 `alpha 0.4`（已有 `ink=0.4f`），**不删** reason |
| A13 | **自动续播询问** | 续播直接起播（无询问） | E3「Avoid asking people if they want to resume」 | **保留 REX**（同向） | 一致，无需动作 | — |
| A14 | **加载态** | 起播无 spinner 回退，仅三点波纹（`MOT-03` 判缺回退态） | E3「超过 2 秒才显示黑底 + 居中 spinner，且只显示到能播为止」 | **吸收 Apple** | 当前"无限三点循环"同时违反 E3（时长门槛）与性能预算 | 门槛 2s 入 `PlayerMotion`；`startupAnimation=false` → `CircularProgressIndicator` 24dp 白 |
| A15 | **一屏卡片密度** | 发现页 5 列海报 / 2 列大卡（`b3-verification.md` 对齐 REX 0330–0335） | tvOS 网格 2–9 列，水平 40pt、垂直 ≥100pt | **保留 REX** | REX 列数是手机视距下的最优解；Apple 的列数是 TV 视距结论 | 5 列 = 手机；电视布局另算（对应 tvOS 5 列 ≈ 320pt 内容宽） |
| A16 | **指针/鼠标** | 无指针 | E2「不要显示指针」 | **保留 REX**（同向） | 一致 | 大屏若接鼠标，只做"高可见光标"，不做自由拖拽 |

**裁决分布（16 行全计）**：保留 REX **9** 项（A1/A2/A4/A6/A9/A12/A13/A15/A16）、吸收 Apple **3** 项（A3/A11/A14）、融合 **3** 项（A5/A8/A10）、明确不吸收 **1** 项（A7 大标题）。
**含义**：Apple TV 在这个项目里主要补的是**"焦点/步数/加载门槛/面板语义"**，不是重做视觉。

---

## 3. Android 落地映射

> 原则只有落到 API 才算数。以下全部指向本仓已有文件，未引入的技术明确标注「新增依赖」。

### 3.1 Focus（D-pad / 遥控）→ Compose 焦点系统

| tvOS 概念 | Android/Compose 对应 | 落地位置 |
| --- | --- | --- |
| directional focus（方向焦点） | `Modifier.focusable()` + `onKeyEvent { Key.DirectionCenter / DirectionUp… }` | 所有卡片：`ui/components/MediaCard.kt`、`CapsuleNavBar` |
| focusable item 必须可放大 | `Modifier.onFocusChanged { it.isFocused }` → `animateFloatAsState(if (focused) 1.05f else 1f)` + `shadow(6dp)` | 同上 |
| 五态 | `unfocused / focused / pressed(interactionSource) / selected(业务状态) / unavailable(enabled=false)` | 五态必须**同时**存在；不可用态保留 reason 副行 |
| 初始焦点 | `FocusRequester` + `LaunchedEffect { requester.requestFocus() }` | 首页首张 hero（`HeroCarousel`）、播放器面板首行 |
| 焦点不自动跳变 | 仅在"焦点项消失"时移向一步内邻项；`focusRestorer` 需要时自写 | `PlayerPanelHost`（面板切换时） |
| 遥控"返回" | `onKeyEvent(Key.Back)` 优先关面板，再退播放 | `PlayerComposeActivity` 已处理部分，需核 |

**依赖说明**：`androidx.tv:tv-material`（TV 版 Material 组件）**当前工程未引入**。建议先不引入——本仓 UI 是自绘 Compose，引入 tv-material 会带来第二套组件语言（违反单一设计源）。只在「真做电视端」时评估，且**不得**用它替换现有 `GlassSurface`/`MediaCard`。

### 3.2 景深 → `elevation + scale + 提亮`

```
focused:  scale 1.05 (上限 1.05，见 P2) + shadow(6dp) + 白 6% 覆盖
pressed:  scale 0.98 + 180ms（沿用 visual-polish-spec.md:19）
selected: 白胶囊 / 白勾（沿用既有）
reduce:   MotionGate.reduce() → scale 恒 1.0，只改 elevation（瞬时）
```
判定要点：**scale 只是"最近"，elevation 才是"离桌面多高"**；两者同时动才像"浮起来"，只放大显得廉价。

### 3.3 毛玻璃 → 半透明 + 模糊近似（分级）

| 级别 | 条件 | 实现 |
| --- | --- | --- |
| L0（默认） | API < 31 或性能档低 | `GlassSurface(alpha = 0.70f)` + 1dp 白 8% 描边（现状，`GlassSurface.kt:22-37`） |
| L1 | API ≥ 31 且非播放中 | `Modifier.blur(radius = 24.dp)` 作用于**背景层**（不是面板自身），面板仍用半透明 |
| L2 | 播放中 | **禁用模糊**（视频层模糊需 RenderEffect 桥接，成本高于收益；且 `PNL-09` 已判"不划算"） |

验收：同一张亮画面（白墙/雪景）截图上，面板文字与背景边界可辨；否则升到黑 85%。

### 3.4 减弱动效 → `MotionGate`（已存在，只需接线）

```
现有：MotionGate.rememberAppReduceMotion()   // MotionGate.kt:42-64
契约：reduce == true → 位移/缩放/旋转 = 0，仅保留 α（150ms）
待接：PlayerLongPressHud(scaleIn 0.9) / PlayerSeekPreview(scaleIn 0.92)
      / PlayerStatusLayer(三点无限循环) / 设置页 SwitchRow 灰行改真开关
新增：焦点放大必须读同一个门（否则大屏会出现"菜单不减弱"的例外）
```

### 3.5 大屏 / 10-foot → 新增 `TvTypeScale`（仅在 ≥2m 场景启用）

| 角色 | 手机现状（`DesignTokens.kt:56-63`） | 电视等效（取自 E5 比例） | 说明 |
| --- | --- | --- | --- |
| 页面标题 | 26–28sp | 76 等效（Title 1 比例） | 只在电视布局 |
| 小节标题 | 16–20sp | 48 等效（Title 3） | — |
| 正文 | 13–15sp | **29 等效（Body，下限）** | 硬下限 |
| 说明文字 | 10.5–13sp | **23 等效（Caption 2，下限）** | 硬下限 |
| 字重 | Regular/Bold | 不低于 **Medium** | E5：避免细字重 |

**启用条件必须显式**：`LocalTvMode`（`ui/tokens` 新增）由"外接屏 / 投屏 / TV 启动参数"三选一置真；手机永不启用。**不允许**用屏幕宽度自动推断（平板 ≠ 电视视距）。

### 3.6 转场 → 保留时长、补落点

`SettingsNav.kt:37-46`（350ms slide+fade）作为唯一推入/弹出源；新增两件事：① 返回恢复 `LazyListState` 首个可见项；② 焦点落点恢复（回到触发卡片，而不是列表顶部）。

---

## 4. Kivo 品牌可用的 3 个签名点

> 三条都有同一个前提：**不改变 REX 已定的结构/列数/几何**，只做"加一层语义"。
> 与 `docs/design/originality-matrix.md` §0 三项判定（有支撑 / 可感知 / 可举证）逐条对齐。

### S1 影院幕布式 hero（"今晚看什么"）

- **是什么**：发现页 hero 从"轮播大图 + 分页点"升级为**幕布语义**：四周极淡暗角光晕（≤8% 黑，只在边缘 12% 区域）、片名左对齐、下方一句"今晚看什么"的行动句、分页点改为"片单式"细条。
- **为什么是 Kivo 的**：Kivo 的定位是"私人影院"（本地 + SMB + Emby + 网盘混一），不是流媒体商店；幕布是**影院**最短的视觉语汇，且 REX 的 iOS 首页是"内容货架"语汇，二者叙事不同 → 可举证差异。
- **如何不干扰 REX 结构**：hero 槽位、高度、整卡可点、轮播间隔、分页点位置**全部不变**；只改"边缘光晕 + 标题排版 + 分页点形状"三个描边级属性。云锦（`HeroCarousel.kt` / `home/HeroPage.kt`）不动布局参数。
- **验收**：与 REX 0333 并排，hero 卡片边界位置差 ≤ 4dp；hero 内文字对比度 ≥ 4.5:1（WCAG）。
- **风险控制**：光晕必须 `≤ 8%` 黑且静止（**禁止**流光/呼吸），否则退化为"AI 味"。

### S2 能力徽章（4K / HDR10 / 10bit / 码率 / 音轨）

- **是什么**：详情页元信息行下方一条单行胶囊：最多 5 枚，`accentGreen 14%` 底 + 绿字 + 10.5sp，溢出折叠为「+N」。
- **为什么是 Kivo 的**：这是**唯一能把播放内核优势摆在用户面前**的位置（`MediaTechProbe` 真实探测，P12 证据口径），REX 完全没有；同时是"能不能播得动"的决策依据，不是装饰。
- **如何不干扰 REX 结构**：不进入播放器 overlay（那是逐像素复刻区）；不改详情页既有行顺序，只**插入一行**；空探测结果**不画**（不留占位）。
- **验收**：1080p/10bit 片源出现 `1080p · HEVC · 10-bit` 三枚；探测失败时该行高度 = 0（不是灰胶囊）。行数上限 5，超出折叠。
- **风险控制**：禁止把徽章做成彩色渐变/发光；徽章不是按钮（不可点或点击进"技术信息"面板）。

### S3 源健康度（资源库卡片右侧健康点）

- **是什么**：`SourcesScreen` 每张源卡片右侧一枚 6dp 圆点：`accentGreen`（可达）/ `amber`（慢）/ `dangerRed`（失败），长按进源详情看「最近失败时间 + 延迟」。
- **为什么是 Kivo 的**：多源是产品核心（SMB / Emby / 网盘 / 本地四类同屏），REX 只有单一云端源，**不需要**健康度；kivo 用户会同时挂 3–6 个源，哪一个挂了必须一眼看出。
- **如何不干扰 REX 结构**：不新增图标列、不改网格列数、不改卡片宽高；点位于卡片右侧 padding 内（6dp 点 + 6dp 内距，占用现有 14dp 内边距空间）。
- **验收**：三点颜色在深/浅底上对比 ≥ 3:1（WCAG 非文字）；无数据时显示**灰点 + 「未检测」**（不静默消失）。
- **风险控制**：不做实时心跳动画（闪烁 = 噪音）。

---

## 5. 禁止清单（"AI 味"判定，逐条可查）

| 编号 | 禁止 | 判据（怎么算违规） | 为什么 |
| --- | --- | --- | --- |
| X1 | **霓虹/发光边框** | 任何 `BlurMaskFilter`、外发光、`shadow(color = 彩色)` | 与 REX 克制体系冲突，且与 S2 绿徽章竞争注意力 |
| X2 | **粒子/星空/流光背景** | 页面出现持续运动的非内容图形 | P9：一屏一个焦点；也违反减弱动效 |
| X3 | **彩虹/多色渐变** | 单一元素出现 ≥3 色相渐变 | `DesignTokens` 只有 `accentGreen` 一个强调色 |
| X4 | **过度玻璃（玻璃叠玻璃）** | 面板内再套 `GlassSurface`；同屏 >2 层半透明 | 可读性塌陷（P8） |
| X5 | **巨圆角** | 圆角 > 32dp 且无 REX 实测依据 | 现有 13/16/20 已定，巨圆角会与"影院"语汇冲突 |
| X6 | **无意义动效** | 动效不改变状态、不指示因果（呼吸、漂浮、抖动、循环缩放） | P12 + 性能预算；`MOT-02` 已把三点循环列为待修 |
| X7 | **自动轮播 + 自动播放视频** | hero 每 <5s 自动切；列表内自动播放 | 遥控场景下焦点会被"抢走"（E2：不要替用户改焦点） |
| X8 | **裸色/裸 dp** | 出现 `Color(0xFF…)` 或字面量 `.dp` 而非 token | 已有门禁 `tools/ui_gate.py` |
| X9 | **模拟数据冒充真实** | 徽章/健康点/统计在无数据时显示编造值 | 项目铁律：不静默降级、不造假 |

---

## 6. 验收清单（吸收项落地前逐条回答，全 ✓ 才进批次）

1. 这条原则落在**哪个文件哪个符号**？（必须能写出路径 + 函数名）
2. 与 REX 的关系是哪一类——保留 / 吸收 / 融合？（对应 §2 编号）
3. 是否改动了已验收几何（轨道 6pt / 面板 248-16-42-60 / 按钮 pitch 65 / 胶囊 92% 白）？若改，依据是哪个审计 P0 项？
4. 减弱动效开启后，该动效是否退化为"仅 α 或瞬时"？
5. 是否引入新的强调色 / 新圆角 / 新阴影族？（引入即触发 §5 复核）
6. 有没有同屏并排证据（`docs/audit/golden/cmp/`）与门禁输出（`python tools/ui_gate.py`：RESULT 行）？

---

## 7. 与既有文档的关系

| 文档 | 关系 |
| --- | --- |
| `docs/design/style-unification.md` | **唯一语言**（色/圆角/间距）。本文不改它，只在"大屏/焦点"维度补充 |
| `docs/design/visual-polish-spec.md` | 颜值门五要素。本文的 P2/P9 是其"焦点态"延伸 |
| `docs/design/originality-matrix.md` | 原创/复刻判据。§4 三个签名点均按其三条件筛选 |
| `docs/audit/player-overlay-parity-audit.md` | 播放器冲突判定的证据源（TOP-02 / PNL-01 / BOT-04 / MOT-02 等） |
| `docs/design/ui-quality-gate.md` | 交付门槛：结构过门 ≠ 完成，本文不做替代 |

> 变更记录：2026-11-17 首版。所有 HIG 引用均为当日抓取的 `.md` 原文（E1–E5）；对比度阈值来源已单独标注为 WCAG 2.1，未冒充 HIG 原文。