# 播放器 Overlay 视觉与交互一致性审计（Rex 1.0.2 对照）

> 审计对象：`android/app/src/main/java/com/kivo/cinema/ui/compose/` 下的播放页 Compose 层。
> 审计方式：**只读**源码 + 竞品实拍逐像素比对。**本文不改任何生产代码。**
> 结论口径沿用竞品文档 §0：✅ 确定（随包资源/文案/截图中可直接复算）· ◐ 推断（确定事实 + 平台惯例，注明依据）· ❌ 待确认（静态无法观测，须真机/录屏取证）。

---

## 0. 口径与证据

### 0.1 竞品证据来源（含一处路径修正）

| 来源 | 用法 | 状态 |
| --- | --- | --- |
| `Rex_动效与交互全量拆解.md` §1（G1–G6）、§3.M03（S01–S07）、§5、§6 | 动效治理、播放器手势链、锁定矩阵、待确认清单 | 已全文核对 |
| `Rex_UI结构复刻说明书.md` §6.10 Player | 播放器能力边界（超分/字幕/弹幕/外屏） | 已核对 |
| `Rex_动效参数表.csv`（33 行） | 数值级契约（M01-S08、M03-S01…S05、M08-01…04） | 已逐行核对 |
| 实拍截图 | 布局/几何/文案 | 见下 |

> ⚠️ **路径修正**：任务说明写"播放器相关批次在 `2026_11_16_21_2x_*` 之后"。实测 `.tmp/rex_scaled/` 下 **`2026_11_16_21_16:58–21:21:58` 全部为竖屏首页/详情页**（690×1500）；**真正的播放器实拍是 `2026_11_11_19_21:08–19:23:01` 的 15 张 1500×693 横屏图**（IMG_0159–0173）。本报告以这 15 张为准。若另有 11-16 晚的播放器批次尚未落入 `.tmp/rex_scaled/`，需补图复核，本文相应结论标 ◐/❌ 的位置会受影响。

被引用的关键帧：

| 帧 | 内容 | 用于 |
| --- | --- | --- |
| IMG_0160 | 播放常态 + 字幕面板（"选择字幕"） | 顶栏无遮罩、面板形态/宽度、8 图标被面板遮挡情况 |
| IMG_0165 | "跳过片段方式"面板 | 第 5 图标语义、面板宽 248、双行行高、分区标题 |
| IMG_0168 | AirPlay/DLNA 小卡 | 小卡宽度（≈160pt）与锚点 |
| IMG_0170 | **8 个图标全显** + 选集整替态 | 图标槽位与顺序、选集带几何、**轨道行仍然可见** |
| IMG_0171 | 倍速面板（锚在"倍速"钮上） | **底栏钮面板向上展开**、无标题行、滚动条长 |
| IMG_0172 | 听视频态 | 图标 8→7（PiP 隐）、耳机反白圆、中央块 |
| IMG_0173 | "填充模式"面板（锚在场记板） | 第 4 图标语义、`避开灵动岛`、`画面比例 >` 钻入 |

### 0.2 代码路径与实测行数

| 文件 | 任务给的行数 | 实测 | 备注 |
| --- | --- | --- | --- |
| `player/PlayerOverlay.kt` | 256 | **256** | 60 个参数，纯接线 |
| `PlayerActionRouter.kt` | 406 | **407** | 超 300 自定上限 |
| `PlayerGestureRouter.kt` | 278 | **278** | 含顶层可变全局态 |
| `PlayerPanelRouter.kt` | 289 | **290** | — |
| `PlayerComposeActivity.kt` | 382 | **383** | 注释自称"守 ≤300" |

### 0.3 结论摘要

| 维度 | 已对齐（竞品确定项） | 主要差距 | 条数 |
| --- | --- | --- | --- |
| PlayerTokens | 单一常量源成立，关键几何全部取值自 token | 3 个 token 零引用、1 个仅存在于注释；各壳仍有 6–17 处裸 dp/裸色；一条注释与真值相反 | 7 |
| 顶栏 | 自绘状态行、8 槽位、44 命中盒、间距公式、**无遮罩** | **第 4/5 图标语义位次互换** | 7 |
| 底栏 | 6pt 三段半透明+描边、无滑块、时间位置、按钮 pitch 65、右侧三钮 | **选集态轨道行整条消失**；自创"按下 6→10pt" | 10 |
| 面板 | 248 宽、16 圆角、42/60 行高、分区标题、3pt 滚动条、点击外部关闭 | **锚点方向错（统一右上角）**；skip 面板 300 宽；缺"填充模式"层；滚动条 thumb 定长 | 11 |
| 手势 | 三分区、长按→调速→上滑锁定→恢复、锁定矩阵 **完全一致**、触觉三态+低电量 | **双指缩放缺失**；锁定允许项被做成可关闭开关；亮度开关关闭后**静默无响应** | 11 |
| 动效 | 180/140、200/250、500、150/1000/300 全部与提示一致 | **App 内 Reduce Motion 未接（设置里是灰行）**；起播无 spinner 回退；换集无平滑过渡与失败态 | 10 |
| 职责 | 文件边界有注释、面板壳/过渡/遮挡/动作已分层 | ActionRouter 407 行混 seek 内核调优；**手势文件用 3 个顶层可变全局态**；图标语义存在两份清单 | 8 |

**P0 = 2 条**（PNL-01 面板锚点方向、MOT-01 Reduce Motion 未接）；
**P1 = 12 条**；P2 = 20 条；P3/待确认 = 8 条。

---

## 一、PlayerTokens：常量是否被真正引用（TOK）

### 1.1 对齐确认

- 常量源确实存在且**被主路径引用**：`actionPitch`/`actionHitBox`（`PlayerTopBar.kt:107-109,169`）、`trackHeight`/`trackActiveHeight`/`trackHitHeight`（`PlayerSeekTrack.kt:80,122,187`）、`panelWidth`（`PlayerPanelHost.kt:77`、`PlayerGlassMenu.kt:54`）、`panelRadius`（`PlayerGlassSurface.kt:28`）、`bottomPad`/`bottomSidePad`/`trackStartGap`/`trackEndGap`/`playButtonPitch`/`playGroupInset`/`textButtonGap`（`PlayerBottomBar.kt:116-118,126,142,195,212`）、`lockGlyphSize`/`lockSlotBottomPad`（`PlayerLockLayer.kt:45-47,56`）。
- 常量自带"数值来源 + 复算命令"，符合仓库铁律（`PlayerTokens.kt:5-15`）。

### 1.2 差距

| 编号 | 竞品契约（含等级） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| TOK-01 | 单一常量源（工程契约，非竞品项） | `PlayerTokens.kt:42-43` `topScrimTop/Bottom`；`topScrimBottom` 全仓 **0 引用**，`topScrimTop` 仅出现在 `PlayerTopBar.kt:48` 的注释里 | "顶栏刻意不加遮罩"这一决定**在代码里不可见**，只活在注释；后续有人加渐变时没有任何断言能拦住 | 无直接可见后果；风险是将来被改成渐变（顶部出现黑带，v2 已踩过） | 二选一：① 删除两个常量，在 `PlayerTopBar` 顶部加 `Box(Modifier.background(Brush.verticalGradient(listOf(PlayerTokens.topScrimTop, PlayerTokens.topScrimBottom))))` 显式画 0% 渐变；② 保留常量并在 `PlayerTopBar` 用 `.drawBehind{}` 引用，让"0 遮罩"成为可 grep 的事实 | P1 |
| TOK-02 | 面板内边距 16（§1.3 ◐ 实测 16） | `panelPadding = 16f`（`PlayerTokens.kt:197`）**0 引用**；实际各壳各写：`PlayerGlassMenu.kt:69` `padding(vertical = 6.dp)` + 各行 `rowTrailingPad`；`PlayerDetailCard.kt:63` 用 `innerCardMargin = 15` | token 与实现脱钩，面板实际内边距是 6/14/15/16/18 五种 | 面板左右内边距在不同壳不一致（视觉轻微不齐），用户难以指出但"精致感"下降 | `PlayerGlassMenu` 外层改 `padding(horizontal = PlayerTokens.panelPadding.dp, vertical = 6.dp)` 并核对行 `rowTitleInset` 是否为"含内边距"口径；`PlayerDetailCard.kt:63` 明确区分 `innerCardMargin`（卡内）与 `panelPadding`（壳内） | P2 |
| TOK-03 | 弹幕字号 11.5（§1.3/§1.5 ◐） | `danmakuSize = 11.5f`（`PlayerTokens.kt:267`）**0 引用**；弹幕样式在 `com.kivo.cinema.danmaku.DanmakuSettings`（fontScale 0.6–1.6） | 弹幕字号不归 PlayerTokens 管，token 是死值 | 无（由弹幕设置面板控制，语义不同） | 删除 `danmakuSize` 或在 `DanmakuSettings` 里以它作为 1.0 基准并注明 | P3 |
| TOK-04 | 所有尺寸/字号来自 token（工程契约） | 裸 dp/sp 计数（排除 `PlayerTokens.kt`）：`PlayerResourcePanel` 17、`PlayerGlassMenu` 17、`PlayerEpisodeStrip` 17、`PlayerStatusLayer` 14+1sp、`PlayerStepperRow` 11、`PlayerGestureHud` 9+2sp、`PlayerDetailCard` 8、`PlayerSeekPreview` 6+1sp、`PlayerLongPressHud` 5+1sp、`PlayerBottomBar` 3、`PlayerOverlay` 2 | 22 个壳文件共约 **130 处裸 dp/sp**；其中**视觉契约值**（非布局微调）例如：`PlayerGestureHud.kt:86` 208/288 宽、`:88` 圆角 20、`:90` 18/12 padding、`:105` 图标 22、`:115` 字号 22sp；`PlayerStatusLayer.kt:192` 三点 9dp、`:187` 间距 9dp；`PlayerBottomBar.kt:84-85` 200/210 渐变高、`:105` 60dp；`PlayerGlassMenu.kt:273` 滚动条 72dp；`PlayerEpisodeStrip.kt:149` 96dp 空态高 | 改一处视觉参数（如 HUD 圆角）要全仓 grep；跨壳出现同类值不同步（HUD 圆角 20 / 预览圆角 8 / 面板 16 无统一族） | 建 `PlayerTokens` 第二段"衍生件"：`hudWidth`/`hudWidthSeek`/`hudRadius`/`hudIconSize`/`hudTextSize`/`statusDotSize`/`barScrimEpisodes`/`barScrimResource`/`scrollbarTrackHeight`/`episodeEmptyHeight`；逐文件替换（按文件提交，便于回归）。`PlayerTopBar.kt:102` 的 `start=4/end=8` 也应成为 `titlePadStart/titlePadEnd` | P2 |
| TOK-05 | 颜色单一源（工程契约） | 裸色：`PlayerGestureHud.kt:89` `Color(0xB3000000)`（HUD 胶囊底）；`PlayerGestureLayer.kt:164` `flashScrim = Color(0x99000000)`；`PlayerSeekPreview.kt:93` `Color(0xFF101012)`（无帧空框）；`PlayerStepperRow.kt:151` `Color(0xFF1C1C1E)`/`Color.White`；`PlayerStatusLayer.kt:97,103,198` `Color.White`/`Color.Black`；`PlayerResourcePanel.kt:92,191` | 4 个"黑底/白"族的自创值，与 `panelSurface(0xBF000000)`/`toastSurface(0xE618181B)`/`chipSurface` 三档无关 | HUD 黑 70%、闪示黑 60%、空框近黑 —— 同一屏上出现 4 种黑，暗场里能看出"脏斑" | `PlayerTokens` 增 `hudSurface = 0xB3000000`、`flashScrim = 0x99000000`、`previewEmptySurface`、`toggleOffSurface`；四个文件改引用 | P2 |
| TOK-06 | —（内部一致性问题） | `PlayerSeekTrack.kt:41-43` 注释："三段＝已播 #FFFFFF / 缓冲 ≈#9B9B9B 实色 / 未播＝**黑 45% 罩**视频"；而 `PlayerTokens.kt:23-36` 已是"半透明白 55% + 描边、未播白 12%" | **注释与真值相反**（注释描述的是被 v3.2 推翻的旧方案） | 无直接可见后果；下一次改动会按错注释回退 | 用 `PlayerTokens` 的文件头口径重写 `PlayerSeekTrack` 头注释（把实测三段色值搬进 `PlayerTokens` 注释，此处只引用符号） | P1 |
| TOK-07 | 6pt 厚度，**无滑块**（✅ 实测） | `trackActiveHeight = 10f`（`PlayerTokens.kt:146`）自标"**非实测值**（用户指定的选中增强）"，由 `PlayerSeekTrack.kt:79-83` 在按下/拖动时 6→10 动画 | 竞品在 0160/0170/0171/0172/0173 五张不同态截图里轨道厚度**一致**，没有变粗；"10pt"是自创 | 拖动时进度条变粗，与竞品"静态细轨"观感不同；厚 10pt 时圆角半径随之变化，端点更钝 | 若坚持保留：把常量注释改为"自创增强（非竞品）"并在 `PlayerSeekTrack` 加开关；若要严格对齐：默认 `trackActiveHeight = trackHeight = 6f`，保留常量以便回归 A/B | P2 |

