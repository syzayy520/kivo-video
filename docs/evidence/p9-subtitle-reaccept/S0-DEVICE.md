# StepperRow 拆分 + 投屏 sheet 贴底 · 真机取证（2026-11-21）

团队批次 `stepper-split`（T1/T2 并行），编译 `BUILD SUCCESSFUL in 41s`，覆盖安装 `Success`（`S0-00-install.log`）。

## 1. T1 · 行控件拆分（零行为改动）

| 项 | 读数 |
| --- | --- |
| 旧文件 | `player/PlayerStepperRow.kt` **已删除**（`ls` 报 No such file） |
| 新文件行数 | `PlayerStepperRow 167` / `PlayerStepperButton 185` / `PlayerToggleRow 76` / `PlayerTogglePill 47` / `PlayerResetRow 46`（合计 521，含新增 import；全部 ≤200） |
| 符号 | `PlayerStepperRow` / `StepperButton` / `PlayerToggleRow` / `PlayerTogglePill` / `PlayerResetRow` 各 1 处定义，无重复 |
| 消费方 | `PlayerDetailCard.kt` import 3 条、`PlayerGlassMenuParts.kt` import 1 条，调用点未改 |
| 真机 | `S0-01` 音频延迟微调卡：−/+ 圆底可见（暗圆 + 白符号）、数值 `0.0 s`、底部「重置」在 —— 与拆分前一致 |

**契约澄清（已回写 design.md §2.2）**：`StepperButton` 被 `PlayerStepperRow`（另一文件）调用，
故为 `internal` 而非表内原写的"文件内私有"；同文件 `StepperCircle` / `HoldResult` / `awaitHoldOrRelease` /
`repeatSteps` 仍 `private`。coder 的判断正确，契约矛盾已消除。

## 2. T2 · 投屏 sheet 贴底

| 项 | 读数 |
| --- | --- |
| 位置 | 改前弹在**左上角**；改后 sheet 顶边 **y≈756**（屏高 1080 的 70%），底边到 **1080**（贴底），`fillMaxWidth` 全宽 |
| 材质 | 内部采样 `(113,113,113)` → `(96,96,96)`（自上而下渐变，浅灰玻璃），顶边描边亮线 185 |
| 文字 | 亮像素 255 出现在 y 800–980（标题 / 状态 / 收起） |
| 改动 | `CastSheet` 新增末位 `modifier` 并接到根 Column 首位；`PlayerComposeRoot` 传 `Modifier.align(Alignment.BottomCenter)` |

## 3. 未取证边界

1. 行控件只做了**视觉核对**（步进卡一种形态），未逐一点按 8 类行（复选 / 三态 / 滑杆 / 开关 / 重置）验证交互；
   拆分按"整段搬迁、不改表达式"执行，交互回归风险低但**未实测**。
2. 投屏只测了**无设备**（扫描中）形态；有设备时的列表行排版未截图。
3. `pageEdgeFade`/动效时长等无改动，未重测。

## 4. 收尾

设备临时截图已清；未 commit / push。
