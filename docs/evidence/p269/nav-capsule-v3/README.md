# 269 · 导航胶囊 v3 证据（K40 `b6e950ee` 真机，2026-11-13）

> 规格：[`docs/design/ui/replica/nav-capsule-v3.md`](../../../design/ui/replica/nav-capsule-v3.md)
> 268-v3 唯一遗留收口：共享导航 chrome 从白→淡紫渐变玻璃 + 白描边 + 全黑图标，
> 收进扁平灰 pill `#E8E9ED` + 蓝选中 `#0A84FF`；暗环境保留暗扁平变体（E11）。

## 一、before / after 对照

| before | after | 页 |
| --- | --- | --- |
| b1-discover.png | a1-discover.png | 发现（浅环境） |
| b2-profile.png | a2-profile.png | 我的 |
| b3-sources.png | a3-sources.png | 资源库 |
| b4-settings.png | a4-settings.png | 设置 |
| b5-watching.png | a5-watching.png | 在看 |
| —（before 未捕暗态） | a6-discover-dark.png | 发现（深色 hero 暗环境） |
| — | a7-search.png | 搜索圆跳转回归 |
| — | a8-detail-more.png | 详情页「更多」玻璃弹窗回归 |

before 病征：胶囊半透白底泛紫、选中态白 14% on 白玻璃几乎不可见。
after：扁平灰胶囊、未选中灰 `#4B5563`、选中纯白内 pill + 蓝图标蓝标签；暗态扁平 `#2C2C2E` + 浅灰未选中。

## 二、像素复算（ffmpeg 1×1 crop，原图 1080×2400）

命令：`ffmpeg -v error -i <shot> -vf "crop=1:1:X:Y" -frames:v 1 -f rawvideo -pix_fmt rgb24 - | od -An -tu1`

| 点 (X,Y) | 图 | 实测 RGB | 语义 |
| --- | --- | --- | --- |
| (60,2240) | a1/a2/a3/a4/a5 | 232,233,237 | 胶囊体 = #E8E9ED，五页同色 |
| (60,2240) | a6 | 44,44,46 | 暗态胶囊体 = #2C2C2E |
| (680,2240) | a4 | 255,255,255 | 选中内 pill 纯白 |
| (915,2240) | a4 | 232,233,237 | 搜索圆同胶囊色 |
| (356,2225)、(133,2232) | a4 | 75,85,99 | 未选中 glyph = #4B5563 |
| (740,2228)、(764,2228) | a4 | 10,132,255 | 选中 glyph = #0A84FF |

## 三、回归

| 项 | 结果 |
| --- | --- |
| 四 Tab 切换 | a1–a4 各落对页，内容渲染正常（删捕获层后发现页 hero/继续观看/海报节无回归） |
| 继续观看→在看 | a5 落在看页 |
| 搜索圆 | a7 + `dumpsys` topResumedActivity = `.emby.aggregate.ui.AggregateActivity` |
| 详情页「更多」弹窗 | a8 背景模糊生效（DetailPage 自持捕获桥，零改动） |
| 暗环境变体 | a6 首次左滑 hero 即命中：暗胶囊 + 浅灰未选中 + 蓝选中 + 状态栏白图标（E11 保留） |

## 四、闸门

- 行数：CapsuleNavBar 63 / DesignTokens 63 / SettingsVisuals 48 / SourcesScreen 190，全 ≤300。
  MainActivity 354 / DiscoverPage 398 为 #37/#45 既有行数债，本卡只减不增（−7 / −10）。
- 构建：[`build-p269.log`](build-p269.log) `BUILD SUCCESSFUL in 14s`。
- 安装：`adb install -r` Success（HyperOS 弹窗点继续安装），未 uninstall。

## 五、披露

- 投影 6dp 黑 12% 保留 = v3 既定例外（仅悬浮导航/搜索带投影），非遗漏。
- 详情页三玻璃弹窗不属静态页 v3 语言，本卡零改动；`NavigationBackdrop` 机制保留，
  若后续详情页扁平化再整体退役。
- `hideNavLabels` 代码路径原样保留（267 已有写回证据），本卡未重测。
- 玻璃接线删除范围：MainActivity backdrop state / DiscoverPage 7 注册 / SourcesScreen overviewModifier；
  删前确认消费方仅胶囊，详情页独立 state 不受影响（grep 留痕于设计稿 §一）。