---

## 二、顶栏（TOP）

### 2.1 对齐确认（竞品确定项，全部满足）

| 项 | 竞品 | 实现 | 判定 |
| --- | --- | --- | --- |
| 状态行自绘 | ✅ 沉浸式下仍需时间/网速/电量（0160 顶部 19:21 / 7.5MB/s / 电池） | `PlayerStatusRow.kt:43-95`：时钟（30s 刷新）+ `cache-speed` 真实网速 + `ACTION_BATTERY_CHANGED` | ✅ |
| 槽位数量 | ✅ 8 个（0170 数得：显示器·弹幕·PiP·场记板·圈✓·波形·字幕·耳机） | `PlayerTopBar.kt:139-148` 8 项 | ✅ 数量 |
| 命中盒 | ≥44pt（M03 各条要求"fg 可按"） | `PlayerTopBar.kt:169` `size(actionHitBox=44)` | ✅ |
| 中心距 | 50pt | `PlayerTopBar.kt:107-109` `spacedBy(50-44=6)` ⇒ 中心距 50 | ✅（v2 的死按钮已修） |
| 无遮罩 | ✅ 白图标直压画面（0160/0168/0170 顶部无黑带） | `PlayerTopBar` 无 background；`PlayerBarsSlot` 无 scrim | ✅ |
| 按压反馈 | 本体变色、无涟漪（推断 ◐：8 连排不起涟漪） | `TopIconButton.kt:172-177` 用 `detectTapGestures` 而非 `clickable`；`:186` `alpha=pressAlpha(0.6)` | ✅ |
| 听视频态 | ✅ PiP 隐、耳机反白圆（0172） | `PlayerTopBar.kt:114` 跳过 pip；`:182-190` `inverted` 白圆底黑耳机 | ✅ |
| 弹幕关闭变暗 | 推断 ◐（0160 弹幕图标为白+点，0172 同） | `PlayerTopBar.kt:121,170` `dimmed → alpha 0.45` | ◐ 合理 |

### 2.2 差距

| 编号 | 竞品契约（含等级） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| TOP-01 | ✅ 第 4 槽（场记板）打开**填充模式/画面比例**（0173）；第 5 槽（圈✓）打开**跳过片段方式**（0165） | `PlayerTopBar.kt:139-148` 顺序 `cast, danmaku, pip, clapper("跳过片段"), expand("画面比例"), audio, captions, audioMode`；`PlayerActionRouter.kt:236` `"clapper" -> togglePanel("skip")`；`:237` `"expand" -> togglePanel("quality")` | **第 4/5 两槽语义互换**，且图标名称与竞品实物不符（竞品第 4 是场记板、第 5 是圈内对勾） | 点第 4 个（场记板）期望"填充屏幕/画面比例"，实际弹"跳过片段卡"；点第 5 个期望"跳过"，实际弹纵横比列表。用户需重新学习图标含义，前 3 个与后 3 个都对的中间两个错了，最容易被当成"面板偶尔点错" | 1) `PlayerTopBar.kt:139-148` 改为 `cast, danmaku, pip, expand("画面比例/填充"), clapper("跳过片段"), audio, captions, audioMode`；2) `PlayerActionRouter.kt:236-237` 两条分支对调；3) `PlayerGlyph` 的 `expand` 按竞品画"场记板"轮廓、`clapper` 改画"圈内对勾"（或新增 `skipMark` kind），label 同步（`"画面比例"`/`"跳过片段"`） | P1 |
| TOP-02 | ✅ 面板锚点＝"图标右缘 + 8~12pt"（0173 场记板中心 1110px、面板右缘 1145px，1pt=1.78px ⇒ +20px≈+11pt） | `PlayerPanelHost.kt:32-41` trailing 表只有 `subtitle*→46`、`audio→96`、`quality→146`、`speed→16`、`danmakuOps→296`，**没有 `clapper`/`expand`/`skip` 的键**，`else → 64f` | `clapper` 改走 `skip` 后是 detail 壳（`DETAIL_KINDS` 含 `"skip"`，`:45`），`topPad=16`（`:43`）+ `trailing=16`（`:39`）⇒ 面板被钉在屏幕右上角，**不跟随触发图标** | 点场记板图标，面板出现在屏幕最右上（离图标 300pt 远），视线需要跳到对角 | `PlayerPanelHost.trailingPad` 增 `"clapper" -> 196f`（idx=4 ⇒ 50·4−4）、`"expand" -> 146f`（idx=3）、`"skipMode" -> 16f`；同时 `topPad` 对 detail 壳改为"锚点图标下沿 + 8"而不是固定 16 | P1 |
| TOP-03 | ✅ 8 图标 ink 边距 ≈66pt（066.7 / 779.7） | `PlayerTopBar.kt:71,124` `iconRowPad=8`；`:108` `spacedBy(6)`；右侧仅 8 图标 ⇒ ink 右缘 = 屏右 − 安全区 − 8 − (44−23)/2 | 计算正确（v3 注释已推导 77 vs 实测 74.3，Δ2.7pt）| 无 | 无需改；建议把"Δ2.7pt 已接受"写进 `PlayerTokens.iconRowPad` 注释（现只在 `PlayerTopBar` 头注释） | — |
| TOP-04 | ◐ ✕ ink 17pt、主页 ink 21×18.3pt | `PlayerTokens.kt:130-132` `closeIconSize=20`/`homeIconSize=22`；`PlayerTopBar.kt:85-91` | 0160 实测 ✕ 约 40px≈22.5pt、主页约 34px≈19pt ⇒ **✕ 应更大、主页应更小** | 顶栏左侧两个图标的"重量感"与竞品相反：竞品 ✕ 明显大于主页 | 待真机/更高分辨率帧确认后，交换两值并补 `PlayerTokens` 复算命令参数（`measure_baseline_playback.py icons`） | P3（待真机确认） |
| TOP-05 | ✅ 状态行右侧"网速 + 电池"，电池有充电指示（0168 电池内闪电） | `PlayerStatusRow.kt:85-93` 只画 `fill=batteryLevel` 的电池 glyph | 无充电态 | 充电时电池图标不显示闪电，与系统状态栏观感不一致（用户会以为没在充电） | `PlayerStatusRow.kt` 追读 `BatteryManager.EXTRA_STATUS`（`BATTERY_STATUS_CHARGING/FULL`），`PlayerGlyph(kind="battery", charging=…)` 增一个闪电笔画 | P2 |
| TOP-06 | ❌ 弹幕图标长按 → 弹幕操作面板（`danmaku.js` 存在，但没有"长按"证据） | `PlayerTopBar.kt:123-128` 长按 `danmaku` → `onAction("danmaku-panel")`；代码自注"◐ 待 iPhone 补证" | 入口是推断 | 若竞品无此入口，长按会"多出一个面板"（用户误触概率低但存在） | 保留实现，但在 `PlayerTokens`/`PlayerActionRouter` 注释里统一标 ◐；真机验证后二选一（删除或改成与竞品一致的入口） | P3（待真机确认） |
| TOP-07 | ◐ 激活态用"白色反白/图形变化"，非绿色高亮 | `PlayerTokens.kt:78` `activeAccent=0xFF34C759`，`PlayerTopBar.kt:197` `active → activeAccent` | 竞品截图中唯一可见"激活"是听视频的**白圆反白**（0172）；无任何绿色图标证据 | 打开弹幕/听视频以外的动作会闪 1.5s 绿色（`PlayerActionRouter.kt:404-407`），手机上绿点是"成功提示"语汇，与竞品克制的白/灰体系冲突 | 把 `activeAccent` 的用途收敛为"仅听视频白圆 + 源卡绿点"；顶栏激活改为 `alpha 1.0 + 白`（`PlayerTopBar.kt:193-201`） | P2（推断项） |

