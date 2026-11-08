# kivo 首页 hero 环境色机制——对齐参照逐像素（完整交接）

> 本文档是自包含任务提示词：执行者无需其它上下文，按 §5 工作流做完 §6 验收即交。
> 坐标系约定：参照帧 1170×2532px = 390×844pt @3x（÷3=pt）；我方测试机 1080×2400 @2.75
> （392.7dp 宽，÷2.75=dp）。pt 与 dp 数值可直接比对（差 ≤1%）。

## 0. 任务

kivo（Android/Compose）首页 hero 轮播的「环境色背景」已迭代 15 版仍与参照有差距。
你的任务：按 §3 的实测基准与 §4 的层序，把机制调到与参照帧逐像素一致。
**纪律：先测量后改参；每改一处必须装机截图、与参照帧同列逐行 diff；不许凭感觉调。**

## 1. 参照素材（本机绝对路径）

| 素材 | 路径 | 说明 |
| --- | --- | --- |
| 参照 App S=0 | `C:\Users\Administrator\Pictures\REX\2026_11_08_05_13_34_IMG_0138.PNG` | 未滚动，暗海报（生化危机） |
| 参照 App S≈182 | `C:\Users\Administrator\Pictures\REX\2026_11_08_05_13_45_IMG_0139.PNG` | 滚动中，场=淡紫 (129,119,123) |
| 参照 App S≈301 | `C:\Users\Administrator\Pictures\REX\2026_11_08_05_13_50_IMG_0140.PNG` | 深滚动，场=近白 (202,197,204) |
| 对照播放器视频 | `C:\Users\Administrator\Pictures\REX\2026_11_08_07_44_56_IMG_0142.MP4` | 24s；f1≈1s(蓝海报)、f13≈13s(红海报) 已验证同款机制 |
| 我方当前装机图 | `C:\kivo video\build\p237_analysis\kivo_v15_s0.png` / `kivo_v15_deep.png` | v15 基线 |

## 2. 代码位置（工作目录 `C:\kivo video`，分支 codex/kivo-video-p0-ui-01-playback-overlay-v13）

- `android\app\src\main\java\com\kivo\cinema\ui\design\HeroCarousel.kt` — hero 轮播 + 外层遮罩
- `android\app\src\main\java\com\kivo\cinema\ui\design\DiscoverPage.kt` — 页面骨架 + 下缘场 drawBehind
- `android\app\src\main\java\com\kivo\cinema\ui\design\PosterPalette.kt` — 海报取色
- `android\app\src\main\java\com\kivo\cinema\tmdb\TmdbHome.kt` — /images 海报与 logo 数据域
- `android\app\src\main\java\com\kivo\cinema\MainActivity.kt` — Shell（悬浮导航/状态栏联动）

## 3. 层序与当前参数基线（先读通再改；§5 测出差值后才动）

### 3.1 hero 块（585dp 高，内容锚定，自下而上三层）
1. **海报原色全画**：`/movie|tv/{id}/images` 返回的 posters[0]（不排序），无遮罩无羽化。
   实证：参照海报 = gAgWJhUS（悬挂人影、下部自带英文标题），与参照帧同图。
2. **环境色遮罩（画在海报上层，这就是「底边一条线」的解法）**：
   `Brush.verticalGradient` 颜色 = 当前环境色（见 3.3），alpha stops：
   `(0f,0) (0.55f,0) (0.65f,0.95) (1.0f,0.97)`。
   实测依据：参照里亮路径可见到 fy≈0.55（渲染 199-204）、车辆区 fy 0.62 起已盖住（11,7,8）、
   海报下部自带的英文标题（fy 0.82-0.92）完全不可见。
3. **文字层**（画在遮罩之上）：logo（TMDB logos，语言序 zh→null→en，中心 fy 0.687、
   限高 115dp 限宽 78%、bg 透明、ContentScale.Fit；海报缺图回退 34sp 文本）、
   meta（fy 0.807 起，17sp/行高 17）、简介（15sp/行高 14，两行省略）、
   徽章行（fy 0.892）、圆点（中心 fy 0.982）。

### 3.2 固定屏位（参照帧实测，±3dp 容差，**禁止破坏**）
| 项 | 值 |
| --- | --- |
| hero 块高 | 585dp（2:3 铺满 390pt 宽） |
| meta 字形带 | 476-491.7pt，17sp，「类型 类型 · yyyy-MM-dd」居中 |
| 简介 | 字形 498.3-523，15sp/行高 14，两行省略 |
| 徽章行 | 522.7-532.7，块高 ~10 + 值 13sp |
| 圆点 | 中心 574.8，10 颗，Ø 中段 7.3/次端 6.0/端 3.3，间距 17.7，活动点=前景色 |
| 继续观看 header | 字形 603-625，22sp 粗，左缘 13dp |
| 继续观看卡 | 230×129.4 (16:9) r13 gap10 margin13，左下「看到 hh:mm」15sp 白 + 白条 3pt 贴底，卡下标题 15 粗/「N 天前 · 来源」13 灰 |

### 3.3 环境色与滚动耦合
- **环境色** = 海报位图底缘 10% 行平均色，**不做任何补偿**（参照 0138 场顶 (28,10,9) ≈
  原图底缘 (30,6,6)；灰白底海报 → 灰白场——用户原话「下面跟随海报底部的颜色而来」）。
