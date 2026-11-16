# 335 · 页面切换流畅度验证（2026-11-16 傍晚，K40 b6e950ee）

## 现象
用户：加载页面白屏一会儿才进去，退出来也会白一下。

## 根因（代码级确定）
1. `EmbyActivity` LaunchedEffect 把所有 Emby 源的 `publicServerName()`（网络，单次实测 ~0.73s）
   **串行**挡在 `openSaved()` 之前 —— 与打开当前服无关，却让用户对着白屏等 N×0.73s。
2. `EmbyBrowseScreen` 里 `heroItems()` 与 `resumeItems()` 两个网络请求**串行**
   （日志间隔 3755ms），首屏 hero 被第二个请求拖住。
3. `EmbySessionModel.connect()` 把当前服 `publicServerName()` 串在 login+items 之后、
   `inLibrary=true` 之前，内容晚 ~0.7s 显示。
4. 无 Activity 过渡动画，退出露 `#050506` windowBackground 黑空窗。

## 修复
见 `docs/design/playback/335-page-transition-fluency.md` §六。

## 证据
- hero 请求并行化（net-diag 时间戳）：
  - 优化前串行：1794824606641 → 1794824610396 = **3755 ms**
  - 优化后并行：1794825612081 → 1794825612158 = **77 ms**
- 抽帧宫格：
  - `before-fill-grid.png` 优化前进入段（浅灰白+转圈明显）
  - `after-fill-grid.png` 优化后进入段
  - `after-back-grid.png` 返回段（20fps 密抽帧，~0.15s 淡出，**无整屏黑闪**）
- 编译：`BUILD SUCCESSFUL in 26s`；Kotlin 三文件 161/208/219 行（均 <300）。

## 诚实边界
- 仍有浅灰占位 = 海报/背景图网络下载+解码，属浅色主题内容加载，非卡死空白；
  未加骨架屏（铁律不写 UI）。
- 冷启动系统 SplashScreen 未单独测（MainActivity 有品牌动画覆盖）。