---

## 三、底栏与进度条（BOT）

### 3.1 对齐确认

| 项 | 竞品 | 实现 | 判定 |
| --- | --- | --- | --- |
| 三段 | ✅ 已播纯白 / 缓冲亮白 / 未播暗（0172 纯黑底上三段可数） | `PlayerSeekTrack.kt:209-248`：未播 `0x1F` + 描边 `0x59`；缓冲 `0x8C` + 描边 `0xBF`；已播纯白 | ✅ |
| 半透明 + 1px 提亮描边 | ✅ 轨道能透出画面 | `PlayerSeekTrack.kt:207` `rim = 1.dp`、`:216-222`/`:232-238` `Stroke(rim)` | ✅ |
| 厚度 6pt | ✅ | `trackHeight=6`（`PlayerTokens.kt:140`） | ✅（但见 TOK-07 的 10pt 增强） |
| **无滑块** | ✅ 5 张截图均无圆点 | 全程无 thumb 绘制 | ✅ |
| 时间位置 | ✅ 左"06:52"在轨道同一行前 11pt、右"45:28"后 4pt（0160/0170/0171） | `PlayerBottomBar.kt:125-148` `TimeText` + `trackStartGap=11` + `trackEndGap=4` | ✅ |
| 按钮组 pitch 65 | ✅ ⏸ 心 444px→下一集 1052px? 实测 0170：⏸ 心≈130、下一集≈248（1500px 宽）⇒ 66pt | `PlayerBottomBar.kt:194-196` `spacedBy(65-44=21)` + 两枚 44 盒 | ✅ |
| 右侧三文本钮 | ✅ `4K` `选集` `倍速`（0165/0170/0171/0172/0173 五张一致） | `PlayerBottomBar.kt:215-235` | ✅ |
| 文本钮间距 | ◐ 37–38 ink 间隙 | `textButtonGap=29` + 两侧 `textButtonPad=4` | ✅ |
| 行高封顶 24、命中盒溢出 | ◐ 竞品按钮行 24pt ink、可按 44 | `PlayerBottomBar.kt:189` `.height(24)` + `:249` `size(44)` | ✅（v2 文字裁切已修） |
| 字形下缘不再被裁 | — | `PlayerBarText.kt:53,80` `overflow=Visible` | ✅ |

### 3.2 差距

| 编号 | 竞品契约（含等级） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| BOT-01 | ✅ 选集整替态**轨道行仍然存在**（0170：`06:51` / 轨道 / `45:28` 都在，只是整体被压暗；按钮行被选集带替代） | `PlayerBottomBar.kt:97-108` `if (expandedPanel == "episodes") { PlayerEpisodeStrip(...); return@Box }` —— **提前返回**，整个 `Column`（含轨道行与时间）不组合 | 选集态把进度条整条摘掉 | 进选集再想拖进度**做不到**，必须先点画面退出选集（多一步，且用户会先试拖轨道发现没反应）；且轨道消失导致"当前播到哪"失去唯一就地指示 | 把 `Column` 拆成两段：①`TrackRow`（时间+轨道，`:121-149`）②`ContentRow`（按钮行 or 资源 or 选集带）。episodes 分支改为"保留 ①，用 ② 位置渲染选集带"，并把底栏渐变高度改由内容实测（`onSizeChanged`）。选集态可选加 `alpha 0.5` 压暗（竞品观感） | P1 |
| BOT-02 | ✅ 选集态背景＝画面 + 黑渐变（0170 底部能看见水面与树） | `PlayerBottomBar.kt:79-95` 渐变覆盖全部三态；episodes 时 `.height(200.dp)`（裸值），无"模糊/压暗画面" | 选集带背景是纯黑渐变，与竞品"画面仍可辨识"不同 | 选集态像"切到另一个页面"，失去"还在这个视频里"的上下文 | episodes 分支沿用 `bottomScrimHeight`（110）并把选集带放进 `Scrim + 内容`，或对下方 60% 做 `Modifier.blur(20.dp)`（API31+，低版本回退为加深渐变）。同时把 200/210 两处裸值改 token | P2 |
| BOT-03 | ✅ 无滑块、6pt 恒定 | 见 TOK-07（`trackActiveHeight=10`） | 自创加粗（已在 TOK-07 记） | 拖动时轨道变粗 | 同 TOK-07 | P2 |
| BOT-04 | ✅ 进度条命中区"细轨也要好按"（推断 ◐，M03-S06 拖动） | `PlayerSeekTrack.kt:122` `height(trackHitHeight=24)`；`:126-141` tap 直接 seek；`:142-158` drag | 命中 24pt < 44pt 无障碍下限；tap 与 drag 两套 pointerInput 并存 | 手指略偏就拖不到轨道（尤其横屏单手）；**点击轨道 = 立即跳转**，而竞品语义是"拖动预览后才跳"（M03-S06 与 `thumbnail.enabled` 耦合）⇒ 误触即丢进度 | ① 命中区提到 44（`trackHitHeight=44`，Canvas 仍在中心画 6pt）；② tap 改为"先出预览浮窗，第二次点/拖动才提交"，或保持 tap 但要求 `positionMs` 恢复入口（`seekTo` 已支持）——建议按竞品语义改为"点=预览定位点、拖=跳" | P2 |
| BOT-05 | ◐ 三段色在亮画面上仍可辨 | `PlayerTokens.kt:31` `trackRemaining=0x1F`（白 12%）；`PlayerSeekTrack.kt:211` 拖动时 25% | 未播段在**亮画面**（雪景/白墙）几乎不可见；竞品 0172 是纯黑底，白 12% 足够，但亮底场景无证据 | 浅色画面里用户看不出"总长"，只能靠右侧时间文本 | 给轨道加"底衬"：`drawRoundRect(color = Color(0x33000000), ...)` 先画一层（黑 20%），再画三段（网飞/哔哩通行做法）；或未播段改为 20% 白 + 黑 25% 描边内衬 | P2（待真机确认：需亮场景截图） |
| BOT-06 | ✅ 时间标签 13pt 等宽 semibold | `PlayerTokens.kt:262` `timeSize=13`；`PlayerBarText.kt:39-55` `Monospace + SemiBold` | 竞品数字形态偏 SF Mono（0 有斜杠？0160 看不清） | 无可指认差异 | 保留 | — |
| BOT-07 | ✅ 暂停态左下浮相机钮（48 圆，中心≈(88,195)pt） | `PlayerOverlay.kt:204-209` 仅在 `barsVisible && !isPlaying && !audioOnly && showCaptureButton` 时 `align(CenterStart)`；`PlayerCaptureButton` 用 `captureFloatStart=17` | 位置为 `CenterStart` + token 推的左距，无 `y` 位置控制 ⇒ 实际落在**屏幕垂直中心偏上**，竞品是"暂停态左侧中部"（推断自 §1.5 描述，截图未覆盖） | 截图未覆盖，暂无可指认差异 | 待真机确认后再定；若按描述需 `Alignment.CenterStart + offset(y = -…)` | P3（待真机确认） |
| BOT-08 | ✅ 面板打开时底栏文字不被遮（0165/0171/0173：`4K/选集/倍速` 完整可见） | `PlayerPanelOcclusion.kt:34-58` `hideWhenCoveredByPanel` 把被面板矩形覆盖的**文字像素裁掉**；`PlayerBottomBar.kt:147,219-234` 右侧时间与三个文本钮都挂了它 | 竞品是"面板不覆盖底栏"，我们是"覆盖了就裁掉" | 面板右缘压到"倍速"时，用户看到**半个"倍速"**（跨边时只画外面那截，`PlayerPanelOcclusion.kt:27-29` 注释已承认）；竞品该处是完整的"倍速" | 根因在 PNL-01（锚点/尺寸）与 PNL-02（skip 卡 300 宽）；修好后此裁剪只作兜底。另把"跨边只画一截"改为"跨边整块隐去"（视觉更干净） | P1（依赖 PNL-01） |
| BOT-09 | —（内部一致性） | `PlayerBottomBar.kt:76` `dragTargetMs` 只在拖动时非空；`:125` 左侧时间随之变为拖动目标；`:143-147` 右侧剩余时间同样 | 两侧时间在拖动时同时变，看起来"总长也变了"（右侧是"-剩余"），用户会误以为片长变了 | 拖动时右端跳成 `-00:12`（若 `showRemaining` 开），与竞品 0160/0170 右端恒为总长 `45:28` | `showRemaining` 时右端仍显示总长（不随拖动变），把"剩余"语义移到左侧或独立小字；`PlayerBottomBar.kt:143-148` | P2 |
| BOT-10 | —（可点区一致性） | `BarIconButton.kt:240-266` 与 `TopIconButton` 两个实现；`BarTextButton` 命中盒＝文本+`textButtonPad`（高约 14×1.25+10≈27pt） | 底栏文本钮命中高约 27pt < 44pt；竞品"可按"要求 44 | 横屏单手点"倍速"容易点空（尤其拇指从下方伸） | `BarTextButton` 外套 `Box(Modifier.size(width = 0, height = 44).heightIn(min = 44))` 纵向补足（不改 ink），或统一走 `PlayerHitTarget`（见 DUT-05） | P2 |

