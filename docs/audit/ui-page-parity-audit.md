# Kivo · 页面级 UI 与竞品（Rex 1.0.2）对齐审计

> 任务：T2（页面级）。范围：发现页 / 底部导航 / 资源库 / 我的 / 设置 / 在看 / Shell 编排。
> 只做分析与改法设计，**未改任何生产代码**。
> 审计日期：2026-11-16 · 基线提交：当前工作区（P14 之后）。

---

## 0. 口径、证据与三条订正

### 0.1 证据等级

| 标记 | 含义 |
| --- | --- |
| ✅ | 实拍截图或仓库源码可直接复算 |
| ◐ | 由竞品说明书 / 动效拆解文档的键名与语义推断（文档已自带等级） |
| ❌ | 静态证据不足，需真机复测 |

**竞品证据来源**

| 代号 | 来源 |
| --- | --- |
| `UI§x` | `F:\Files\开发\rex\rex\Rex_UI结构复刻说明书.md` 第 x 节 |
| `FX§x` | `F:\Files\开发\rex\rex\Rex_动效与交互全量拆解.md` 第 x 节 |
| `IMG_xxxx` | `C:\kivo video\.tmp\rex_scaled\2026_11_16_*` / `2026_11_11_*`（iOS rex 实拍缩放副本） |

**本报告用到的实拍索引**

| 截图 | 页面 | 用途 |
| --- | --- | --- |
| `IMG_0330`、`IMG_0331` | 发现页（hero 第 1/第 2 页，含继续观看、正在热映） | hero 结构、分页点、继续观看两列、下方分区、底部导航 |
| `IMG_0340`、`IMG_0341` | 详情页（头图/主操作/播放资源/平台标签） | 评分徽章 4–5 源、资源卡「文件/ISO/iLoveHD」形态 |
| `IMG_0344` | 热门人物 | 3 列圆形头像 + 角色标签 |
| `IMG_0351` | 分类浏览详情（合家欢） | 3 列 2:3 海报网格 + 日期 |
| `IMG_0357` | 我的 → 管理模块 sheet | 模块开关 + 拖拽柄 + 说明文案 |
| `IMG_0362` | 我的页 | 环境切换器、周日历条、统计卡分页、热力图、待看空态 |
| `IMG_0369`、`IMG_0371`、`IMG_0373` | 添加资源库 sheet（连接/线路/信息） | 多段 tab、智能识别大框、HTTPS、测试连通性 |
| `IMG_0384` | 设置根页 | 分组：反向代理/自建服务订阅/应用组/页脚 |
| `IMG_0420` | 设置 → 导航 | **主 Tab 四项定义**（发现/我的/资源库/设置）+ 隐藏标题等开关 |
| `IMG_0429`、`IMG_0430` | 我的 / 资源库 + 环境浮层（在看/待看/收藏/下载/日历） | 「在看」归属、资源库概览 |

### 0.2 三条订正（实拍推翻文档推断，必须先对齐口径）

1. **主 Tab = 发现 / 我的 / 资源库 / 设置，共 4 个，不含「在看」。**
   证据：`IMG_0420` 的「主 Tab」区逐行列出这四项且带拖拽柄；`IMG_0330/0331` 底部胶囊同样只有这四项 + 独立搜索圆钮。
   ⇒ `UI§2.1` 的“Libraries / Discover / Watching / My”顺序是 **◐ 推断**（文档自己也标注需真机确认），**不得**作为验收契约。
   ⇒ 当前 `MainActivityShell.kt:89` 的 tab 顺序**与竞品一致**，不属于差距。

2. **「在看」不是 Tab，但也不是「发现的子页」。**
   证据：`IMG_0429/0430` 的环境浮层「打开」区列出 `在看 / 待看 / 收藏 / 下载 / 日历`；`IMG_0362` 我的页里也有这些模块。
   ⇒ 竞品归属：**「我的」页的模块流 + 继续观看区的 › 入口**。当前把它做成发现页的子页（`WatchingPage` 顶部「‹ 发现」返回条）是**入口归属错误**，但“不占 Tab”这一点是对的。

3. **发现页顶部没有环境切换器。**
   `UI§6.1` 推断“顶部：环境切换器 / 全局 Search / Add Library”，但 `IMG_0330` 顶部只有系统状态栏，全局搜索是底部独立圆钮，环境切换器只出现在「我的 / 资源库」页（`IMG_0362/0429/0430`）。
   ⇒ 发现页缺顶部工具条是**预期形态**，本报告不把它列为差距；环境切换器的缺失归到「我的」页。

---

## 1. 发现页（Discover）

### 1.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| 页面骨架 | 全屏 hero → 分页点 → 继续观看 → 分区流（标题 + 右 › + 横向 rail） | `UI§6.1`、`IMG_0330/0331` |
| Hero 内容 | 全幅 16:9 背景图、竖排/艺术字片名、`类型 类型 · 上映日`、`「片名」+ 简介两行`、**4–5 个评分徽章**（IMDb / THE MOVIE DB / 8.7 / 94% / 豆瓣） | `IMG_0330`（✅ 同一屏内可见 5 枚徽章） |
| 分页指示器 | 居中小圆点；**选中点明显更大**，非选中点小 | `IMG_0330/0331`（✅ 第 2 页时第 2 点变大、第 1 点变小） |
| 继续观看 | 两列横向 16:9 卡（卡宽约屏宽 59%）；卡上左下 `看到 mm:ss` + 卡底细进度条；卡下标题含 **`第1季·第1集`**；次行 `相对时间 · 平台名`（如 `10 小时前 · nyamedia`） | `IMG_0330/0331`、`UI§5 viewing.progress.full.width` |
| 分区 | 正在热映（3 列 2:3 海报 + 名称 + 日期）、正在播出（更窄多列）、本周趋势（16:9 宽卡）、近一月·热门综艺（全宽玻璃卡）、今日动漫 | `IMG_0331`（正在热映 3 列）、`UI§5`、`UI§6.1` |
| 内置模块全集 | 继续观看、近一月热门剧集/综艺、今日趋势、本周趋势、高分剧集/电影、备受欢迎电影、热门人物、正在播出、正在热映、最新电影、今日动漫、分类浏览、电影公司、播出平台 | `UI§5 HomeBuiltIn.strings（37 键）`（✅ 键存在） |
| 热门人物 | 3 列圆形头像 + 姓名 + 「演员」角色标签 | `IMG_0344`（✅ 专页同构） |
| 海报网格 | 2:3 海报网格 + 名称 + 日期 | `IMG_0351`（分类页 3 列） |
| 首页可编辑 | 分区拖动排序 / 上移下移 / 置顶置底 / 显示隐藏 / 插入模块 / 删除分组二次确认 / `%@筛选` | `UI§5 分区操作契约`、`FX§M04-S02` |
| 状态位 | 5 类网络错误文案各自独立 + 重试；Loading More / Widget building / Refresh 三态 | `FX§M04-S04` |
| 轮播 | 可独立开启；自动轮播约 4–6s，用户滑动打断后恢复；吸附分页 | `FX§M04-S01`（◐ 参数） |
| 长按卡片 | 长按节目卡片 → 收藏 / 待看 上下文菜单 | `FX§M06-S01` |

### 1.2 当前实现

`android/app/src/main/java/com/kivo/cinema/ui/design/DiscoverPage.kt`

- 页面骨架：`LazyColumn`，第一个 slot 固定为 hero（`133-158`，注释明确“先占位避免锚点跳”）；其后依次为继续观看（`159-186`）、正在热映/正在播出（`187-195`）、本周趋势（`196-198`）、近一月·热门综艺（`199-201`）、今日动漫（`202-204`）、错误卡（`205-208`）。
- 色场：`HomeAtmosphere.background(heroFieldColor, scrollDp)`（`93-100`）→ hero 环境色以 `wash=.28` 向 `#F2F2F7` 洗淡，滚动 390dp 后完全变成页面底色；`chromeDark = luminance < .40`（`HomeAtmosphere.kt:23`）。
- Hero：`HeroCarousel`（`HeroCarousel.kt`）+ `HeroPage`（`home/HeroPage.kt`）；常量 `HERO_HEIGHT_DP=585 / LOGO_CENTER_DP=402 / META_TOP_DP=472 / BADGE_TOP_DP=522 / DOTS_CENTER_DP=574.5`（`HeroCarousel.kt:118-124`）。
- 分区组件：`PosterSection`（`221-241`，`TmdbPosterCard` 固定 **104×156**）、`WideSection`（`246-281`，161×91）、`VarietySection`（`287-344`，全宽玻璃卡 + 150dp 顶图 + 103×58 横排）。
- 数据来源：`MainActivityHomeFeed.kt:20-28` 只抓 `trendingWeek / nowPlaying / onTheAir / varietyPastMonth / todayAnime` 五路；`tmdbError` 只保留五路里的**第一个**错误键尾（`49-50`）。

