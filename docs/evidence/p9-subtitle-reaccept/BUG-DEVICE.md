# 关闭面板「中间闪一下」· 根因与修复（2026-11-21）

用户报告：**关闭字幕等设置窗口时，中间部分会瞬间显现一下。**

## 1. 取证方法（关键）

- 静止画面（`--ez kivo.freeze true`）录屏只有 **≈4.9 fps** —— 编码器对静止画面丢帧，抓不到 160ms 动画。
- 改用**正在播放的片源**（`1080p30_60s.mp4`，画面持续变化）录屏 ⇒ **60 fps**，关闭过程可逐帧观察。
- 判定量：面板区（x1700–2350, y250–900）与**屏幕中间区**（x700–1500, y300–800）的逐帧均值。

## 2. 根因

`PlayerOverlayPanel.FloatingPanel` 收到 `kind = activePanel`，并用调用方传入的
`anchor = PlayerPanelHost.anchorFor(activePanel)` 摆位。

关闭时 `activePanel = ""` ⇒ **`anchorFor("")` 落 `else → PlayerPanelAnchor.Center`** ⇒
正在淡出的面板**瞬移到屏幕正中**，于是"中间闪一下"。

（`lastKind` 快照只保住了"画哪个壳"，没保住"摆在哪"；同理 `items` 被清空会让淡出期变成空玻璃。）

## 3. 读数（60fps 逐帧）

| 版本 | 关闭帧的中间区亮度 | 跳变 |
| --- | --- | --- |
| **修复前** `BUG-02-close-live.mp4` | g194 `129.75` → g195 **`166.90`** → g196 `129.50` | **+37.15** |
| **修复后** `BUG-08-final.mp4` | q185 `130.13` → q186 `129.92` → q187 `129.91` | **+0.01** |

面板区（右侧）两版都在 1 帧内 165.6 → 106.7（硬切，见 §5）。

## 4. 修复

`ui/compose/player/PlayerOverlayPanel.kt`
- 删除 `FloatingPanel` 的 `anchor` 参数；锚点改为 **由 `shownKind` 派生**
  （`val effectiveAnchor = PlayerPanelHost.anchorFor(shownKind)`），关闭的 160ms 里位置不跳。
- 与 `lastKind` 同思路，补 **title / items 快照**（只在非空时更新），淡出期不再变成空玻璃。
- `ui/compose/player/PlayerOverlay.kt` 调用处删掉 `anchor = ...` 实参。

## 5. 附带发现（未修，已登记）：退场动画不生效，5 轮探针结论

P10 §8 写"退场 160ms"，实测**完全没有退场动画**。逐轮探针（全部已回滚）：

| # | 探针 | 结果 |
| --- | --- | --- |
| 1 | `exitMs` 160 → **800ms** | 面板区仍 **1 帧** 165.6→106.7（`BUG-06-probe.mp4`）⇒ 不是时长问题 |
| 2 | 弃用 `AnimatedVisibility`，改 **alpha + 位移手动 Animatable** | 60fps 逐帧仍 1 帧硬切（`ANIM-01-close.mp4`） |
| 3 | `if (true)` 强制渲染 + `alpha = 1f` | 关闭后面板区不再出现 ⇒ **组合被移除**，不是动画值问题 |
| 4 | `if (false)` | 打开时面板也不显示 ⇒ 证明改的这段**确实在真实渲染路径**上 |
| 5 | `targetKind.isBlank()` 时画 40dp 品红兜底 | 打开/关闭都不出现（`ANIM-13/14`） |

**结论**：关闭的一帧内，`FloatingPanel` 的整个组合被移除（或内容被尺寸化为 0），
所以**任何组件内部的动画（Enter/Exit、Animatable、手动 alpha）都不可能生效**。
要修必须改显示时序（`PlayerOverlay` / Activity 在关闭时不再同帧移除组合），
范围从"调一个动画时长"变成"改面板生命周期"，风险也随之上升。

### 5.1 追加 3 轮（当晚第二轮，`EXIT-*` 证据）

| # | 探针 | 结果 |
| --- | --- | --- |
| 6 | 关闭时**延迟 260ms 再清 kind**（open 立即 false） | 仍 1 帧（`EXIT-01-close.mp4`） |
| 7 | 改成"关闭期用 open 判 title/items 快照"（避免空壳 0 高） | 仍 1 帧（`EXIT-03-close2.mp4`） |
| 8 | 给 `FloatingPanel` 加 `key("floating-panel")` 钉住组合身份 | 仍 1 帧（`EXIT-05-close3.mp4`） |

**结论（12 轮探针，全部回滚）**：关闭那一帧里面板内容**同步消失**，不是动画时长、不是 remember 丢失、
也不是组合身份移位 —— 组件内的任何方案（Enter/Exit、Animatable、延迟清空、快照、key）都无法让它淡出。

**决定**：本轮**不做**退场动画。若必须做，只有两条路，成本都显著高于"修一个动画"：
① 把面板挪到独立窗口（Popup/Dialog Window）再动画；② 在 Activity 层用固定周期自绘面板。
建议单独立项评估，不建议在现有结构上继续试。

### 5.2 最终态（回滚后）

只交付"中间闪现"修复。最终包复验：关闭帧中间区跳变 **0.34**（`EXIT-07-final.mp4`，回归通过）；
面板区仍 165.0→106.7（硬切，已知、已登记）。

## 6. 未取证边界

1. 只实测了字幕卡；其它面板共用同一 `FloatingPanel` 路径，未逐个录屏。
2. 中间区采样只覆盖横屏中部（x700–1500）；若面板宽度不同、闪现位置另有偏移，采样可能漏检。
3. 未测量"打开"方向是否有同类问题（本次只针对关闭）。

## 7. 收尾

设备临时录屏已清；未 commit / push。
