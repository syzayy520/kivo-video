# 面板材质对照研究：SenPlayer / 哔哩哔哩

> 目的：把 `p10-panel-material-spec.md` §14-1「SenPlayer / 哔哩哔哩材质对照——本轮未取得」补齐。
> 范围：只回答"面板做出来是什么材质、用什么数字、我们该抄哪几条、不该抄哪几条"，**不改几何、不谈功能**。
> 结论落点：只允许改 `PlayerPanelStyle.kt`（面板族令牌）＋ `PlayerPanelSurface.kt`（绘制）＋ `PlayerTokens.bottomScrim*`（底栏压暗，同属"面板与控制栏统一"）。
> 取证日期：2026-11-20。取证对象：b6e950ee 上已安装的 `com.wuziqi.SenPlayer`（1.0.0 build 12）、`com.bilibili.app.in`，只读拉取 APK 离线解析，**未上机、未占设备**。

---

## 0. 结论摘要

1. **哔哩哔哩（资源级，硬数值）**：播放器面板一律是**高不透明黑**——设置侧栏 `#E0000000`（88%）、字幕设置 `#D8000000`（85%），**没有渐变、没有描边、没有内嵌卡**；四周靠 1dp `#1A999999`～`#33999999` 分隔线（左右内缩 12dp）。字幕设置最小高 220dp，行高 44dp、正文 14sp，**控件在行尾、行首无图标**。
2. **哔哩哔哩底栏压暗（像素级，硬数值）**：130dp 高三段渐变，底边黑 **76%**、中点 23.5%、顶边 0；实测 alpha 沿高度是**二次曲线 t²**（0/3/13/49/110/159/194），不是线性。我们当前是 110dp **线性** 0→55%。
3. **压暗曲线不照搬（本报告最重要的一次自我否定）**：把两条曲线分别叠到亮场实测帧 `b17-a.png` 上算白色图标对比度，哔哩哔哩的 t² 曲线在 t=0.2–0.6（图标实际所在高度）**比我们更低**（3.14 vs 3.46 @t=0.5），只在最底边更强（8.81 vs 7.61）。照搬会**降低**图标可读性。→ 结论：现行线性 110dp/55% 保留，不因"像哔哩哔哩"而改。
4. **SenPlayer（架构级，无数值）**：Compose + **真实背景模糊 `dev.chrisbanes.haze`**（`hazeEffect`，blurRadius ≤25dp，含 tint/noise/blurEnabled 策略），上层再叠 **AGSL RuntimeShader 玻璃**：`tint + softLight(noise)` 与**边缘折射＋色散**（`RefractionWithDispersion`、`Highlight(width=)`），动效取自 Kyant0 `LiquidBottomTabs` / `GlassMotion`。**精确数值未取得**（R8 混淆、本机无 jadx/java），置信度中。
5. **对 Kivo 的意义**：SenPlayer 的"玻璃" = 能看到背后视频的模糊 + 折射；我们的视频在 SurfaceView（SurfaceFlinger 独立合成），这条路在本架构内**不可达**（与 v1.1 §1 非目标一致，本次复核未推翻）。Kivo 的路线仍是"不透明冷调玻璃 + 单边受光"；本次对照**没有产生需要落地的参数改动**，产出的是一份"不抄清单"与亮场验收口径。
6. **一句话差异**：哔哩哔哩靠"更黑"（85–88%）保住亮场可读性；SenPlayer 靠"真模糊"吃掉背景；Kivo 靠"冷调 + 渐变 + 单边受光"在没有模糊的前提下做到不像黑板——三条路各自自洽，**不要把哔哩哔哩的 88% 搬过来**（会让 v1.1 退回 v1 的黑板）。

---

## 1. 取证方法与可信度分级

| 级别 | 手段 | 本文用于 |
|---|---|---|
| A · 硬数值 | `aapt2 dump resources/xmltree` 解析 `resources.arsc` 与二进制 XML；`PIL+numpy` 逐像素读 PNG 的 alpha | 哔哩哔哩全部数字 |
| B · 结构证据 | APK 内 `assets/licenses/*NOTICE`、`META-INF` 依赖清单、DEX 字符串常量扫描 | SenPlayer 的材质架构 |
| C · 推断（显式标注） | 由 B 推断默认参数区间 | SenPlayer 的模糊/折射量级 |