### 1.3 差距清单

| 编号 | 差距 | 证据（文件:行 / 截图） | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| DISC-01 | 评分徽章只有 1 个来源（TMDB），竞品一屏 4–5 枚（IMDb / TMDB / 8.7 / 94% / 豆瓣） | `home/HeroPage.kt:145-149` 只构造 `listOf(card.ratingSource to rating)`；`RatingBadges.kt:58-65` 已有 IMDb/豆瓣/Emby 配色分支但无数据；`IMG_0330` | Hero 的信息密度与“专业选片”观感缺失，用户无法一眼比对多个评分体系 | 新增 `home/hero/RatingBadgeRow.kt`：数据源改为 `List<Rating>`（source/score/side），`HeroPage` 传 `card.ratings`；`RatingBadges` 扩为可渲染 ≥3 枚并支持百分比型（94%）与图标型（豆）。数据先接 TMDB + OMDb/豆瓣（无源则**不渲染**，沿用 `RatingBadges.kt:29` 的“缺源不显 0”约定） | P1 |
| DISC-02 | 分页指示器选中点不变大，仅靠透明度区分 | `HeroCarousel.kt:126-131` `dotDiameter` 只在 `active>=count-3` 时对尾部收缩，非尾页时**所有点都是 7.3**；`IMG_0331` 选中点明显大于其余 | 用户看不出当前在第几页，只能靠颜色深浅猜 | 改 `dotDiameter(index,count,active)`：`index==active → 7.3f`，其余 `3.3f`（或 4.5f），去掉“远端收缩”分支；保留 `HeroCarousel.kt:107-108` 的插值以获得滑动过渡 | P0 |
| DISC-03 | 继续观看卡缺「第 X 季 · 第 X 集」，次行无平台名 | `DiscoverPage.kt:172-173` 只传 `meta = "${relDays} · ${sourceOf(item)}"`；`DiscoverPage.kt:217-218` `sourceOf` **恒返回「本地」**；`IMG_0330` 为 `清潭国际高中 第1季·第1集` / `10 小时前 · nyamedia` | 剧集无法区分看到哪一集；“本地”字样对所有条目都相同，等于噪声 | `ContinueCard` 增 `episodeLabel` 参数（`DesignComponents.kt:175-219`）；`WatchStore.Progress` 增 `season/episode` 与 `sourceId`；`sourceOf` 改成 `sourceName(item)`（`SourcesScreen` 的 `SourceRepository` 反查）；无剧集信息时省略该行（不写占位） | P0 |
| DISC-04 | 正在热映 / 正在播出共用同一列宽 104dp，无 3 列 / 4 列差异 | `DiscoverPage.kt:188-195` 两区都走 `PosterSection` → `TmdbPosterCard(width=104)`（`379-393`）；`IMG_0331` 正在热映为 3 列 | “正在播出”的剧集密度远低于竞品，一屏只能看 3 部 | `PosterSection` 增 `columns`/`cardWidth` 参数（`221`）；`PosterSection("正在播出", cardWidth=76f)`，`"正在热映"` 保持 104（=3.5 列露头）；抽 `home/sections/PosterRowSection.kt` | P1 |
| DISC-05 | 发现页缺 6 类竞品内置分区：热门人物（圆形）、播出平台、分类浏览、电影公司、高分剧集/电影、5 列海报网格 | `DiscoverPage.kt:159-208` 只有 6 个 section；`rg "人物|分类浏览|播出平台|电影公司" ∈ backend/android` **零命中**；`UI§5 HomeBuiltIn`、`IMG_0344` | 首页信息量只有竞品一半，用户滑到底就没了；「热门人物/分类」这类发现路径完全不可达 | 新增 `home/sections/{PeopleCircleSection.kt, ProviderRowSection.kt, GenreGridSection.kt, CompanyRowSection.kt, TopRatedSection.kt, PosterGridSection.kt}`；数据层 `TmdbHome` 增 `popularPeople / watchProviders / genres / companies / topRated`（`TmdbHome.kt:178-203` 已有同构 `items()` 管线）；按 `HomeSection` 枚举顺序组装 | P1 |
| DISC-06 | 分区无「更多 / 筛选」动作，也无显示·隐藏·排序·插入模块（首页编辑器整体缺失） | `DiscoverPage.kt:405-425` `HomeSectionHeader` 的 `onMore` 只有「继续观看」传了；`UI§5 分区操作契约`、`FX§M04-S02` | 用户无法把首页调成自己的样子；点分区标题无反馈 | 新增 `home/HomeSectionModel.kt`（`data class HomeSection(kind, title, visible, order)`）+ `HomePreferences` 持久化；`HomeSectionHeader(title, fg, onMore, onFilter)`；编辑模式 UI 走 `HomeTitleSheet`（当前是死代码，见 SHELL-06）改造成 `home/HomeLayoutEditor.kt` | P2 |
| DISC-07 | hero 不自动轮播 | `HeroCarousel.kt:54-98` 只有 `HorizontalPager`，无 `LaunchedEffect(pagerState)` 定时器；`FX§M04-S01`（◐ 4–6s，用户滑动打断） | 首页是静态一张图，竞品的“精选轮播”气质没有 | `HeroCarousel` 增 `autoAdvanceMs=5000`：`LaunchedEffect(pagerState.isScrollInProgress, page)` 延时后 `animateScrollToPage`；`AppearancePrefs` 增 `reduceMotion` 门（见 SET-03）时禁用；`isScrollInProgress` 打断后重新计时 | P1 |
| DISC-08 | hero 整块不可点，无法进详情 | `DiscoverPage.kt:141-157` 未传 `onClick`；`HeroCarousel.kt:51,75-82` `onClick == null` 时不加 clickable | 点最大的一张海报没有任何反应（用户第一动作就落空） | `DiscoverPage` 传 `onClick = { card -> heroCandidates.firstOrNull { it.id.toString()==card.id }?.let(onOpenTmdbItem) }`；在 `HeroPage` 上加「播放 / 加入待看 / 更多」三键（`IMG_0340/0341` 的主操作组）留到 DETAIL 卡 | P0 |
| DISC-09 | 5 类网络错误被压成“第一处错误 + 一张卡”：空数据时错误卡占据 hero 位置，有数据时只在列表最末尾出现 | `MainActivityHomeFeed.kt:49-50` 取第一个 `errorKind`；`DiscoverPage.kt:134-140, 205-208`；`FX§M04-S04` 要求 5 类独立 | 用户只看到一句泛化错误，不知道是超时还是离线；也无法按区重试 | `HomeShellState` 改 `errors: Map<Section, String>`；`DiscoverPage` 每 section 渲染自己的 `SectionErrorCard(kind,onRetry)`；重试回调带 section 参数（`MainActivityHomeFeed.refreshHomeFeed(section?)`） | P1 |
| DISC-10 | 全页无 Reduce Motion / 低电量降级门 | `DesignComponents.kt:47-54` 无限动画；`HeroCarousel.kt:72-73` spring；`FX§G1/G2` | 无障碍用户无法关动效；低电量下首页动画照跑 | 见 SET-03：`AppearancePrefs.reduceMotion`；`AnimatedGradientBg` / `HeroCarousel` / `PosterPalette` 动画统一读该开关 | P2 |
| DISC-11 | hero 落榜标题与 logo 的中心基线不一致（fallback 文本 368dp vs `LOGO_CENTER_DP=402`） | `HeroPage.kt:127,134-136`；`HeroCarousel.kt:119` | logo 取不到时片名整体上移 ~34dp，两套 hero 观感不一致 | 统一为 `LOGO_CENTER_DP - LOGO_MAX_H/2` 计算 fallback 文本 top；抽 `home/hero/HeroTitleBlock.kt` | P2 |
| DISC-12 | 继续观看的「›」用 28×22 的 Canvas 箭头，与分区标题样式并存两套 | `DiscoverPage.kt:412-424` vs `DesignComponents.kt:120-136 SectionHeader`（26sp 文字「›」） | 两处箭头粗细/大小不同，页内不一致 | 只保留一种：抽 `home/SectionMoreGlyph.kt`，`SectionHeader` 与 `HomeSectionHeader` 都调用它 | P2 |

