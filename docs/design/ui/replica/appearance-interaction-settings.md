# 267 · 外观与交互 设置子页（REX 1:1 设计门）

> 输入：用户 2026-11-13 发 10 张参考截图（外观与交互根页 3 张滚动态 + 外观模式/首页封面/详情封面/触感反馈 4 弹层 + 导航钻入 + Badge 钻入 + 1 滚动重复态）。
> 节奏延续 261→266：**审计 → 家族树 → 实现 → K40 真机取证 → 收口**。未完成本设计门不得落代码。
> 参考产品名只出现在本文档（docs 冻结区）；长期代码与用户可见文案一律不出现（D6 口径）。

## 〇、后端能力审计（逐行，file:line 为证）

| 参考行 | 后端现状 | 判定 |
| --- | --- | --- |
| 外观模式（跟随系统/浅色/深色） | `ui/design/DesignTokens.kt:23` 注释「深色态 v2」；全仓 grep `isSystemInDarkTheme/DarkTheme/ThemeMode` 0 命中 ⇒ 固定浅色，无主题切换 | 灰·只读值「浅色」+原因 |
| 减弱动画效果 | 无全局动效门；各页动效参数独立硬编码（如 `SettingsNav.kt:38` 350ms） | 灰开关+原因 |
| 减弱视觉效果 | 无全局视觉门；玻璃/模糊各宿主独立（`CapsuleNavBar.kt:35`、`DetailCalendarSheet.kt`） | 灰开关+原因 |
| 豆瓣评分入口 | `ui/design/RatingBadges.kt:19`「IMDb/豆瓣无公开数据源——缺源的徽章不渲染」；:61 仅色板 | 灰开关+原因 |
| 导航 › | 钻入页见下；主 Tab 定义 `MainActivity.kt:239` 固定四 Tab | 真钻入 |
| 首页封面样式（海报/横向背景图） | 行卡固定海报比：`DesignComponents.kt:138` width=posterWidth；`DiscoverPage.kt:246/306` 横图仅用于既有横卡位，无「行卡改横向」渲染模式 | 灰·只读值「海报」+原因 |
| 详情封面样式（海报/横向背景图） | `DetailBackdrop.kt:31` 固定满幅幕布 heroArtworkUrl（240 §5.4 已验收基线），无海报模式布局 | 灰·只读值「幕布」+原因 |
| 显示剧照 | `DetailPage.kt:249` `if (d.stills.isNotEmpty())` 渲染 StillsRow；数据 `TmdbDetail.kt:109-114` | **真开关**（门控渲染） |
| 优先使用 TMDB 高清无字海报 | 择优按位硬编码：`TmdbDetail.kt:117` poster neutralFirst=false / :118 backdrop=true；`tmdb/artwork/ArtworkSelection.kt:10` 首页 hero 仅无字；行小海报跟元数据语言。全局开关需改三处择优策略=新建功能 | 灰开关+原因（披露按位现状） |
| 固定首页布局 | home 家族 grep 编辑/editMode/reorder 0 命中 ⇒ 首页布局不可编辑，无锁定对象 | 灰开关+原因 |
| 自动将待看添加到日历 | 无 `CalendarContract/ACTION_INSERT`（grep 0）；应用内追剧日历存在（`DetailCalendarSheet.kt`、`DetailOverlayHost.kt:27`「添加到日历＝真待看标记+开日历」） | 灰开关+原因 |
| 预告片·自动播放 | `home/HomePreferences.kt:9` `preview_autoplay` 真键；根页首页卡已接（`SettingsPage.kt:30`） | **真开关**（同键共享，跨页同步） |
| 预告片·播放时开启声音 | `home/trailer/HeroTrailer.kt:57` muted 初值 true、:72 重置 true、:109 `setMuted(muted)`；`YoutubeTrailerView.kt:90` setMuted 真消费 | **真开关**（新键 preview_sound，初值/重置读键） |
| 预告片·显示全部预告片 | 首页 hero 单条预告（HeroTrailer），无预告片列表 UI | 灰开关+原因 |
| 家长指南·显示内容提示 | `DetailPage.kt:218-224` certifications/contentNotes 非空渲染 ParentalGuide；数据 `TmdbDetail.kt:143/211` | **真开关**（门控渲染） |
| 操作反馈·触感反馈（始终/自动/禁用） | 共享入口 `PlayerGestureRouter.kt:131-133` `PlayerComposeActivity.haptic()`，3 调用点（:77、:90、`PlayerActionRouter.kt:178`）；电量真读法 precedent `PlayerStatusRow.kt:101-105` sticky 广播 | **真步进**（门控 haptic；自动=系统 EXTRA_BATTERY_LOW） |
| 图标 › 图标库 | 无图标库后端（grep 图标库/icon pack 0）；NavGlyph 固定线框 | 灰行+原因（不假钻入） |
| 图标 › 资源卡片 Badge | 无 Badge 包/规则引擎（grep Xperience/badges.json 0）；库卡无质量标记条（TitleMatch.kt:17 分辨率词仅用于匹配）；「未配置」为真（无包） | 真钻入·钻入页全灰+原因 |
| 导航钻入·自定义菜单/增强图标视觉/首页图标颜色自适应 | 无对应后端（Tab 固定、NavGlyph 单套、fg 固定 Black `CapsuleNavBar.kt:33`） | 灰开关×3+原因 |
| 导航钻入·隐藏标题 | `CapsuleNavBar.kt:54` Text(label) 单点 | **真开关**（门控标签） |
| 导航钻入·主 Tab 拖拽排序 | Tab 顺序固定（MainActivity.kt:239-242）；无排序持久化 | 只读四行+脚注（**不放假拖拽柄**，266 教训） |
| Badge 钻入·添加包表单/内置预览/黑白图标自适应 | 无下载/校验/规则引擎/逐张检测后端 | 全灰+原因（**不放假输入框/假预览/假计数**） |