工具约束（如实记录）：本机 `C:/Android/sdk/cmdline-tools` 的 `apkanalyzer` 因 **未装 java / JAVA_HOME 未设**不可用；无 apktool、无 jadx，**无法反编译 DEX 取方法体常量**——这是 SenPlayer 数值只能到"架构级"的根本原因，不是没查。

---

## 2. 哔哩哔哩：资源级硬数值（A 级）

### 2.1 面板底色

| 资源 | 值 | 用在哪 |
|---|---|---|
| `color/black_alpha85` | `#D8000000`（α 84.7%） | 横屏字幕设置面板（`bili_player_new_subtitle_setting_widget`，`minHeight=220dp`） |
| `color/black_alpha88` | `#E0000000`（α 87.8%） | 播放器设置侧栏（`bili_player_new_setting_function_widget`，`width=320dp`，`match_parent` 高） |
| `color/black_alpha94` | `#E2000000`（α 88.6%，注意与命名不符） | 备用 |

**形态**：纯色 `solid`，无 gradient、无 corners（侧栏贴边）、无 stroke、无内阴影。即"一整块 88% 黑玻璃"，亮场可读性极稳，代价是亮画面上读作深灰板。

### 2.2 分隔线

| 位置 | 值 |
|---|---|
| 字幕设置标题下 | `color/gray_dark_alpha20` = `#33999999`（20% 灰），1dp，左右内缩 12dp |
| 设置侧栏分节 | 字面量 `#1a999999`（10% 灰），1dp，左右内缩 12dp |

### 2.3 排版与行结构（字幕设置面板）

| 元素 | 规格 |
|---|---|
| 标题 | 14sp，`#FF999999`（灰 60%），`marginTop=28dp`，`marginStart=12dp` |
| 标题后缀说明 | 12sp，`#FFFFFFFF`（纯白），基线对齐，前距 6dp |
| 行高 | 44dp（`bili_player_subtitle_main_item_holder`），`paddingStart=16dp`，上下 12dp，14sp，单行省略 |
| 行尾控件 | `AppCompatCheckBox`（`button` 自绘），右对齐；**行首无图标** |
| 分组头 | `FixedDrawableTextView` 48dp 高、上下各 8dp、12dp 字号、居中 |

### 2.4 圆角

| 形态 | 半径 |
|---|---|
| 底部半屏面板 | 仅上两角 15dp（`bili_player_shape_top_round_corner_15_bg1`） |
| 居中弹层 | 四角 20dp（`bili_player_shape_round_corner_20_wh0_bg`） |
| 播放器侧栏 | 0（贴边满高） |

### 2.5 压暗（scrim）——**本次最值得抄的一条**

两份独立证据互相印证：

- 形状：`drawable/player_shape_shadow_bottom.xml`：`angle=90`（自底向上），`startColor=#99000000`（60%），`centerColor=#3C000000`（23.5%），`endColor=透明`。
- 像素：`res/drawable-xxhdpi-v4/bili_player_bar_bottom_bg.png` 是 **1×390px 的纯 alpha 灰阶图**（xxhdpi → **130dp 高**），自顶向底 alpha 依次为：

| 高度位置 t | 0.00 | 0.10 | 0.25 | 0.50 | 0.75 | 0.90 | 1.00 |
|---|---|---|---|---|---|---|---|
| 实测 alpha | 0 | 3 | 13 | 49 | 110 | 159 | **194（76%）** |
| 拟合 194·t² | 0 | 1.9 | 12.1 | 48.5 | 109 | 157 | 194 |

→ **二次缓入曲线，峰值 76%，高度 130dp**。顶部栏 `bili_player_bar_top_bg.png` 是同一曲线的镜像。

### 2.6 其他可直接对照的数字

- 面板内所有"字号"写成了 `dp`（14dp/12dp）——哔哩哔哩的既有瑕疵，**不要学**。
- 字幕条目底色 `#00000000`（透明），选中态只靠 checkbox；行本身不画卡片。

---

## 3. SenPlayer：架构级证据（B 级）