---

## 2. 底部导航（CapsuleNavBar）

### 2.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| Tab 集合与顺序 | 发现 / 我的 / 资源库 / 设置（4 项） | `IMG_0330`、`IMG_0420`（✅ 设置页显式列出「主 Tab」四项） |
| 形态 | 底部横向胶囊（毛玻璃/半透明，浮在内容上）+ 右侧**独立圆形搜索钮** | `IMG_0330/0331/0362/0384/0420/0430` |
| 选中态 | 选中项 = 白色圆角凹槽/胶囊底 + **图标保持自身颜色**（发现是彩色头像），无蓝色 tint | `IMG_0331`（发现选中：白底 + 头像）、`IMG_0420`（设置选中：白底 + 灰齿轮） |
| 图标语义 | 发现 = 人物头像；我的 = 人形；资源库 = 影院幕布/箱体；设置 = 齿轮 | `IMG_0330/0420` |
| 标签 | 默认显示文字标签，可在「导航」里「隐藏标题」 | `IMG_0420`（✅ 开关存在） |
| 可配置 | 自定义菜单 / 增强图标视觉 / 首页图标颜色自适应 / 主 Tab 拖拽排序 | `IMG_0420`、`UI§2.3` |
| 全局动作 | 搜索钮进入「选择搜索源」再搜索 | `UI§2.2 app.search.chooseSources`（◐ 未在实拍中展开） |

### 2.2 当前实现

`ui/design/CapsuleNavBar.kt`（64 行）、`home/NavGlyph.kt`（73 行）

- 布局：左侧 `weight(1f)` 胶囊（高 50dp、圆角 25dp、`navPill #E8E9ED`、`shadow(6dp)`），右侧 50dp 圆钮（`36-63`）。
- 图标：`NavGlyph(label, fg, 25dp)`（`50`）；`NavGlyph` 有 `search / 我的 / 资源库 / 设置 / 在看` 五个分支（`NavGlyph.kt:22-70`），**`"发现"` 落到 `else` → 指南针**（`63-69`）。
- 选中态：`fg = navActive #0A84FF`（`43`），底 `Color.White`（`46`）。
- 暗态：`onDark → navPillDark #2C2C2E / navIdleDark`（`32-34`），由 `MainActivityShell.kt:102` 控制。
- 标签：`AppearancePrefs.hideNavLabels` 控制（`51-52`）。

### 2.3 差距清单

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| NAV-01 | 「发现」图标是指南针（else 兜底），竞品是人物头像 | `NavGlyph.kt:63-69` 无 `"发现"` 分支；`CapsuleNavBar.kt:50`；`IMG_0330` | 第一个 Tab 的图标语义错误（“发现=指南针”与“我的=人形”撞概念），与竞品一眼可辨的差异 | `NavGlyph` 增 `"发现"` 分支画头像剪影；头像需要彩色（竞品是彩色插画）→ 拆 `nav/DiscoverAvatarGlyph.kt` 接受 `tint` 与可选位图；`NavGlyph` 保留单色版作降级 | P0 |
| NAV-02 | 选中态用蓝色 tint，竞品用「白底 + 原色图标」 | `CapsuleNavBar.kt:43,46`；`DesignTokens.kt:33 navActive #0A84FF`；`IMG_0331/0420` | 去掉蓝色后整条胶囊更接近竞品；当前选中态像 Material 导航而不是 iOS 玻璃胶囊 | 新 token `navActiveFg`（= `textMain` 或图标原色）+ 保留白底；`navActive` 仅用于文字强调类控件；`CapsuleNavBar` 选中 `fg = navActiveFg` | P0 |
| NAV-03 | 胶囊是扁平不透明灰 + 投影，竞品是半透明毛玻璃（内容透过可见） | `CapsuleNavBar.kt:32-39`（`background(pill)`）；`DesignComponents.kt:97-102` `GlassSurface` 只用 `alpha=.92` 的白，**无 blur**；`IMG_0330`（胶囊后能看到海报） | 首页深色 hero 下胶囊是“一块灰板”，竞品是可透出内容的玻璃片 | 新增 `nav/GlassPill.kt`：`Modifier.blur`（API31+ 用 `RenderEffect`，低版本回退 `Color.White alpha .86`）+ `hairline` 描边；`CapsuleNavBar` 与 `GlassSurface` 共用同一实现，删掉 `fill.copy(alpha=alpha+0.22f)` 的隐式提亮 | P1 |
| NAV-04 | 暗环境把胶囊转深色，竞品暗底上仍是浅色玻璃胶囊 | `MainActivityShell.kt:102` `onDark = page == 1 && state.ambientDark`；`CapsuleNavBar.kt:32-34`；`IMG_0330`（深色 hero 下胶囊仍为浅灰） | 深色首页底部出现深胶囊，与“玻璃浮层”气质相反，也让图标对比度抖动 | 删 `onDark` 分支（或保留但仅改 `navIdle` 不提亮底色）；`onDark` 参数保留签名以减小改动面 | P1 |
| NAV-05 | 主 Tab 顺序与竞品一致（**非差距**），但映射靠魔法数组 | `MainActivityShell.kt:89,91-93` `listOf(1,3,0,4)` 出现两次、`indexOf` 可能返回 -1 | 加第 5 页时必然错位；`page==2` 时「发现」被点亮 | 用 `enum class Tab(val label:String, val page:ShellPage)`（见 SHELL-01），`selected = ShellPage.entries.indexOf(current)`，禁止裸 int | P1 |
| NAV-06 | 资源库页的胶囊由 `SourceWorkspace` 另一处渲染，底部避让 94dp、而其他页 84dp | `sources/ui/workspace/SourceWorkspace.kt:24-27`；`MainActivityShell.kt:116,175-178` | 资源库 ↔ 其他页切换时整条胶囊上下跳 10dp | 统一为 `DesignTokens.navReserve = 94f`；`SourceWorkspace` 改为接收 `navigation` 并从 `MainActivityShell` 单一处渲染（`page != 0` 判断删除） | P0 |
| NAV-07 | 搜索钮直接跳聚合搜索，跳过「选择搜索源」 | `MainActivityShell.kt:99-101` → `AggregateActivity`；`UI§2.2 app.search.chooseSources` | 用户没机会选择搜索范围；与竞品的两段式入口不同 | `AggregateActivity` 起页前置「选择搜索源」sheet（复用 `SettingsDataRoot` 的来源开关）或先弹 `SearchSourceSheet` | P2 |
| NAV-08 | 标签字号/图标尺寸未跟字体缩放，竞品支持 Dynamic Type | `CapsuleNavBar.kt:50,52`（固定 25dp/10sp） | 系统大字号下标签被截断，仍画 50dp 高胶囊 | 用 `with(LocalDensity){ (25 * fontScale.coerceAtMost(1.3f)).dp }` 并让胶囊高度 `navHeight * min(fontScale,1.25)` | P2 |

---

## 3. 资源库（SourcesScreen）

### 3.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| 页面骨架 | 大标题「资源库」+ 右上「编辑」胶囊；分组标题（`媒体服务器` + 排序图标）；卡片网格；右下浮动「+」添加 | `IMG_0430`（✅） |
| 库类型枚举 | Emby / Jellyfin / WebDAV / SMB / 百度网盘 / 阿里云盘 / Drivemby / 本地文件 / Trakt | `UI§6.2`（✅ 键前缀） |
| 分组 | `library.groups.new.placeholder`、`library.groups.editor.footer`（拖动排序、隐藏分组不显示） | `UI§6.2` |
| 段展开/收起 | `library.section.expand/collapse` | `UI§6.2` |
| 文件匹配流程 | 读目录 → 识别 → `Saving %lld/%lld` → 解绑 | `UI§6.2` |
| Drivemby 进度 | `waiting / recognizing / uploading / done` | `UI§6.2` |
| 卡片信息 | 库图标 + 状态点 + 名称 + `主线路` + `电影:2470 剧集:1921` | `IMG_0430`（✅） |
| 添加流程 | **单个多段 sheet**：`连接 / 信息 / 线路 / iCloud / 高级`（5 段），顶部智能识别大文本框 + 「粘贴并识别」，地址（域名 + 端口）、`开启 HTTPS`、用户名、密码（眼睛）、`测试连通性` | `IMG_0369`（✅）、`IMG_0371`（线路/备用服务器）、`IMG_0373`（SMB 版同构） |
| 空态 | 分组空态 placeholder（`library.groups.new.placeholder`） | `UI§6.2` |