---

## 四、面板（PNL）

### 4.1 对齐确认

| 项 | 竞品 | 实现 | 判定 |
| --- | --- | --- | --- |
| 宽度 248 | ✅ 三张截图均 ≈440px / 1500px = 29.3% × 844 = 247pt | `panelWidth=248`（`PlayerTokens.kt:195`）、`PlayerGlassMenu.kt:54` | ✅ |
| 圆角 16 | ✅ 0171/0173 圆角 ≈28px≈16pt | `panelRadius=16`、`PlayerGlassSurface.kt:28` | ✅ |
| 行高 42 / 双行 60 | ✅ 0171 倍速行距 75px≈42pt；0165 双行 107px≈60pt | `PlayerTokens.kt:200-201`、`PlayerGlassMenu.kt:160-166` | ✅ |
| 分区标题 12pt 灰 | ✅ 0160"操作"、0173"填充模式" | `sectionHeadSize=12`、`PlayerGlassMenu.kt:80-90` | ✅（但见 PNL-05） |
| 滚动条 3pt 白 40% | ✅ 0171 右侧细白条 | `scrollbarWidth=3`、`scrollbarInset=6`、`PlayerGlassMenu.kt:267-283` | ✅ 宽度/颜色/位置 |
| 无遮罩、点外部关闭 | ✅ 面板外画面清晰可点 | 无 scrim；`PlayerOverlay.kt:144-150` `panelOpen → onDismissPanel()`；面板内 `PlayerGlassMenu.kt:63` 吃点击 | ✅ |
| 与顶栏图标的锚点关系 | ✅ 面板右缘 ≈ 图标右缘 + 8~12 | `PlayerPanelHost.kt:16-24` 注释与公式正确，`trailingPad` 对 5 个 kind 正确 | ✅（5 个），❌（其余，见 TOP-02） |

### 4.2 差距

| 编号 | 竞品契约（含等级） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| PNL-01 | ✅ 面板锚在**触发控件**上：顶栏图标 → 从图标下方展开（0160/0165/0173 面板顶边 y≈70px≈39pt，紧贴顶栏下沿）；底栏"倍速" → 从**按钮上方**展开（0171 面板底边 y≈640px≈360pt，按钮行 y≈628px） | `PlayerOverlayPanel.kt:63-69` `Modifier.align(Alignment.TopEnd)` + `top = PlayerPanelHost.topPad(shownKind)`（43 或 16）；`PlayerPanelHost.kt:43` | **所有面板统一出现在右上角**，与触发控件脱钩；`speed` 面板（`trailingPad=16`）出现在屏幕**顶部** 43pt 处，而竞品在屏幕**底部**按钮上方 | 点底栏"倍速"后，面板出现在屏幕另一端（右上），用户视线要从右下跳到右上；同时面板遮住片名与顶栏（竞品不遮）。这是本次审计**最明显的观感差异** | 1) `PlayerPanelHost` 增锚点枚举 `enum class PanelAnchor { TOP_END, BOTTOM_END }` + `fun anchor(kind) = if (kind in BOTTOM_KINDS) BOTTOM_END else TOP_END`（`BOTTOM_KINDS = {"speed"}`）；2) `FloatingPanel` 的 `align`/`padding` 改为按 anchor 分支：`BOTTOM_END → align(BottomEnd) + padding(bottom = bottomPad + buttonRowHeight + 8)`；3) 更彻底：由触发控件通过 `onGloballyPositioned` 发布 `Rect`（`PlayerBarsSlot` 里顶栏图标与底栏文本钮各自上报），`FloatingPanel` 用 `offset` 定位——与"面板遮挡裁剪"共用同一套矩形，去掉 hardcode 的 43/16 | **P0** |
| PNL-02 | ✅ 跳过面板宽 ≈248（0165：面板 x 705–1145 = 440px = 247pt） | `PlayerPanelHost.kt:68` `width = if (kind == "skip") 300.dp else PlayerTokens.detailCardWidth.dp` | skip 被改成 300dp（宽 18%），**竞品是标准 248**；其余 detail 卡 357dp 无截图证据 | skip 面板比同族面板宽一截，视觉"胖"且右缘溢出更多（进一步压住底栏"选集"） | 删除 300 特例，`skip` 一律用 `panelWidth`；357 保留给 `audioDelay`/`subtitleDisplay`/`danmakuSettings`（这三类无截图，标 ◐） | P1 |
| PNL-03 | ✅ 顶栏图标面板顶边≈顶栏下沿（39–43pt） | `topPad = if (isDetail) 16 else 43`（`PlayerPanelHost.kt:43`） | detail 壳从 16pt 开始，会**盖住顶栏图标行**（0–77pt 区域） | 打开"音频延迟/字幕显示/弹幕显示"时顶栏被面板压住，无法一边看图标一边调参数 | detail 壳的锚点改为"触发图标下沿 + 8"（依赖 TOP-02 的图标矩形发布）；或临时改为 `topPad = 77 + 8` | P1 |
| PNL-04 | ✅ 0171 倍速面板**无标题行**（第一行直接 `2x`） | `PlayerPanelItems.kt:57` `panelTitle("speed")="倍速"`；`PlayerGlassMenu.kt:71-73` 有标题就画 `MenuHeader`（16pt 白，`panelTitleSize`） | 多一行 16pt 标题，7 档倍速 + 标题 = 8 行，超出 `panelMaxHeight=330` ⇒ 触发滚动（竞品 7 行整屏可见，0171 也无滚动挤压） | 倍速面板需要滚动才能看到 0.5x（竞品一屏全见），且顶部多一行"倍速"冗余 | `panelTitle("speed")` 返回 `""`（或 `PlayerPanelHost` 给 `{"speed","quality"}` 置 `showHeader=false`）；同时把 7 档行高合计（7×42=294）与 `panelMaxHeight` 校核 | P2 |
| PNL-05 | ✅ 0173 顶部"填充模式"是 **12pt 灰 section 头**，无 16pt 白标题 | `PlayerPanelForms`/`PlayerPanelData.qualities` 目前无 section；`PlayerGlassMenu` 统一 16pt 标题 | 面板标题层级与竞品不同（我们用 16pt 白题，竞品用 12pt 灰分区名） | 亮度层级不匹配：用户看到的最大字是"画质比例"，而竞品最大字是行标题"适应屏幕" | `PlayerOptionItem` 的 `section` 用于"填充模式"；`panelTitle("quality")` 返回 `""`；把 `MenuHeader` 仅在 `subtitle` 非空时使用（与 `skipMode` 的"跳过片段方式 + 跟随默认 + ⌄"一致） | P2 |
| PNL-06 | ✅ 滚动条长度反映可视比例（0171 滚动条 ≈300px / 面板 ≈570px = 53%） | `PlayerGlassMenu.kt:273` `.height(72.dp)` + `:275` `thumbH = size.height * 0.4f` ⇒ thumb 恒 28.8dp，轨道恒 72dp | **写死 72dp**，与 `scroll.maxValue` 无关；注释（`:265` "高度按可视比例"）与实现不符 | 长列表（字幕 6 行、8 档倍速）里滚动条只有 29pt 长、只走 43pt 行程 ⇒ 用户看不出还有多少内容、拖动错觉 | `MenuScrollbar` 改为 `BoxWithConstraints`：`trackH = maxHeight`、`thumbH = trackH * (viewport / (viewport + scroll.maxValue))`，并让 thumb 随时间淡出（compose 的 `ScrollState.isScrollInProgress`） | P2 |
| PNL-07 | ✅ 0173"填充模式"面板含 **`避开灵动岛`** 独立行（Android 对应"避开挖孔"） | `PlayerWindow.kt:52-55` 无条件 `LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES`（画进挖孔）；无开关、无面板行 | 能力**缺失**，且当前是"总是画进挖孔"的固定行为 | 挖孔机横屏时画面内容可能被摄像头挡住；用户没有任何办法切换 | `PlaybackPrefs` 增 `avoidCutout: Boolean`（默认 true 与非挖孔机无关）；`PlayerWindow.applyImmersivePlayerWindow` 读它决定 `SHORT_EDGES`/`NEVER`；`PlayerPanelData` 的填充模式面板增一行 Toggle"避开挖孔" | P1 |
| PNL-08 | ✅ 填充模式三选：`适应屏幕` / `填满屏幕` / `拉伸填满`（0173，均 ✅ 文案） | `PlayerPanelData.qualities()` 产出的是 `video-aspect-override` 纵横比档（`PlayerPanelRouter.kt:276`）；无 fill/cover/stretch 三态 | 语义档位不同：我们用"比例"，竞品用"填充模式 + 比例钻入" | 用户找不到"填满/拉伸"（黑边无法消除），只能试各种比例 | `PlayerPanelData` 增 `fillModes()`（写入 mpv `panscan` / `video-aspect-override`+`video-zoom` 或 `keepaspect`/`video-unscaled` 组合，按 Kivo 内核实际可用项）；`PlayerPanelItems` 的 `"quality"` → `"fill"`，`fill` 内末行 `chevron → "quality"`（钻入比例列表） | P1 |
| PNL-09 | ✅ 面板背景是**强背景模糊**（0160/0171：面板后画面明显糊化，但轮廓可辨） | `PlayerGlassSurface.kt:7-15` 注释明确"Android 做不到模糊背后视频"，用"黑 75% + 白 8% 描边 + 顶部高光"代替 | 无模糊；`panelSurface=0xBF`（75%）在亮画面上仍然透出细节 | 面板放在亮画面（雪景/白墙/字幕区）上时，白字与背景细节叠在一起，可读性下降（用户第一反应是"字看不清"） | 低成本：`panelSurface` 提到 `0xD9FFFFFF`→不，应为 `0xD9000000`（85%）并在 `playerGlassSurface` 内加一层 `drawRoundRect(black 0.1f)` 内阴影；高成本：对视频层做 `RenderEffect`（需架构改动，标 ◐ 不做） | P2 |
| PNL-10 | ✅ 0173 面板底部"...`画面比例 >`" 为**钻入行**，右缘 12pt chevron | `PlayerGlassMenu.kt:243-249` `CHEVRON → chevron-right 12`；`PlayerPanelRouter.kt:23-26` `DRILL_KINDS` 不含 `"quality"`（从 fill 钻入需要新增） | 钻入机制已具备，但 `fill → quality` 关系未登记 | 新面板做完后若不加 `DRILL_KINDS`，点行会走 `writeSelection` 失败并 toast"内核未就绪" | `PlayerPanelRouter.kt:23-26` `DRILL_KINDS += "quality"`；同时 `PlayerPanelItems.panelTitle("quality")="画面比例"` | P1（随 PNL-08） |
| PNL-11 | ◐ 面板 header 的 `⌄` 表示"可收起/原地换内容"（0165 有 ⌄，0173 无） | `PlayerGlassMenu.kt:134-140` 仅 `subtitle.isNotBlank()` 时画 `chevron-down` | 字幕面板（`subtitle` 无副题）没有 ⌄，跳过多行有 ⌄ | 有无 ⌄ 取决于副题，用户看到的是"随机出现的小箭头" | 把 `chevron-down` 与 `subtitle` 解耦：有钻入/可换内容的面板统一画（由 `PlayerPanelHost.canSwap(kind)` 决定） | P3 |

