# 质量审查：我的页 + 首页分区重构（未入库改动）

- 审查对象：`C:\kivo video` 工作区未入库改动（profile 36 文件 / home sections 14 文件 / components 6 / tokens 2 / motion 2 / shell 2，以及 12 个已改文件）
- 审查方式：只读静态审查（未修改任何产品代码、未执行编译）。行数用 awk 的 NR 计数（比 wc -l 多算末尾无换行文件的一行）
- 结论速览：分层与令牌化做得扎实，但存在 2 个用户可感知的功能性缺陷、4 处主线程 IO/解码、大量新旧共存死代码，以及 1 处明确超 300 行铁律

---

## 一、铁律：单 Kotlin ≤ 300 行 / 单文件单职责

### 1.1 【P1】超 300 行铁律：ProfileStats.kt 实际 301 行
- 【文件:行号】android/app/src/main/java/com/kivo/cinema/ui/profile/ProfileStats.kt:301
- 【现象】wc -l 报 300 行，但文件末尾无换行符，awk 实测 301 行（第 301 行为文件结束的右花括号）。已越过 ≤300 上限。
- 【为什么是问题】规则是单文件 ≤300 行，301 行即违规；且 wc -l 会漏报，容易在自检时误判为合规。
- 【可落地改法】把纯函数 weeklyHeadline（:251-256）连同 GENRE_DOMINANT_PERCENT 常量迁到新文件 WeeklyInsightText.kt（腾出 6 行余量）；或把 weeklyInsight/weekDayKeys（:206-238）迁出。顺带在文件末尾补一个换行，让 wc -l 与实际行数一致。

### 1.2 【P2】多个文件贴线，无改动余量
- 【文件:行号】ui/profile/calendar/CalendarPageChrome.kt:299、ui/profile/ContinueHero.kt:298、ui/profile/CollectionsRow.kt:298、ui/design/DiscoverPage.kt:297、ui/profile/WeeklyReport.kt:295、MainActivityShell.kt:300
- 【现象】上述文件实际行数 295–300，全部顶格。
- 【为什么是问题】任何小修小补都会立刻破线，形成改一次拆一次的恶性循环。
- 【可落地改法】下次触碰这些文件时顺手把预览数据或纯函数拆到独立文件，预留 ≥20 行缓冲；MainActivityShell.kt 尤其应拆分（见 1.3）。

### 1.3 【P2】MainActivityShell.kt 单文件单职责被破坏（与自身 KDoc 矛盾）
- 【文件:行号】MainActivityShell.kt:84-85（声明唯一职责是把 HomeShellState 与 Activity 能力接成页面树、数据供给不在这里），实际数据供给写在 :171-199（6 个 produceState 抓 TMDB 人物/平台/类型/海报墙/高分剧/公司），:295-299 还承担 Window insets 副作用。
- 【现象】KDoc 声明的职责与实际代码不符：数据抓取、路由编排、导航胶囊配置（:108-126）、状态栏管理混在一个 300 行文件里。
- 【为什么是问题】违反单文件单职责，也让 KDoc 误导后续维护者。
- 【可落地改法】把 :171-199 的 6 个 produceState 抽成 rememberHomeDiscoverData(client, home) 返回数据类；把 :295-299 的状态栏副作用抽成独立 Composable（见 2.4）。

---

## 二、功能正确性

### 2.1 【P1】管理模块是假开关：文案承诺与实际行为相反
- 【文件:行号】ui/profile/modules/ModuleManagerSheet.kt:155（提示文案 关闭的模块不在我的页显示）对比同文件 KDoc :47-48（关闭的模块不影响我的页渲染：页面仍按基准截图顺序渲染全部区块）；ui/profile/ProfilePage.kt:85、:183-189
- 【现象】用户在弹层里关掉一个模块，页面内容一个都不会少；开关只改 enabledModules 这个 remember 内存集合，既不影响 LazyColumn 的 item 显隐，也不落盘，离开页面即恢复。
- 【为什么是问题】这是用户可感知的功能性错误：UI 明确承诺不在我的页显示，实际完全无效果，属于欺骗性交互。
- 【可落地改法】二选一：① 让 enabledModules 真正驱动 ProfilePage 的 item 显隐（每个 item 包一层 module in enabledModules 判断）并持久化到 SharedPreferences；② 若本卡确实只做视觉复刻，就把开关改为不可操作的展示态，并把文案改为模块开关即将开放，去掉假承诺。