### 3.2 当前实现

`android/app/src/main/java/com/kivo/cinema/sources/ui/SourcesScreen.kt`（190 行）+ `cards/*` + `workspace/*`

- 概况网格：`LazyVerticalGrid(Adaptive(min 160dp))`（`72-75`），头部「资源库 / 连接你的私人影院」+ 锁钮 + `＋ 添加`（`76-90`）。
- 分组：`本地与存储`（本地影片 / 私密区 / **SMB 源** 混排，`91-101`）→ `聚合搜索` 绿色横幅（`102-116`）→ `媒体服务器`（非 SMB 源，`117-127`，60s 轮询 `146-153`）。
- 空态：仅在「媒体服务器」无源时出现一张「把你的电影库带进来」（`136-144`）；`loadError` 有重试（`128-135`）。
- 卡片：`SourceCard`（`SourceCard.kt:27-58`）→ `SourceTile`：图标 + 状态点（在线绿/离线红/未知灰）+ `⋯` 管理；副标题 `电影:N  剧集:M`。
- 添加流程：`＋ 添加` → `SourceTypeSheet`（类型选择，`SourceTypeSheet.kt:27-51`）→ `EmbySourceForm`/`SourceConnectionContent`（名称、地址、账号、密码、`智能识别`按钮 + 预览确认、`来源信息·选填`折叠：协议/端口/备注、`连接并保存`）。
- 协议枚举：`SourceRecord.PROTOCOL_EMBY / PROTOCOL_SMB`（`SourceRecord.kt:49-50`），`PROTOCOLS` 仅 2 个。

### 3.3 差距清单

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| LIB-01 | 库类型只有 Emby + SMB 可用，其余 4 类灰置「暂未接入」；竞品 9 类 | `SourceTypeSheet.kt:37-48`；`SourceRecord.kt:49-50`；`UI§6.2` | 用 Jellyfin/WebDAV/网盘的用户在第一步就被挡住 | 加 `sources/protocol/SourceProtocol.kt`（枚举 + 能力矩阵 + 未接入原因）；优先接 `Jellyfin`（Emby API 复用度最高）与 `WebDAV`（已有 HTTP 栈） | P1 |
| LIB-02 | 无「分组」概念（竞品可新建分组、拖动排序、隐藏分组不显示） | `SourcesScreen.kt:91-127` 分组是硬编码三段；`UI§6.2 library.groups.*` | 多库用户无法组织；「本地与存储 / 媒体服务器」两分法把所有 SMB 归到“存储”而非用户自己的分组 | 新增 `sources/ui/LibraryGroups.kt` + `LibraryGroupEditor.kt`；`SourceRecord` 增 `groupId/sortIndex`；`SourcesScreen` 按 group 渲染 | P2 |
| LIB-03 | 无段展开/收起（`library.section.expand/collapse`） | `SourcesScreen.kt:117-127` 只有静态标题行 | 库数量增长后无法收起某段，只能一直滚动 | `SectionHeaderRow` 增 `expanded` + `AnimatedVisibility(expandVertically(tween(300)))`（`FX§P08`） | P2 |
| LIB-04 | 添加流程是「先选类型 → 再填表单」两段，竞品是单 sheet 五段 tab，且类型由地址/粘贴推断 | `SourcesScreen.kt:87,174-178`；`SourceTypeSheet.kt:27-51`；`IMG_0369/0371/0373` | 多一步决策，且类型选错后要退回重选；「线路/iCloud/高级」无处可放 | 合并为 `sources/ui/AddSourceSheet.kt`：`TabRow(连接/信息/线路/高级)`，`连接` 段直接复用 `SourceConnectionContent`；类型从 `SourceAddress.schemeOf` + 粘贴识别结果推断，纯 `file://` → 本地 | P1 |
| LIB-05 | 无「测试连通性」独立动作（只在保存时验证） | `SourceConnectionContent.kt:113-126` 只有 `连接并保存`，`model.verifying` 是保存态文案；`IMG_0369` | 用户必须先保存才知道密码错，失败后要重新进入编辑 | `SourceManagerModel` 增 `test()`（不落盘，复用 `verifying` 状态机）；按钮 `TextButton("测试连通性")` 置于表单中部 | P1 |
| LIB-06 | 「智能识别」是表单内一个小按钮 + 二次确认弹窗，竞品是顶部大文本框 + 「粘贴并识别」即时填入 | `SourceConnectionContent.kt:72-78`；`SourcePastePreview.kt:18-44`；`IMG_0369` | 多两次点击；识别结果无法在表单里回看原文 | `AddSourceSheet` 顶部 `OutlinedTextField(minLines=3)` + `TextButton("粘贴并识别")`；`SourcePastePreview` 降级为字段级高亮（识别不到的字段标红） | P2 |
| LIB-07 | 卡片无「主线路 / 备用线路」概念（竞品有备用服务器与线路同步） | `SourceCard.kt:32-37`；`IMG_0430`（`主线路`）、`IMG_0371` | 多线路容灾不可见、不可配 | `SourceRecord` 增 `backupAddress`；`SourceCard` 副标题首行渲染 `主线路`/`备用`；线路编辑进 `AddSourceSheet.线路` | P2 |
| LIB-08 | 空态只有媒体服务器一处；本地影片/私密区/SMB 无空态，且空态文案与竞品「分组 placeholder」不同 | `SourcesScreen.kt:136-144`；`UI§6.2` | 新建用户看到 2 张空入口卡 + 一张大卡，不知道该点哪个 | 每段各自 `EmptyHint`（本地：`扫描到 0 个视频`；SMB：`添加共享文件夹`），共用 `sources/ui/SourceEmptyState.kt` | P2 |
| LIB-09 | 无 Drivemby 识别进度（waiting/recognizing/uploading/done） | 全仓 `rg "drivemby"` 零命中；`UI§6.2` | 不在本轮范围（无该协议），但“文件匹配”进度同样缺失 | 待 LIB-01 接入 Drivemby 后再做；先建 `sources/ui/match/MatchProgressRow.kt` 骨架 | P3 |
| LIB-10 | 无「编辑」模式入口（竞品右上角胶囊「编辑」） | `SourcesScreen.kt:83-88` 只有锁钮 + `＋ 添加`；`IMG_0430` | 分组/排序/删除只能靠长按卡片（`SourceTile.kt:22`），可发现性低 | 头部加 `EditChip`，进入后卡片露出拖拽柄与删除（复用 `FX§M04-S02` 的拖拽基座） | P2 |

---

## 4. 我的（ProfilePage）

### 4.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| 顶部 | **环境切换器**：环境头像 + 「默认环境 ⇅」+ 右侧「编辑」胶囊 | `IMG_0362`、`UI§2.4`（✅ 跨页全局状态，影响首页/进度/日历） |
| 周日历条 | 一行 7 天（15 周日 … 21 周六），今天高亮为白底红字，右侧「展开日历」方钮 | `IMG_0362` |
| 当日空态 | 卡内「这天没有播出的剧 / 换一天看看，或者去日历里添加追剧」 | `IMG_0362` |
| 统计卡 | 「今天 / 2026年11月16日 / 0.0 小时 / 0 部电影 / 2 集剧」+ 24h 时间轴 + 柱状；**横向分页 3 张**（下方 3 点指示器） | `IMG_0362`、`IMG_0429`（刷剧 TOP3 / 月度） |
| 历史观看 | 热力图（约 7 列 × 4 行方格，深紫/浅紫表示强度） | `IMG_0362` |
| 模块流 | `在看 / 待看 / 收藏 / 订阅 / 下载 / 本月观看 / 类型分布 / 分类走势 1 / 分类走势 2`，每块可关，关闭后不在本页显示；在看/订阅/下载/走势“没有内容时也不显示” | `IMG_0357`（✅ 管理模块 sheet 文案原文） |
| 待看 | 「待看 0 ›」+ 空态「还没有待看的影片」 | `IMG_0362` |
| 模块编辑 | 「管理模块」sheet：开关 + 拖拽柄 + 底部说明 | `IMG_0357` |
| 打开菜单 | 环境浮层里另有 `在看 / 待看 / 收藏 / 下载 / 日历` 五个快捷入口 | `IMG_0429` |

### 4.2 当前实现

