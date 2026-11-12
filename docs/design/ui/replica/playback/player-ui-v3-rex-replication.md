# 播放页 v3 · REX 复刻设计稿与文件家族树

> 状态：**设计稿 v1（2026-11-12），家族树完成＝铁律闸门通过**；实现期间用户可随时叫停改向。
> 拍板依据：用户 2026-11-12「把 REX 的 UI 复刻下来，前助手做的我不满意」；范围＝**只播放页**；
> 前助手第四轮未提交改动已存档 `artifacts/p240-player-ui/round4-archive/round4.patch`（**不作基线**）。
> 素材：`C:/Users/Administrator/Pictures/REX 播放UI/`（15 张 2532×1170 截图 + 60fps 录屏）、
> `C:/Users/Administrator/Pictures/REX/rex/rex/`（结构/动效拆解）、`docs/reference/rex-teardown/05-ui-spec-from-recording.md`。
> 证据等级：✅＝像素实测或随包资源可复算；◐＝有依据推断；❌＝待真机/iPhone 补证。
> 复算：`python tools/measure_baseline_playback.py {icons|bottom|panel|text} <png> …`，
> （测量脚本已由 `tools/measure_rex_playback.py` 更名，见 `docs/evidence/p260/code-derex/README.md`。）
> 原文存 `artifacts/rex-measurements.txt`；网格读图 `artifacts/rex-grid-0160/0163/0169/0170/0171.png`。
> 换算：2532×1170 = 844×390pt @3x，**pt = px/3**，误差 ±2pt。

---

## 一、实测基线（✅，0159 为控制条基准帧）

### 1.1 顶栏（无遮罩渐变，白图标直压画面）

| 部件 | 实测 | 等级 |
| --- | --- | --- |
| 状态行 | 左时钟 x≈63pt、y≈10–23pt；右网速文本止于 ≈733pt + 电池 ≈760–783pt | ✅ |
| 图标行垂直中心 | y≈55pt（ink y44–66pt） | ✅ |
| ✕ / 主页 | 中心 x=74.3 / 121.8pt；glyph 17 / 20pt | ✅ |
| 标题 | x148–385pt，ink 高 14pt → **16pt** 常规 | ✅ |
| 右 8 图标中心 | 419.7 / 470.7 / 519.7 / 569.8 / 619.8 / ≈669.8 / 719.7 / 769.0 → **pitch 50pt** | ✅ |
| 图标 glyph | 19–22pt（盒按 22pt） | ✅ |
| 左右 ink 边距 | ≈66pt（＝横屏安全区 ≈47 + 内边距 ≈19） | ✅ |
| 图标语义序 | AirPlay · 弹幕开关(圈弹) · PiP · 场记板(跳过) · 画面比例(圈箭头) · **音轨(圈波形)** · 字幕(气泡) · **听视频(耳机)** | ✅ 0162 锚定+用户更正 |

> **语义更正（2026-11-12 用户指出 + 0162/0172/iPhone 现屏复核）**：圈波形＝**选择音轨**（形态4），
> 耳机＝**听音频模式**（形态13 入口，短按切换）。旧稿"均衡器(圈波形)/音轨(耳机)"是误判：
> 0162 音轨卡右缘锚在圈波形 icon（右缘+12pt 规则），耳机未亮；0172 听视频态耳机白圆亮。
> **REX 没有均衡器面板** —— 自造 equalizer 形态删除（§六 gap5 结案）。
> 听视频态（0172）：**控制条保留**（顶栏隐 PiP、耳机白圆亮；底栏照旧），仅视频区换黑底+居中 glyph。

> 05 号文档「间距 36pt / 标题 17pt」被实测推翻：pitch=50、标题 16。以本稿为准。
> Android 沉浸全屏无系统状态行 → 自绘时钟+网速+电池 glyph 复刻观感（◐ 电池取系统电量）。

### 1.2 底栏（收起态）

| 部件 | 实测 | 等级 |
| --- | --- | --- |
| 进度行 y 中心 | 304.8pt；**左时间—轨道—右时间同一行** | ✅ |
| 轨道 | x[110,740]pt，**厚 6pt**（05 的 3pt 推翻），圆角端，**无滑块** | ✅ |
| 轨道三段 | 已播 #FFFFFF；缓冲 ≈#9B9B9B 实色；未播＝半透明深灰罩视频（≈黑 45%） | ✅ |
| 时间 | 左 x[63,99]、右 x[744,779]；ink 9.3pt → **13pt semibold 等宽数字** | ✅ |
| 按钮行 y 中心 | 345pt（与轨道行 Δ40pt；距屏底 45pt） | ✅ |
| 左按钮 | ⏸ c75.5（ink 17.7×24pt）· 下一集 c140.7（ink 18×15pt） | ✅ |
| 右文本钮 | 4K c639.5 · 选集 c699.5 · 倍速 c766.8；**14pt**；ink 间隙 38–41pt | ✅ |
| 左右 ink 边距 | 66 / 64pt（与顶栏对齐） | ✅ |