**真接线合计 6**：自动播放（共享键）、播放时开启声音、显示剧照、显示内容提示、触感反馈、隐藏标题（钻入）。
**根页 18 行 = 5 真 + 11 灰 + 2 真钻入**；导航钻入 4 开关 = 1 真 3 灰 + 主 Tab 只读 4 行；Badge 钻入全灰。

## 一、页面结构与行表（结构照搬、形态按 Android 重画）

根页 `外观与交互`（SubPageScaffold 固定头+圆返回，继承 262）：

| 卡 | 行 | 形态 | 接线 |
| --- | --- | --- | --- |
| （无头卡） | 外观模式 | ValueRow dim 值「浅色」 | 灰+原因：深色色板 v2 未实现，当前固定浅色 |
| | 减弱动画效果 | SwitchRow enabled=false | 灰+原因：未接全局动效门，各页动效参数独立 |
| | 减弱视觉效果 | SwitchRow enabled=false | 灰+原因：未接全局视觉门，玻璃/模糊按宿主独立 |
| | 豆瓣评分入口 | SwitchRow enabled=false | 灰+原因：豆瓣数据源未接入，评分徽章仅 TMDB |
| | 导航 | ChevronRow › | 真钻入 NAV |
| 封面与剧照 | 首页封面样式 | ValueRow dim 值「海报」 | 灰+原因：横向封面布局未接入，行卡固定海报比 |
| | 详情封面样式 | ValueRow dim 值「幕布」 | 灰+原因：海报模式未接入，详情固定满幅幕布 |
| | 显示剧照 | SwitchRow 默认 ON | 真：AppearancePrefs.showStills → DetailPage 门控 |
| | 优先使用 TMDB 高清无字海报 | SwitchRow enabled=false | 灰+原因：择优按位硬编码（hero 无字优先/详情中文优先/行卡跟元数据语言），全局开关未接 |
| 首页 | 固定首页布局 | SwitchRow enabled=false + Footnote | 灰+原因：首页布局编辑未开放，无锁定对象 |
| （无头卡） | 自动将待看添加到日历 | SwitchRow enabled=false | 灰+原因：系统日历写入未接入；应用内追剧日历在详情页 |
| 预告片 | 自动播放 | SwitchRow 默认 OFF | 真：HomePreferences.preview_autoplay（与根页首页卡同键同状态） |
| | 播放时开启声音 | SwitchRow 默认 OFF | 真：HomePreferences.preview_sound → HeroTrailer muted 初值/重置 |
| | 显示全部预告片 | SwitchRow enabled=false | 灰+原因：预告片列表 UI 未接入，首页 hero 单条 |
| 家长指南 | 显示内容提示 | SwitchRow 默认 ON + 副标题 | 真：AppearancePrefs.showContentNotes → DetailPage 门控 |
| 操作反馈 | 触感反馈 | StepperRow 值「自动」+ Footnote | 真：AppearancePrefs.hapticMode → haptic() 门控；弹层三选 |
| 图标 | 图标库 | ChevronRow enabled=false + 副标题原因 | 灰：图标库未接入，保持默认线框图标 |
| | 资源卡片 Badge | ChevronRow 值「未配置」› | 真钻入 BADGE（值真实：无包） |

