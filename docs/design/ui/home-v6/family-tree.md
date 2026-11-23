# 首页 v6 落地 · 文件家族树

> 2026-11-23 · 视觉依据：`docs/design/ui/home-desktop/kivo-home-mobile-v6.html`（三份外部评审稿吸收后的定稿候选）
> 目标：**新旧两版并存、设置开关切换对比**；旧版 `DiscoverPage` 及其 `home/sections/` 一行不改。

## 0. 门闸记录

| 门闸 | 结论 |
| --- | --- |
| 构思 | v6 = 参考作的沉浸式取色 Hero + kivo 自己的取色卡语言（分类 / 高分）+ 片源签条 + 想看闭环 |
| 规划 | 只用**已存在的真实数据**落视觉；缺数据的区块本轮不出现（见 §3），不伪造 |
| 家族树 | 本文 §1 |
| 实现 | 每个文件一句话职责，≤ 300 行 |

## 1. 家族树

```
ui/design/home/
├── HeroCardsState.kt          【新】hero 候选 → HeroCard 列表（含 IO 补竖版海报）；新旧两版共用，
│                              从 DiscoverPage 原样抽出（行为不变）
└── v6/                        【新】首页 v6 家族
    ├── DiscoverPageV6.kt      编排：区块顺序 + 滚动驱动（取色底 / 舞台明暗）+ 数据接线；不含卡片排版
    ├── V6Metrics.kt           v6 尺寸 / 圆角 / 字号 / 间距常量（单一真相）
    ├── V6SectionHeader.kt     区块标题行：标题 / 可选副标题 / 36dp「更多」
    ├── V6Badges.kt            片源签条（继续观看）与单枚画质标（海报）两个视觉原子
    └── sections/
        ├── V6ContinueRow.kt   继续观看：16:9 300dp 卡，信息压图，签条 + 进度
        ├── V6LibraryRow.kt    片库分类行（NAS 分节）：S 海报 + 文件名解析的画质标
        ├── V6PosterRow.kt     TMDB 海报行（院线新片 / 今日动漫）：S 海报，海报上不放操作按钮
        ├── V6WideRow.kt       16:9 剧照行（热门剧集）：打破连续 2:3 的第二种比例
        ├── V6RankedRow.kt     排行榜海报行（高分剧集）：海报左上大号名次，整榜可滑可点（对标 Apple TV Top Charts）
        └── V6GenreCards.kt    分类浏览：整张剧照打底的图卡 + 大号中文名 / 英文小字
```

改动（非新增）：

| 文件 | 改动 | 理由 |
| --- | --- | --- |
| `MainActivityShell.kt` | 发现 Tab 分支整段迁出 | 文件已 300 行（铁律上限），「发现页数据接线」是独立变化原因 |
| `MainActivityDiscoverTab.kt`【新】 | 发现 Tab：数据加载 + 按开关选 v5 / v6 | 与 `MainActivityHomeFeed.kt` 同族（Shell 的 Tab 级接线） |
| `ui/design/AppearancePrefs.kt` | 加 `homeV6`（默认 true） | 外观策略单例，已有同类开关 |
| `settings/SettingsAppearanceSub.kt` | 加「首页新版（预览）」开关 | 外观设置页 |
| `ui/design/DiscoverPage.kt` | hero 卡片构建改调 `rememberHeroCards` | 去重；行为不变 |
| `tmdb/TmdbGenres.kt` | 加 `fillDistinctBackdrops` | 每个类型一张**互不重复**的剧照（热门片跨多个类型，取首条会撞图） |

## 2. 区块与数据（全部真实来源）

| v6 区块 | 数据 | 来源 |
| --- | --- | --- |
| Hero | heroCandidates / logo / trailer | 复用 `HeroCarousel`（已按参考作验收） |
| 继续观看 | `HomeEntry`：进度、时长、季集、来源名；画质 = 文件名解析（Emby 无则不显示画质段） | `state.homeEntries` + `MediaQualityInfo` |
| 片库分类行 | `NasLibrarySource.Section` | `state.nasSections` |
| 院线新片 | `TmdbHome.nowPlaying` | 已有 |
| 热门剧集（16:9） | `TmdbHome.onTheAir`（backdrop） | 已有 |
| 今日动漫 | `TmdbHome.todayAnime` | 已有 |
| 高分剧集 | `TmdbHome.topRatedTv`（排行行，副标题「TMDB 评分」，不写豆瓣） | 已有 |
| 分类浏览 | `TmdbGenres`（名称 + 部分类型有剧照） | 已有 |