### 1.3 面板玻璃材质（✅ 共性）

黑 ≈75% 毛玻璃 + 背景模糊；圆角 ≈16pt；内边距 16–20pt；滚动条 3pt 白 40% 贴右内 6pt；
分隔线白 8% 从文本左缘起；section 头 12pt 灰；行标题 16pt 白、副标题 11–12pt 灰；
选中＝白勾（列表）或白描边胶囊（chips）；chevron 白 60% 12pt。

### 1.4 十三种面板形态 → 四个复用壳（✅ 形态，✅ 锚点 K40 校准）

| # | 触发 | 形态壳 | 实测矩形 / 行高 | 帧 |
| --- | --- | --- | --- | --- |
| 1 | 字幕气泡 | 玻璃列表 | x[495,743] w248 top35；双行行 60 / 单行行 42；icon18@+30、题@+65。**行序（iPhone 现屏 ✅）**：内嵌字幕>（副=当前轨/关闭）· 外挂字幕>（副=无/名）· 搜索字幕 · 导入字幕 · sep+「操作」头 · 显示设置> · 关闭字幕 | 0160/0161+live |
| 2 | 倍速(底钮) | 玻璃列表 | x[532,780] w248 y[43,365]；行 42；勾@+25、题@+43 | 0171 |
| 3 | 画面比例 | 玻璃列表 | 同 2（含 separator + 子菜单行） | 0173 |
| 4 | **圈波形** | 玻璃列表 | 小卡：✓轨 rows + sep + **sliders icon** 音频延迟行（副=当前秒数）→钻入 5；**无"听视频"行** | 0162 |
| 5 | 4 的延迟行 | 微调卡 | x[470,827] w357 top16；✕圆 38@右上 16；内嵌卡 margin15 圆角 10 黑 30%；行 50：题 16 + stepper 93×30（−/+ 半分、竖分隔）；重置行居中 | 0163 |
| 6 | 场记板 | 微调卡(窄) | 行＝icon+题 16+副 11；末行钻入 7 | 0164 |
| 7 | 6 的方式行 | 玻璃列表 | 头＝题+副+⌄；选项带「自动」副标签+chevron | 0165 |
| 8 | 圈弹(长按?) | 玻璃列表 | 3 行带勾圆 icon：显示设置/过滤关键词/开启弹幕 | 0166 |
| 9 | 8 的显示设置 | 微调卡 | 同 5：toggle 行（固定转滚动）+ 4 stepper 行 | 0167 |
| 10 | AirPlay | 下拉小卡 | 2 行 icon+题：AirPlay / DLNA，w≈160 | 0168 |
| 11 | 4K 徽章 | **底栏内联展开** | 轨道行上移至 ≈198pt；chips 行 y223–253（h30 全圆角：选中白描边 1.5、未选白 12%），行首「播放资源:」16pt；源卡 x[63,272] w208 top265 圆角 16 描边白 35%：SDR 20pt bold + meta 14pt（**体积+码率，各带 film/bitrate 小图标**）+ 源名+**绿圆 20 内白勾**；**按钮行消失**（0169 复测推翻旧稿"按钮行不动"） | 0169 |
| 12 | 选集(底钮) | **底栏整替** | 头 y217：✓+剧名 16 左 77 / →顺序播放 14 右；缩略图 y[242,294] h52 w125 pitch137 圆角 8，当前集白描边 2 外扩 3；第N集 12pt@303、集名 14pt@315（当前 bold）；背景＝模糊视频+黑渐变 | 0170 |
| 13 | **耳机短按** | 黑底+居中块 | **控制条保留**：顶栏隐 PiP、耳机**反白圆**（白底黑 glyph，非绿高亮）；底栏照旧（**分辨率标保留**＝缓存最后真值）。居中：耳机 glyph ≈60pt y≈180 + 题 16「正在听视频」+ 副 12 灰「画面已关闭，锁屏后继续播放」。**黑底必须 Compose 自绘**：`vid=no` 只停解码，SurfaceView 留住最后一帧（K40 实测） | 0172 |