- **下缘场**：内容坐标 [585, 1100dp]，平色 + 底部 29% α 渐隐（防下缘硬线），
  颜色与遮罩**同源同值**（585 处无缝）。
- **t(S) 混白比例**（场/遮罩颜色 = mix(环境色, pageBackground, t)）分段线性：
  `(0,0) (182,0.49) (301,0.81) (502,0.85) (900,1)`——三态夹逼：
  S=0 全暗 (28,10,9) / S≈182 淡紫 (129,119,123) / S≈301 近白 (202,197,204)。
- **遮罩整体不随滚动淡出**（颜色混合已承担过渡）；场也不做额外 α 淡出。

### 3.4 前景黑白
- **hero 组**（meta/简介/徽章/logo）：环境色 luminance <0.55 → 白，整页不变
  （参照 0138/0139 meta 都白；灰白底海报黑字）。
- **chrome 组**（节标题/卡标签/圆点/导航玻璃）：暗海报且滚动 S<110dp → 白，否则黑
  （0138 白 / 0139 黑夹逼；亮海报恒黑）。导航玻璃同步：暗环境=暗玻璃白字，
  亮环境=白玻璃黑字。

## 4. 已踩过的坑（真机实测，禁止重蹈）
1. `.background()` 修饰符画在 graphicsLayer(Offscreen) **外**——DstIn 羽化后浅色底原样
   露出（v10）。当前方案已无羽化，但若引入任何层内渐变，底色必须画在层内。
2. `Brush.verticalGradient` 缺省铺满**整块画布**——drawRect 的局部渐变必须显式
   `startY/endY`（v12 踩过：场渐变被拉伸到整个视口）。
3. 双重淡出（位图自带底部 α 渐隐 + 层羽化）叠加会在 fy 0.9-1.0 露出壳层（v9）。
4. 遮罩渐变区间过窄（v14 的 12dp）：海报中下部 220dp 裸露，白场下极刺眼（用户痛批）。
5. 场色与遮罩色必须**同源同值**，否则 585 处出现接缝（v13 前的割裂根源）。
6. 随滚动过渡用**颜色混合**（t(S)），不要用 α 淡出——α 淡出会露出层下内容。

## 5. 工作流（强制顺序）
1. **先测量后改参**：用 Python/PIL 对参照帧与我方截图做同列（x=180pt/180dp）逐行
   颜色 diff（步进 8pt），列出全部差值行——差值即改参依据。
2. 一次只改一个参数，装机 → 同角度截图 → 同列 diff → 记录。
3. 三态（S=0 / S≈180 / S≈300+）都过才能进下一项。
4. 通过后：注释中性化——项目铁律「源码零参照词」，参照帧编号/来源写进
   `docs/design/backend-core/239-home-rewrite.md` §6 与 `docs/evidence/p237/`，代码注释
   只写「帧实测/规格」。
5. 证据入库：三态并排图存 `docs/evidence/p237/home-rw-vN-*-compare.png`。

## 6. 验收标准
1. S=0 中心列（x=180）逐行：fy 0.45-0.55 亮路径可见（±15%）、fy 0.62+ 场暗
   （与参照 Δ≤15/通道）、fy 0.78-1.0 平色与参照 Δ≤10/通道；
2. S≈180/301：场色与参照 Δ≤15/通道；海报顶部残条可见度与参照一致；
3. 全滚动区间无硬线（每 120px 取色带采样，相邻差 ≤25/通道）；
4. 固定屏位表（§3.2）逐项 ±3dp；
5. 亮海报（灰白/米色底）一套同验：场=底缘色、文字黑、无遮罩压黑；
6. 资源库/在看 tab 回归正常。

## 7. 环境（本机实测可用）
```bash
# 构建（JAVA_HOME 必须显式）
cd "C:/kivo video/android" && JAVA_HOME="C:/Android/jdk-17.0.13+11" ./gradlew.bat :app:assembleDebug
# 安装（-r -t 必须；路径参数必须 MSYS_NO_PATHCONV=1）
ADB="C:/Android/sdk/platform-tools/adb.exe"; export MSYS_NO_PATHCONV=1
"$ADB" -s b6e950ee install -r -t app/build/outputs/apk/debug/app-debug.apk
# TMDB 代理（key 已在设备 filesDir/tmdb_key.txt；代理文件 tmdb_proxy.txt=127.0.0.1:10808）
"$ADB" -s b6e950ee reverse tcp:10808 tcp:10808
# 启动 + 进发现 tab + 截图
"$ADB" -s b6e950ee shell "am force-stop com.kivo.cinema; am start -n com.kivo.cinema/.MainActivity"
sleep 4; "$ADB" -s b6e950ee shell "input tap 416 2265"; sleep 10
"$ADB" -s b6e950ee exec-out screencap -p > shot.png
# 滚动：input swipe 540 1600 540 1000 400（值=像素，÷2.75=dp）
```
- 设备 Redmi K40 `b6e950ee`，1080×2400，density 2.75。
- HyperOS 无 logcat——调试不要依赖日志，用截图/逐行测量。
- TMDB 数据约 10s 到位再截图。

## 8. 已知数据源偏差（不要试图修，记录即可）
- 标题 logo：TMDB /images 的 zh 变体与参照所用变体不同（同为红色 zh 标题、同区同尺寸级）；
- 徽章行仅 TMDB 一项（IMDb/烂番茄/豆瓣无公开数据源，缺源不渲染——这是设计决定）。
