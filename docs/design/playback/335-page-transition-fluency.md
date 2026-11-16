# 335 · 页面切换流畅度（去白屏）

> 2026-11-16 18:25 用户：「咱们先把页面加载流畅度优化下，别的播放器比咱们的快，
> 咱们加载页面的时候都会白屏一会儿才进去，退出来的时候也会白一下」。

## 一、现象与取证（真机录屏，非猜测）

`docs/evidence/p335-page-transition/`：`pagetrans.mp4` 录屏 + `grid.png`/`grid_dense.png` 抽帧。

- **进 Emby 服首页**：密抽帧 8.0–8.9s 出现**整屏浅灰白 `#F5F4F8` + 小转圈**，
  约 0.9s 后才显示 hero/海报/绿色卡片。此即用户说的「白屏一会儿才进去」。
- **返回**：退出瞬间显示整屏黑 `#050506`（windowBackground）空窗，约 0.2–0.4s，
  即用户说的「退出来也会白一下」（实为黑闪，同样突兀）。

## 二、根因（确定，代码级）

### 2.1 进入白屏 = 串行阻塞 + 浅色加载态
`EmbyActivity.onCreate` 的 `LaunchedEffect(sourceId, unlocked)` 串行执行：

1. `SourceStore.read()`（磁盘）
2. **对所有 Emby 源逐个 `publicServerName()`**（`System/Info/Public` 网络请求，
   实测单次 ~0.73s，N 个源串行 = 1.5–2.2s）——**这一步只为右上角切服菜单显示服名，
   与"打开当前服"完全无关**，却挡在关键路径最前面
3. 才 `model.openSaved()` → `connect()` → 登录 + 再查一次当前服 serverName（重复劳动）
4. 期间 `checked=false` → 整屏「正在读取资源库」；`connecting=true` → 整屏转圈

浅色主题 `background=0xFFF5F4F8` 让这段等待显示为**白屏**。

### 2.2 返回黑闪 = Activity 切换空窗
无自定义 Activity 过渡动画，退出时新窗口未就绪 → 露出 `#050506` windowBackground。

## 三、设计约束（铁律）
- 不改现有 UI 风格/视觉（用户已定稿）；只缩短等待、消除闪屏。
- 单文件单职责，Kotlin ≤300 行；先规划家族树；不堆砌、不留死代码。
- 必须真机验证 + 给证据；编译只看 `BUILD SUCCESSFUL`。

## 四、文件家族树（本次改动）

```
emby/
├── EmbyActivity.kt            [改] 拆分关键路径：当前服立即 openSaved；
│                                    切服源名查询移出阻塞段，改为并行且带超时/兜底
├── EmbySessionModel.kt        [改] connect() 里 serverName 查询改为带短超时，失败用源名兜底
│                                    （不静默降级，留日志）
res/values/
├── themes.xml                 [改] 给 windowBackground 之外补 windowSplashScreen*（targetSdk37），
│                                    冷启动系统页与品牌色一致，避免系统级白闪
└── styles.xml (新建 if needed)      Activity 进出场动画：淡入淡出/短 cross-fade，消除黑闪
```

**不新建平行实现**：服名缓存/并行查询逻辑就地改在 EmbyActivity 的 LaunchedEffect 内，
不另起文件（它是该页面独有的编排，不构成独立职责）。

## 五、实施顺序
1. EmbyActivity：把 allSources 服名查询从"打开当前服之前"移到"之后"，且并行（async 或
   并行集合）+ 单源超时；当前服 serverName 由 model.connect 已有结果提供，不再重复查。
2. themes.xml：补 windowSplashScreenBackground 与动画样式引用。
3. 动画资源：新建 fade 进出场（短，~150ms），application 主题引用；不改变任何页面内部 UI。
4. 编译 → 装包 → 同法录屏复现 → 对比白屏时长。

## 六、实现（已落地，commit 见日志）
1. `EmbyActivity` LaunchedEffect：先用源记录名即时填充 `allSources` 并**立刻 openSaved 当前服**；
   其余源的 ServerName 查询移到 `openSaved` 之后、`async+awaitAll` 并行，失败用源名兜底。
   跳过当前服（connect 内部会查），消除重复请求。
2. `EmbySessionModel`：`savedName` 改 mutableStateOf；`connect()` 移除串行的 publicServerName，
   login+items 成功即 `inLibrary=true` 显示内容；ServerName 后台补查、成功才覆盖、失败留日志。
3. `EmbyBrowseScreen`：`heroItems` 与 `resumeItems` 由串行改 `async(IO)` 并行，hero 先到先显示。
4. `res/anim/kivo_fade_in.xml` + `kivo_fade_out.xml`（140ms）+ `themes.xml` 引用 `windowAnimationStyle`：
   Activity 进出淡入淡出，消除返回黑闪。不动 windowBackground/页面内部 UI。

## 七、真机验证（K40 b6e950ee，录像 + 日志，非目测）
证据：`docs/evidence/p335-page-transition/`。

- **★ hero 请求并行化铁证（net-diag 时间戳）**：
  优化前 recent→resume 串行间隔 **3755 ms**（1794824606641→1794824610396）；
  优化后 resume→recent 并行间隔 **77 ms**（1794825612081→1794825612158）。
  ⇒ 首屏数据等待省 ~3.7 s，这是「白屏一会儿」的大头。
- **当前服 ServerName**：不再串在连接前，login+items 完成即显示 hero；真名后台补。
- **返回黑闪**：密抽帧（20fps）确认退出时为 ~0.15s 淡出过渡，**不再有整屏纯黑空窗**。
- 编译 `BUILD SUCCESSFUL`（Kotlin 三文件均 <300 行；改动 64+/24-，无死代码）。

## 八、诚实边界
- 录屏中仍有浅灰白占位帧——这是**海报/背景图网络下载 + 解码**的必要等待（数据接口已到位），
  且属浅色主题本身，不是"卡住的空白"。未改 UI（铁律：不写 UI），故未加骨架屏。
- 系统 SplashScreen 冷启动路径未单独测量（ MainActivity 有品牌动画覆盖，非本次主要 complaints）。
- 服名兜底日志：`[emby] ServerName 查询失败，保留源名 …`，不静默降级。