---

## 五、手势与锁定（GST）

### 5.1 对齐确认（竞品 §3.M03 + §5 铁律 3）

| 项 | 竞品 | 实现 | 判定 |
| --- | --- | --- | --- |
| 双击三分区 | ✅ 左/中/右三热区各可配动作 | `PlayerGestureLayer.kt:128-136`：`x<0.34w` 退、`x>0.66w` 进、其余播放暂停 | ✅ |
| 双击识别窗 0.3s | ◐ 系统 `numberOfTapsRequired=2` 容差 | Compose `detectTapGestures` 默认 300ms；单/双击共存由框架保证（`PlayerGestureLayer.kt:34-36` 注释） | ✅ |
| 双击图标闪现 0.5s | ◐ α 0→1→0，d=0.5 | `PlayerGestureFlash.kt:38-46` 500ms | ✅（但见 GST-02） |
| 长按 ≥0.5s | ◐ 系统长按阈值 | `detectDragGesturesAfterLongPress`（默认 `longPressTimeout=500`） | ✅ |
| 长按进入倍速 | ✅ `longPressDefault` 可配 | `PlayerGestureRouter.kt:75-82` 用 `longPressBaseSpeed`（默认 2.0） | ✅ |
| 左右滑调速 | ✅ `longPressPan` | `PlayerGestureRouter.kt:102-108` 320px/倍速、夹在 0.5–3.0 | ✅ 链路（参数自创，见 GST-03） |
| 上滑锁定 Δy≈-60 | ◐ 手势阈值族 | `PlayerComposeActivity.kt:221` `LOCK_UP_THRESHOLD_DP=60`；`PlayerGestureRouter.kt:92-98` | ✅ |
| 松手恢复两策略 | ✅ `longPressEnd.normal/.current` | `PlayerGestureRouter.kt:112-116` `previous`/`keep` | ✅ |
| 锁定=长按第 1 次触觉 + 锁定再 1 次 | ◐ G2 | `PlayerGestureRouter.kt:83`（LONG_PRESS）、`:96`（VIRTUAL_KEY） | ✅ |
| 上下滑左右半屏 | ✅ 全屏上下滑（哪侧是音量未定 ❌） | `PlayerGestureRouter.kt:16-35` 左亮度/右音量，量程屏高 55% | ✅（分区为 ◐，见 GST-05） |
| HUD 0.15 淡入 / 停 1.0s / 0.3 淡出 | ◐ | `PlayerMotion.kt:43-45` + `PlayerGestureHud.kt:72-77` + `PlayerGestureRouter.kt:49-54` | ✅ **逐值一致** |
| 锁定矩阵 | ✅ 长按✅ 双击✅ 竖滑❌ 双指❌ 调速/上滑❌ | `PlayerGestureLayer.kt:101-108,115-122`（locked 拦横/竖滑）；`PlayerGestureRouter.kt:71,90`（锁定时长按仍进、调速/上滑 return） | ✅ **完全一致** |
| 触觉三态 + 低电量 | ✅ 自动/禁用(+启用)，低电量自动关 | `AppearancePrefs.kt:61-79`（`always/disabled/auto` + `EXTRA_BATTERY_LOW`） | ✅ |
| 亮度必须还原系统值 | ✅ `independentBrightness` 退出/回桌面恢复 | `PlayerGestureRouter.kt:154-160` 用**窗口级** `screenBrightness`（天然随窗口销毁失效） | ✅ 更省事且正确 |

### 5.2 差距

| 编号 | 竞品契约（含等级） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| GST-01 | ✅ `doubleTap.sides` / `doubleTap.center` **各自可配动作**（seek / toggle） | `PlaybackPrefs.kt:120-134` 只有 `doubleTapCenterAction: playpause|none`、`doubleTapSidesAction: seek|none`；`PlayerActionRouter.kt:373-385` 按 zone 二选一 | 可配项是"开/关"，竞品是"两个动作二选一" | 无法把双击右侧改成"播放暂停"（或把中间改成 seek），与竞品设置项不对齐 | `PlaybackPrefs` 两个字段放开取值 `{"seek","playpause","none"}`；`onDoubleTap` 的 `when(zone)` 改为先读动作再分派（`PlayerActionRouter.kt:371-386`） | P2 |
| GST-02 | ◐ 图标闪现 d=0.5 单一节奏 | `PlayerActionRouter.kt:379-380` `flashZone = zone` + `Handler.postDelayed({ flashZone = null }, 600)`；`PlayerGestureFlash.kt:44` 另有 `delay(500)` | 两套计时（600 / 500），且 Handler 回调**未在 onDestroy remove**（`PlayerComposeActivity.kt:362-382` 只 remove 两个 poll） | 闪示消失时刻由内层 500ms 决定，state 630ms 才清；极短间隔连点两次时第二次可能被前一次的 600ms 清掉（少闪一次） | 删除 Handler，`flashZone` 由 `PlayerGestureFlash` 的 `LaunchedEffect(zone)` 生命周期管理；或反过来只留 Handler 并 remove。二选一，单来源 | P2 |
| GST-03 | ✅ "（可选）速度条"（M03-S03 参数表：作用对象含速度条） | `PlayerLongPressHud.kt:44-70` 只有"倍速数字 + 可选锁图标"；无速度刻度 | 缺速度条；且调速步长 320px 无参考 | 左右滑时只有数字在跳，用户不知道滑到哪里是上限、也不知道"再滑一点就 3x" | `PlayerLongPressHud` 增一条 3pt 刻度轨（0.5–3.0 映射，当前值圆点），数据来自 `longPressSpeed`（已有）；`PlayerOverlay.kt:228-232` 传参 | P2 |
| GST-04 | ✅ 亮度/音量 HUD 数值 + 线性映射 | `PlayerGestureHud.kt` 数值 + 3pt 细进度线 ✅；`PlayerGestureRouter.kt:32` 量程=屏高 55% | 量程 55% 无依据（竞品未给）；HUD 位置 `TopCenter + topBarHeight+14`（`PlayerOverlay.kt:241-243`） | 长屏设备要滑半个屏高才到 100%，短屏则一滑到底（手感不一致）；位置高于竞品（截图未覆盖） | 把 `0.55f` 提为 `PlayerTokens.hudDragSpanRatio`（默认 0.55）并在设置页可调；位置待真机确认后校准 | P3（待真机确认） |
| GST-05 | ✅ `verticalPan` = 全屏上下滑调音量与亮度；❌ 哪侧是音量未定 | `PlayerGestureRouter.kt:18-24`：`verticalSwipeEnabled` 关 → 竖滑完全不接管；`independentBrightness` 关 → **左半屏直接 return**（不弹 HUD、不改值、无提示） | 关闭开关后是**静默失效**（违反"不许静默降级"铁律） | 用户关掉"单独调整亮度"后左半屏怎么滑都没反应，会以为播放器卡了/手势坏了 | 二选一：① 关闭独立亮度时**仍改系统亮度**（`Settings.System.SCREEN_BRIGHTNESS`，需 `WRITE_SETTINGS` 或改窗口亮度并提示"跟随系统"）；② 至少弹一次非空 toast "亮度调节已关闭（设置 › 播放 › 单独调整亮度）"。改 `onVerticalDragStart`（`:23`） | P1 |
| GST-06 | ✅ `pinch` = 双指缩放画面；范围 ◐ 1.0–2.0；松手回弹 ❌ | 全仓无 `detectTransformGestures`/`pinch`/`scaleGesture`（已 grep） | **能力完全缺失** | 用户无法放大画面看细节（字幕、画面边缘信息），在竞品对照/试机时是"少一个手势"的硬空白 | `PlayerGestureLayer` 增第三个 `pointerInput { detectTransformGestures { _, pan, zoom, _ -> ... } }`，只上报 `zoom`；`PlayerOverlay` 增 `zoom: Float` 参数，作用在 `PlayerScreen` 的视频容器（`graphicsLayer { scaleX/Y = zoom }`，**不动 SurfaceView 本身**，零拷贝路径不受影响）；缩放范围 1.0–2.0，双击/切集时归 1.0 | P1 |
| GST-07 | ✅ `gestures.locked` = "锁定时允许长按和双击" —— 产品固定行为 | `PlaybackPrefs.kt:150-153` `lockAllowGestures` **默认 true 且暴露给机制**；`PlayerGestureRouter.kt:71`/`PlayerActionRouter.kt:372` 用它门控 | 竞品是固定行为，我们做成了可关闭设置 | 关掉该开关后锁定态双击/长按全失效，与竞品确定文案直接矛盾（用户对照时会认为我们行为相反） | 删除该设置项（设置页对应行），`onDoubleTap`/`onLongPressStart` 不再读它（保持 true 语义）；或改名"锁定态允许手势（实验）"并默认 hidden | P1 |
| GST-08 | ✅ `followRotation` / `autoLandscapeRotation` 两个独立开关；旋转 r=90/180/270 d=0.3 easeInOut（◐） | `AndroidManifest.xml:43` `screenOrientation="landscape"`；无旋转相关代码 | 固定横屏，无"跟随设备/双横屏方向切换" | 竖持手机时仍是横屏画面（需手动转机）；平板上无法用竖屏看竖片 | 短期：`screenOrientation="userLandscape"`（API 18+，允许两个横屏方向）；完整：`PlaybackPrefs.followRotation` + `requestedOrientation = FULL_SENSOR`/`LANDSCAPE`，并在旋转时保留播放（`configChanges` 已含 `orientation|screenSize`，无需重建） | P2 |
| GST-09 | ◐ 横滑/竖滑互斥"按主方向" | `PlayerGestureLayer.kt:99-124` 两个独立 `pointerInput`（水平与垂直各一），谁先过 slop 谁消费 | 方向判定依赖两个 detector 的竞争顺序（Compose 中同一节点多个 pointerInput 按声明顺序参与），斜向 45° 时结果不稳定 | 用户斜着滑一下，可能"想调音量却变成 seek"（尤其右手拇指自然弧线） | 合并成一个 `pointerInput` + `awaitPointerEventScope`：累计 dx/dy，`|dx|>|dy|*1.2` 判横、否则竖，再分派——单一决策点，可写单测 | P2 |
| GST-10 | ✅ 横滑 seek 属竞品**无确定项**（M03-S06 只确定拖动预览与换集过渡） | `PlayerGestureRouter.kt:214-278` 全屏横滑 seek（时长的 10%、夹 15–300s、150ms 实时 seek + 松手精确 seek） | 这是**超出竞品的自创增强**，且与"双击 seek""拖进度条"构成第三条 seek 入口 | 不构成差距；但三条入口的节流/落点策略不同（`onSeek` 走 landing 等待，横滑走实时 seek）⇒ 用户在不同入口感到"跳转快慢不一样" | 报告层面标注为自创；技术上把三条入口收敛到 `seekTo(ms, mode)`（`PlayerActionRouter.kt:47`）已有唯一入口，建议补 `SeekOrigin` 参数统一节流与落点策略 | P3（标注项） |
| GST-11 | ❌ 锁定入口未知（文案只有 `gestures.locked`） | `PlayerActionRouter.kt:297-313` 长按播放键加锁；`PlayerLockLayer.kt:27-31` 注释自认 ◐ | 推断入口 | 用户按竞品习惯找不到加锁 | 待真机确认（长按播放键 / 长按画面 / 顶栏某图标三选一）；确认后只改 `PlayerComposeRoot.kt:122` 一处接线 | P3（待真机确认） |