| 证据 | 内容 | 推论 |
|---|---|---|
| `assets/licenses/senplayer-glass-NOTICE.txt` | 明确声明使用 **Backdrop + Shapes by Kyant0（Apache-2.0）**，`AndroidLiquidGlass/tree/2.0.1`；`LiquidTabBar.kt`、`GlassMotion.kt` 改编自 `LiquidBottomTabs`、`DampedDragAnimation`；视觉参考 Coolapk | 材质不是"调出来的渐变"，而是**液体玻璃渲染 + 阻尼动效** |
| DEX 字符串 | `dev/chrisbanes/haze/HazeBackdropRenderer`、`HazeEffectNode`、`blurRadius must be … <= 25`、`TintColorHazeColorEffect`、`NoiseFactor`、`BlurEnabled` | 面板/控件用 **Haze 实时模糊**：blurRadius 0–25dp、可配 tint 与 noise，可因设备能力降级（`BlurEnabled`） |
| DEX 中的 AGSL 源码 | `uniform shader content; uniform shader noise; … withNoise = softLight(noiseColor, blurred); return tint + withNoise*(1-tintColor.a)` | 玻璃 = **模糊底 + tint 覆盖 + 噪声 softLight**，tint 以 alpha 线性混合 |
| 同上 | `uniform float refractionHeight; uniform float refractionAmount; … refractedCoord = coord + d*grad;` 与七通道 `red/orange/…/purple` 采样 | 边缘**折射 + 色散（chromatic dispersion）**，即 Apple 液态玻璃标志性的"边缘弯折光" |
| 同上 | `Highlight(width=…)`、`BlurredEdgeTreatment(shape=…)` | 边缘高光有独立宽度参数，按形状裁切 |

**未取得 + 原因 + 置信度**：具体 blurRadius/tint ARGB/refractionHeight/Amount 常数、行高与字号、面板圆角——**未取得**；原因：DEX 经 R8 混淆且本机无 jadx/Java，AGSL 以字符串形式内联，数值写在方法体常量池里；置信度**中**（架构可确证，数字不可引用）。

---

## 4. R1–R10 逐条回答

| # | 问题 | 哔哩哔哩 | SenPlayer | 对 Kivo 的结论 |
|---|---|---|---|---|
| R1 | 底色 ARGB/alpha | `#D8000000` / `#E0000000` | 模糊底 + tint（数值未取得） | 维持 `#B80A0F14`→`#C20A0F14`；**不上 88%** |
| R2 | 是否渐变 | 否（纯色） | 由背后内容决定（真玻璃） | 维持纵向 72→76 渐变 |
| R3 | 圆角 | 15（上两角）/20/0 | 未取得 | 维持 16 |
| R4 | 描边/受光线 | 无（只分隔线） | `Highlight(width)` 边缘高光 | 维持仅顶边 1dp 7% + 24dp 渐隐 |
| R5 | 模糊方案 | 无（不透明） | Haze 实时模糊 + AGSL 折射/色散 | **不实现**（SurfaceView，v1.1 已否决，本次复核仍成立） |
| R6 | 阴影 | 仅 scrim，无面板投影 | 未取得 | 维持 24dp/30% |
| R7 | 文字 alpha/字号/字重 | 白 100%（行）/ 灰 60%（标题）/ 14sp / sans-serif-medium（按钮） | 未取得 | 维持 90/68/80/55/68 五档（对比度已核算） |
| R8 | 行结构 | 44dp、左 16dp、尾控件、**无行首图标** | 未取得 | 与 v1.1 完全一致（无前导图标、尾值/chevron）→ 佐证正确 |
| R9 | 面板几何/scrim | 侧栏 320dp 满高；字幕设置 minH 220；scrim 130dp、76%、t² | 未取得 | 几何不动；scrim **经核算不照搬**（见 §5.1） |
| R10 | 亮场可读策略 | 靠更黑（88%）；scrim 压底 | 靠真模糊吃掉背景 | 我们靠冷调+单边受光；亮场由 scrim 兜底 |

---

## 5. 参数级建议（只改三个位置）

### 5.1 核查了但**不采纳**：照搬哔哩哔哩压暗曲线

现状：`PlayerTokens.bottomScrimHeight = 110f`，`bottomScrimTop = 0x00000000` → `bottomScrimBottom = 0x8C000000`（55%），线性。

把"现行线性"与"哔哩哔哩 t²（130dp，底 60%）"分别叠到亮场实测帧 `b17-a.png`（2400×1080，白字假设，Rec.709 相对亮度）上，逐高度算白色图标对比度：