### 2.2 【P1】MotionGate 语义与文档相反，会覆盖系统级无障碍设置
- 【文件:行号】ui/motion/MotionGate.kt:16（KDoc：两者取或）vs :42-46 实现 false 分支直接返回 false；实际调用点 ui/design/HeroCarousel.kt:66（首页轮播）、ui/compose/player/PlayerBarsHost.kt:72（播放器）
- 【现象】KDoc 说 App 开关与系统减弱动画取或，但实现里 App 开关为 false 时直接返回 false，无视 systemReduced = true。即：系统里已开启移除动画（无障碍需求），只要 App 内开关是关的，动画照样全量播放。
- 【为什么是问题】违反 Android 无障碍语义（系统动画缩放是全局用户意愿，App 不应反向覆盖），也和本文件自己的设计文档冲突；影响首页轮播和播放器两个生产路径。
- 【可落地改法】把 reduce 改为 appEnabled == true || systemReduced；App 开关语义调整为该开关只能加严不能放松。若确实要保留三态，至少让 false 分支回落到 systemReduced。

### 2.3 【P1】主线程磁盘遍历与 Bitmap 解码
- 【文件:行号】ui/design/settings/SettingsPage.kt:59-60（remember 中调 store.imageCacheBytes / ratingCacheBytes，定义见 SettingsStore.kt:49-50 的 dirBytes 遍历缓存目录）；ui/profile/thumbs/EntryCover.kt:34（remember 中 decodeCover，见 :55 decodeImageFileCapped）；ui/profile/ContinueHero.kt:74（remember 中 decodeCover，见 :252，CoverDecodeMaxDim = 1280 见 :280）
- 【现象】以上均在 Composable 组合期同步执行磁盘遍历/图片解码。项目内已有正确范式：ui/design/home/HeroPage.kt:72 与 ui/detail/DetailRelatedVideos.kt:131-132 都用 produceState + withContext(Dispatchers.IO)。
- 【为什么是问题】首次进入设置页/我的页会阻塞主线程；EntryCover/ContinueHero 的注释（:30）只靠压 maxDim 缓解，没有真正切线程，1280px 解码仍可能掉帧。
- 【可落地改法】统一改为 produceState(initial, key) 内 withContext(Dispatchers.IO)；SettingsPage 的缓存字节用 produceState(0L) 内 withContext(IO) 求 imageCacheBytes + ratingCacheBytes。

### 2.4 【P2】组合期直接改 Window（副作用未包 LaunchedEffect）
- 【文件:行号】MainActivityShell.kt:295-299
- 【现象】在 Composable 函数体里直接 findViewById 并 WindowCompat.getInsetsController(...).apply 设置 isAppearanceLightStatusBars。
- 【为什么是问题】典型的组合期副作用：任何重组（state 变化）都会重复执行系统 Window 写入，违反 Compose 的组合无副作用约定，且与 DiscoverPage.kt:259 用 LaunchedEffect(chromeDark) 回调的规范做法不一致。
- 【可落地改法】用 LaunchedEffect(page, state.ambientDark) 包住这段逻辑。

### 2.5 【P2】裸线程无生命周期管理
- 【文件:行号】MainActivityHomeFeed.kt:59-64
- 【现象】refreshContinue 用 Thread + runOnUiThread 更新 shellState.continueItems，而同文件 :16-55 的 refreshHomeFeed 用的是 lifecycleScope.launch。
- 【为什么是问题】Activity 销毁后线程仍可能回调 runOnUiThread 更新已销毁宿主的 State；风格也不统一。
- 【可落地改法】改为 lifecycleScope.launch，内部 withContext(Dispatchers.IO) 求 ContinueWatching.aligned，回到主线程赋值。

### 2.6 【P2】本周口径在两处不一致
- 【文件:行号】ui/profile/ProfileStats.kt:207-212（weekDayKeys：周一到周日）对比 :143-155 calendarDays / :259-267 weekStart（周日到周六）
- 【现象】周报统计的是周一到周日，而周历高亮的是周日到周六，两个本周不是同一周。
- 【为什么是问题】同一页面内两个本周概念不一致，跨周查看时数据与日历对不上，且 :206 注释已自认两者口径不同却未统一。
- 【可落地改法】让 weekDayKeys 复用 weekStart(Calendar.getInstance()) 生成 7 个键，统一周日为起点。

### 2.7 【P2】同一份 daily 被重复读盘
- 【文件:行号】ui/profile/ProfileStats.kt:88（watch.dailyMs(HISTORY_DAYS)）与 :118（watch.dailyMs(DAYS_PER_WEEK)）；:288-289 注释声称复用同一份 map 避免为头图再读一次盘
- 【现象】注释承诺复用，但 :118 又发起一次磁盘读取。
- 【为什么是问题】多一次无谓的 MediaStore/磁盘 IO，且注释与实现矛盾。
- 【可落地改法】用 :88 的 daily 计算 weeklyHours（取最近 7 天键求和）。