---

## 六、动效治理（MOT）

### 6.1 对齐确认

| 项 | 竞品 | 实现 | 判定 |
| --- | --- | --- | --- |
| 控制条 180 / 140 | ◐ 短状态提示族 | `PlayerMotion.kt:27-28` | ✅ |
| 面板 spring(0.25,0.8) + 0.94 缩放 / 150 easeOut | ◐ | `PlayerMotion.kt:31-33`、`PlayerOverlayPanel.kt:57-62` | ✅ |
| 面板钻入 150 / 120 交叉淡化 | ◐ | `PlayerOverlayPanel.kt:74-77` | ✅ |
| 轻提示 200 / 250（停留 1.0–2.0） | ✅ 文案 / ◐ 参数（M08-03） | `PlayerToast` + `PlayerMotion.kt:36-37`；`PlayerActionRouter.kt:398` 默认 1800ms | ✅ |
| 轨道过渡 150 | ◐ | `PlayerMotion.kt:38` | ✅ |
| 预览浮窗 150/150 + 0.92 | ◐（M03-S06 α 0.15） | `PlayerMotion.kt:40-41`、`PlayerSeekTrack.kt:160-166` | ✅ |
| HUD 150 / 1000 / 300 | ◐（m03-S03 0.15 / 1.0 / 0.3） | `PlayerMotion.kt:43-45` + `PlayerGestureRouter.kt:49-54` | ✅ **逐值一致** |
| 长按标签 200 / 150 + 0.9 | ◐（0.2 / 0.15，s 0.9→1） | `PlayerLongPressHud.kt:37` | ✅ |
| 双击闪示 500 | ◐ | `PlayerGestureFlash.kt:44` | ✅ |
| 触觉与视觉解耦 | ✅ G2 | `AppearancePrefs.hapticAllowed` 只影响 `performHapticFeedback`，视觉不依赖 | ✅ |
| 动效常量唯一源 | 工程契约 | `PlayerMotion.kt`（49 行，全页引用） | ✅ |

### 6.2 差距

| 编号 | 竞品契约（含等级） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| MOT-01 | ✅ **G1**：App 内有显式"减弱动画效果"开关，且**每个动效**都必须有降级分支 | `SettingsAppearanceSub.kt:51-52` `SwitchRow("减弱动画效果", false, enabled = false, subtitle = "未接全局动效门，各页动效参数独立。")`；`PlayerPanelHost.kt:88-94` `rememberReduceMotion()` 只读系统 `ANIMATOR_DURATION_SCALE==0` | **App 内开关是灰行（未实现）**；系统级只在 `scale==0` 时生效（=播放器只认"系统关动画"，不认"系统减弱动画"的部分档位） | 用户在设置里看到该行是灰的、点了没用；无障碍用户（ vestibular 敏感）无法关闭播放器动效；与竞品"确定有开关"直接不符 | ① `AppearancePrefs` 增 `reduceMotion by mutableStateOf(false)` + `updateReduceMotion`；② 开启 `SettingsAppearanceSub` 该行；③ `PlayerPanelHost.rememberReduceMotion()` 改为 `AppearancePrefs.reduceMotion || systemScale==0`；④ 通过 `CompositionLocalProvider(LocalReduceMotion provides …)` 下发到各壳（见 MOT-02） | **P0** |
| MOT-02 | ✅ G1：位移/缩放/旋转归零，仅保留 α 或瞬时 | 降级只覆盖 2 处：`PlayerBarsHost.BarSlot`、`PlayerOverlayPanel`（fadeIn/Out）；**未覆盖**：`PlayerLongPressHud.kt:38-39`（0.9 scaleIn）、`PlayerSeekPreview` 经 `PlayerSeekTrack.kt:162-165`（0.92 scaleIn）、`PlayerSeekTrack.kt:79-83`（6→10 粗）、`PlayerGestureFlash.kt:53-58`（0.86→1 scale）、`PlayerStatusLayer.kt:156-203`（三点无限缩放循环）、`PlayerBottomBar`/`PlayerEpisodeStrip` 无过渡（OK） | 降级不完整：开"减弱动画"后仍有缩放/呼吸/变粗 | 用户开了减弱动画，画面里仍有 3 处缩放 + 1 处循环呼吸，**降级承诺落空**（无障碍场景下这比"没有开关"更糟：用户以为已关） | 用 `CompositionLocal` 统一：新增 `val LocalReduceMotion = compositionLocalOf { false }`（`PlayerMotion.kt`）；`PlayerOverlay` 提供值；`LongPressHud`/`SeekTrack`/`GestureFlash`/`StatusLayer` 读它，`if (reduceMotion)` → 仅 `fadeIn/out`、`scaleIn(1f)` 或静态点 | P1 |
| MOT-03 | ✅ G3 / M01-S08：`startupAnimation` 开→角色动画、**关→系统 spinner**（文案确定"关闭后显示加载圈"） | `PlayerStatusLayer.kt:156-203` 只有自创"三点波纹"；`PlayerPhase.Buffering` 是唯一加载态 | **只有一套表现，没有回退态**；也没有开关 | 用户无法在"轻量"与"角色动画"间选择；弱机上三点波纹仍在跑无限动画（与 G6 性能预算冲突） | `PlaybackPrefs` 增 `startupAnimation: Boolean`（默认 true）；`PlayerStatusLayer.Buffering` 分成两支：`true → 三角色位（或保留三点）`、`false → CircularProgressIndicator(24dp, 白)`；`false` 时按 G1 不缩放 | P1 |
| MOT-04 | ✅ G4：集间"平滑过渡"开关；关→硬切、开→预加载后交叉溶解（d=0.3–0.5 easeInOut ◐） | `PlayerEpisodeSwitch.kt:23-64` 内核 `loadfile` 无缝换源（本 Activity 内），**无过渡层、无开关** | 缺过渡与开关；`AutoNextEpisode.kt:65-80` 只是"无缝 or 重启 Activity"二选一 | 换集瞬间直接切画面（硬切），竞品开启时是交叉溶解；用户设置里也找不到"平滑过渡" | `PlaybackPrefs` 增 `smoothTransition`；`PlayerEpisodeSwitch.switchEpisodeImmediately` 增加 `crossfade: Boolean`：在 `startUrlPlayback` 前把当前帧 `Bitmap` 贴到 overlay（`PlayerSeekPreview` 已有的 `screenshot-to-file` 能力可复用），新首帧后 300ms 淡出；关闭时保持现状 | P1 |
| MOT-05 | ✅ G4/G5 + `player.episode.transition_failed` = "暂时无法切换剧集"（**确定**文案） | `AutoNextEpisode.kt:80-86` 失败只 `NetDiag.log`；`PlayerEpisodeSwitch.kt:31-63` 返回 false 时静默 | **失败态缺失**（连 toast 都没有） | 连播失败时用户看到"播放结束"面板，不知道"是没下一集还是切集失败"；竞品有明确失败文案 | `AutoNextEpisode` 的失败分支 + `switchEpisodeImmediately` 返回 false 时调 `a.showToast("暂时无法切换剧集")`（`PlayerActionRouter.showToast` 已存在）；`PlayerPanelRouter.kt:74` 的"该集信息已失效"保持 | P1 |
| MOT-06 | ✅ G5：过渡依赖预加载就绪，网络慢时推迟/取消 → 需超时回退（阈值 ❌） | `AutoNextEpisode.kt:57` 固定 `DELAY_MS` 后换集；`nextPrewarmed` 幂等保护 | 无"预加载完成度"驱动，也没有超时回退 | 网络慢时"无缝"变成"黑屏等缓冲"（用户感知为卡住） | 把 `nextPrewarmed` 从 bool 升级为状态（`ready/inFlight/failed`）；`maybeSchedule` 改为"ready 立即切、inFlight 最多等 N ms、failed 直接提示失败" | P2 |
| MOT-07 | ✅ M03-S02：长按标签 `α 0→1 / s 0.9→1` | `PlayerLongPressHud.kt:47-51` 内层 `animateFloatAsState(targetValue = 1f)` —— **目标恒 1f**，永远不产生动画 | 死动画：`scale` 变量算出来就是 1.0，未使用 | 无（视觉已由 `AnimatedVisibility(scaleIn 0.9)` 实现），但代码读者会误以为"标签有 0.9→1 微弹" | 删除 `:47-51` 的无效 `animateFloatAsState`；若要 0.9→1 弹入，改为 `scaleIn(initialScale = 0.9f, spring(0.25,0.8))` | P2 |
| MOT-08 | —（资源/性能） | `PlayerSeekTrack.kt:93-103` `LaunchedEffect(Unit) { while (true) { withFrameNanos { … } } }` | **常驻无限帧循环**：`BarSlot` 已改常驻（`PlayerBarsHost.kt:70-76` 注释），控制条隐藏时该循环仍在每帧跑（`smoothFraction` 计算） | 控制条隐藏时仍每帧读状态/写 `mutableFloatStateOf`（触发 Snapshot 写入）⇒ 待机耗电、发热；G6 明确"动效需让位性能预算" | 在 `if (visible)` 外或 `dragging/previewMs>=0` 时才启动循环：把 `LaunchedEffect(Unit)` 改 `LaunchedEffect(active)` 且 `while (active)`；`active = dragging || previewMs >= 0 || tickNeeded`。或保留常驻但用 `if (!dragging) return@withFrameNanos` 跳过写入 | P2 |
| MOT-09 | —（死代码） | `PlayerBarsHost.kt:131-139` `barEnter()/barExit()` 返回 `EnterTransition/ExitTransition`，**无调用方**（BarSlot 用 `animateFloatAsState`）；`PlayerOverlay.kt:4-11` 导入 `AnimatedContent/AnimatedVisibility/fadeIn/fadeOut/scaleIn/scaleOut/togetherWith` 全未使用；`PlayerOverlay.kt:14` `WindowInsetsSides` 未使用 | 两套过渡代码并存（一套死）+ 7 个死 import | 无 | 删除 `barEnter/barExit` 与未用 import；`PlayerBarsHost` 注释里"与原 AnimatedVisibility 一致"的说明指向不存在的代码，需同步改写 | P2 |
| MOT-10 | ✅ G6：高画质/超分时动效需降级 | 无任何"性能档"接入（`PlaybackMetrics` 有数据但 UI 层不读） | 动效不知情性能 | 4K HDR + 插值开启时，控制条淡入与三点波纹仍按固定时长/无限循环，可能加剧掉帧 | 在 `PlayerMotion` 加"性能压制"分支：当 `hdrOutput/videoInterpolation` 开启或 `frame drop` 超阈值时，把 `barEnterMs` 等降为 100 并停三点循环改用静态点+文本 | P3 |

