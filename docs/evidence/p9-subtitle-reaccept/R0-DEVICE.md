# 面板 v3 低项（4 项）· 真机取证（2026-11-21）

团队批次 `panel-polish-v3`（T1–T4），编译 `BUILD SUCCESSFUL`，覆盖安装 `Success`（`R0-00-install.log`）。

## 1. 四项与读数

| # | 项 | 改动 | 真机读数 | 判定 |
| --- | --- | --- | --- | --- |
| R1 | 整圈内描边可见宽度 | `Stroke(hairline)` → `Stroke(2*hairline)`（外侧一半被 `clipPath` 裁掉） | 同底色（`kivo-panel-mid`）左缘剖面：改前 `1674=56, 1675=44, 1676=33`（可见 ≈1.4px）；改后 **`1674..1676 = 180/181/178`（3px ≈ 1.09dp）** | PASS |
| R2 | 子内容溢出卡片圆角 | 两壳内容层 `clip(RoundedCornerShape(radius))`（fade 在外、clip 在内）；`GlassMenu` 内再加一层 clip 容器把 `PanelScrollbar` 一起包住；`panelPressable` 覆盖改 `drawRoundRect` | 按住最底行截 `R0-04`：右下角逐行收口（`95→98→118→67→0`），圆角外**无 4% 黑方角**，描边沿弧连续 | PASS |
| R3 | 滚动期每帧重建 | `LaunchedEffect(state)` + `snapshotFlow { state.isScrollInProgress }`（key 不含 `state.value`）；`panelVerticalFade` 改 `drawWithCache` | 代码核对（`PlayerPanelScroll.kt:102-104`）；滚动中截图 `R0-03` 滚动条正常显隐 | PASS |
| R4 | 投屏 sheet 硬编码 | 字号/圆角/padding 全部取既有 `PlayerPanelStyle` token | `CastSheet.kt` 内已无 `fontSize = <数字>.sp`；`R0-06` 截图为浅灰玻璃、标题/状态/收起排版正常 | PASS |
| R5 | 规范与代码一致 | §15 标为历史；§16.2 按代码回写 4 处（分隔线黑 10%、按压 白 8%例外/黑 4%/黑 16%、步进圆底 黑 12%+描边 20%、补 `insetSurface` 黑 8%） | `grep` 规范文本与 `PlayerPanelStyle.kt` 取值一致 | PASS |

## 2. 过程中的误判与纠正（如实记录）

- 首测在 `kivo-panel-dark` 上看到左缘"双线"（1674-1676 与 1680-1681 各一条亮线），一度怀疑描边被画了两遍。
  与 `V3-SEN-base-dark`（纯色底 30）对照：纯色底上**只有一条线**（1674 一条）⇒ 第二条是**测试片底图内容**，不是绘制缺陷。
  结论以纯色/`kivo-panel-mid` 对照为准。

## 3. 未取证边界

1. R3 未做帧级分配计数（无 systrace/Perfetto 采样），只做了代码结构与显隐行为核对。
2. 投屏 sheet 实际弹在**左上角**（`R0-06`），与"贴底 sheet"的描述不符；这是**既有布局**（`CastSheet` 自身不含对齐代码），本批未改，登记待查。
3. 圆角溢出仅在"按住最底行"一种状态取证；滚动条顶/底端溢出用几何推算 + `R0-03` 截图确认，未逐像素量顶端。

## 4. 收尾

设备临时截图已清；未 commit / push。