`ui/design/Pages.kt:34-57`

- 只有：标题「我的」(28sp) + 一张玻璃卡（56dp 圆形 👤 + 「kivo 观众」+ 「媒体库 N 个视频 · 统计随功能接入」）。
- 文件注释自认「数据未建，v1 极简档案位，明示占位」（`Pages.kt:33`）。
- 无环境、无日历、无统计、无模块、无入口菜单。

### 4.3 差距清单

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| PROF-01 | 整页是占位：只有头像 + 一句「统计随功能接入」 | `Pages.kt:34-57`；`IMG_0362` | 「我的」Tab 是全 App 信息量最低的页，点进去像未完成品 | 新建 `ui/design/profile/ProfilePage.kt`，`Pages.kt` 只保留时间格式化函数（迁到 `ui/format/TimeText.kt`） | P0 |
| PROF-02 | 无环境切换器（跨页全局状态） | `Pages.kt` 无；全仓无 `Environment` 模型；`UI§2.4`、`IMG_0362/0429` | 首页/进度/日历无法按“观影环境”隔离，竞品的多用户/多场景能力整体缺失 | 新增 `environment/EnvironmentStore.kt`（`data class Environment(id,name,avatar)` + 当前选中）+ `profile/ProfileEnvironmentHeader.kt`；`WatchStore` 记录带 `environmentId` | P1 |
| PROF-03 | 无周日历条与「这天没有播出的剧」空态 | `Pages.kt` 无；`IMG_0362` | 追剧用户看不到“今天播什么” | `profile/ProfileWeekStrip.kt`（7 天 + 今日高亮 + 展开钮）+ `ProfileDayCard.kt`（空态/当日剧集） | P1 |
| PROF-04 | 无观看统计卡与横向分页 | `Pages.kt` 无；`WatchStore` 只有 `Progress`；`IMG_0362/0429` | 观看时长/部数/集数完全不可见（`WatchStore` 已存时长，纯展示缺口） | `profile/ProfileStatsPager.kt`（`HorizontalPager` 3 页：今日 24h 轴、刷剧 TOP3、本月对比）+ 分页点复用 `HeroCarousel` 修好的 `dotDiameter` | P1 |
| PROF-05 | 无历史观看热力图 | `Pages.kt` 无；`IMG_0362` | 长期使用没有任何“回看”入口 | `profile/ProfileHeatmap.kt`：按日聚合 `WatchStore.recentKeys` + 自绘 7×N 方格（`Brush` 分 4 级） | P2 |
| PROF-06 | 无模块流（在看/待看/收藏/订阅/下载/本月/类型分布/分类走势） | `Pages.kt` 无；`IMG_0357`（模块清单原文） | 「我的」没有内容区，且「在看」因此被迫塞进发现页（见 SHELL-02） | `profile/ProfileModuleList.kt`（按 `ProfileModule` 枚举渲染，空内容时整块不显示）+ 复用 `WatchingPage` 的行卡 | P0 |
| PROF-07 | 无「编辑 / 管理模块」入口，模块无法开关或排序 | `Pages.kt` 无；`IMG_0357` | 用户不能裁剪自己的“我的”页 | `profile/ProfileModuleEditor.kt`（ModalBottomSheet：`Switch` + 拖拽柄 + 底部说明）；持久化到 `AppearancePrefs` 同级 `profileModules` | P2 |
| PROF-08 | 无环境浮层里的快捷入口（在看/待看/收藏/下载/日历） | `Pages.kt` 无；`IMG_0429/0430` | 五个高频目的地只能各绕一圈 | 复用 `ProfileEnvironmentHeader` 的长按/点击弹 `EnvironmentMenuSheet(entries)`；「在看」指向 `WatchingPage` | P2 |

---

## 5. 设置（SettingsPage）

### 5.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| 根页 | 大标题「设置」；分组 1（网络）：`反向代理`、`自建服务订阅` | `IMG_0384`（✅） |
| 分组 2 | 标签「应用」：`PRO 与授权`、`反馈与改进`、`重新认识 Rex` | `IMG_0384`（✅） |
| 页脚 | 吉祥物插画 + `版本: 1.2 (2)` + TMDB logo + 「This product uses the TMDB API but is not endorsed or certified by TMDB.」+ `隐私协议` / `使用条款` | `IMG_0384`（✅） |
| 子模块规模 | 699 键 + 8 子模块（Settings/AppSettings/Playback/Subtitle/Modules/Environment/Feedback/Dino） | `UI§6.7`（✅） |
| 关键开关为真 | `减弱动画效果`、`触感反馈（自动/始终/禁用，低电量自动关）`、`隐藏标题`、`固定首页布局`、`豆瓣评分入口`、`优先使用 TMDB 高清无字海报`、`自动将待看添加到日历` | `IMG_0416/0418/0421/0423`、`FX§G1/G2`（✅ 均为可操作开关） |
| 导航子页 | 自定义菜单 / 增强图标视觉 / 隐藏标题 / 首页图标颜色自适应 + **主 Tab 四行可拖拽** | `IMG_0420` |
| Badge 子页 | 添加 Badge 包（URL + 别名 + 验证并添加）+ 内置 Xperience（9 标记）+ 黑白图标自适应 | `IMG_0424` |
| 播放子页 | 预缓冲/缓存容量选择、下一集预加载、预加载范围（库列表开关）、画面关键词、后台播放、字幕 HDR 等 | `IMG_0376/0379/0382` |

### 5.2 当前实现

`ui/design/settings/`（29 个文件，已按单职责拆分）

- 根页 `SettingsPage.kt:45-87`：`SettingsStorageHero`（缓存字节）→ `SettingsEntryGrid`（播放/音画/操控/AI 2×2）→ `SettingsStatusTiles`（Emby/SMB 数）→ `使用偏好`（自动播放 + 外观与交互 + 数据与资源）→ `安全与关于`（数字密码 + 版本信息）→ `VersionFootnote`。
- 路由：`SettingsRoute` 8 条（`SettingsNav.kt:24`），水平滑入 350ms。
- 真开关：`显示剧照`、`隐藏标题`、`触感反馈`（`SettingsAppearanceSub.kt:76-77`、`SettingsAppearanceNav.kt:29-31`、`SettingsAppearanceFeedback.kt:45-46`）。
- 灰置（`enabled=false`）：`外观模式`、`减弱动画效果`、`减弱视觉效果`、`豆瓣评分入口`、`首页封面样式`、`详情封面样式`、`优先使用 TMDB 高清无字海报`、`固定首页布局`、`自动将待看添加到日历`、`自定义菜单`、`增强图标视觉`、`首页图标颜色自适应`（`SettingsAppearanceSub.kt:47-85`、`SettingsAppearanceNav.kt:18-28`）。
- `资源卡片 Badge` 为灰说明页（`SettingsAppearanceBadge.kt:11-20`）。
- 主 Tab：只读四行 + `Footnote("顺序固定，拖拽排序未接入。")`（`SettingsAppearanceNav.kt:32-36`）。
- 不存在：PRO 与授权、反馈与改进、重新认识 Rex、隐私协议、使用条款、反向代理、自建服务订阅、管理模块、吉祥物页脚。

