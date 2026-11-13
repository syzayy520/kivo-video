# 270 · 详情页玻璃弹窗扁平化（detail-popups-flat）

> 门闸：家族树先行、单文件单职责、≤300 行；真机证据收口；不静默降级。
> 上游：268-v3 视觉语言（冷灰底 + 扁平纯白卡）、269 导航胶囊 v3（捕获桥已无首页消费者）。

## 一、现状与问题

详情页三个浮层仍是 v2 白毛玻璃配方，且是 `NavigationBackdrop` 捕获桥
（GraphicsLayer record/replay + RenderEffect 模糊）**仅存的消费者**：

| 浮层 | 文件 | 玻璃配方 |
| --- | --- | --- |
| 「更多」菜单 | `ui/detail/actions/DetailMorePopup.kt` | 圆角 18、shadow 24、gloss 白 .28 渐变 + 白 .35 描边、blur 48 + 白 .68（API31 以下 .94） |
| 选季菜单 | `ui/detail/DetailSeasonMenu.kt` | 圆角 20、shadow 24、同款 gloss/描边、blur 48 + 白 .68/.94 |
| 追剧日历 | `ui/detail/DetailCalendarSheet.kt` | 顶圆角 30、scrim 黑 .32、blur 60 + 白 .80/.97 |

问题：① 与 v3「扁平纯白卡、无描边无投影（浮起 chrome 除外）」语言不一致；
② 捕获桥为三张浮层单独维护一套 record/replay 管线，269 之后首页已不用，纯负债；
③ API31 降级分支（blurOk）是模糊能力的兜底，扁平化后**能力需求消失**，分支应整体删除而非保留。

## 二、视觉决策

- **表面**：三浮层一律不透明纯白 `DesignTokens.canvasFill`（#FFFFFF）。
  现状即白玻璃，扁平化＝把「透出的模糊背景」换成实白，**不做深色_sheet**——
  详情页有浅色（无海报）与深色（海报色场）两种环境，白卡在两者上都成立（before b1/b3 已证）。
- **去 gloss、去描边**：删除 `drawWithContent` 顶部白渐变与 1dp 白描边（v3 拒框）。
- **浮起投影保留但收敛**：更多/选季是浮起菜单，沿用 v3 浮起例外（导航胶囊同口径），
  shadow 24dp → **12dp**；日历是底_sheet，靠 scrim 分层，**不加投影**。
- **圆角**：更多 18 → **20**，与选季统一；日历顶圆角 30 不变。
- **scrim**：日历保留黑 .32 × fade；更多/选季保留透明点击兜底（不压暗），不变。
- **文字**：更多/选季行字本就是 `DesignTokens.textMain`（#1A1A1A），实白上对比 ≈16:1；
  日历 CalInk/CalMuted 不变。行按压 wash（黑 .06）保留。
- **动效/几何全保留**：scale+fade 进出场、锚点右对齐/上翻数学、行高（更多 50 / 选季 42）、
  选季勾左、触发器粗体「第 N 季」+ 自绘 ⌄ 旋转、日历月历/分享/移除逻辑。

## 三、家族树（改后）

```
ui/detail/
  DetailPage.kt              编排：删捕获桥接线（glassSources/navigationSource×2/imports）  ~286 行
  DetailOverlayHost.kt       浮层编排：删 backdrop 参数与透传                                ~66 行
  DetailSeasonMenu.kt        选季状态机+触发器+扁平浮层（SeasonState/Trigger 不动）           ~235 行
  DetailSeasonPicker.kt      剧集节（不动）
  DetailCalendarSheet.kt     日历扁平底_sheet（scrim/月历/底栏逻辑不动）                      ~175 行
  actions/
    DetailMorePopup.kt       更多扁平菜单（锚点/行不动）                                     ~110 行
ui/design/home/
  NavigationBackdrop.kt      **删除**（捕获桥退役，全仓零消费者）
```

不新增文件、不新增抽象；每文件职责不变，只换表面配方。全部 ≤300 行。

## 四、退役清单（逐条）

1. `DetailMorePopup.kt`：删 `backdrop` 参数、`NavigationBackdrop/State` import、`Build` import、
   `blurOk` 分支、`drawWithContent` gloss/描边；shadow 24→12；圆角 18→20；白 .68/.94 → `canvasFill`。
   连带删 import：`drawWithContent/CornerRadius/Brush/Stroke`（仅 gloss 用）。
2. `DetailSeasonMenu.kt`：`SeasonMenuOverlay` 删 `backdrop` 参数与同款玻璃栈（shadow 24→12）；
   连带删 import：`Build/NavigationBackdrop/NavigationBackdropState/drawWithContent/CornerRadius/Brush/Stroke`
   （`StrokeCap` 保留——触发器与勾仍用）；更新两处 doc 注释（106–114、151–154 行段）。
3. `DetailCalendarSheet.kt`：删 `backdrop` 参数、`blurOk` 分支、`Build/NavigationBackdrop/State` import；
   白 .80/.97 → `canvasFill`；scrim/顶圆角/逻辑不动；更新头注释。
4. `DetailOverlayHost.kt`：删 `backdrop` 参数、import、两处透传；更新头注释「毛玻璃」措辞。
5. `DetailPage.kt`：删 import L45/46、`glassSources` L130、`Box(Modifier.navigationSource(...))` L143 → `Box {`、
   LazyColumn `.navigationSource(glassSources)` L150、两处实参 L282/285。
6. **删除** `ui/design/home/NavigationBackdrop.kt`（63 行）。删后全仓 grep `NavigationBackdrop` 必须 0 命中。

## 五、范围外（明确不动）

- `GlassSurface` 其余消费者：DetailActivity chrome、AggregateSearchScreen、DiscoverPage、
  DesignComponents 内部、Pages.kt —— 另卡处理。
- 播放器 `playerGlassSurface`（独立层模糊，非捕获桥）。
- `DesignTokens.glassAlpha/glassFallback` 仍被 GlassSurface 使用，保留。

## 六、验收（真机 K40 b6e950ee）

证据目录 `docs/evidence/p270/detail-popups-flat/`：

| 件 | 内容 | 状态 |
| --- | --- | --- |
| before/b1-more.png | 电影详情「更多」白玻璃（浅色环境） | ✅ 已拍 |
| before/b2-calendar.png | 日历白玻璃底_sheet | ✅ 已拍 |
| before/b3-season.png | 宝可梦选季菜单（深色海报环境，8+ 季滚动） | ✅ 已拍 |
| after/a1-more.png / a2-calendar.png | 同机位浅色复拍 | 待 |
| after/a3-season.png / a4-more.png / a5-calendar.png | 宝可梦深色环境三浮层 | 待 |
| 像素复算 | 浮层内部取样 = (255,255,255)；顶/中无 gloss 差；日历 scrim 仍压暗 | 待 |
| 行为回归 | 添加到日历＝真待看+开日历；下载＝toast 如实；追剧进度＝开日历；选季切季真换集列表；分享＝ACTION_SEND；移除＝待看开关；scrim/BACK dismiss | 待 |
| build-p270.log | BUILD SUCCESSFUL（`git add -f`） | 待 |
| 退役自证 | grep NavigationBackdrop = 0；行数表 | 待 |

收口：TASKS.md / DISPATCH.md 回写 → 提交 → 推送（推前复核 remote URL）。