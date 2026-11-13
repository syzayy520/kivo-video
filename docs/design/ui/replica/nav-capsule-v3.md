# 269 · 导航胶囊 v3（扁平灰 pill + 蓝选中）

> 268-v3 唯一明示遗留：底部导航仍是白→淡紫渐变玻璃 + 白描边 + 全黑图标。
> 本卡把共享 chrome 收进 v3 视觉语言，**只改视觉层与随之死掉的玻璃接线**，
> 页面切换 / 搜索跳转 / `hideNavLabels` / E11 暗环境语义一律不动。

## 〇、红线

- 同目录同包，不新建项目包；单文件单职责；`.kt` ≤300 行。
- 不静默降级：暗环境（发现页深色 hero）保留暗色变体，不是一刀切浅灰。
- 详情页三玻璃弹窗（`DetailMorePopup` / `DetailSeasonMenu` / `DetailCalendarSheet`）
  用 **DetailPage 自持的** `NavigationBackdropState`，与导航接线无关 → **零改动**，机制文件保留。
- 真机 K40 `b6e950ee` 取证；before/after 五页 + 像素复算 + 切换回归。

## 一、现状审计（代码 + 真机 before b0–b5）

| 件 | 现状 | 处置 |
| --- | --- | --- |
| `CapsuleNavBar.kt`(71) | 玻璃渐变 `White.82→#EAE3EF.76`（暗 .63/.59）+ `.7dp` 白 45% 描边 + shadow 6dp；选中 = 白 14% 内 pill；图标/标签全黑 | 视觉重写为扁平；删 `backdrop` 参数与两处 `NavigationBackdrop` 渲染 |
| `MainActivity.kt` | `backdrop` state → 胶囊渲染 + 内容 Box `navigationSource` + `SourcesScreen.overviewModifier` + `DiscoverPage.backdrop` | 胶囊不消费后整链死代码 → 删（6 处 + import） |
| `DiscoverPage.kt` | `backdrop` 参数 + 7 处 `navigationSource` 注册（只喂胶囊玻璃） | 删参数 + 7 处 + 2 import |
| `SourcesScreen.kt` | `overviewModifier` 参数仅 `.then()` 一处 | 删参数 + `.then()` |
| `NavigationBackdrop.kt` / `NavGlyph.kt` | 捕获桥 / 单色 glyph | **保留不动**（详情页依赖桥；glyph 与颜色无关） |
| `SettingsVisuals.navPill` | 已定义 `#E8E9ED`，零消费 | 迁往 `DesignTokens`（chrome 归全局 token），设置侧删行 |

before 真机：胶囊半透白底泛紫、选中态几乎不可见（白 14% on 白玻璃）——正是要治的两个病。

## 二、视觉规格

**浅态**（资源库/在看/我的/设置 + 发现浅环境）：

| 元素 | 值 | 说明 |
| --- | --- | --- |
| 胶囊/搜索圆 | 扁平 `#E8E9ED`（navPill） | 无渐变无描边；圆角 25 / 正圆、高 50 不变 |
| 投影 | 保留 6dp 黑 12% | v3 例外：仅悬浮导航/搜索带投影 |
| 未选中图标+标签 | `#4B5563`（navIdle） | 对 #E8E9ED ≈6.2:1，10sp 标签可读 |
| 选中内 pill | 纯白（圆角 22 不变） | 呼应 v3 纯白卡语言 |
| 选中图标+标签 | `#0A84FF`（navActive） | 与设置链接同蓝；白 pill 上对比同 v3 链接惯例 |
| 搜索图标 | navIdle | 动作钮不抢选中色 |

**暗态**（`onDark` = 发现页深色环境，E11 语义保留）：

| 元素 | 值 |
| --- | --- |
| 胶囊/搜索圆 | 扁平 `#2C2C2E`（navPillDark），无描边，投影保留 |
| 未选中 | `#EBEBF5` 66%（navIdleDark） |
| 选中内 pill | 白 14%（沿用现暗态行为）+ navActive 蓝 |

几何（50dp 高 / 22dp 距底 / 16dp 侧距 / 12dp 间距）、10sp 标签、`hideNavLabels`、
Tab 顺序与页映射、搜索跳 AggregateActivity：**全部不变**。

## 三、家族树（无新文件，视觉收口不膨胀家族）

```
ui/design/DesignTokens.kt        +5 nav chrome 色 token（navPill 自 SettingsVisuals 迁入，单一来源）
ui/design/CapsuleNavBar.kt       扁平重写 ~70 行；删 backdrop 参数/渲染
ui/design/settings/SettingsVisuals.kt  −1 行（navPill 迁出）
MainActivity.kt                  −6 处死接线 + import
ui/design/DiscoverPage.kt        −参数 −7 注册 −2 import
sources/ui/SourcesScreen.kt      −overviewModifier 参数 −.then()
ui/design/home/NavigationBackdrop.kt  不动（详情页玻璃桥）
ui/design/home/NavGlyph.kt            不动
```

## 四、闸门

1. 行数全 ≤300；`assembleDebug` 读日志 BUILD SUCCESSFUL。
2. K40 after 五页截图；像素复算：胶囊体 = (232,233,237)、选中内 pill = (255,255,255)、选中 glyph 蓝。
3. 回归：四 Tab 切换落对页；搜索圆跳聚合搜索；继续观看→在看页；详情页「更多/季菜单」玻璃模糊仍生效（独立捕获桥）。
4. 发现暗环境变体：尝试自然捕获（hero 轮播深色场）；捕不到则明示「代码审查、未真机捕获」。

## 五、披露

- 投影保留是 v3 既定例外（悬浮 chrome），非遗漏。
- 详情页玻璃弹窗不属于静态页 v3 语言，本卡不动，留后续卡。
- 删玻璃接线后 `NavigationBackdrop` 仅服务详情页；若后续详情页也扁平化，机制文件届时整体退役。