### 5.3 差距清单

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| SET-01 | 根页分组结构与竞品不同，且缺 3 个“应用组”入口与 2 条法律链接 | `SettingsPage.kt:45-73`；`IMG_0384` | 用户找不到授权/反馈/隐私；商业化与合规入口缺失 | 新增 `settings/SettingsProSub.kt`、`SettingsFeedbackSub.kt`、`SettingsLegalSub.kt`；根页重排为「网络 / 播放与外观 / 数据与资源 / 安全 / 应用 / 关于」；页脚补 `SettingsFooter.kt`（版本 + TMDB 声明 + 两条法律链接） | P1 |
| SET-02 | 「反向代理 / 自建服务订阅」无对应入口（竞品在网络组） | `SettingsPage.kt` 无；`IMG_0384`、`IMG_0426` | 反代是自建用户刚需，当前只能填完整地址 | `settings/SettingsNetworkSub.kt`（代理列表 + 添加框），接 `SourceAddress` 生成访问地址 | P2 |
| SET-03 | `减弱动画效果 / 减弱视觉效果` 是灰开关，全 App 无动效门 | `SettingsAppearanceSub.kt:51-55`；`FX§G1`（竞品为真开关 + 响应系统 `isReduceMotionEnabled`） | 无障碍用户无法关闭首页渐变/轮播/缩放；这也是 DISC-10 的根因 | `AppearancePrefs` 增 `reduceMotion`（持久化）+ `MotionPolicy` 对象（读 `Settings.Global.TRANSITION_ANIMATION_SCALE` 逗号）；所有 `rememberInfiniteTransition/spring` 处读它 | P1 |
| SET-04 | `外观模式` 固定浅色，无深色色板 | `SettingsAppearanceSub.kt:49`（灰行）；`DesignTokens.kt:9-35` 只有 light 值 | 深色环境用户（`IMG_0330` 形态）无法获得暗色页面 | `DesignTokens` 改 `object` → `DesignTokens(light, dark)` + `CompositionLocal`；先做 `pageBackground/canvasFill/textMain/textSub/hairline` 5 枚（对齐 `UI§3.1` 语义色板） | P1 |
| SET-05 | `豆瓣评分入口` 灰（数据源缺失），导致 DISC-01 只能单源 | `SettingsAppearanceSub.kt:57-58`；`RatingBadges.kt:58-65` | Hero 评分徽章永远是 1 枚 | 与 DISC-01 合并：先接 `OMDb`（IMDb 分数）与烂番茄/豆瓣其一的公开接口；开关转真后 `RatingBadges` 才有多源 | P1 |
| SET-06 | `主 Tab` 只读、无拖拽，与竞品「可拖动排序」不一致 | `SettingsAppearanceNav.kt:32-36`；`IMG_0420` | 用户不能改 Tab 顺序（Tab 顺序是产品定义的强诉求之一） | 引入 `shell/ShellRoutes.kt` 的 `Tab` 枚举 + `AppearancePrefs.tabOrder`；`SettingsAppearanceNav` 四行改为 `Reorderable`（复用 M04-S02 基座），`CapsuleNavBar` 按该顺序渲染 | P2 |
| SET-07 | `资源卡片 Badge` 是纯灰说明页，竞品有 URL 添加 + 内置 Xperience 预览 + 黑白自适应 | `SettingsAppearanceBadge.kt:11-20`；`IMG_0424` | 资源卡上的 4K/HDR/Atmos 徽章完全不可配，详情页也没有徽章 | 新建 `settings/SettingsBadgeSub.kt`；资源徽章渲染放 `ui/design/badges/ResourceBadgeRow.kt`（数据先内置 9 枚静态规则，规则引擎后置） | P2 |
| SET-08 | 无「管理模块」（我的页模块开关） | `settings/*` 无；`IMG_0357` | 与 PROF-07 同源，设置侧无闭环 | `settings/SettingsModulesSub.kt` 复用 `profile/ProfileModuleEditor` 的同一份状态 | P2 |
| SET-09 | 页脚只有一行 `Kivo Video + 版本号` | `SettingsScaffold.kt:56-65`；`IMG_0384` | 无 TMDB 归属声明（法律要求）、无隐私/条款 | 见 SET-01 的 `SettingsFooter.kt`；文案按竞品结构（吉祥物插画可后置） | P1 |

---

## 6. 在看（WatchingPage）

### 6.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| 顶部 | **环境切换器**（同我的页） | `UI§6.3`（✅ 键）、`UI§2.4` |
| 主体 | 继续观看卡片流，**可移除** | `UI§6.3` |
| 剧集进度 | 进度条 + 集数 + 折叠记录：`折叠 %lld 条` / `点击展开` | `UI§6.3 watching.format.collapse.records` |
| 未识别记录 | 可展开/收起（`expand|collapse.unidentified.records`） | `UI§6.3`、`FX§M06-S04` |
| 追剧日历 | **滚到底部后长按**才看完整日历（`you've reached the bottom, long press to view the full calendar`） | `UI§6.3`、`FX§M06-S03`（✅ 前置条件明确） |
| 弃剧 | 隐藏整部剧 + 移出待看；可撤销（`mac.tracking.dropUndone`） | `UI§6.3` |
| 卡片形态 | 16:9 缩略图 + 标题（含季集）+ `相对时间 · 平台` + 进度条 + `看到 mm:ss` | `IMG_0330/0331`（同款卡在发现页） |

### 6.2 当前实现

`ui/design/WatchingPage.kt`（142 行）

- `BackHandler { onBack() }`（`45`）+ 顶部返回条「‹ 发现」（`48-57`）+ 大标题「在看」(28sp) + 副标题「观看进度自动保存 · 上划管理」（`58-64`）。
- 空态：两行文案 + 「去发现」按钮（`66-86`）。
- 列表：`continueItems.forEach`（`87`）逐行卡（`90-139`）：103×58 缩略图（`102`）、标题 17sp（`113`）、`relDays`（`117`）、**3dp 黑进度条**（`120-133`）、`看到 mm:ss / 总时长`（`135-137`）。
- 注释自认「弃剧/折叠记录/追剧日历 = v2」（`33`）。

### 6.3 差距清单

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| WATCH-01 | 入口归属错误：顶部「‹ 发现」把它做成发现子页，竞品归「我的」模块 + 继续观看 › | `WatchingPage.kt:33-37,48-57`；`MainActivityShell.kt:155-162`；`IMG_0429` | 用户从“我的”找不到“在看”；从发现进去又要先返回发现才能去别处 | 删返回条与 `BackHandler`；`WatchingPage` 变 `profile` 模块的目标页（`ProfileModuleList` 的「在看」行）+ `DiscoverPage` 的继续观看 › 仍可进入；路由进 `ShellRoutes`（见 SHELL-01） | P0 |
| WATCH-02 | 无环境切换器 | `WatchingPage.kt:46-64`；`UI§6.3` | 多环境下进度混淆（与 PROF-02 同根） | 复用 `profile/ProfileEnvironmentHeader.kt` 置顶 | P1 |
| WATCH-03 | 卡片不支持移除 | `WatchingPage.kt:87-139` 无 swipe/long-press；`UI§6.3` | 看错/看弃的记录永久占位（只能等 WatchStore 自然淘汰） | `WatchingRow` 内 `SwipeToDismiss`（`-80dp` 露出「移除」）+ `WatchStore.remove(key)`；移除后 `refreshContinue()` | P1 |
| WATCH-04 | 无「折叠 %lld 条 / 展开」 | `WatchingPage.kt` 无；`UI§6.3` | 记录一多就是无穷长列表 | `watching/WatchingRecords.kt`：先渲 N=5 条，尾部 `Text("折叠 ${n-5} 条")`，点击 `AnimatedVisibility` 展开（`FX§P08`） | P2 |
| WATCH-05 | 无未识别记录分组 | `WatchingPage.kt` 无；`UI§6.3` | 匹配失败的条目被静默丢弃（`refreshContinue` 里 `mapNotNull` 直接丢，`MainActivityHomeFeed.kt:60-66`） | `WatchStore` 记录 raw key；`refreshContinue` 不再丢弃，未匹配项进 `unmatched` 列表，由 `WatchingRecords` 分组渲染 + 「重新匹配」 | P2 |
| WATCH-06 | 无底部长按进完整日历 | `WatchingPage.kt` 无；`FX§M06-S03` | 追剧日历入口只有详情页（`DetailCalendarSheet`），在看页无法直达 | `watching/WatchingCalendarEntry.kt`：`LazyColumn` 最后一项；`LazyListState.canScrollForward == false` 时启用 `combinedClickable(onLongClick=…)`；未到底时长按给 toast「滑到底部长按」 | P1 |
| WATCH-07 | 无弃剧与撤销 | `WatchingPage.kt` 无；`UI§6.3` | 用户无法表达“不看了”，推荐继续被污染 | `WatchStore` 增 `dropped` 集合；长按卡 → `DroppedSheet(弃剧/取消)`；弃剧后卡片消失 + 底部 `Snackbar`「已移出，撤销」 | P2 |
| WATCH-08 | 卡片次行只有相对时间，无平台标签；标题无季集 | `WatchingPage.kt:113-118`；`IMG_0330` | 同 DISC-03；「在看」页与首页两套卡信息不一致 | 抽 `ui/design/cards/ProgressWideCard.kt`，`WatchingPage` 与 `ContinueCard` 共用（一次修 DISC-03 + WATCH-08） | P0 |
| WATCH-09 | 无进度百分比数字；进度条 3dp 全黑硬边 | `WatchingPage.kt:120-137`；`IMG_0330`（细条 + 圆角） | 用户只能靠条估长度，看到第几集/剩多久不直观 | 百分比以 `12sp` 附在「看到 mm:ss」后（`fmtSeen` 旁加 `pct`）；条改 `2dp + RoundedCornerShape(1dp)` + `textMain alpha .15` 轨道 | P2 |
| WATCH-10 | 卡片底色固定 `Color.White alpha .92`，深色环境不适用 | `WatchingPage.kt:96` | 深色环境下来白块，页面割裂 | 改 `DesignTokens.canvasFill`/`glassFallback`，暗色时取 dark 值（依赖 SET-04） | P2 |