锚点规则 ✅（K40 2026-11-12 校准）：列表/小卡右缘≈触发 icon 右缘+12pt；top：列表 43pt、微调卡 16pt。
顶栏右排中心距右安全区 = 8+22+50·idx ⇒ **trailingPad = 50·idx − 4**（与设备安全区无关）：
字幕 46 / 音轨 96 / 比例 146 / 弹幕操作 296 / 投屏 346；听视频隐 PiP 后弹幕/投屏 −50。
倍速面板 trailing 16（右缘≈倍速文字右缘，0171）；微调卡 16。落点 [PlayerPanelHost]。

### 1.5 拖动预览 / 锁定 / 弹幕 / 字幕（✅ 来自 05 §6.2/6.4，帧 frames2）

预览浮窗 167×90pt、白描边 2、圆角 8、中心跟拖点；左相机圆钮 48pt 毛玻璃；左端时间实时目标；拖动中轨道提亮。
锁定态＝仅左下**开锁** glyph（sk_16 逐像素复测：30×28.3pt、线宽 2.5pt，锁体中心 x≈65pt、
整 glyph 心 y≈344.8pt —— **与播放键同槽**，推翻旧稿"16pt"）；弹幕/字幕继续渲染；
弹幕顶两行 11.5pt 白字黑描边；字幕底居中 15pt 带阴影。
加载＝中央三点波纹（非 spinner）。
暂停态浮钮（iPhone 现屏 ✅）：左中 48pt 毛玻璃圆＋相机 glyph，中心 ≈(88,195)pt；
仅**暂停且控制条显形**时出现（0162/0172 播放态无此钮）；点＝截当前帧。

### 1.6 字幕显示设计（✅ 渲染链路 + ◐ 设置卡内容，2026-11-12 用户要求设计）

**渲染唯一来源＝libmpv 内置 libass，Compose 不再自绘字幕。**
理由：带特效字幕（ASS/SSA 的 karaoke、\pos/\move _typesetting_、淡入淡出、描边阴影分层）
只有 libass 能正确合成；自绘 Text 只能画纯文本，等于把特效静默降级（违铁律）。
mpv render/vo 路径默认把字幕合成进视频帧 ⇒ 特效、双语、字体回退全部原生保留。

| 决策 | 取值 | 理由 |
| --- | --- | --- |
| 默认风格 | `sub-ass-override=no` | 跟随源特效（REX 观感＝源字幕原样） |
| 标准统一样式 | 开关→`sub-ass-override=yes` | 用户要"标准显示"时：全部字幕统一为 sub-* 样式（字号/位置/描边/阴影生效），特效剥离但**不丢内容** |
| 纯文本字幕(SRT) | 同走 libass | 统一样式即 sub-* 真值；不做第二套渲染 |
| 设置真值 | mpv `sub-font-size/sub-pos/sub-delay/sub-border-size/sub-ass-override` | 面板数字＝内核读回，写后 revision++ 重读 |
| 显示设置卡 | 形态9 同壳：toggle「标准统一样式」+ 4 stepper（字号/垂直位置/字幕延迟/描边）+ 重置 | 0167 同壳复用；◐ 卡内行序待 iPhone 补证 |
| 导入字幕 | SAF OpenDocument → 拷入 cache/subs → `sub-add <path> select` | 真功能；失败 toast 非空 reason |
| 搜索字幕 | 置灰＋reason「字幕搜索服务尚未接入」 | 无后端不画成能点 |
| 关闭字幕 | `sid=no`；内嵌字幕行副题同步变「关闭」 | 行状态＝内核真值 |

退役：`PlayerSubtitleLayer.kt`（Compose 假字幕层，subtitleText 恒 ""）移入
`docs/design/ui/replica/playback/retired/`；Overlay 的 SubtitleSlot 一并删除。
验收：K40 播带 ASS 特效字幕片源截图对照特效保留；切「标准统一样式」后截图对照统一观感（§七）。

## 二、手势与状态矩阵（✅ 链路来自动效拆解 M03，◐ 参数）

