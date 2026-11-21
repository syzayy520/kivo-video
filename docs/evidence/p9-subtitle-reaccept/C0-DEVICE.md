# 投屏卡 v2（SenPlayer 面板语言）· 真机取证（2026-11-21）

设计：`docs/design/subtitle-plane/p10-panel-material-spec.md` **§17**。编译 `BUILD SUCCESSFUL`，三次覆盖安装 `Success`（`C0-0{0,1,2}-install*.log`）。

## 1. 参考取证：SenPlayer 没有自绘投屏面板

点 SenPlayer 顶栏投屏图标后**直接退出到系统桌面**（`C2-sen-cast.png`，竖屏 Launcher）——
它走系统级投屏。因此"参考"落在它的**面板语言**（`SEN-02/04` 倍速面板）：
窄宽浮动卡、13.5dp 圆角、左对齐行、右侧 chevron、无多余按钮。

## 2. 改动

| 项 | 改前 | 改后 |
| --- | --- | --- |
| 形态 | 全宽贴底 sheet | **248dp 浮动卡**（`PlayerTokens.panelWidth`） |
| 圆角 | 只圆上两角 16dp | `PlayerPanelStyle.radius`（13.5dp，四角） |
| 位置 | 左上角（无对齐） | **投屏图标下方**：left `368.7dp`、top `82dp`（`topBarHeight 74 + 8`） |
| 设备行 | 左「投」字 + 文字右对齐 + `insetSurface` 卡片内嵌块 | `cast` 20dp 图标 + 左对齐 名/主机名 + 右侧 `chevron-right`，直接铺玻璃上 |
| 重扫 | 标题右侧绿胶囊 | **footer 行**（`reset` 16dp + 14sp，扫描中置灰禁用） |
| 关闭 | 底部「收起」独占一行 | footer 右侧 12sp「关闭」；**再点投屏图标也关**（`cast` 改 toggle） |
| 状态 | 无 | 标题右侧：`发现 N 台` / `未发现`；扫描中留空 |

## 3. 读数

| 项 | 实测 | 期望 |
| --- | --- | --- |
| 卡片左缘 | **368.7 dp**（1014 px） | 368.7（cast 中心 492.7 − 124） |
| 卡片宽 | **247.6 dp**（681 px） | 248 |
| 卡片顶边 | **82.18 dp**（226 px） | 82 |
| 卡片高（空态） | **210.5 dp** | 自适应 |
| 文案 | 标题「投屏」/「正在搜索…」+「确认电视与手机在同一网络」/「重新扫描」「关闭」 | 无重复状态 |
| toggle | 卡片开着时再点图标 → y=320 行**无亮区**（卡已关） | 关 |

（`C0-04-final.png`：最终空态；`C0-05-toggle.png`：再点后无卡。）

## 4. 两个坑（已修，留档）

1. **`LocalConfiguration.current.screenWidthDp` 比顶栏坐标系少 30dp**：用它算 x 时卡片整体左偏 30dp（实测左缘 339dp）。
   改用 `BoxWithConstraints` 的 `maxWidth` 后与顶栏严格对齐。
2. **状态文案重复**：首版标题右侧与 footer 同时写「扫描中…」（`C0-02` 截图指出）。已改为标题留空、footer 固定「重新扫描」。

## 5. 未取证边界

1. **没有真实 DLNA 设备**，只取证了"扫描中 / 无设备"形态；设备行（cast 图标 + 名称 + 主机名 + chevron）与多设备滚动未截图。
2. 分隔线（黑 10%）在暗画面上的对比只有 ≈11 级，肉眼可辨但偏弱；未按亮度另调。
3. 点击卡片外是否关闭未单独验证（当前依赖 footer「关闭」与图标 toggle）。

## 6. 收尾

设备临时截图已清；未 commit / push。