---

## 七、文件职责边界（DUT）

### 7.1 现状（实测）

| 文件 | 行数 | 声明职责 | 实际内容 | 判定 |
| --- | --- | --- | --- | --- |
| `PlayerComposeActivity.kt` | 383 | "组合根：状态所有权 + 层叠接线 + 生命周期；业务在扩展文件" | 状态 40+ 字段、三条链路会话、投屏 5 字段、观看域、探针、生命周期、`onBackPressedDispatcher`、HDR Surface | ⚠️ 超 300（其自身 `PlayerComposeRoot.kt:26` 注释称"Activity 要守住 ≤300 行"） |
| `PlayerActionRouter.kt` | 407 | "只有点哪儿调哪儿，没有面板内容、没有手势数学" | 动作分发 **+ seek 落点内核调优（`seekTo`/`seekAfterLandingReady`/`performSeek`/`LandingSeekState` 线程 + 1200ms 预算）** + 截图 + 锁屏 + 双击 + toast/flash + 分辨率读 | ❌ 三类职责混合 |
| `PlayerGestureRouter.kt` | 278 | "手势数学（连续量）" | 竖滑/长按/横滑 **+ 系统亮度/音量读写 + Haptic + 速度格式化 + 全屏横滑实时 seek 节流 + 3 个顶层可变全局态** | ⚠️ 混入平台副作用与全局状态 |
| `PlayerPanelRouter.kt` | 290 | "面板选择 → 内核写入（写侧）" | 钻入/季集切换/弹幕本地态/重置/字幕/音轨/倍速写 mpv | ✅ 边界清晰 |
| `PlayerOverlay.kt` | 256 | "叠放顺序编排" | 60 参数 + 层序 + PiP 早退 | ✅ 层序清晰；参数表膨胀 |

### 7.2 差距

| 编号 | 契约（工程职责，非竞品） | 当前实现（文件:行） | 差距 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| DUT-01 | 单文件单职责 + ≤300 行（仓库铁律） | `PlayerGestureRouter.kt:198-203` `private var hSeekStartMs/hSeekAccumMs/hSeekTargetMs/hSeekLastAt` 是**Kotlin 顶层 private var** ⇒ 进程级单例 | 手势状态不随 Activity 销毁重置：第二次进播放器后若先"横滑结束"再开始，`hSeekStartMs` 初值 0 ⇒ `onHorizontalDrag` 里 `if (hSeekStartMs <= 0L) return` 会**吞掉第一次横滑**；若上次 `hSeekTargetMs` 残留则在 `onHorizontalDragEnd(commit=true)` 里跳到旧位置 | 偶发"第一次横滑没反应"或"一横滑画面跳到上次的位置"（复现率低、难归因，最伤信任） | 把四个字段搬进 `PlayerComposeActivity`（与 `hudKind` 等同组），或新建 `PlayerSeekGestureState` 数据类并挂在 Activity 上；`PlayerGestureRouter` 只做纯函数（入参→出参） | P1 |
| DUT-02 | 同上 | `PlayerActionRouter.kt:47-177`（`seekTo` 130 行含 40 行注释、`seekAfterLandingReady` 线程、`performSeek`） | 动作路由里塞了**内核 seek 编排**（落点窗口、等待预算、票据序号、A/B 开关） | 改动作表（图标语义）与改 seek 策略动同一个文件，评审难、冲突多；本次 TOP-01 的位次错配正因两方混在一处 | 拆出 `PlayerSeekController.kt`：`seekTo`/`seekAfterLandingReady`/`performSeek`/`LANDING_WAIT_TIMEOUT_MS`/`seekMode` 全部迁入（`PlayerActionRouter` 只保留 `onDoubleTap` 调用 `seekTo(...)`）；文件降到 ≈200 行 | P1 |
| DUT-03 | 同上 | `PlayerActionRouter.kt:396-407` `showToast`/`flashAction`；`:279-294` `onCapture`；`:300-324` 锁定 | 反馈（toast/flash/haptic）与截图、锁定混在动作文件；`haptic()` 却在 `PlayerGestureRouter.kt:138-141` | 同一"反馈"概念散在两个文件，新增第 3 种反馈要改两处 | 新建 `PlayerFeedback.kt`：`showToast`/`flashAction`/`haptic`/`showLockHint` 一处收口；两个 Router 都调它 | P2 |
| DUT-04 | 单一入口 | 弹幕开关两条路径重复实现：`PlayerActionRouter.kt:224-232`（顶栏）与 `PlayerPanelRouter.kt:89-98`（面板）—— 逻辑同构但**副作用不同**（前者 `flashAction`，后者 `panelRevision++` 而不 flash） | 同一业务两份实现且已分叉 | 顶栏关弹幕会闪绿 1.5s，面板关弹幕不闪 ⇒ 用户感觉"两个地方不一样" | 抽 `fun PlayerComposeActivity.toggleDanmaku()`（含 hasItems 门控 + toast + flashAction + panelRevision），两处只调用 | P2 |
| DUT-05 | 一类控件一份实现 | "44 命中盒 + pressed alpha"重复 5 份：`PlayerTopBar.TopIconButton:157-203`、`PlayerBottomBar.BarIconButton:239-266`、`PlayerLockLayer:40-58`、`PlayerBarText.BarTextButton:60-92`、`PlayerStepperRow`（`minTouchTarget` 处） | 命中盒/按压/无障碍描述三类语义各写一遍 | 5 处命中区尺寸/反馈不一致（如 BOT-10 的 27pt 文本钮、TOK 里 44/38 混用）；TalkBack 朗读不一致 | 新建 `PlayerHitTarget.kt`：`@Composable fun PlayerHitTarget(size: Dp = minTouchTarget, label: String, onTap, onLongPress = null, content: @Composable () -> Unit)`；5 处替换 | P2 |
| DUT-06 | 面板开关单一入口 | `togglePanel` 在 `PlayerActionRouter.kt:338-340`；`activePanel` 另有 7 处直接赋值：`PlayerOverlay.kt:133`（读）、`PlayerComposeRoot.kt:118`、`PlayerPanelRouter.kt:55,77,110,196,253`、`PlayerActionRouter.kt:262,303` | 打开面板没有唯一函数（谁负责 `lastTouchMs`/`castSheetVisible`/`audioOnly` 互斥不明确） | 面板切换时控制条计时、投屏 sheet、听视频态可能互相打架（例：听视频态下面板被 `setAudioOnly` 清空，但 `activePanel` 由别的路径写入时不清理） | `PlayerActionRouter` 增 `fun openPanel(kind)`/`closePanel()`，统一写 `activePanel` + `lastTouchMs` + 互斥；其余文件改调用 | P2 |
| DUT-07 | 参数表瘦身（可读性） | `PlayerOverlay.kt:61-129` 60 个参数 + 32 个回调 | 每次新增状态要动 3 处（Overlay 参数、Root 传参、测试） | 无直接 UI 后果 | 抽 `data class PlayerOverlayState` + `data class PlayerOverlayActions`（`PlayerOverlay.kt` 内），函数签名变 4 个参数；行为零变 | P2 |
| DUT-08 | 图标语义单一清单 | `PlayerTopBar.kt:139-148` `TOP_ACTIONS`（kind→label）与 `PlayerActionRouter.kt:218-244` `onTopAction` 的 `when(kind)`（kind→行为）**两处各写一份** | 新增/删改图标必须同步两个文件，无编译期约束 | 正是 TOP-01 位次错配的温床；下一次改动仍会漏 | 新建 `PlayerTopActions.kt`：`enum class TopAction(val kind, val label, val panel: String?, val longPressPanel: String?)`，UI 遍历枚举、路由 `when (action)` 收敛为 `action.panel?.let { togglePanel(it) }`；位次只在一个有序列表里 | P1 |

