# 播放器面板改造 · 只读质量审查报告

时间：2026-11-18
审查人：Explore-2（只读审查，未改任何产品代码，未运行构建/真机）
范围：`PlayerComposeRoot.kt`、`PlayerGestureRouter.kt`、`player/{PlayerBarsHost,PlayerBottomBar,PlayerGestureLayer,PlayerMotion,PlayerOverlay,PlayerOverlayPanel,PlayerPanelHost,PlayerTopBar}.kt`、`ui/design/AppearancePrefs.kt`，新增未跟踪 `HandSeekState.kt`、`player/{PlayerPanelAnchor,PlayerPinch}.kt`。
对照基准：`.codex/memory/kivo-video-iron-gate.md`（铁律）、`AGENTS.md` 二、`docs/evidence/p318-gesture-audit/status.md`（横滑定稿）、`docs/design/rex-dna-player.md`（REX 定稿）、`docs/design/kivo-play-baseline.md`。

---

## 维度 1 · 铁律（单文件 ≤300 行 / 单文件单职责 / 真机证据）

### 1.1 【P2】行数：合规
【文件:行号】实测 14 个文件行数：最大 `PlayerBottomBar.kt:282`、`PlayerGestureRouter.kt:270`、`PlayerOverlay.kt:261`，其余均小于 282。
【现象】全部 ≤300 行，未触碰铁律硬线（`.codex/memory/kivo-video-iron-gate.md:22-23`）。
【为什么是问题】不是问题，记为合规证据。
【可落地改法】保持；但 `PlayerBottomBar`/`PlayerGestureRouter` 已逼近上限，后续加功能前先拆。

### 1.2 【P2】单文件单职责：PlayerPanelHost.kt 混入三类职责
【文件:行号】`PlayerPanelHost.kt:27-76`（kind到壳/宽/锚点决策表）、`:81-111`（Composable 渲染 `PlayerPanelShell`）、`:117-124`（读系统设置的 `rememberReduceMotion`）。
【现象】一个文件同时是决策表 + 渲染器 + G1 设置读取器。
【为什么是问题】违反铁律每个文件只承担一个清晰职责（iron-gate:19、31）。尤其 `rememberReduceMotion` 与面板决策毫无语义关系，却是面板层唯一入口，导致 G1 逻辑散成两套（见 3.2）。
【可落地改法】`rememberReduceMotion` 删除、统一走 `MotionGate`（`ui/motion/MotionGate.kt:62-64` 已有正确实现）；`PlayerPanelShell` 移到 `PlayerPanelForms.kt` 或独立文件；本文件只留 `object PlayerPanelHost` 决策表。

### 1.3 【P1】真机证据缺失：整批改动零证据
【文件:行号】`PlayerPanelAnchor.kt:27,33` 自述推断、未真机复算；`PlayerPinch.kt:40-44` 自述静态无法确定，真机验证若无效需改内核 video-zoom；`docs/evidence/p318-gesture-audit/status.md:3` 仍为已实现并装机、待用户四组手势复验。
【现象】本次 B4-T1（面板锚点）、B4-T3（双指缩放）、G1 降级改造均无 `docs/evidence/` 真机截图/复算，却已进入待入库状态。
【为什么是问题】铁律明写没有可复算的证据路径不许说完成、改完要真机自检并与参考逐项对比（iron-gate:36-37；AGENTS.md:203-207）。锚点几何是推断值，双指缩放对 SurfaceView 是否生效是未确认，这两项恰恰是静态审查无法判定的。
【可落地改法】K40 `b6e950ee` 上逐项截图：8 个顶栏图标面板锚点、3 个底栏面板锚点、双指缩放+横滑组合、系统移除动画开关下的控制栏；证据落 `docs/evidence/p340-player-panel/status.md`，与 REX 同区域并排（±2pt）。

---

## 维度 2 · 手势正确性

### 2.1 【P1】0 位置哨兵：从片头开始的横滑被整体吞掉
【文件:行号】`HandSeekState.kt:29` `var startMs: Long = 0L`（0 当无手势哨兵）；`:41-46` `begin(fromMs)` 直接 `startMs = fromMs`；`PlayerGestureRouter.kt:228` `if (hSeek.startMs <= 0L || durationMs <= 0L) return`。
【现象】若横滑起点 `positionMs == 0`（片头、刚起播、精确 seek 到 0 后），`begin(0)` 后 `startMs` 仍为 0，`onHorizontalDrag` 首帧即 return，整次横滑无效。
【为什么是问题】`HandSeekState.kt:9-12` 的注释专门批评旧实现用 0 做哨兵会吞掉第一次横滑，声称改成组合持有后一定是干净的起点，但哨兵值本身没改，bug 原样保留。这是注释与实现自相矛盾，且是本批最容易在真机被用户撞到的缺陷。
【可落地改法】`HandSeekState` 增加独立 `var active: Boolean`，`begin` 置 true、`clear` 置 false，`onHorizontalDrag` 改判 `if (!hSeek.active) return`；或把哨兵改为 `-1L` 并同时改 begin/clear/判断三处。附单测：`begin(0)` 后拖动应产生非 0 `targetMs`。

