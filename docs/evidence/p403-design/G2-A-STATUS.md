# G2-A · P1 手势与动画门修复验证（403 规划第二批）

时间：2026-11-18 03:40 GMT+8
设备：K40 `b6e950ee`（A13，用户指定机型）
范围：D7 播放器三处手势 + D9 动画门（G2 的 D5/D6/D8/D10/D11 留下一批）。

## 改动文件（本轮 8 个）
| 文件 | 缺陷 | 变化 |
|---|---|---|
| `ui/compose/HandSeekState.kt` | D7-1 | 加 `active:Boolean` 替 `startMs=0` 哨兵；`begin` 置 true、`clear` 置 false |
| `ui/compose/PlayerGestureRouter.kt` | D7-1 | `onHorizontalDrag` 判 `!hSeek.active`，片头 position=0 不再被吞 |
| `ui/compose/player/PlayerBarsHost.kt` | D7-2 | BarSlot 退场动画结束后卸载内容（hidden 不再命中），保留单树无双进度条 |
| `ui/compose/player/PlayerPinch.kt` | D7-3 | `PinchScaleState` 加 `pinching` 状态，手势中维护、结束时清 |
| `ui/compose/player/PlayerGestureLayer.kt` | D7-3 | 四个单指检测器统一 `singleFingerBlocked()` 闸门，多指时显式排除 |
| `ui/motion/MotionGate.kt` | D9 | `reduce = appEnabled==true \|\| systemReduced`（系统无障碍为硬约束） |
| `ui/compose/player/PlayerPanelHost.kt` | D9 | 删第二套只查单 scale 的 `rememberReduceMotion` |
| `ui/compose/player/PlayerOverlay.kt` | D9 | 统一用 `MotionGate.rememberAppReduceMotion()` |

## 编译
- `build-g2a.log` / `build-g2a-full.log` / `build-g2a-final.log`：**BUILD SUCCESSFUL**。
- 首轮 6 引入错误为验证脚手架问题（internal 属性跨 sourceSet accessor），非生产代码；脚手架已删。

## 验证结果

### D9 动画门 —— ✅ 真机逻辑 6/6 PASS
- 证据：`g2-d9-motiongate-assertions.txt`（K40 上 instrumentation 实跑 `MotionGate.reduce` 全真值表）。
- 关键：`reduce(false, true)=true`（App 关 + 系统开 → 减弱），旧代码此处错误返回 false，修复后系统无障碍不再被 App 开关覆盖。

### D7-2 隐藏控制栏横滑 —— ✅ 真机通过
- 证据：`g2-player-localonly.png`（控制栏隐藏态）、`g2-hidden-bar-swipe.png`（横滑后）。
- 现象：控制栏隐藏时横滑 → 控制栏被唤出 + 画面中央 seek HUD（`00:37 / 05:28`）+ 缩略图浮窗 + 进度条跳到 `00:38`。若透明控制栏仍吃事件，横滑根本不会生效。
- 素材：本地视频 `VID_20240729_195421.mp4`（5:28），经 `--es kivo.media <path>` 起播（**注意**：不能同时传 `kivo.playurl`，那会被当 HTTP URL 导致黑屏 duration=0）。

### D7-3 双指缩放不跳进度 —— ✅ 真机通过
- 证据：`g2-pre-pinch.png` → `g2-post-pinch.png`（`input motionevent` 模拟双指张开）。
- 现象：双指缩放后**无任何 seek HUD / 快进图标 / 进度条浮层**，控制栏保持隐藏，画面继续播放 ⇒ 多指未误触发横滑 seek。
- 未覆盖：缩放本身是否放大画面（SurfaceView + Compose graphicsLayer 的效果取决于设备/API，PlayerPinch 注释已标 ◐ 推断，正解是写内核 video-zoom，本轮按约束不做）。此项是"缩放能否生效"，不是本轮"缩放不跳进度"的目标。

### D7-1 片头横滑哨兵 —— ✅ 逻辑修复 + 横滑链路真机通过；严格 position=0 瞬间未精确复现
- 代码：`active` 布尔替哨兵，`begin(0)` 后 `active=true`，判定不再依赖 `startMs>0`。
- 真机：`g2-hidden-bar-swipe.png` 证明整条横滑链路（HUD/seek/进度条）工作；横滑发生在起播后数秒内，非严格 position=0。
- 缺口：严格 `positionMs=0 且 durationMs>0` 的瞬间窗口极短，adb 无法精确命中；逻辑层已消除该边界的失效可能（`begin(0)` 不再被判为无效），诚实标注为"逻辑修复 + 链路真机通过，边界瞬间未精确复现"。

## 清理
- 所有验证 instrumentation 脚手架已删（grep 0 命中）；设备临时文件已清；最终 `assembleDebug` = BUILD SUCCESSFUL。
- **未提交、未推送。**

## 下一批（G2-B）
D5 返回键统一（新建 KivoBackPill + 5 处替换）、D8 preview 失败重试、D10 模块假开关、D11 主线程 IO。