| 手势 | 正常态 | 锁定态 | 动作 |
| --- | --- | --- | --- |
| 单击 | 唤出/收起控制条 | 仅唤出挂锁 | fade 0.25s |
| 双击左/中/右 | seek−/播放暂停/seek+ | ✅ 保留 | 图标闪现 0.5s |
| 长按 | 倍速 2× 标签 | ✅ 保留 | 左右滑调速、上滑 60pt 锁定、松手恢复 |
| 竖滑左/右半 | 亮度/音量 HUD | ❌ | HUD 淡入 0.15、停 1s 淡出 0.3；**退出还原系统亮度** |
| 双指 pinch | 画面缩放 1–2× | ❌ | 松手回弹 ◐ |
| 进度条拖动 | 预览浮窗+目标时间 | ❌ | 松手 seek |

## 三、动效参数（◐ 基线，全部带 G1 降级分支）

面板弹入 α0→1 + s0.94→1，0.2s spring(0.25,0.8)；收起 0.15s easeOut；
底栏内联展开＝高度动画 0.3s easeInOut（轨道行随之上移）；选集整替＝交叉淡入 0.25s；
HUD/标签淡入 0.15–0.2s；三点波纹 n=∞；Reduce Motion（G1）→ 位移/缩放归零仅 α 或瞬时；
触觉（G2）与视觉解耦，低电量自动关触觉但视觉保留。

## 四、文件家族树（单文件单职责，每 .kt ≤300 行）

目录 `android/app/src/main/java/com/kivo/cinema/ui/compose/player/`：

```
player/
├─ PlayerTokens.kt          重写：全部尺寸/色值换 §一 实测常量（唯一常量源）
├─ PlayerOverlay.kt         重写：层序 + 三整屏态互斥 + 面板/挂锁挂载点
├─ PlayerBarsHost.kt        新：顶/底栏显隐过渡宿主（G1 降级只留 alpha）
├─ PlayerTopBar.kt          重写：状态行 + 图标行（pitch50/44pt 命中盒）
├─ PlayerStatusRow.kt       新：自绘时钟/网速/电池状态行（30s 刷新）
├─ PlayerBottomBar.kt       重写：轨道行+按钮行+内联资源槽+选集整替
├─ PlayerSeekTrack.kt       重写：6pt 三段轨 + 拖动态提亮 + 预览回调
├─ PlayerSeekPreview.kt     重写：缩略图浮窗+相机钮+目标时间（§1.5）
├─ PlayerGlassMenu.kt       新：玻璃列表壳（行=icon/题/副/勾/chevron/section/scroll）
├─ PlayerDetailCard.kt      新：微调卡壳（✕+头+内嵌卡槽）
├─ PlayerStepperRow.kt      新：−/+ pill 行、toggle 行、重置行
├─ PlayerResourcePanel.kt   重写：chips + 源卡（形态11，按钮行消失）
├─ PlayerEpisodeStrip.kt    新：底部横滑选集（形态12）
├─ PlayerAudioOnly.kt       重写：听视频居中块（形态13，控制条保留⇒不再整屏替换）
├─ PlayerCaptureButton.kt   新：暂停态左中浮相机钮（48pt 毛玻璃，live 实测）
├─ PlayerGlyphPanel.kt      新：面板行 glyph（gear/doc/search/import/xcircle/sliders）
├─ PlayerLockLayer.kt       新：锁定态唯一控件=左下开锁（与播放键同槽）
├─ PlayerPanelHost.kt       新：panel kind → 壳/锚点/过渡 的唯一决策点
├─ PlayerPanelData.kt       重写：内核真值→菜单项（纯装配，不取数）
├─ PlayerPanelForms.kt      新：无内核依赖形态 + 缺能力行（reason 非空）
├─ PlayerPanelItems.kt      重写：取数时机（LaunchedEffect+IO+revision）+ 面板标题
├─ PlayerGestureLayer.kt    改：§二 手势矩阵 + 锁定通道（竖滑❌/双击长按✅）
├─ PlayerGestureHud.kt      保持：亮度/音量 HUD pill
├─ PlayerLongPressHud.kt    保持：倍速标签
├─ PlayerStatusLayer.kt     保持：三点波纹 + 错误态（reason 非空可见）
├─ PlayerGlyph.kt           改：三族分发（core/feature/chrome）+ fill 参数
├─ PlayerGlyphCore.kt       改：+lock-open（sk_16 实测开锁形）
├─ PlayerGlyphFeature.kt    改：图标集对齐 §1.1 语义序
├─ PlayerGlyphChrome.kt     新：home/battery/chevron/plus/minus/camera/reset/film/bitrate
└─ PlayerWindow.kt          保持：safeDrawing 安全区
compose/
├─ PlayerComposeActivity.kt 重写≤300：状态所有权 + 生命周期（接线移出）
├─ PlayerComposeRoot.kt     新：层叠接线（状态→Overlay 参数、回调→Router）
├─ PlayerSessionBridge.kt   改：mpv null-safe 读写 + 轮询 + 网速/缓冲 1Hz 采样
├─ PlayerSessionStart.kt    保持：本地/URL/网络硬解起播 + probe extras
├─ PlayerSmbStart.kt        保持：SMB 起播（引擎选择）+ 软解音频挂载
├─ PlayerSessionTeardown.kt 保持：收尾顺序（先停会话再关 fd 所有者）
├─ PlayerActionRouter.kt    改：顶/底/状态层离散动作 + 主页/截图/锁定/选集顺序
├─ PlayerGestureRouter.kt   改：竖滑亮度音量 + 长按快进（锁定态禁调速）
├─ PlayerPanelRouter.kt     重写：钻入/stepper/toggle/重置/切轨（后台+非空 reason）
├─ PlayerSubtitleImport.kt  新：SAF 选字幕→cache/subs→sub-add（§1.6，失败非空 reason）
├─ PlayerCastBridge.kt      保持：DLNA 扫描/投屏
├─ PlayerDanmakuBridge.kt   保持：弹幕 XML 侧载
└─ PlayerProbeHooks.kt      保持：253 字幕取证 + 215 缩略图压测
```