### 2.2 【P1】双指缩放的跨 pass consume 假设不成立
【文件:行号】`PlayerPinch.kt:90-104`（`pointerInput(locked)` + `awaitPointerEvent(PointerEventPass.Initial)` + `event.changes.forEach { it.consume() }`）；`PlayerGestureLayer.kt:90-94` 注释声称祖先 Initial pass 早于下游主分发，先 consume 能让下游看到 isConsumed 自行取消。
【现象】四个单指检测器（`detectHorizontalDragGestures` 等）用默认 Main pass。按 Compose PointerEventPass 语义，Initial pass 的 consume 不会传播到 Main pass，下游检测器仍会处理第一根手指的横滑/长按。
【为什么是问题】双指缩放时可能同时触发横滑 seek / 竖滑亮度，画面一边放大一边跳进度；`PlayerGestureLayer` 的互斥设计意图落空。文件自己也承认静态无法确定（`PlayerPinch.kt:40-44`），没有真机反证。
【可落地改法】把缩放手势改到 Main pass 消费（如 `detectTransformGestures` 并在单指时放行 pan），或在四个单指检测器回调里显式加 `if (change.changes.count { it.pressed } >= 2) return`；补真机双指缩放同时横滑用例。

### 2.3 【P2】取消手势无法回滚已发生的实时 seek
【文件:行号】`PlayerGestureRouter.kt:250-254`（150ms 节流实时 seek）、`:258-269`（`commit=false` 仅跳过最后一次精确 seek）；`PlayerGestureLayer.kt:114,128,161`（`onDragCancel`）。
【现象】`onHorizontalDragEnd(commit=false)` 的注释仍写取消则什么都不做，但节流分支早已 `seekTo(targetMs)`；来电/切后台触发 `onDragCancel` 时画面停在中间落点。
【为什么是问题】语义不清。定稿（p318 status 七/九）要求实时跟手，故取消回到起点未必必要，但当前注释误导，且 `onDragCancel` 与 `onDragEnd(false)` 行为相同却无说明。
【可落地改法】明确改为跟手即提交，commit 仅决定是否补精确 seek，同步改注释；若产品要求真取消，则在 cancel 分支 `seekTo(hSeek.startMs)`。

### 2.4 【P2】缩略图每帧 seek 未节流，与既有带宽优化结论冲突
【文件:行号】`PlayerGestureRouter.kt:242` `thumbnailReader?.seekTo(hSeek.targetMs * 1000)`（在 150ms 节流判断之外）vs `:250-254`（只有主链路 seek 被节流）；`PlayerComposeRoot.kt:154-158` 明确记录 v29 教训，每 50ms seek 预览导致一次拖动预览下掉 5 MiB，且 seek+解码要约 1s，浮窗永远显示旧位置。
【现象】横滑时预览内核每帧回源站拉字节，重演 v29 已修掉的带宽互踩。
【为什么是问题】同一份代码库对进度条拖动和全屏横滑给出相反结论，后者会拖慢落点首帧，直接伤害定稿要求的实时跟手体验。
【可落地改法】把 `thumbnailReader.seekTo` 移入 150ms 节流分支；或复用 `pendingPrefetchMs` 的 settle 机制，停手 200ms 后再 seek 预览。

### 2.5 【P2】边界条件覆盖缺口（拖到头尾、横竖屏切换）
【文件:行号】`PlayerGestureRouter.kt:231-232`（`accumMs` 累加、`targetMs` clamp 到 `[0,durationMs]`）；`PlayerComposeRoot.kt:47-49`（`handSeek`/`pinch` 用 remember）。
【现象】拖到头尾时 `targetMs` 被 clamp，但 `accumMs` 继续累加，反向拖回需要等量还债才能离开边界，手感发黏；横竖屏切换导致 Activity/组合重建，`handSeek`、`pinch` 重置。
【为什么是问题】定稿的形态矩阵（`rex-dna-player.md:91-107`）要求各状态自洽；铁律要求真机覆盖。
【可落地改法】clamp 时同步回写 `accumMs = targetMs - startMs`；补拖到片尾再回拖、横滑中切横竖屏真机用例。