钻入 NAV `导航`：卡{自定义菜单 灰 / 增强图标视觉 灰 / 隐藏标题 **真** / 首页图标颜色自适应 灰}+「主 Tab」只读四行（发现/我的/资源库/设置，真实固定顺序，无拖拽柄）+脚注「顺序固定，拖拽排序未接入。」
钻入 BADGE `资源卡片 Badge`：灰卡 InfoRow「添加 Badge 包」+原因（下载与规则校验未接入，不放假输入框）+Footnote；灰 SwitchRow「黑白图标自适应」+原因（逐张检测未接入）。**不放假预览条/假「9 个」计数**。
弹层（复用 SettingsPicker）：触感反馈 = 始终开启 / 自动✓ / 禁用。外观模式/封面样式不放假弹层（灰只读行）。

## 二、家族树（先于实现）

新增 5 文件（各 ≤300 行，单职责）：

| 文件 | 行数预算 | 唯一职责 |
| --- | --- | --- |
| `ui/design/AppearancePrefs.kt` | ≈90 | 外观与交互策略读写（4 键 observable 单例 + bind）+ 触感可用性判定（auto→系统低电量） |
| `ui/design/settings/SettingsAppearanceSub.kt` | ≈150 | 子页路由（NAV/BADGE 内枚举+rememberSaveable+内 BackHandler，266 模式）+ 外观/封面与剧照/首页/日历 4 卡 |
| `ui/design/settings/SettingsAppearanceFeedback.kt` | ≈120 | 预告片/家长指南/操作反馈/图标 4 卡（含触感弹层、autoplay 共享态入参） |
| `ui/design/settings/SettingsAppearanceNav.kt` | ≈75 | 导航钻入页：1 真 3 灰开关 + 主 Tab 只读四行 + 脚注 |
| `ui/design/settings/SettingsAppearanceBadge.kt` | ≈65 | Badge 钻入页：灰添加卡+原因 + 黑白自适应灰开关 |

修改 7 文件（净增 ≤10 行/文件；**MainActivity 净 0**，不加重 #37 债）：

| 文件 | 改动 |
| --- | --- |
| `settings/SettingsNav.kt` | enum +APPEARANCE（1 行） |
| `settings/SettingsPage.kt` | 根页入口 ChevronRow + sub 分支（≈4 行；preview 态经闭包传入子页） |
| `home/HomePreferences.kt` | +preview_sound 键（≈4 行） |
| `home/trailer/HeroTrailer.kt` | muted 初值/重置读 preview_sound（≈3 行） |
| `design/CapsuleNavBar.kt` | bind + 标签门控（≈5 行） |
| `detail/DetailPage.kt` | bind + 剧照/家长指南两门（≈5 行） |
| `compose/PlayerGestureRouter.kt` | haptic() 内加 AppearancePrefs.hapticAllowed 门（≈2 行；3 调用点一次覆盖） |

