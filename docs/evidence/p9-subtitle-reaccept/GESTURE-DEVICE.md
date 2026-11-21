# 播放页手势全部失灵 · 根因与修复 · 2026-11-21

> 用户报告「播放中唤醒不了任何控制面板，手势都没了」。本文记录二分定位与修复全过程。

## 1. 结论

**根因**：`PlayerPinch.kt` 的 `pinchZoom` 在 `awaitFirstDown` 之后**无条件** `state.pinching = true`，
而 `PlayerGestureLayer` 把 `pinching` 当「双指进行中」的打断信号（interrupted 判定第一项）。
Compose 事件分发里 pinch 层（Modifier 链外层 = 事件树祖先）在 `PointerEventPass.Initial`
先于手势层（后代，Main pass）——**任何单指手势刚 down 就被自己的 pinch 层宣判"双指进行中"**，
下一个事件到手势层就 `interrupted → detector.cancel()` ⇒ tap/双击/横滑 seek/竖滑亮度音量/长按
**全部失灵**。控制面板靠 tap 唤出 ⇒ 用户看到的正是"唤醒不了任何控制面板"。

该组合是手势层重写批次（gesture/ 新目录 + PlayerGestureLayer/Router/Root 大改）引入的回归；
HEAD 无此问题。文件归属：`PlayerPinch.kt`（untracked，收尾批次新建）。

## 2. 二分定位过程（都留了证据）

| 步骤 | 手段 | 结果 |
| --- | --- | --- |
| 1 | 静态看 Root 接线 | `PlayerOverlay`/`onTap`/`controlsVisible` 都在 ⇒ 接线未断 |
| 2 | `--ez kivo.touchLog true` 探针（Root 自带取证） | 3 次 `touch probe x=1200 y=540 consumed=false` ⇒ 事件到达、无消费 |
| 3 | tap 后 1.2s 截屏 | 无控制层 ⇒ 手势判定未生效（`GESTURE-1-after-tap.png`） |
| 4 | `consumed=false` 反推 | 手势层 `up()` 返回 false ⇒ `state` 在 up 时已非 PENDING ⇒ 被中途 cancel |
| 5 | 排查会 cancel 的条件 | `interrupted` 第一项 `currentPinch.pinching`；读 `PlayerPinch` ⇒ down 无条件置位 |

**证据链闭环**：pinch 只在 ≥2 指才 `consume()` ⇒ probe 的 `consumed=false` 与「pinching 置位但未消费」精确吻合。

## 3. 修复（PlayerPinch.kt 一处）

`pinching` 改为按**真实指数**置位：循环内 `twoFingers = pressed >= 2`，`state.pinching = twoFingers`；
down 事件本身 1 指 ⇒ false。双指落下的事件 ⇒ true + 消费（打断单指判定），原竞品契约不变。

## 4. 修复后真机验证（b6e950ee）

| 手势 | 结果 | 证据 |
| --- | --- | --- |
| tap 唤控制条 | ✅ 顶栏（标题/投屏/弹幕/画中画/选集）+ 底栏（进度条/暂停/倍速）完整出现 | `GESTURE-2-fixed.png` |
| 竖滑音量 HUD | ✅ 右侧面板 + 扬声器图标 + 100% + 竖向滑块 | `GESTURE-4-volume-slow.png` |

⚠️ 取证注记：`GESTURE-3-volume-hud.png` 为一次**假阴性**——tap 唤出控制条后立刻 `input swipe … 250`
（250ms 快滑），HUD 未出现；改 400ms 慢滑（GESTURE-4）即 PASS。判定为注入时序干扰而非功能缺陷，
如后续需自动化验收竖滑，固定用 ≥400ms 的 swipe 注入。

## 5. 边界

- 双击快进/后退、横滑 seek、长按倍速未逐一取证（同一 detector 状态机，且 pinching 修复
  移除的是它们的共同阻断条件）；用户日常使用若再遇到具体手势失灵，按 §2 的 touchLog 探针法复验。
- `ui_gate` 复跑 RESULT: PASS；本轮改动仅 `PlayerPinch.kt`（untracked 文件内 +2 行逻辑）。