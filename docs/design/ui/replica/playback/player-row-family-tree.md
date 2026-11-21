# 播放页「微调卡行控件」文件家族树（拆分方案 · 未动代码）

> 状态：**已落地 2026-11-21**（团队批次 `stepper-split` T1）。5 个文件现位于
> `ui/compose/player/row/`，行数 167/185/76/47/46（旧 450 行文件已删除）。
> 实测契约修正：`StepperButton` 为 `internal`（被 `PlayerStepperRow` 跨文件调用）。
> 证据 `docs/evidence/p9-subtitle-reaccept/S0-DEVICE.md`。
> 触发原因：`ui/compose/player/PlayerStepperRow.kt` 现 **450 行**，超过铁律「每个 `.kt` ≤ 300 行」。
> 该文件同时承载 **5 个变化原因**（步进行 / 数值列 / −+ 按钮 / 开关行 / 开关 pill / 重置行），
> 任何一处要改都得翻开另外四处的代码 —— 这正是"按变化原因拆"该解决的事。

## 1. 现状职责盘点（行号即证据）

| 行号 | 内容 | 变化原因 |
| --- | --- | --- |
| 60–159 | `PlayerStepperRow`：行高、标题/副题、失败态判定（1.2 s 未落地 / 1.5 s 提示） | 行的排版与"没落地"提示节奏 |
| 160–192 | `StepperValue`：数值列 64 宽、右对齐、tnum、100 ms alpha 交叉 | 数值列的视觉（**零件，不单独成文件**） |
| 193–263 | `StepperButton`：44 命中盒、常驻圆底、按压 12% 圆、按下/取消/长按入口 | 按压反馈与连击入口 |
| 264–290 | `StepperCircle`：常驻圆底（32 dp、7% 填充 + 12% 描边） | 按钮的零件（**并入按钮**） |
| 291–346 | `HoldResult` + `awaitHoldOrRelease` + `repeatSteps`：抬起单步 / 位移 8 dp 取消 / 400 ms 后 80 ms→60 ms、连续 3 步未变判边界 | 连击节奏与边界判定 |
| 347–397 | `PlayerToggleRow`：64 高、整行可点、题 16/22 + 副 12/17、行尾 pill | 开关行排版 |
| 398–425 | `PlayerTogglePill`：46×28、圆角 14、旋钮 240 ms spring | 开关视觉与动效（**两处共用**） |
| 426–450 | `PlayerResetRow`：48 高、14 sp/68% 居中、固定卡底 | 重置行的位置与字号 |

## 2. 目标家族树

```
ui/compose/player/row/                        ← 微调卡的三类行（P10 §7）
├── PlayerStepperRow.kt      步进行：行编排 + 失败态（1.2s/1.5s）+ 数值列零件
├── PlayerStepperButton.kt   −/+ 按钮：常驻圆底 + 按压 12% + 按下/取消/长按连续状态机
├── PlayerToggleRow.kt       开关行：64 高、题/副、行尾 pill
├── PlayerTogglePill.kt      开关 pill 本体（微调卡与列表两处共用）
└── PlayerResetRow.kt        重置行：48 高、固定卡底
```

预估行数（含 import）：约 110 / 190 / 75 / 45 / 40，全部远低于 300。

**为什么不拆 `StepperValue` / `StepperCircle`**：两者各 31 / 27 行、唯一调用方就是宿主控件，
拆出去只会得到"只有一个调用方的空壳"，违反同一条铁律。它们是宿主的零件，留在宿主文件。

## 3. 落位与迁移边界

- `package` 变更：`com.kivo.cinema.ui.compose.player` → `...player.row`。
- 需要新增 import（子包不再共享父包作用域）：
  `PlayerPanelStyle`、`PressIndicator`、`panelPressable`、`PlayerOptionItem`、`PlayerRowControl`。
- 消费方（**只改 import，不改调用**）：
  - `ui/compose/player/PlayerDetailCard.kt`（`PlayerStepperRow` / `PlayerToggleRow` / `PlayerResetRow`）
  - `ui/compose/player/PlayerGlassMenuParts.kt`（`PlayerTogglePill`）
- **零行为改动**：不调整任何数值、不改 `PlayerPanelStyle`、不改语义（contentDescription / role / stateDescription）、
  不改 12% 按压色与控制栏 8% 的拆分（`stepperPressColor` vs `pressCircleColor` 保持分离）。

## 4. 验收（拆分后必须重做，不沿用旧截图）

1. `:app:assembleDebug` 绿；`grep` 确认原 450 行文件已不存在、无重复定义。
2. 真机 `b6e950ee` 像素回归：
   - 步进按钮按住 ≈**12%**（旧证据 59/62/67）、控制栏文字钮纯黑区 **(20,20,20)=8%**；
   - 行高与「底部边距 / 仅文本字幕」不裁剪（L5 口径）；
   - 开关 pill 240 ms spring 位移肉眼可见、无瞬跳。
3. 证据落 `docs/evidence/p9-subtitle-reaccept/`（与字幕轮同目录，前缀 `ROW-*`）。

## 5. 不在本次范围

- `PlayerDetailCard.kt`（284 行）里的 `when (control)` 分派：那是"卡如何选行"的变化原因，
  与"行怎么画"不同，暂不抽。
- 面板材质参数（`PlayerPanelStyle`）重组：属 P10 面板线，另开。

## 6. 风险

| 风险 | 处置 |
| --- | --- |
| 子包 import 漏改导致编译红 | 一次迁完再编译，不逐个文件半途提交 |
| 迁移时手改逻辑（最危险） | 按行号段**整段搬**，搬完逐段 diff 对照，不改一行表达式 |
| 与工作区其他助手的在途改动冲突 | 拆前 `git status` 核对该文件无他人未保存改动；本方案不 commit/push |