### 2.8 【P2】死状态：selectedTitle 永远为 null
- 【文件:行号】MainActivityShell.kt:107（初始化为 null）、:287-289（读取与清空）；全仓 grep 无任何赋值非 null 的代码
- 【现象】HomeTitleSheet 弹层永远不会出现。
- 【为什么是问题】无效状态与不可达 UI，徒增维护成本。
- 【可落地改法】在 DiscoverPage 的影片点击回调里给 selectedTitle 赋值，或删除该状态与 HomeTitleSheet 调用。

### 2.9 空态/错误态闭环评价（正面）
- DiscoverPage.kt:169-175（hero 空/错误）、:251-253（列表错误）、TmdbErrorCard.kt:27-33（五类原文+重试）、CalendarPage.kt:141-144（无记录空态）、:176/188（加载中）、WatchingPage.kt:59-79（空态引导）闭环完整，错误文案保留五类未合并，符合 DISO-09。未发现静默降级成假数据的情况。

---

## 三、设计一致性

### 3.1 【P2】已改文件仍有硬编码色值绕过令牌
- 【文件:行号】ui/design/WatchingPage.kt:97（Color 0xFF24242A）、:118（0x1A000000）、:124（Color.Black）、:89（Color.White.copy 0.92f）；ui/design/CapsuleNavBar.kt:22（White 0.60f）、:25（White 0.14f）、:49（Black .12f）；ui/design/home/NavGlyph.kt:58（White .75f）；ui/components/GlassSurface.kt:27（默认色 Color.White）
- 【现象】这些色值可用 DesignTokens.canvasFill / hairline / navPill 等表达，却直接写了裸色。
- 【为什么是问题】暗态切换、全局换色时这些点不会跟随令牌，形成视觉不一致；NavGlyph.kt:58 在浅灰 pill 上画 75% 白点，对比度尤其可疑。
- 【可落地改法】逐个替换为语义令牌 + alpha；若确属令牌未覆盖的规格，按 GlassSurface.kt:39 的做法在旁边注明令牌无对应项、实测取值。
- 【正面】新增的 ui/profile/** 与 ui/design/home/sections/** 全部文件中没有一处 Color(0x...) 硬编码（唯一出现在 ui/tokens/DesignPalette.kt 的定义处），说明新代码的令牌纪律明显好于被改的旧文件。

### 3.2 【P2】暗态令牌已就绪但 UI 明确不接
- 【文件:行号】ui/tokens/DesignPalette.kt:47-77（Dark 全量，多处标推断待真机取色）、:115-139（PageGradient.Dark）；ui/profile/ProfilePageGradient.kt:11（深色态暂无 isSystemInDarkTheme 分支，先用 Light）；ui/design/settings/SettingsAppearanceSub.kt:49（深色色板 v2 未实现，当前固定浅色）
- 【现象】令牌层做了完整的明暗两套，页面层全部固定用 Light。
- 【为什么是问题】令牌投入没有转化为用户可见能力；且 Dark 值多为推断未经真机验证，贸然接入反而有风险，需要在接与标清楚未接之间做决策，目前是中间态。
- 【可落地改法】短期保持现状但把 pageGradientDark 等标为 Deprecated（说明未接入、待暗态真机取证）避免误用；中期统一接 isSystemInDarkTheme 分支。

### 3.3 【P2】注释与代码不同步
- 【文件:行号】ui/design/DesignTokens.kt:45-46（注释当前无任何调用点、B-1 只出令牌），实际 ProfilePageGradient.kt:14、SettingsPage.kt:184 已调用 pageGradient。
- 【现象】无任何调用点的陈述已失效。
- 【为什么是问题】过时注释会误导后续开发者判断令牌是否被使用。
- 【可落地改法】更新为已被我的页/设置页接入。

### 3.4 【P2】全局视觉参数变更缺少本批内取证
- 【文件:行号】ui/design/DesignTokens.kt:42（radiusPoster 10 改 12）、:43（新增 elevationCard = 1f）
- 【现象】海报圆角与阴影高度是全项目共享参数，一次改动影响所有海报卡。
- 【为什么是问题】属于跨页视觉回归风险；依据写在注释里（REX 实测文档），但本批改动的目录下未附真机对比证据。
- 【可落地改法】在 docs/evidence/ 补一张改前/改后同屏对比，或在提交说明中链接 docs/design/kivo-play-baseline.md 的具体条目。

---

## 四、重复 / 死代码

### 4.1 【P1】旧我的页 5 个组件被完全替代却留在库里（约 1405 行）
- 【文件:行号】ui/profile/ProfileHeader.kt（242 行）、ui/profile/ContinueHero.kt（298 行）、ui/profile/CollectionsRow.kt（298 行）、ui/profile/WeeklyReport.kt（295 行）、ui/profile/WeeklyReportCharts.kt（272 行）；自认证据在 ui/profile/ProfilePage.kt:65-67（不再被调用，文件保留待 P4 清理）
- 【现象】全仓 grep 确认：这 5 个 Composable 只在各自文件的 Preview 和互相的注释里被引用，ProfilePage 生产编排完全没有调用它们。
- 【为什么是问题】新旧实现共存，占约 1405 行死代码；WeeklyReport.kt:69 甚至还在调用 MotionGate，造成死了还会被静态分析扫到的干扰。
- 【可落地改法】直接删除这 5 个文件（或移入 docs/archive/ 文本存档）。若担心证据留存，只保留 KDoc 描述，不留可编译代码。

### 4.2 【P2】新增 ui/components 中 3 个组件无调用点，且与旧实现重复
- 【文件:行号】ui/components/GlassSurface.kt:23、ui/components/EmptyState.kt:26、ui/components/ErrorState.kt:10
- 【现象】全仓 grep 无任何对 com.kivo.cinema.ui.components 下这三个组件的 import。TmdbErrorCard.kt:19 import 的是旧的 com.kivo.cinema.ui.design.GlassSurface（定义于 ui/design/DesignComponents.kt:90）；WatchingPage.kt:61-77、CalendarPage 空态、ContinueHero.EmptyHero 各自手绘空态，没用 EmptyState。
- 【为什么是问题】统一空态/错误态唯一实现的目标没有落地，反而新增了 3 个孤儿组件与 2 套 GlassSurface。
- 【可落地改法】确定唯一实现后，把 TmdbErrorCard、WatchingPage 空态、CalendarEmptyState、ContinueHero.EmptyHero 全部改为调用 ui/components 版本，并删除 DesignComponents.kt:90 的旧 GlassSurface（或反向统一）。

### 4.3 【P2】五类错误文案重复实现（枚举版 + 字符串版）
- 【文件:行号】ui/components/ErrorState.kt:21-27（ErrorKind 到文案）与 ui/design/home/sections/TmdbErrorCard.kt:27-33（String 到文案）
- 【现象】同一组五条中文文案维护了两份，且入参类型不同（ErrorKind vs 字符串键尾）。
- 【为什么是问题】改一处漏一处会导致错误提示不一致。
- 【可落地改法】让 TmdbErrorCard 调用 ErrorState.messageOf，或在 components 提供 messageOf(kind: String) 单一实现。

### 4.4 【P2】ProfileViewModelState 无调用点
- 【文件:行号】ui/profile/ProfileViewModel.kt:19-20
- 【现象】全仓 grep 仅找到定义本身；实际取数走 MainActivityShell.kt:235-241 的 produceState 调 ProfileStats.build。
- 【为什么是问题】20 行死代码，且其 KDoc 描述的调用方在 IO 取快照约定没有强制者。
- 【可落地改法】删除该文件，或让 Shell 改调 ProfileViewModelState.dashboard(context)（但仍需在外层 withContext(IO)）。

### 4.5 【P2】其他可接受但应登记的兼容残留
- 【文件:行号】ui/design/home/NavGlyph.kt:73-79（在看字形分支，ShellTab.kt:21-25 已把在看移出路由）；DiscoverPage.kt:102（genrePosters 兼容参数，Shell 只传 genrePosterUrls，:236-239 回退分支永不触发）；SettingsPage.kt:43-45（KDoc 自认 SettingsEntryGrid/SettingsStatusTiles/SettingsStorageHero 不再被调）；ui/shell/ShellTab.kt:25（WatchingPageCode = 2 死常量）
- 【为什么是问题】这些是有意为之的兼容层，本身不算错误，但数量多且无统一清单，容易在后续重构中被误删或误用。
- 【可落地改法】集中在 docs/design/legacy-compat.md 登记保留原因与删除条件；NavGlyph 的在看分支可直接删。

---

## 五、可运行性

### 5.1 【P2】同名组件跨包共存，import 易错
- 【文件:行号】ui/components/GlassSurface.kt:23 vs ui/design/DesignComponents.kt:90；ui/components/SectionHeader.kt:31 vs ui/design/DesignComponents.kt:120 vs ui/detail/DetailSections.kt:57；ui/components/MediaCard.kt:30 vs ui/detail/DetailResource.kt:74
- 【现象】3 组同名 Composable 分布在不同包，新 section 文件（如 TmdbErrorCard.kt:19）用的是旧包版本。
- 【为什么是问题】虽然 Kotlin 包名隔离不会直接编译失败，但 IDE 自动 import 容易选错包，导致视觉表现不一致。
- 【可落地改法】删除旧实现并统一到 ui/components，或在旧实现上加 Deprecated 注解（说明改用 ui/components 版本）。

### 5.2 【P2】文件末尾换行不统一影响自检
- 【文件:行号】ui/design/Pages.kt:32（文件末尾无换行）、ui/profile/ProfileStats.kt:301（末尾无换行，导致 wc -l 少报一行）
- 【为什么是问题】行数门禁若用 wc -l 会漏判（本报告已用 awk 修正），CI 与本地自检结论可能不同。
- 【可落地改法】统一补末尾换行；行数门禁脚本改用 awk 的 NR 计数或 git diff numstat。

### 5.3 import 链与 Preview（正面）
- 抽查本次新增/改动涉及的全部跨文件引用：HomeTitleSheet（ui/design/home/HomeTitleSheet.kt:18）、SettingsNavHost/SettingsRoute（SettingsNav.kt:20,28）、SettingsGroupList（SettingsGroupList.kt:46）、decodeImageFileCapped（ui/design/TmdbImage.kt:87）、ProfilePageBackdrop（ProfilePageGradient.kt:13）、previewDashboard（ProfilePagePreviewData.kt:23）均有定义。
- DesignTokens 的公开成员在改动中只增不删（diff 已核对），不会破坏既有调用点；Pages.kt 删除的旧 ProfilePage(videoCount: Int) 无残留调用。
- 所有 Preview 均为 private fun 且带 Preview 注解，只在 IDE 预览生效，不影响生产 APK 行为（仅带来少量编译期代码体积）。
- 未执行 gradle 编译（只读约束），以上为静态引用核对结论。

---

## 总体评价

### 好在哪
1. 分层清晰：编排（ProfilePage/DiscoverPage/MainShell）—数据（ProfileStats/CalendarSource/ContinueSource）—视图（sections/列表卡）—令牌（DesignPalette 到 DesignTokens）四层界限明确，KDoc 普遍写明了数据来源、口径与取舍。
2. 令牌化改造干净：DesignTokens 平滑代理 DesignPalette，公开成员零删除；新增的 profile 与 sections 目录零硬编码色值，纪律明显好于历史代码。
3. 不造假数据：ProfileStats.kt:66-74 明确列出无数据源的项并返回空集合，UI 渲染真实空态；错误态五类原文保留，没有合并成泛化提示。
4. 状态管理基本规范：produceState 的 key（CalendarPage 的 year/month、MainShell 的 continueItems）、rememberSaveable、LaunchedEffect(card.id, imageAttempt) 大多设置正确。
5. 线程意识较强：阻塞函数统一在 KDoc 标注请在 Dispatchers.IO 调用；VideoThumbnails 有守护线程池 + 3s 超时 + tmp 原子改名，HeroPage/DetailRelatedVideos 用了正确的 IO 解码范式。

### 差在哪
1. 两处假功能是可感知的正确性问题：模块开关（2.1）和不达标的无障碍门控（2.2）。
2. 主线程 IO/解码仍散落 4 处（2.3），且注释与实现不一致。
3. 死代码体量大：旧我的页 5 文件约 1405 行 + 3 个无调用新组件 + ProfileViewModelState，新旧共存。
4. 行数纪律松弛：1 处超标、6 处贴线；MainActivityShell 的 KDoc 与职责实际不符。
5. 暗态令牌与 UI 呈现脱节，已改文件仍有裸色值绕过 token。

### 最该修的 5 件事（按优先级）
1. 【P1】修 MotionGate.kt:42-46 改为或语义，恢复系统减弱动画的优先级（影响首页轮播与播放器）。
2. 【P1】让管理模块开关真正生效，或在未实现前撤掉开关并改正 ModuleManagerSheet.kt:155 的假承诺。
3. 【P1】把 SettingsPage.kt:59-60、EntryCover.kt:34、ContinueHero.kt:74 的磁盘遍历/Bitmap 解码统一改成 produceState + withContext(IO)。
4. 【P1/P2】删除旧我的页 5 个文件（约 1405 行）+ 3 个无调用 ui/components 组件，并为必须保留的兼容残留登记清单。
5. 【P1】拆 ProfileStats.kt 至 ≤300 行并补末尾换行；同步把行数门禁从 wc -l 改为 awk 计数。