> 落地实测（2026-11-12 v3 收口）：Activity **226 行**、Overlay **259 行**、
> 全家族最大 **279 行**（PlayerGlassMenu），`compileDebugKotlin` BUILD SUCCESSFUL。
> 取数在 `PlayerPanelItems`（LaunchedEffect+IO），写侧每次成功 `panelRevision++`
> 驱动重算 —— round-4 的"打开必空"与"stepper 数字不动"都在**结构上**消失。

依赖单向：Activity → Router/Bridge → Overlay → 壳组件 → Tokens；壳组件互不 import。
**禁止**：Activity 内写面板内容计算；壳组件内读内核；常量散落在组件里。

## 五、round-4 两缺陷的结构性防再犯

1. **面板空列表**：旧 `panelItemsCache` 只在 `onPanelSelect` 回调里刷新 → 打开必空。
   v3：`PlayerPanelData` 用 `LaunchedEffect(activePanel)` 打开即算 + 内核就绪事件再算；无节流缓存。
2. **按钮死点击**：旧固定 Row 高裁切命中区 + 轮询闭包过期。v3：命中区＝图标盒 44pt 最小、
   轮询一律 `rememberUpdatedState` + null-safe accessor；真机逐钮点验（§七）。

## 六、iPhone 补证清单（❌→✅，用户做手势、助手截图）

1. 长按倍速标签形态与位置；2. 亮度/音量 HUD 形态；3. 字幕「显示设置」子页内行序
   （入口 ✅：iPhone 现屏已见「显示设置>」行；卡内 stepper 项按 mpv sub-* 真值设计，◐）；
4. 弹幕开关短按 vs 长按语义（形态 8 触发）；5. ~~均衡器面板~~ **结案**：圈波形＝音轨
   （0162 锚点 + 用户更正），REX 无均衡器；耳机＝听视频（0172）；
6. **屏幕锁定的加锁入口**（strings 只有 `gestures.locked`「锁定时允许长按和双击」，
   没有入口文案；录屏 sk_12→sk_16 之间的手势没拍到）；
7. 听视频态的退出手势 ◐（现实现＝再点耳机；0172 只拍到进入态）。
命令：`pymobiledevice3 developer dvt screenshot <png>`（≈8s/张，不能注入触摸）。

**已落地的 ◐ 推断入口（补证后只改一处）**：加锁＝长按播放键
（[PlayerActionRouter.onToggleLock]，不占任何已实测槽位）；解锁＝点左下开锁；
锁定态单击＝唤出挂锁 2.5s 自隐；圈弹长按＝形态 8。四处都带触觉/轻提示，不静默。

## 七、验收（K40 b6e950ee，证据 `docs/evidence/p240/player-ui/v3/`）

逐项对照表（每项：REX 帧 vs K40 截图 vs 差值 pt）：顶栏 10 项 / 底栏 8 项 / 13 形态各 1 项 /
手势 6 项 / 动效降级 2 项。判据：布局差 ≤3pt、字号差 ≤1pt、形态缺项=0、死钮=0、
面板打开即有内容（内核未就绪显示非空 reason 行）、退出还原亮度。
每轮证据附复算命令与截图路径；**无证据路径不说完成**。