行原语零新增：SwitchRow(subtitle/enabled)/StepperRow/ValueRow(dim)/ChevronRow(enabled/value)/InfoRow/Footnote/SectionCard/SettingsPicker 全部复用（262–266 既有）。
策略键：sp `appearance_interaction` = show_stills(true)/show_content_notes(true)/hide_nav_labels(false)/haptic_mode("auto")；sp `home_display` +preview_sound(false)。默认值=当前已验收基线（剧照/家长指南现显、nav 现有标签、预告现静音、触感现有震动）。

## 三、披露（D1–D9）

- D1 外观模式固定浅色（深色 v2 未实现），与参考三态不同；不假装跟随系统。
- D2 触感「自动」低电量判据=系统 sticky 广播 `EXTRA_BATTERY_LOW`（不自设阈值）；震感不可截图，复算走 vibrator dump 或披露。
- D3 自动播放/声音默认 OFF=参考截图态=现基线；声音生效时机=下次预告挂载（挂载中改键不即时切换，披露）。
- D4 显示剧照/显示内容提示默认 ON=参考态=现基线；隐藏标题默认 OFF=参考态=现基线；触感默认 auto=参考态。
- D5 家长指南副标题改写：参考「分级投票摘要」→「分级与内容描述符摘要」（TMDB 无票数，DetailSections.kt:203 实证），不夸大。
- D6 封面样式/无字优先灰：按位现状披露于原因（hero 无字优先/详情中文优先/行卡跟元数据语言）；接全局开关=改三处择优策略，属新建功能，本卡不做。
- D7 Badge 钻入不 reproduces 参考内置 Xperience 预览/「9 个」计数/表单输入（无规则引擎）；「未配置」为真值。
- D8 主 Tab 只读、无假拖拽柄（266 MARKS 死代码教训：灰内容钻入仍可导航，但无内容不假钻入——图标库无钻入内容故灰行）。
- D9 钻入形态=push 子页（参考导航为 modal X）；push/pop 350ms 候选值继承 261 披露。

## 四、脚注/副标题原文（参考逐字，除 D5 改写）

- 固定首页布局脚注（参考）：「开启后，首页布局不可编辑，可避免误改。关闭后即可继续调整。」→ 本卡灰态改用原因脚注（见 §一），不保留描述未实现能力的原文。
- 触感脚注（逐字保留，能力真实）：「自动模式将在低电量时自动关闭。」
- 家长指南副标题（D5 改写）：「查看性与裸露、暴力、粗俗语言、烟酒与药物、惊悚场面的分级与内容描述符摘要，作为观影参考。」
- Badge 参考脚注「支持 Xperience。只有下载和规则校验全部成功后才会保存。」→ 不保留（能力不存在），改原因脚注。

## 五、闸门（收口判据）

1. **行数**：新增 5 文件各 ≤300；修改 7 文件净增 ≤10/文件；MainActivity 净 0；不新增超闸文件。
2. **灰行非空原因**：根页 11 灰 + 导航 3 灰 + Badge 钻入灰，全部 subtitle/Footnote 非空原因且用户可见。
3. **真开关写回+持久化**：6 真键 XML 读回；整退设置再进读回（266 模式）；autoplay 子页↔根页同键同步帧。
4. **消费复算**：剧照节显/隐两态帧；家长指南显/隐两态帧；nav 标签显/隐两态帧；触感 disabled→vibrator dump 无新记录（拿不到则披露）；声音键写回+挂载门代码路径。
5. **不回归**：首页/详情/播放子页默认态帧对照改动前（默认值=基线）；设置根页既有条目 intact。
6. **设备恢复**：appearance_interaction 恢复默认（删键或写回默认）+ home_display preview_sound=false + 竖屏锁恢复 auto-rotate。

证据目录：`docs/evidence/p267/appearance-interaction/`（c00–c15 + 2 XML + README 复算链）。