---

## 八、优先级汇总与建议落地顺序

### 8.1 汇总

| 优先级 | 编号 | 一句话 |
| --- | --- | --- |
| **P0** | PNL-01 | 面板一律从右上角出现 —— 竞品是"跟随触发控件"（倍速面板在底栏上方） |
| **P0** | MOT-01 | App 内"减弱动画效果"是灰行，播放器只认系统 `ANIMATOR_DURATION_SCALE==0` |
| P1 | TOP-01 | 第 4/5 顶栏图标语义互换（场记板 vs 跳过） |
| P1 | TOP-02 | `clapper`/`skip` 面板锚点缺失，落回右上角 16pt |
| P1 | TOK-01 | 顶栏"无遮罩"只活在注释里（两个 0 值 token 零引用） |
| P1 | TOK-06 | 进度条头注释描述的是被推翻的旧三段色 |
| P1 | BOT-01 | 选集态把整条进度条摘掉（竞品保留） |
| P1 | BOT-08 | 面板压到"倍速"时裁出半个字 |
| P1 | PNL-02 | skip 面板 300 宽（竞品 248） |
| P1 | PNL-03 | 微调卡从 16pt 起，盖住顶栏 |
| P1 | PNL-07 | 缺"避开挖孔"能力与开关 |
| P1 | PNL-08 | 缺"填充模式"三态（适应/填满/拉伸） |
| P1 | GST-05 | 关掉独立亮度后左半屏竖滑**静默失效** |
| P1 | GST-06 | 双指缩放完全缺失 |
| P1 | GST-07 | "锁定允许手势"被做成可关闭开关（竞品是固定行为） |
| P1 | MOT-02 | Reduce Motion 只覆盖 2/6 类动效 |
| P1 | MOT-03 | 起播无 spinner 回退态 |
| P1 | MOT-04 | 换集无平滑过渡 |
| P1 | MOT-05 | 换集失败无用户可见文案 |
| P1 | DUT-01 | 横滑手势状态是进程级顶层 var |
| P1 | DUT-02 | ActionRouter 混入 seek 内核编排（407 行） |
| P1 | DUT-08 | 图标语义两份清单 |
| P2 | TOK-02/04/05/07、TOP-05/07、BOT-02/04/05/09/10、PNL-04/05/06/09、GST-01/02/03/08/09、MOT-06/07/08/09、DUT-03/04/05/06/07 | 见上文各条 |
| P3/待真机 | TOK-03、TOP-04/06、BOT-07、PNL-11、GST-04/10/11、MOT-10 | 需更高分辨率帧/真机录屏确认 |

### 8.2 建议落地顺序（4 批，每批独立可回归）

1. **批 A（几何与锚点，P0+P1 的视觉面）**：PNL-01 → TOP-02 → PNL-02/03 → BOT-01/BOT-08。
   验收：5 张竞品帧 1:1 对照（顶栏/底栏/字幕面板/跳过面板/倍速面板/选集态），面板出现位置与触发控件的相对关系必须可复现。
2. **批 B（能力补齐）**：PNL-07/08/10（填充模式 + 避开挖孔 + 钻入）→ GST-06（双指缩放）→ GST-05（亮度静默）→ GST-07（锁定开关）。
   验收：竞品 0173 面板逐行对照；缩放 1.0–2.0 在 10bit 片源上不掉帧（P12-B 基线 175fps 不退化）。
3. **批 C（动效治理）**：MOT-01 → MOT-02 → MOT-03 → MOT-04/05。
   验收：设置项开/关两组录屏对照（G1 硬约束），每类动效在开启时**只剩 α**。
4. **批 D（工程收口）**：DUT-01 → DUT-08 → DUT-02/03/04/05 → TOK-01/06 + 逐文件裸值清理。
   验收：`PlayerActionRouter` ≤300 行；全页 `dp/sp` 裸值 ≤ 每文件 3 处；`gradlew assembleDebug` + 现有单测全绿。

### 8.3 不要动的（本次核对为"已对齐"，避免过度改动）

- 底栏 6pt 三段 + 描边 + 无滑块 + 时间位置 + pitch 65 + 右侧三钮（BOT 3.1 全表）。
- 顶栏 8 槽 + 44 命中盒 + `spacedBy(pitch−hitBox)` 公式 + 无遮罩 + 无涟漪按压。
- 长按链（0.5s → 倍速 → 左滑调速 → 上滑 60dp → 松手两策略）与**锁定矩阵**（竞品 §5 第 3 条：锁定不是全禁）。
- 触觉三态 + `EXTRA_BATTERY_LOW` 自动降级（这是竞品 G2 的完整实现，全仓唯一）。
- HUD 150/1000/300 与面板 248/16/42/60/3pt 的数值本身。

---

## 附录 A：竞品契约等级索引（本文出现的）

| 编号 | 契约 | 等级 | 依据 |
| --- | --- | --- | --- |
| G1 | 减弱动画效果开关 + 每个动效有降级分支 | ✅ | `settings.interface.page.reduceMotion` |
| G2 | 触觉 自动/禁用（+启用），低电量自动关 | ✅ | `settings.interface.page.haptics.*` |
| G3 | 起播动画开关，**关→spinner** | ✅ | `settings.playback.preference.startupAnimation.detail` |
| G4 | 平滑过渡开关（与预加载耦合） | ✅ | `cache.smoothTransition` / `pro.copy.smooth.transition` |
| G5 | 过渡依赖预加载就绪，门槛未知 | ✅/❌ | footer 文案；阈值不可静态获取 |
| 双击三分区 | 左/中/右可配动作 | ✅ | `gestures.sideDoubleTap`/`middleDoubleTap` |
| 双击识别窗 0.3s / 闪示 0.5s | — | ◐ | `UITapGestureRecognizer` 容差；状态提示惯例 |
| 长按 ≥0.5s / Δy -60 / 0.2-0.15s / 标签 0.9→1 | — | ◐ | 系统阈值 + 状态提示族 |
| 锁定矩阵 | 长按✅ 双击✅ 其余❌ | ✅ 结构 / ◐ 语义 | `gestures.locked` |
| 上下滑左右分区 | 哪侧音量 | ✅ 存在 / ❌ 分区 | `verticalPan` 文案未指定 |
| 双指缩放 1.0–2.0、松手回弹 | — | ✅ 存在 / ◐ 范围 / ❌ 回弹 | `gestures.pinch` |
| 旋转 0.3s easeInOut / 两个独立开关 | — | ✅ 开关 / ◐ 参数 | `followRotation`、`autoLandscapeRotation` |
| 换集过渡 0.3–0.5s 交叉溶解 | — | ◐ | "平滑过渡"需交叉而非串行 |
| 换集失败"暂时无法切换剧集" | — | ✅ | `player.episode.transition_failed` |
| 面板 248 / 16 / 42-60 / 3pt | — | ✅（截图 + `artifacts/rex-measurements.txt`） | 本报告 §4.1 |
| 图标 8 槽语义序 | — | ✅（0170 全显 + 0173/0165 面板内容） | 本报告 §2.2 TOP-01 |
| 面板锚在触发控件 | — | ✅（0160/0165/0171/0173 四张一致） | 本报告 §4.2 PNL-01 |
| 顶栏无遮罩 | — | ✅ | 0160/0168/0170 |
| 底栏 6pt 三段无滑块 | — | ✅ | 0160/0170/0171/0172/0173 |

## 附录 B：待真机确认清单（本文未能定论的项）

| # | 待确认 | 影响条目 | 验证方法 |
| --- | --- | --- | --- |
| 1 | 播放器是否有 2026-11-16 晚的第二批截图（`.tmp/rex_scaled` 中 11-16 全为竖屏） | 全篇基线 | 补图后重跑 §2.1/§3.1/§4.1 对照 |
| 2 | 顶栏 ✕/主页 glyph 实际 ink（20/22 是否颠倒） | TOP-04 | 更高分辨率帧 + `measure_baseline_playback.py icons` |
| 3 | 亮度/音量左右分区（竞品未指定） | GST-05 | 真机左右各滑一次并录屏 |
| 4 | 长按倍速标签的位置（我们放屏幕正中） | GST-04 | 竞品真机长按录屏 |
| 5 | 锁定入口（长按播放键是否与竞品一致） | GST-11 | 竞品真机手势录屏 |
| 6 | 弹幕图标长按 → 操作面板是否为竞品行为 | TOP-06 | 真机长按弹幕图标 |
| 7 | 面板背景模糊强度（我们的 75% 黑是否足够） | PNL-09 | 亮画面截图 A/B |
| 8 | 未播段在亮画面上的可见度（12% vs 20%） | BOT-05 | 雪景/白墙片源截图 |

---

*本报告所有"当前实现"均给出文件:行；所有竞品值均标注 ✅/◐/❌。凡标 ◐ 的值均来自竞品文档的推断且注明了依据；凡标 ❌ 的均已给出验证路径 —— 未在本文中静默当成真值。*