---

## 7. Shell 编排（MainActivityShell）

### 7.1 竞品契约

| 契约 | 内容 | 来源 |
| --- | --- | --- |
| 路由 | 4 Tab（发现/我的/资源库/设置）；「在看」「待看」「收藏」「下载」「日历」为二级目的地 | `IMG_0420`、`IMG_0429` |
| 导航避让 | 胶囊浮在内容上（不挤压内容），底部避让系统手势区；内容滚动到底不被胶囊遮挡 | `IMG_0330/0384/0430` |
| 状态栏 | 内容为深色时状态栏图标转白（hero 页） | `IMG_0330`（✅ 白图标） |
| 私密内容保护 | 未在实拍中体现（❌）；私密库锁定时应只有私密内容受保护，而非全 App | 无证据 → 以最小影响为准 |
| 环境切换 | 全局状态，影响首页/进度/日历；切换需整页刷新避免残留旧卡片 | `FX§M08 数据隔离` |

### 7.2 当前实现

`MainActivityShell.kt`（194 行）

- `page`：`rememberSaveable { mutableStateOf(1) }`（`67`），取值 `0=资源库 / 1=发现 / 2=在看 / 3=我的 / 4=设置`（`119-171`）。
- Tab 映射：`tabs = listOf("发现","我的","资源库","设置")`（`89`），`onSelect` 用 `listOf(1,3,0,4)[it]`（`93`）。
- 选中态：`selected = if (page == 2) 0 else listOf(1,3,0,4).indexOf(page)`（`91`）。
- 导航渲染：`if (page != 0)` 才在 Shell 渲染胶囊（`175-178`）；资源库页由 `SourceWorkspace` 内嵌（`120-124` + `SourceWorkspace.kt:27`）。
- 避让：非 0/1 页 `padding(bottom = 84.dp)` + `windowInsetsPadding(statusBars)`（`116-117`）；胶囊 `navigationBarsPadding().padding(bottom = 22.dp)`（`176`）；资源库 `padding(bottom = 94.dp)`（`SourceWorkspace.kt:24`）。
- FLAG_SECURE：`SideEffect { if (SourceAccessSession.unlocked && records.any{hidden}) addFlags else clearFlags }`（`80-84`）。
- 死代码：`selectedTitle` + `HomeTitleSheet`（`86, 181-183`）从未被赋值。
- 状态栏：`isAppearanceLightStatusBars = !(page == 1 && ambientDark)`（`189-193`）。
- 背景：`AnimatedGradientBg(flatColor = if (page == 1) null else pageBackground)`（`105-110`）。

### 7.3 差距清单

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| SHELL-01 | 路由用裸 int + 魔法数组，`page` 编码与 Tab 序号不同构 | `MainActivityShell.kt:67,89,91-93,119-171` | 每次加页/调序都靠人肉对表；`page==2`（在看）时「发现」被点亮 | 新增 `shell/ShellRoutes.kt`：`enum class Tab(label){ DISCOVER, ME, LIBRARY, SETTINGS }` + `sealed interface ShellDest { data class Tab(...); object Watching; ... }`；`MainShell` 只持 `ShellDest`，`selected` = `dest.tabIndex()` | P0 |
| SHELL-02 | 「在看」被降级为发现子页 | `MainActivityShell.kt:155-162`（`onShowWatching = { page = 2 }`、`onBack = { page = 1 }`）；`WatchingPage.kt:48-57` | 点「继续观看」› 进入后，底部「发现」高亮、顶部又写「‹ 发现」；从「我的」无法进入 | 见 WATCH-01；`ShellDest.Watching` 作为二级页，`selected` 归到 `Tab.ME`（与竞品相同） | P0 |
| SHELL-03 | FLAG_SECURE 以「解锁 + 存在隐藏源」为条件，对**整个 Activity** 生效，并且写在 `SideEffect` 里每帧执行 | `MainActivityShell.kt:80-84`；`SourceAccessSession.kt:12-15` | 只要解锁过私密库，首页/设置/详情也全部禁止截屏与录屏（截图全黑、投屏/无障碍工具失效）；锁屏回到前台前 flag 可能残留 | 改 `shell/ShellSecure.kt`：只在**私密目的地**（`PrivateActivity` / 隐藏源详情）`onResume` 时 `addFlags`、`onPause` 时 `clearFlags`；`MainShell` 侧删除 `SideEffect`；若仍需全窗保护则改为 `DisposableEffect(unlocked, hiddenCount)` 而非 `SideEffect` | P0 |
| SHELL-04 | 导航避让常数两套（84 / 94），跨页胶囊跳动 | `MainActivityShell.kt:116`；`SourceWorkspace.kt:24` | 资源库 ↔ 其他页切换时胶囊上下跳 10dp（真机可感知） | `DesignTokens.navReserve = 94f`；navigate 容器统一 `padding(bottom = navReserve.dp)`；`SourceWorkspace` 不再自算 | P0 |
| SHELL-05 | 资源库页的胶囊位置由两处代码决定（Shell 隐藏 + Workspace 内嵌） | `MainActivityShell.kt:175`（`if (page != 0)`）；`SourceWorkspace.kt:27` | 任何一处调整都会让两个页面不一致；`navigation` 参数一路透传增加耦合力 | 单一渲染点：`MainShell` 底部 `Box { shellNavigation() }` 对所有 Tab 页生效；`SourceWorkspace` 去掉 `navigation` 参数（保留 `panel`） | P1 |
| SHELL-06 | `selectedTitle` + `HomeTitleSheet` 是死代码 | `MainActivityShell.kt:86,181-183` | 无用户可见后果，但占用重组与阅读成本 | 删除两处；`HomeTitleSheet` 改造成 `home/HomeLayoutEditor`（见 DISC-06）或一并删除 | P2 |
| SHELL-07 | 无环境（Environment）全局状态 | 全仓零命中；`UI§2.4`、`FX§M08` | 与 PROF-02/WATCH-02 同根；切换环境后首页/进度不隔离 | `environment/EnvironmentStore.kt` 作为 Shell 级状态；`MainShell` 在环境变更时 `homeGeneration++` 强制刷新（`MainActivityHomeFeed` 已用 `shellState.homeGeneration` 做 palette key） | P1 |
| SHELL-08 | 状态栏反色只覆盖发现页，其余页固定浅色图标 | `MainActivityShell.kt:189-193` | 「我的/设置」若将来接深色主题（SET-04）会出现白底白字 | 反色条件改为读 `chrome` 的 `lightForeground(pageBackground)`，不写死 `page == 1` | P2 |
| SHELL-09 | 发现页内容底部 padding 100dp 与 Shell 84dp 两套 | `DiscoverPage.kt:129`；`MainActivityShell.kt:116` | 列表末尾留白与导航位置不匹配（约 16dp 误差） | 统一用 `DesignTokens.navReserve + 6`；`DiscoverPage` 的 `contentPadding` 读 token | P2 |

---

## 8. 页面 → 现有文件 → 目标文件家族