舞台明暗：沿用 `HomeAtmosphere.lightForeground` 阈值（一次性翻转），导航玻璃已由 `state.ambientDark` 驱动 —— v6 不另造。

## 3. 本轮不做（缺真实数据，下一轮补数据层）

| 区块 | 缺什么 |
| --- | --- |
| 最近入库 | NAS `addedMs` 恒 0；Emby `recent` 丢了 DateCreated |
| 本周更新 | 无任何片库剧集播出表拉取 |
| 高分电影 | 无 top_rated movie 拉取 |
| TMDB 行「库内」画质标 | 首页无 TMDB id ↔ 片库映射 |
| 继续观看「下一集」/ 章节刻度 | `HomeEntry` 无此字段 |
| Hero 548 高 / 分段进度分页 | `HeroCarousel` 高度与分页是共享常量，改它会动旧版；下一轮加参数 |

## 4. 进度（2026-11-23）

| 项 | 状态 |
| --- | --- |
| §1 家族树全部落文件（14 新 + 5 改），最大文件 `DiscoverPageV6.kt` 178 行；Shell 300 → 223 行 | ✅ |
| `JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat :app:assembleDebug` | ✅ BUILD SUCCESSFUL（1m59s） |
| K40（`b6e950ee`）覆盖安装（`install -r`，未卸载） | ✅ Success |
| 真机截图 | ✅ `docs/evidence/ui-shot/home-v6/`：`v6-01-first` ~ `v6-07-scroll`（首屏 + 6 屏滚动）、`v6-fix-veil` / `v6-fix-genres`（修复后复拍）、`toggle-off` + `legacy-01-first`（开关关 ⇒ 原版） |
| 真机发现并已修 | ① 亮场内容钻到状态栏下与时间叠字 → 加状态栏遮罩（亮场淡入）；② TMDB 给多个类型同一张剧照 → 分类卡剧照去重，无图卡按 id 取色相 |
| 开关往返 | ✅ 关 ⇒ 原版（继续观看「看到 3:33」旧卡）；再开 ⇒ v6 |

### 与 v6 稿对照的剩余差距（下一轮）

| 差距 | 原因 / 计划 |
| --- | --- |
| 首屏「继续观看」被导航压住 | `HERO_HEIGHT_DP=585` 新旧共用；给 `HeroCarousel` 加高度参数，v6 传 548 |
| Hero 分页仍是 10 个圆点 | 同上，`HeroCarousel` 加「分段进度」样式参数 |
| 导航选中态仍是旧版（白底块） | 导航是全 App 共用组件，单独一张卡做，避免影响其它页 |
| 无剧照的分类卡偏空 | 数据只补了 8 张剧照且 TMDB 有重复；下一轮按类型取「该类型人气第一」的剧照 |
| 最近入库 / 本周更新 / 高分电影 / 画质标 / 下一集 | 见 §3，先补数据层 |

## 5. 验收

1. `:app:assembleDebug` 绿；新增文件均 ≤ 300 行。
2. K40 真机：开关开 / 关各截首屏 + 三个滚动位置，存 `docs/evidence/ui-shot/home-v6/`。
3. 与 `kivo-home-mobile-v6.html` 逐区块对照，列出差距。

## 6. 第二轮（2026-11-23 晚，用户审阅 + 22 张同类 App 参考图后）

| 用户意见 | 改法 | 真机证据 |
| --- | --- | --- |
| 高分片单只能点 3 部 | 深色大卡 → 排行榜海报行（整榜 20 部，名次压海报左上） | `r2-mid.png` `r2-bottom.png` `r2-bottom-swiped.png` |
| 分类卡丑 | 浅色取色卡 → 整张剧照图卡 + 大字；剧照用 `fillDistinctBackdrops` 去重 | `r2-genres-2.png` `r2-genres-3.png` |
| 海报右上「＋」画蛇添足 | 全部移除；「想看」留在详情页 | 同上 |

删除（不再被引用，不留死代码）：`V6TopRatedCard.kt` `V6GenreTiles.kt` `V6Watchlist.kt` `V6Tint.kt` `V6SeedColor.kt`。

## 7. 第三轮家族树（「查看全部」+ 更多维度行 + 紧凑继续观看）