---

## 维度 3 · Compose 状态 / 动画主线程安全

### 3.1 【P2】rememberReduceMotion() 名不副实且每次重组查设置
【文件:行号】`PlayerPanelHost.kt:117-124`：函数名为 remember，内部没有 remember，每次重组执行一次 `Settings.Global.getFloat(ContentResolver, ...)`；且只查 `ANIMATOR_DURATION_SCALE` 一个 scale。
【现象】重组时反复跨进程读 Settings；与 `MotionGate.systemReduced`（`ui/motion/MotionGate.kt:25-34` 查三个 scale）口径不一。
【为什么是问题】命名误导 + 性能浪费；口径不一导致 G1 判定不一致（见 3.2）。
【可落地改法】删除该函数，所有调用点统一用 `MotionGate.rememberReduceMotion(...)` / `rememberAppReduceMotion()`（`MotionGate.kt:49-64`，内部已 `remember(context)`）。

### 3.2 【P1】G1 双门语义冲突：App 默认关闭会短路系统移除动画
【文件:行号】`PlayerBarsHost.kt:71-73` `reduced = reduceMotion || gateReduce`；`PlayerOverlay.kt:133` `reduceMotion = rememberReduceMotion()`（来自 3.1 的单 scale 读取）；`MotionGate.kt:42-46` `reduce(appEnabled=false, system)=false`；`AppearancePrefs.kt:33` `reduceMotion` 默认 `false`。
【现象】系统只关闭 transition/window 动画（`ANIMATOR_DURATION_SCALE` 仍为 1）、App 开关默认 false 时：`reduceMotion=false`、`gateReduce=false`，于是 `reduced=false`，控制栏仍播放位移/缩放动画。
【为什么是问题】违反定稿 G1：系统 animator scale + App 开关取或（`rex-dna-player.md:115`、`kivo-play-baseline.md:86`），也违反 `MotionGate.kt:13-16` 自己的注释。根因是 MotionGate 的 `reduce(false, system)=false` 与或语义相反，播放器又叠了两套门。
【可落地改法】`MotionGate.reduce` 改为真正取或（`appEnabled == true || systemReduced`，false 不再短路系统值）；删除 `PlayerPanelHost.rememberReduceMotion`，播放器只保留一个门；补 JVM 单测覆盖 `(false, true) => true`。

### 3.3 【P2】面板淡出期间内容先变空，lastKind 防闪没意图落空
【文件:行号】`PlayerOverlayPanel.kt:61-66`（`lastKind` 只保留 kind）；`PlayerComposeRoot.kt:106-110` `panelTitle(activePanel)` / `rememberPanelItems(activePanel, ...)` 随 activePanel 变空而返回空标题/空列表。
【现象】关闭面板时 activePanel 置空导致 `items`/`title` 立刻变空，但 `shownKind` 靠 lastKind 保留到淡出结束（150ms）；用户看到标题还在、列表已空的闪变。
【为什么是问题】`PlayerOverlayPanel.kt:62-63` 注释写不保留的话面板会瞬间变成空壳再淡出，实际只保留了壳类型，未保留内容，治标不治本。
【可落地改法】在 `FloatingPanel` 内同时快照 `lastTitle`/`lastItems`（`LaunchedEffect` 在非空时更新），淡出期间用快照；或由调用方延迟到 150ms 后再清空 activePanel。

### 3.4 【P2】动画对象本身主线程安全（合规证据）
【文件:行号】`PlayerMotion.kt:31` `val panelSpring = spring<Float>(...)` 作为 object 单例字段；`:27-47` 全为常量；`reduceMotionMs` 从 150 改 100（diff `:43-47`）。
【现象】AnimationSpec 无实例状态，跨重组/跨 composable 复用安全；`BarSlot` 的 `animateFloatAsState`、`graphicsLayer`（`PlayerBarsHost.kt:165-184`）均在主线程按 Compose 正常路径运行，未起线程、未直接操作 View。
【为什么是问题】不是问题，记为合规证据。
【可落地改法】保持；勿在 `graphicsLayer` 回调里做 I/O 或 Settings 查询（当前没有）。

---

## 维度 4 · 回归风险（对照 git diff）

