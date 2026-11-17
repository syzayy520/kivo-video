# G2-C · P2 质量修复家族树（冻结版）

> 2026-11-18 04:45 GMT+8
> 范围：D6 命中区 / D13 TMDB 请求合并 / D14 TalkBack 语义。D12 双详情页合并**不在本批**。

## 裁决记录

1. **D6 的"视觉零变化"口径**：按 403 规划 D6 原文执行——**视觉件（圆/图标/pill）尺寸不变，用透明盒把命中区扩到 48dp**。透明盒会让布局占位 +3~8dp，这是规划已授权的代价；凡"扩盒会改变控件本身绘制尺寸"的项（`DetailActionRow` 45dp、`SettingsRowItem` 46dp、`ManageModulesPill` 40dp、日历格 44dp）**本批不做**，列为待授权项。
2. **播放器保留 44dp 例外**：`PlayerTokens.actionHitBox=44f` 同时参与面板锚点数学（`PlayerPanelAnchor.kt:65`），直接改 48 会让浮层横移 2dp；按 `ui-governance-audit.md:133` 结论保留 44 并注释理由。
3. **进度条不做 24→48**：轨道下方 16dp 就是按钮行，双向扩会重叠 8dp 抢事件（R1）；本批只补 TalkBack 的 `progressBarRangeInfo`/`setProgress`，命中保持 24，标注为"需先定 tap 语义"。
4. **D13 先做单飞 + 埋点，不做节流**：报告实证"约 26 次"是部分命中磁盘的抽样，冷启真实上界 42 次；节流会叠加到仍在主线程的 10 次 `/images` 上，放大卡顿。本批同时把 hero 取图挪出组合期。
5. **D14 优先修哑按钮**：`pointerInput` 不产生 `onClick` 语义动作 ⇒ TalkBack 能念名字但双击无效，共 9 处；这是比"缺描述"更严重的缺陷，本批优先。

## 本批文件家族树

```
android/app/src/main/java/com/kivo/cinema/
├─ ui/components/hit/
│   └─ TouchTarget.kt                 [新] ≤80  Modifier.hitTarget(min,h,v)：透明盒扩命中，视觉件尺寸不变
├─ ui/components/
│   └─ HitBox.kt                      [改] 44→48；KDoc 说明"布局盒，非越界命中"
├─ tmdb/
│   └─ TmdbInFlight.kt                [新] ≤85  同 URL 单飞（FutureTask）+ [tmdb-net] START/JOIN/DONE
├─ tmdb/TmdbHome.kt                   [改] get() 包一层单飞；错误分类原样保留
├─ net/NetDiag.kt                     [改] log() 加 @Synchronized（并发计数可靠）
├─ ui/design/DiscoverPage.kt          [改] hero 取图从组合期 remember → produceState + IO
├─ ui/components/KivoBackPill.kt      [改] 48 透明命中盒 + 原尺寸绘制；调用方传 visualSize
├─ ui/detail/DetailHeroContent.kt     [改] BackCircle 传 visualSize=42
├─ ui/profile/calendar/CalendarPageChrome.kt  [改] 拆控件 + 箭头 44→48
│   └─ CalendarChromeControls.kt      [新] ≤90  SwitchCapsule/PeriodArrow/Chevron + 常量（从 296 行文件迁出）
├─ ui/profile/calendar/CalendarMonthGrid.kt   [改] DayCell 语义（日期全称 + 观看量）
├─ ui/profile/calendar/CalendarYearGrid.kt    [改] MonthCard 语义（年份 + 观看量）
├─ ui/design/settings/SettingsScaffold.kt     [改] 返回 48 透明盒 + 36 绘制
├─ ui/profile/modules/ModuleManagerSheet.kt   [改] ✕ 48 透明盒 + 32 绘制
├─ ui/compose/player/PlayerBottomBar.kt       [改] BarIconButton 补 contentDescription/role/onClick
├─ ui/compose/player/PlayerTopBar.kt          [改] TopIconButton 补 role/onClick/stateDescription
├─ ui/compose/player/PlayerGlassMenu.kt       [改] MenuRow 补语义（7 面板共用）
├─ ui/compose/player/PlayerDetailCard.kt      [改] ✕ + DetailPlainRow 语义
├─ ui/compose/player/PlayerSeekTrack.kt       [改] 进度语义（rangeInfo + setProgress）
├─ ui/design/CapsuleNavBar.kt                 [改] tab selectable + 搜索钮语义
├─ emby/ui/EmbyBrowseTopBar.kt                [改] 返回键 contentDescription/role
├─ ui/detail/DetailCalendarGrid.kt            [改] MonthArrow 语义
└─ docs/evidence/p403-design/G2-C-STATUS.md   [验收集]
```

**禁止**：不改颜色/字号/圆角数值；不改播放器 44dp 命中常量；不改进度条 24dp 命中；不重构 `pointerInput` 为 `clickable`（会引入涟漪）；不给装饰 Canvas 加描述；不碰 D12。

## 验收判据

| 缺陷 | 判据 | 证据 |
|---|---|---|
| D6 | 5 处目标的 **视觉件像素尺寸不变**、命中盒 ≥48；真机 `uiautomator` 量 bounds 高宽 ≥48 | `g2c-hit-*.png` / `g2c-hit-bounds.txt` |
| D13 | 冷启 `[tmdb-net] START` 唯一 URL 数 ≤ 改造前；重复 URL 的第二次记为 JOIN；热启 START=0 | `g2c-net-*.txt` |
| D14 | `uiautomator` 中 `clickable=true` 且 `text`/`content-desc` 双空的节点为 0；播放器底栏可聚焦且双击生效 | `g2c-a11y-*.xml` |

## 实施顺序

D13（数据层，可独立验证）→ D14（纯语义追加）→ D6（透明盒，视觉边界最敏感）→ 一次编译 → 真机逐项取证。