```
tmdb/
├── TmdbList.kt            【新】首页 TMDB 榜单的**唯一路径表**（enum：路径 + 是否剧集）；
│                          TmdbHome 原有 nowPlaying/onTheAir/topRatedTv/todayAnime 改为引用它（行为不变）
└── TmdbHome.kt            【改】加 list(kind, page)：同一路径分页（page=1 不带参数，沿用旧缓存键）

ui/design/home/v6/list/    【新】「查看全部」二级页家族
├── HomeListActivity.kt    入口：从 Intent 取榜单与标题，承载页面、打开详情
├── HomeListPager.kt       分页状态：逐页拉取、去重、加载中 / 失败原因 / 到底
└── HomeListScreen.kt      视图：玻璃返回钮 + 居中标题 + 三列海报网格 + 加载 / 失败 / 空态

ui/design/home/v6/sections/V6ContinueRow.kt   【改】紧凑卡 228×128：进度与剩余时间压在图内，片名移到卡下
ui/design/home/v6/DiscoverPageV6.kt           【改】各行接「查看全部」；新增 4 行
MainActivityDiscoverTab.kt                    【改】加载新增 4 行数据
AndroidManifest.xml                           【改】注册 HomeListActivity
```

新增行（全部 TMDB 真实接口，无豆瓣数据 ⇒ 不写「豆瓣」）：

| 行 | 接口 | 形态 |
| --- | --- | --- |
| 高分电影 | `/movie/top_rated` | 排行海报行（与高分剧集同组件） |
| 热门华语电影 | `/discover/movie` 原声 zh、按人气、≥50 票 | 海报行 |
| 近期热门韩剧 | `/discover/tv` 原声 ko、近一年首播、按人气 | 16:9 行 |
| 近期热门日剧 | `/discover/tv` 原声 ja、近一年、排除动画 | 16:9 行 |

### 第三轮进度（2026-11-23）

| 项 | 结果 |
| --- | --- |
| 构建 / K40 覆盖安装 | ✅ BUILD SUCCESSFUL；`install -r` Success |
| 新增 4 行（高分电影 / 近期热门韩剧 / 热门华语电影 / 近期热门日剧）全部真实数据出图 | ✅ `r3-01` ~ `r3-08` |
| 紧凑继续观看（228×128，进度 + 剩余时间在图内，片名在卡下） | ✅ `r3-02` |
| 「查看全部」：点院线新片 › → 三列网格；滚动自动翻页；到底「已显示全部 98 部」（5 页去重）；返回键回首页原位 | ✅ `r3-list-top` `r3-list-deep` `r3-list-end` |
| 新增 / 改动文件均 ≤ 300 行（最大 `DiscoverPageV6.kt`） | ✅ |

## 8. 第四轮：NAS「查看全部」+ 分类全量剧照 + 去噪（2026-11-23）

```
library/NasLibraryIndex.kt          【新】索引「可用条目」读取（读索引 + 剔除已删 / 隐藏源）；refreshNasLibrary 改调它（行为不变）
ui/design/home/NasEntryRoute.kt     【新】NAS 条目 → 详情 Intent；MainActivity.openNasEntry 改调它，解析失败改为 Toast 原因（原为静默 return）
ui/design/home/v6/list/
├── V6ListChrome.kt                 【新】二级列表页共用外框（浮动顶栏 + 状态文案），从 HomeListScreen 抽出
├── NasCategoryActivity.kt          【新】NAS 分类页外壳：IO 读完整索引（不触发扫描）
└── NasCategoryScreen.kt            【新】三列 NAS 海报 + 画质标 + 片名 / 年份
```

| 改动 | 理由 |
| --- | --- |
| 片库行加「›」→ NAS 分类完整页 | 用户反馈：NAS 行没有更多入口 |
| 分类剧照 10 → 全部 19 个类型 | 用户反馈：恐怖之后全是色块 |
| 片库卡副信息去掉「NAS」、继续观看卡下不重复来源、高分行去掉「TMDB 评分」副标题 | 去噪：整行同源的信息每张卡重复一遍只是噪音；不加「本周 +23」类计数 |
| Hero 高度保持 585 | HeroPage 用绝对坐标排版，改高度会动旧版；紧凑续播卡后首屏已露出完整卡图 + 进度 |

真机证据：`r4-01-library`（片库行「›」）、`r4-02/03-nas-movies*`（378 部，到底「共 378 部」）、`r4-04-nas-open`（点开进详情）、`r4-05-genres-strip`（19 类剧照不重复）。
