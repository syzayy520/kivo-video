# G2-C · P2 质量修复验证（D6 命中区 / D13 请求合并 / D14 无障碍）

时间：2026-11-18 05:15 GMT+8
设备：K40 `b6e950ee`（A13，用户指定机型）
家族树冻结：`G2-C-PLAN.md`（含 D6 与"视觉零变化"的裁决口径）

## 改动文件（22 个，2 新建 + 1 拆分新建）

| 文件 | 缺陷 | 变化 |
|---|---|---|
| `ui/components/hit/TouchTarget.kt` | D6 | **新建** 26 行：`Modifier.hitTarget()` 透明扩命中，视觉件尺寸不变 |
| `ui/components/HitBox.kt` | D6 | 下限 44→48；注明"布局盒，非越界命中" |
| `ui/components/KivoBackPill.kt` | D5/D6 | 加 `visualSize` 参数：48 透明命中盒 + 原尺寸圆底；一处修两处调用点 |
| `ui/detail/DetailHeroContent.kt` | D5/D6 | 传 `visualSize=42.dp`，圆与箭头像素不变 |
| `ui/profile/calendar/CalendarPageChrome.kt` | D14/D16 | 296→231：`SwitchCapsule`/`PeriodArrow`/`Chevron` 迁出 |
| `ui/profile/calendar/CalendarChromeControls.kt` | D6/D14 | **新建** 121 行：月/年 `selectable(Role.Tab)`；箭头 44→48 + "上个月/下个月"；`Chevron` 改 internal 供顶栏复用 |
| `ui/profile/calendar/CalendarMonthGrid.kt` | D14 | 日期格 `clearAndSetSemantics`：`11月17日，今天` + 观看量 |
| `ui/profile/calendar/CalendarYearGrid.kt` | D14 | 月卡 `clearAndSetSemantics`：`2026 年 11月` + 观看量；新增 `year` 参数 |
| `ui/profile/calendar/CalendarPage.kt` | D14 | 期间行传"上一年/下一年"或"上个月/下个月" |
| `tmdb/TmdbInFlight.kt` | D13 | **新建** 67 行：同 URL 单飞（FutureTask）+ `[tmdb-net] START/JOIN/DONE` + 线程名 |
| `tmdb/TmdbHome.kt` | D13 | `get()` 网络段包单飞；错误分类与 TTL 原样 |
| `net/NetDiag.kt` | D13 | `log()` 加 `@Synchronized`（并发计数可靠） |
| `ui/design/DiscoverPage.kt` | D13 | hero 取图离开组合期：首帧用列表海报，IO 线程按 id 升级 |
| `ui/design/home/HeroCardMapping.kt` | D13 | **新建** 20 行：`TmdbHome.Item` → `HeroCard` 纯映射（DiscoverPage 触顶后拆出） |
| `ui/compose/player/PlayerBottomBar.kt` | D14 | `BarIconButton`：播放/暂停/下一集 名字 + `onClick` 语义动作 |
| `ui/compose/player/PlayerTopBar.kt` | D14 | `TopIconButton`：补 role / onClick / 已开启-已关闭 |
| `ui/compose/player/PlayerGlassMenu.kt` | D14 | `MenuRow` 压平语义（7 个面板共用） |
| `ui/compose/player/PlayerDetailCard.kt` | D14 | ✕ 与普通行补语义 |
| `ui/compose/player/PlayerSeekTrack.kt` | D14 | 进度 `progressBarRangeInfo` + `setProgress` |
| `ui/compose/player/PlayerBarText.kt` | D14 | 1080P/选集/倍速 补 role/onClick/已开启 |
| `ui/compose/player/PlayerEpisodeStrip.kt` | D14 | 顺序播放/季标签/集卡补语义 |
| `ui/compose/player/PlayerStepperRow.kt` | D14 | −/+ 名字、开关 `Role.Switch`、重置 |
| `ui/design/CapsuleNavBar.kt` | D14 | tab `selectable(Role.Tab)`；隐藏文字时补名字；搜索钮"搜索" |
| `emby/ui/EmbyBrowseTopBar.kt` | D14 | 返回键补"返回" + role（`GlassPillSurface` 加 label 参数） |
| `ui/detail/DetailCalendarGrid.kt` | D6/D14 | 月箭头 40→48 命中 + "上个月/下个月" |

## 编译
- 6 轮：`build-g2c.log`（缺 `semantics.role` 属性 import，12 文件）、`build-g2c2`~`g2c6`（补 import / 日历 clear 语义）、
  `build-g2c7.log`：**BUILD SUCCESSFUL**。
- 全部改动文件 ≤300 行（最大 `PlayerGlassMenu`/`PlayerBottomBar` 296）。

## 验证结果