| 高度 t（自底向上） | 0.1 | 0.2 | 0.3 | 0.4 | 0.5 | 0.6 | 0.7 | 0.8 | 0.9 | 1.0 |
|---|---|---|---|---|---|---|---|---|---|---|
| 现行线性 CR | 2.01 | 2.28 | 2.61 | 3.00 | 3.46 | 4.02 | 4.68 | 5.49 | 6.45 | 7.61 |
| 哔哩哔哩 t² CR | 1.98 | 2.21 | 2.47 | 2.78 | 3.14 | 3.79 | 4.63 | 5.71 | 7.08 | 8.81 |

判读：
- **图标实际所在区（t≈0.2–0.6）现行更好**，最多领先 0.3 个对比度单位；哔哩哔哩只在最底缘更强。照搬＝以 130dp 高度 + 三段色标换一张"更均匀的渐变"，代价是图标区可读性下降，**净收益为负**。
- t<0.3 处两者都 <3:1，但那里没有字形（只有渐变尾巴）；底栏文字/图标的真实落点在 t≈0.35–0.6，均 ≥3:1。
- 因此：**保留 `bottomScrimHeight = 110f`、线性 0→55%**，不做任何改动。这条结论同时否掉了我此前"底边偏弱、上半偏重"的直觉判断——用数字推翻了它。

如后续真机亮场验收发现图标区仍不足，首选调整是**同高度下把端点 55%→60%**（t=0.4 时 CR 3.00→3.24），而不是改曲线形状；且必须同帧 A/B。

### 5.2 明确不采纳

- **不上 85–88% 面板底**：v1.1 的全部设计意图是"亮场也不像黑板"，88% 会把结论反转。
- **不做实时模糊/折射**：SurfaceView 拿不到视频像素；PixelCopy 抓帧会把烧进视频帧的字幕一起糊成"字幕影子"（v1.1 §1 已论证，本次复核无新证据推翻）。
- **不学 14dp 当字号**、不学 320dp 满高侧栏（会推翻 REX 几何）。

### 5.3 可选实验（需真机 A/B，不在本轮落地）

1. 面板底叠 **2% 噪声**（64×64 tile，`BlendMode.SoftLight`）：哔哩哔哩无、SenPlayer 有（`softLight(noiseColor, blurred)`）；在纯色渐变上可消 banding，成本一次 draw。验收：亮场截图逐像素比较，噪声不进入文字层。
2. 亮场面板底 72/76 → 76/80：仅当 5.1 落地后仍出现过灰感才试，**默认不做**。

---

## 6. 未取得项与恢复路径

| 未取得 | 原因 | 恢复路径 | 置信度 |
|---|---|---|---|
| SenPlayer blurRadius/tint/refraction 常数 | R8 混淆 + 无 jadx/Java | 装 jadx 后反编译 `classes.dex` 取 AGSL 常量；或在上游 Kyant0/AndroidLiquidGlass 2.0.1 与 Haze 文档核对默认值 | 中（架构）/ 低（数值） |
| SenPlayer 面板行高字号 | 同上（Compose 内联） | 同上 | 低 |
| 哔哩哔哩亮/暗两套主题下的同场景截图 | b6e950ee 重启后停在锁屏（需机主 PIN/图案解锁），本轮无法上机 | 解锁后补一组"亮画面/暗画面"同场景屏；只做旁证，不阻塞 5.1 | — |

---

## 7. 不抄清单（一句话版）

抄：11 条已核对一致的"无行首图标 + 尾控件"结构、1dp 10–20% 灰分隔线与 12dp 内缩、44dp 紧凑行高（对应我们 Compact 42dp）。
不抄：88% 实心黑、320dp 满高侧栏、15/20 混用圆角、`dp` 当字号、以及"有模糊所以敢用低 alpha"的 SenPlayer 前提。

---

## 8. 验收口径（供后续上机时直接执行）

| 项 | 判据 |
|---|---|
| V-S1 | 亮场截屏（2400×1080）：底栏渐变自顶向底单调递增，底边可见 alpha ≈55%，无分带/色阶断层 |
| V-S2 | 亮场下时间戳/图标与局部背景亮度比 ≥3:1（图标落点 t≈0.35–0.6 取 5 点取样；离线帧 b17-a.png 已算出 3.00–4.02） |
| V-S3 | 暗场截屏与 `p10-03-panel-crop.png` 对比，面板底、顶边线、圆角、行距无变化（只允许 scrim 区域差异） |
| V-S4 | 选集整替态展开时无"黑带/断层" |