### 4.1 【P1】控制栏常驻 alpha=0 仍参与命中测试，隐藏时可误触或吃横滑
【文件:行号】`PlayerBarsHost.kt:155-187` `BarSlot` 用 `graphicsLayer { alpha = alpha }` 替代 `AnimatedVisibility`（diff `:74-80,175-184`）；`PlayerOverlay.kt:142-166` 手势层在底、控制层在上。
【现象】`visible=false` 时顶栏/底栏 alpha 归 0，但 Compose 的 alpha 不影响 hit test，透明按钮与进度条仍可点击/拖动。
【为什么是问题】这是为修双进度条（diff 注释 `:76-80`，真机 bug）而改常驻的副作用。修复手段绕开了根因：原本两条进度条是因 `thickness` 是 per-instance 状态；现改为常驻后，隐藏态的透明控件会吃掉画面点击和全屏横滑（进度条在上层先拿事件），直接威胁本批重点的横滑 seek。
【可落地改法】`visible=false` 时给 BarSlot 加吞事件修饰（pointerInput + awaitPointerEvent 并 consume），或让进度条 thickness 提升为父级状态后恢复 `AnimatedVisibility`（消除双树根因）。补真机隐藏控制栏时全屏横滑用例。

### 4.2 【P2】手势层注释仍写松手才 seek，与实时 seek 实现冲突
【文件:行号】`PlayerGestureLayer.kt:108-109`（为什么松手才 seek、避免拖动过程中反复 seek 把内核打乱）vs `PlayerGestureRouter.kt:244-254`（150ms 节流实时 seek，diff 未改这段）。
【现象】注释停留在 p318 一阶段（松手提交），实现已是七/九阶段（实时跟手）。
【为什么是问题】后来者按注释理解会误判横滑期间不 seek，进而在错误位置加逻辑；这是定稿两轮演进后遗留的文档腐化。
【可落地改法】把注释改为实时跟手口径，并指向 `PlayerGestureRouter.kt:238-254` 与 p318 status 九。

### 4.3 【P2】顶栏第 4/5 格互换，锚点反查依赖字符串一致
【文件:行号】`PlayerTopBar.kt:170-179`（新版 `expand`/`clapper` 互换，符合 `rex-dna-player.md:47`）；`PlayerPanelHost.kt:53-72` 与 `PlayerPanelAnchor.kt:85-88` 均以 `action` 字符串 `indexOfFirst` 反查。
【现象】映射找不到时静默回落 `Center`（`PlayerPanelHost.kt:69`、`PlayerPanelAnchor.kt:87`）。
【为什么是问题】找不到就 Center、不猜是合理兜底，但会掩盖错配：一旦 action 名改动，面板会悄悄回到屏幕中央且无任何提示，违反不许静默降级精神（应带可见 reason）。
【可落地改法】调试构建下对回落 Center 的未知 kind 打日志/Toast；补一张 kind 到锚点映射单测锁住 8 个顶栏 + 3 个底栏。

### 4.4 【P2】选集态轨道行重构后高度写死，未真机验证裁切
【文件:行号】`PlayerBottomBar.kt:88-92`（episodes=200.dp / resource=210.dp 写死）、`:105-182`（轨道行常驻 + 选集带替换按钮行，diff 删除了旧 `return@Box`）。
【现象】行为方向正确（定稿 `rex-dna-player.md:102` 选集态保留轨道），但 scrim 高度 200/210dp 是估值。
【为什么是问题】轨道行 + gap + 选集带 + bottomPad(33dp) 若超过 200dp，会压住或裁切；铁律要求真机复算。
【可落地改法】用 `wrapContentHeight` 或按实测重定高度；K40 截图核对 IMG_0170 的轨道仍完整在。

### 4.5 【合规】已验证行为未被本批破坏（对照 diff）
- 横滑实时跟手三件套：进度条读 `previewMs`（`PlayerGestureRouter.kt:241`）、缩略图浮窗复用（`PlayerGestureHud` 未改）、150ms 节流 + 松手精确 seek（`:250-268`）——保持。
- HUD 双行 `26:21 / 43:43` + 带符号偏移、288dp 宽度：`PlayerGestureHud.kt:80-129` 未改，符合 p318 status 八第 3 轮。
- 玻璃面板：`PlayerPanelShell.kt:103-109` 仍走 `PlayerGlassMenu`；detail 卡走 `PlayerDetailCard`——保持。
- 返回键：`PlayerComposeRoot.kt:127` `onClose = { finish() }`——保持。
- 默认 vo=gpu-next：`playback/PlaybackPrefs.kt` 本批未改，常量与注释仍在——保持。
【可落地改法】回归测试时把上述 5 项纳入固定清单（本批未做真机，故只能确认代码未改，不能确认运行正确）。