### D13 请求合并 —— ✅ 真机：冷启 42 次全部在 IO 线程；热启 0 次
证据：`g2c-net-cold2.txt`（冷启）、`g2c-net-warm.txt`（热启）。

| 指标 | 冷启（清 cacheDir/tmdb-json-v1 + 清日志） | 热启（force-stop 重启） |
|---|---|---|
| `[tmdb-net] START` | **42** | **0** |
| `thread=main` | **0** | 0 |
| `[tmdb-net] JOIN` | 0 | 0 |
| `[tmdb-cache] HIT` | 0 | **43** |
| 同一 tag 的重复 START | 0（42 条 tag 各不相同） | — |

- 判据达成：**没有任何一次 TMDB 请求跑在主线程**（改造前 `DiscoverPage` 的 10 次 `/images` 是组合期同步 HTTP）。
- `JOIN=0` 是如实结果：冷启这批请求集合里没有同 URL 并发；单飞机制本身的证据是代码 + 日志三态（START/JOIN/DONE），未构造出并发重复场景。
- **过程中发现并修复一个回归**：hero 首帧改用 `produceState` 后，`initialValue` 只在首次组合求值，`heroCandidates` 从空变非空时不会重算 ⇒ 真机 hero 整块高度 0、"继续观看"顶到屏幕最上（`g2c-home.png` 为失败态，`g2c-home2.png` 为修复后）。改为 `remember` 即时映射 + IO 升级 URL。

### D14 TalkBack 语义 —— ✅ 三个页面可点节点 100% 有名
判据：`uiautomator` 中 `clickable="true"` 且 `text`/`content-desc` 双空的节点数。

| 页面 | 可点节点 | 无名节点 | 证据 |
|---|---|---|---|
| 日历 | 35 | **0** | `g2c-cal2.xml` |
| 播放器控制栏 | 15 | **0** | `g2c-player2.xml` |
| 倍速面板（7 档） | 18 | **0** | `g2c-panel4.xml` |

- 关键修复：`pointerInput` 不产生 `onClick` 语义动作 ⇒ 底栏"暂停/下一集"、倍速面板 7 行之前"能念名字/无声，双击无效"；现在都有 `onClick` 动作。
- **必须 `clearAndSetSemantics`**：`Material3` 的 `minimumInteractiveComponentSize` 会在 44dp 视觉件外套一层无名可点节点，普通 `semantics` 会裂成"外层无名 + 内层按钮"两层（`g2c-cal.xml` / `g2c-panel3.xml` 为失败态）。
- 进度条：`content-desc="播放进度"` 已存在（`g2c-player2.xml`，bounds 1960×66），但 uiautomator 不导出 `progressBarRangeInfo`，且设备无 TalkBack 服务 ⇒ **TalkBack 播报未手工验收**，如实标注。

### D6 命中区 48dp —— ✅ 已改处达标；未改项如实列出
证据：`g2c-cal2.xml`（bounds 量测）。

| 目标 | 改前 | 改后命中 | 视觉件 |
|---|---|---|---|
| 日历返回（KivoBackPill） | 44 | **132px = 48dp** | 44dp 圆不变 |
| 日历上/下月箭头 | 44 | **132px = 48dp** | 16dp 箭头不变 |
| 详情返回（KivoBackPill） | 42 | 48dp（`visualSize=42`） | 42dp 圆不变 |
| 详情日历月箭头 | 40 | 48dp | 11×18 折线不变 |
| `HitBox` 下限 | 44 | 48 | — |

**未做（需视觉授权，不在本批）**：
- 设置子页返回 36dp、模块管理 ✕ 32dp：扩到 48 会把标题行高 +16dp、标题右移 12dp；
- 日历日期格 44dp、`DetailActionRow` 45dp、`SettingsRowItem` 46dp、`ManageModulesPill` 40dp：扩盒直接改控件绘制尺寸；
- 播放器 `actionHitBox=44dp`：显式例外，该常量同时参与面板锚点数学；
- 进度条命中 24dp：下方 16dp 就是按钮行，双向扩会重叠 8dp 抢事件；本批只补语义。

## 清理
- 设备临时 xml/png 已清；主包最终 `assembleDebug` = BUILD SUCCESSFUL。
- 后端基线回归：`ctest --test-dir build/vs-debug -C Debug` = **294/296**，两个失败与 G4 时完全相同
  （`kivo_redline_gate_production_scan` 历史误报 23 条、`kivo_p14z_step_repay_tests` 既有红），本轮无新增回归。
- 与代码同批提交。

## 下一批候选
D12 双详情页/双播放 Activity 合并（需单独授权）、D16 超 300 行文件顺手拆、D13 阶段 B（若有同 URL 并发场景再验 JOIN）。