| 页面 / 区块 | 现有文件（职责） | 目标文件家族（建议） |
| --- | --- | --- |
| 发现页编排 | `ui/design/DiscoverPage.kt`（426 行，编排 + 6 个 section + 错误卡 + 标题件都在一个文件里） | `ui/design/home/HomePage.kt`（只做 LazyColumn 编排）；`home/HomeSectionModel.kt`（枚举 + 顺序 + 显隐）；`home/sections/*.kt`（每个 section 一文件，见下） |
| Hero | `home/HeroPage.kt`、`HeroCarousel.kt`、`home/HeroCard.kt`、`home/hero/trailer/*`、`RatingBadges.kt` | `home/hero/HeroPager.kt`（分页 + 自动轮播）、`HeroPageLayout.kt`（图形层）、`HeroDots.kt`（指示器）、`HeroMetaBlock.kt`（类型/日期/简介/徽章）、`RatingBadgeRow.kt`（多源徽章） |
| 分区组件 | `DiscoverPage.kt` 内的 `PosterSection / WideSection / VarietySection / TmdbPosterCard / HomeSectionHeader` | `home/sections/PosterRowSection.kt`（含列宽参数）、`WideRowSection.kt`、`VarietyFullWidthSection.kt`、`PeopleCircleSection.kt`（新）、`ProviderRowSection.kt`（新）、`GenreGridSection.kt`（新）、`CompanyRowSection.kt`（新）、`TopRatedSection.kt`（新）、`PosterGridSection.kt`（新）、`SectionErrorCard.kt`、`SectionHeader.kt` |
| 继续观看 / 在看卡 | `DesignComponents.kt:ContinueCard`、`WatchingPage.kt` 内联行卡 | `ui/design/cards/ProgressWideCard.kt`（16:9 + 季集 + 平台 + 进度 + 百分比，两处共用） |
| 底部导航 | `ui/design/CapsuleNavBar.kt`、`home/NavGlyph.kt`、`DesignTokens.nav*` | `ui/design/nav/CapsuleNavBar.kt`、`nav/NavTabModel.kt`、`nav/NavGlyph.kt`、`nav/DiscoverAvatarGlyph.kt`（新）、`nav/GlassPill.kt`（新，统一毛玻璃） |
| Shell 路由 / 避让 / 安全 | `MainActivityShell.kt`、`MainActivityHomeFeed.kt` | `shell/ShellRoutes.kt`（Tab 枚举 + ShellDest）、`shell/ShellChrome.kt`（状态栏反色 + 导航避让）、`shell/ShellSecure.kt`（FLAG_SECURE 策略）、`shell/ShellHost.kt`（只做 when 分发） |
| 资源库概览 | `sources/ui/SourcesScreen.kt`（190 行）、`cards/{SourceTile,StorageEntryCard,SourceIcon}.kt`、`SourceCard.kt` | `sources/ui/SourcesOverview.kt`（只做网格编排）、`SourcesHeader.kt`（编辑 + 锁 + 添加）、`LibraryGroups.kt`、`LibraryGroupEditor.kt`、`SourceEmptyState.kt`、`protocol/SourceProtocol.kt`（类型枚举 + 能力矩阵 + 未接入原因） |
| 添加资源库 | `SourceTypeSheet.kt`、`EmbySourceForm.kt`、`SourceManagementSheet.kt`、`SourcePastePreview.kt`、`workspace/{SourceConnectionContent,SourceOptionalFields}.kt` | `sources/ui/AddSourceSheet.kt`（五段 tab：连接/信息/线路/高级）、`add/PasteRecognizeField.kt`、`add/AddressFields.kt`、`add/LineFields.kt`、`add/TestConnectionAction.kt` |
| 我的 | `ui/design/Pages.kt:ProfilePage`（占位） | `ui/design/profile/ProfilePage.kt`、`ProfileEnvironmentHeader.kt`、`ProfileWeekStrip.kt`、`ProfileDayCard.kt`、`ProfileStatsPager.kt`、`ProfileHeatmap.kt`、`ProfileModuleList.kt`、`ProfileModuleEditor.kt`、`ProfileEnvironmentMenu.kt` |
| 设置 | `settings/`（29 文件，单职责已达标） | 保留现有拆分；新增 `settings/SettingsNetworkSub.kt`、`SettingsProSub.kt`、`SettingsFeedbackSub.kt`、`SettingsLegalSub.kt`、`SettingsModulesSub.kt`、`SettingsBadgeSub.kt`、`SettingsFooter.kt`；`SettingsAppearanceNav.kt` 接拖拽排序 |
| 在看 | `ui/design/WatchingPage.kt`（142 行，列表 + 空态 + 返回条） | `ui/design/watching/WatchingPage.kt`、`WatchingContinueRow.kt`、`WatchingRecords.kt`（折叠 + 未识别分组）、`WatchingCalendarEntry.kt`（底部长按）、`WatchingMenu.kt`（弃剧/撤销） |
| 时间格式化 | `Pages.kt:60-82`（`relDays/fmtSeen/formatDate`） | `ui/format/TimeText.kt`（`Pages.kt` 删除后由各页共用） |
| 环境（新域） | 无 | `environment/EnvironmentStore.kt`、`environment/Environment.kt`（id/name/avatar/pin） |

**拆分依据**：仓库铁律（`AGENTS.md` → `.codex/memory/kivo-video-iron-gate.md`）要求单文件单职责、禁止平铺；`DiscoverPage.kt` 现有 426 行且混编排 + 7 个组件 + 错误卡 + 文案映射，是本次拆分的第一优先。

---

## 9. 优先级汇总（可直接排卡）

### P0（结构性错位，用户第一眼就能看到）

| 编号 | 一句话 |
| --- | --- |
| DISC-02 | 分页点选中不变大 → 不知道在第几页 |
| DISC-03 / WATCH-08 | 继续观看卡缺季集与平台名，`sourceOf` 恒「本地」 |
| DISC-08 | hero 整块不可点，点海报无反应 |
| NAV-01 | 「发现」Tab 图标是兜底指南针，应为头像 |
| NAV-02 | 选中态蓝 tint 应为白底 + 原色图标 |
| NAV-06 / SHELL-04 | 导航避让常数 84 vs 94，跨页胶囊跳 10dp |
| PROF-01 | 「我的」是占位页 |
| PROF-06 | 「我的」无模块流，导致「在看」无处安放 |
| WATCH-01 / SHELL-02 | 「在看」被做成发现子页，入口归属与竞品相反 |
| SHELL-01 | 路由裸 int + 魔法数组 |
| SHELL-03 | FLAG_SECURE 全窗生效且每帧执行 → 解锁后全 App 不可截屏 |

### P1（契约缺块，决定“像不像竞品”）

DISC-01 多源评分 · DISC-04 分区列宽 · DISC-05 六类缺失分区 · DISC-07 自动轮播 · DISC-09 分区分错 · LIB-01 库类型 · LIB-04 添加 sheet 五段 · LIB-05 测试连通性 · PROF-02 环境 · PROF-03 周日历 · PROF-04 统计 · SET-01 设置分组 · SET-03 动效门 · SET-04 深色 · SET-05 豆瓣/IMDb · SET-09 页脚声明 · WATCH-02 环境 · WATCH-03 移除 · WATCH-06 日历入口 · SHELL-05 单一导航渲染 · SHELL-07 环境状态 · NAV-03 毛玻璃 · NAV-04 暗态胶囊

### P2（打磨 / 可延后）

DISC-06 首页编辑器 · DISC-10 动效降级 · DISC-11 标题基线 · DISC-12 箭头统一 · NAV-07 搜索源前置 · NAV-08 字体缩放 · LIB-02 分组 · LIB-03 段折叠 · LIB-06 粘贴识别形态 · LIB-07 备用线路 · LIB-08 分段空态 · LIB-10 编辑模式 · PROF-05 热力图 · PROF-07 模块编辑 · PROF-08 快捷菜单 · SET-02 反向代理 · SET-06 Tab 排序 · SET-07 Badge · SET-08 管理模块 · WATCH-04 折叠记录 · WATCH-05 未识别 · WATCH-07 弃剧 · WATCH-09 百分比 · WATCH-10 暗色卡 · SHELL-06 死代码 · SHELL-08 状态栏 · SHELL-09 底部留白

### P3（依赖外部能力，不在本轮）

LIB-09 Drivemby 识别进度（无该协议）· PRO 授权主体（`SET-01` 的 PRO 子页只立入口，不伪造购买流程）

---

## 10. 复核方式

| 项 | 方法 |
| --- | --- |
| 分页点（DISC-02） | 真机滑动 hero，逐页对照 `IMG_0330/0331`，量选中点直径 |
| 导航跳动（NAV-06/SHELL-04） | 资源库 → 我的 → 想起切换，录屏量胶囊底边 y 差（应 0） |
| FLAG_SECURE（SHELL-03） | 解锁私密库 → 回发现页 `adb shell screencap`；再退出解锁复测 |
| 继续观看卡（DISC-03） | 播一集剧到中途 → 回首页看卡上是否出现「第 X 季 · 第 X 集」与平台名 |
| 设置灰开关（SET-03/04/05） | 逐行点按，确认 `enabled=false` 的行不可切换（当前行为即“灰”） |

> 本报告不动生产代码；所有“可落地改法”仅为文件与符号级建议，落地前需按仓库铁律补家族树与单测门闸。