# 面板背景「抓帧 + 模糊」· 真机取证（2026-11-21）

团队批次 `panel-backdrop`（T1/T2/T3 并行），编译绿，覆盖安装 `Success`（`BD-00/03-install*.log`）。

## 1. 方案

- 抓帧走**内核通道** `mpv.command("screenshot-to-file", "<cache>/panel-backdrop.jpg", "video")`
  （与截图按钮同一实现），**不用 PixelCopy**：视频层是 SurfaceView，PixelCopy 在部分机型/PiP 会拿黑帧。
- 降采样 `inSampleSize`（实得 **480×270**）→ 三遍可分离盒式模糊（半径 clamp 3..16）→ `ImageBitmap`。
- 绘制：`playerPanelSurface(backdrop=…)` 在玻璃渐变**之前**铺图，裁在圆角内；**不叠额外白洗**。
- 只在**面板打开那一刻**抓一次（`rememberPanelBackdrop(open)`），面板开着时画面继续动但背景不更新。

## 2. 读数

| 项 | 实测 | 参照 |
| --- | --- | --- |
| 抓帧 | trace `panel-backdrop ok 480x270` | — |
| 暗场面板合成 | **114.0** | SenPlayer 117–120 ✓ |
| 亮场面板合成 | **234.7** | SenPlayer 等价场景 218–226（其底 208；本测试片纯白 255） |
| 模糊（面板内相邻像素差 <8 占比） | 暗场 **0.981** / 亮场 **0.992** | SenPlayer 彩条底卡片内部零锐边 ✓ |
| 加 backdrop 前（无图） | 亮场 **200.5** | 见 `V3-kivo-panel-bright-panel.png` |

证据：`BD-04-kivo-panel-{bright,dark}.png`、`BD-02-bright-late.png`。

## 3. 一处修正（首版做错，已改）

首版在模糊图上又叠了一层 **35% 白**，再叠 40% 玻璃 ⇒ 亮场被推到 **241.3**，比 SenPlayer 更亮、白字更糊。
删掉那层后回到"只有一层 40% 玻璃"，与对方公式等价：`0.4·255 + 0.6·L`。

## 4. 边界（不夸大）

1. **亮场白字仍不可读**（合成 234.7、白字 255）。这是"白玻璃 + 白字"的固有代价，
   SenPlayer 在亮底同样如此（218–226）。**持平，不是解决**。要真正可读需按背景亮度切换深色卡片/深色字，
   那是材质体系的主题自适应，超出本轮（且会推翻用户已拍板的 v3）。
2. 背景**只抓一次**（打开时），面板开着不随画面更新 —— mpv 截图是同步命令，每帧抓会抢内核实锁。
3. 内部无明显延迟：`LaunchedEffect` 在 IO 调度器抓帧，首帧前 `backdrop=null`（纯玻璃）。
4. 未测：投屏卡（`CastSheet` 不传 backdrop，保持纯玻璃）、Toast、`PlayerCaptureButton`（默认 null，逐像素不变）。
5. 抓帧失败路径未在真机人为触发（代码按 `no-session`/`command-rejected`/`no-file`/`decode-null` 四类写 trace）。

## 5. 收尾

设备临时截图已清；未 commit / push。