---

## 维度 5 · 重复 / 死代码

### 5.1 【P2】barEnter/barExit 整段死代码
【文件:行号】`PlayerBarsHost.kt:135-143`；Grep 全仓仅此定义处，无任何调用；随带 import `EnterTransition`、`ExitTransition`、`slideInVertically`、`slideOutVertically`、`fadeIn`、`fadeOut` 全部失去使用者。
【现象】`BarSlot` 改用 `animateFloatAsState` 后，旧的 AnimatedVisibility transition 工厂未删除。
【为什么是问题】死代码 + 死 import，增加误读风险（后来者可能以为控制条还在用 slide 动画）。
【可落地改法】删除 `:135-143` 及相关 import。

### 5.2 【P2】G1 门两处实现、语义不一致
【文件:行号】`PlayerPanelHost.kt:117-124` vs `ui/motion/MotionGate.kt:49-64`（详见 3.1/3.2）。
【现象】同一件事（是否减弱动画）有两套读取与判定，且结论可相反。
【为什么是问题】重复实现 + 不一致语义，是改一处要翻两处的典型。
【可落地改法】统一到 MotionGate，删除前者。

### 5.3 【P2】三件套职责边界：基本清晰，但 PlayerPanelHost 偏胖
【文件:行号】`PlayerOverlay.kt:60-261`（层序编排）、`PlayerOverlayPanel.kt:46-140`（开合过渡 + 锚点摆放）、`PlayerPanelHost.kt:27-124`（决策表 + 渲染 + G1）。
【现象】前两者边界清晰（编排 vs 过渡），第三者多背了渲染与 G1；旧的 `trailingPad`/`topPad` 已删除干净（Grep 仅注释残留），生产代码无新旧面板并存；`artifacts/p240-player-ui/v2-archive/PlayerOverlay.kt` 为归档，不参与编译。
【为什么是问题】不是严重重复，但 `PlayerPanelHost` 违反单职责（见 1.2），且 `PlayerPanelAnchor.kt`/`PlayerPanelGeometry` 与 `PlayerPanelHost.anchorFor` 分居两文件互相调用，任一处 action 改名都可能错位。
【可落地改法】按 1.2 拆分；保持 `PlayerPanelGeometry` 为纯几何函数（便于无设备复算，这点设计是对的）。

---

## 总体评价

方向对、注释勤奋、分层意图清晰：把横滑状态从进程级顶层 var 改成随组合生死的 `HandSeekState`（`HandSeekState.kt:6-24`）、面板从一律右上角改为按触发控件锚定（`PlayerPanelHost.kt:17-25`）、G1 降级收口、动画常量单一来源，这些都符合铁律的先设计家族树再落代码精神，代码可读性在同类项目里属上乘。

但这批改动还不到可入库质量，三类硬伤必须处理：
1. 一个确定存在的边界 bug（0 位置哨兵吞掉横滑，2.1）；
2. 一个很可能错误的技术假设（Initial pass consume 挡不住 Main pass 检测器，2.2）；
3. 一个交互回归风险（透明控制栏仍吃点击/横滑，4.1），叠加 G1 双门语义冲突（3.2）与整批零真机证据（1.3）。

注释里大量推断、未真机复算、静态无法确定是诚实的，但这些正是铁律要求用证据关闭的项，不能带着推断入库。

## 最该修的 5 件事（按优先级）

1. 修 0 位置横滑失效（2.1）：`HandSeekState` 用 `active` 布尔替代 0 哨兵，`begin(0)` 必须可用；补单测。
2. 修/验双指缩放与单指手势的互斥（2.2）：改到能被下游看到的 pass，或在单指检测器中显式排除多指；K40 真机做双指缩放同时横滑组合用例。
3. 修透明控制栏命中（4.1）：hidden 态吞事件，或恢复 AnimatedVisibility 并把进度条粗细状态提升父级；真机验隐藏控制栏时全屏横滑仍生效。
4. 统一 G1 门并删死代码（3.1/3.2/5.1/5.2）：`MotionGate.reduce` 改为真正取或，删 `PlayerPanelHost.rememberReduceMotion` 与 `barEnter/barExit` 死段。
5. 补真机证据并把缩略图 seek 纳入节流（1.3/2.4）：8+3 个面板锚点逐项与 REX 并排、横竖屏/拖到头尾/长按+横滑全覆盖；横滑预览改为 150ms 节流或 settle 后 seek，避免重演 v29 带宽问题。
