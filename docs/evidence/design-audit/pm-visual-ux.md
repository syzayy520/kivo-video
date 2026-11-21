# Kivo Video 视觉 / UX / 一致性设计审计（对标 REX）

> 审计人：产品经理 许清楚（Xu）｜日期：2026-11-19｜范围：只审不改，UI 已定稿
> 用户审美基准：**Apple/iOS 质感 · 极简 · 全系统一种风格 · 不让用户逐个发现问题**
> 证据口径：所有"现象"均来自真机截图，每条都给「我们 vs 竞品」双路径，可复核。

**路径缩写**（下文表格内使用）：
- 我们 = `docs/evidence/ui-shot/`，播放器 = `docs/evidence/p240/player-ui/`
- `REX/IMG_00xx` = `C:\Users\Administrator\Pictures\REX\2026_11_07_..._IMG_00xx.PNG`
- `PLAYUI/IMG_01xx` = `C:\Users\Administrator\Pictures\REX 播放UI\..._IMG_01xx.PNG`
- 竞品联系表 = `docs/evidence/rex-audit/sheet_{rex|playui|emby}.png`，单张缩略 = `thumbs_*\NNN.png`
- 已定稿基线文档：`docs/design/visual-polish-spec.md`、`ui/replica/*`（player-ui-v3、settings-ui-redesign、nav-capsule-v3 等）

---

## 一、REX 设计语言提炼（8 条可复用原则）

### 原则 1 · 海报自带美术字即标题，界面不再自绘标题文字
REX 详情页/首页**直接用海报里的片名美术字做视觉主角**，界面层只在海报下方放一行元信息，不额外盖一层自绘片名。这样字体永远和海报风格一致，也不会出现"两套字打架"。
佐证：`REX/IMG_0082`（OVERACTING 美术字嵌在海报内）、`REX/IMG_0110`（荒野丛林书法字）、`PLAYUI/IMG_0169`（Netflix 片库海报自带字）、`thumbs_rex/016`（奥德赛 logo）。

### 原则 2 · 详情页背景＝从海报主色提取的**饱和色场**，全页一色贯穿
REX 详情背景不是中性灰，而是取海报主色做**高饱和暖色垂直渐变**（玫红→砖红、粉紫、深棕），从状态栏贯通到页底，海报、按钮、卡片都压在同一色场上，观感通透、有电影氛围。
佐证：`REX/IMG_0082`（玫红→砖红，与海报完全同色系）、`thumbs_rex/008`（粉紫主题）、`thumbs_rex/016`（深棕主题）、`thumbs_rex/050`（墨绿主题）。**Emby 端同原则**：`REX-EMBY/IMG_0182`（时差一万公里：海报花卉主色→棕红渐变，与原生 REX 详情同一套色场逻辑）。

### 原则 3 · 信息密度极度克制：一行元信息 + 两行简介 + 一个评分，其余全不留
REX 详情页标题下方只有「类型 · 日期」一行小字 + 两行截断简介 + 一个小评分徽章，**没有勋章墙、没有 chip 行、没有参数卡**。分页点之后直接进入"继续观看"。
佐证：`REX/IMG_0082`（元信息/简介/IMDb6.0+5.4 一行结束）、`REX/IMG_0110`、`thumbs_rex/060`（无资源态也只一行说明 + 三圆钮）。

### 原则 4 · 主次操作成组：一个胶囊主按钮 + 一组等规格玻璃圆钮
播放是唯一的白底胶囊主按钮；收藏/更多/投屏等是一组**同尺寸、同材质、同间距**的圆形玻璃小钮（约 46pt、白 12% 描边、半透明模糊底），紧贴主按钮右侧排成一行。
佐证：`REX/IMG_0082`（播放胶囊 + 三枚玻璃圆钮）、`thumbs_rex/008`（「未找到相关资源」胶囊 + 三圆钮）、`thumbs_rex/060`。

### 原则 5 · 横向卡片"画面撑满"，圆角 8–12，进度细线叠在图上，绝不空框
REX 的"继续观看/剧集"横卡是 16:9 画面铺满卡片、圆角 8–12pt、底部一条细进度线 + 白色"看到 mm:ss"叠在画面上；无图时给**统一插画占位**而不是黑色/灰色空矩形。
佐证：`REX/IMG_0082`（两张继续观看均有实图 + 进度叠字）、`PLAYUI/IMG_0170`（剧集横条 125×52、当前集白描边 2pt）、`sheet_rex.png` 中大量 16:9 横卡。

### 原则 6 · 玻璃面板有严格材质规范，与其对应的锚点规则
黑 75% 毛玻璃 + 背景模糊 + 16pt 圆角；分隔线白 8% **从文本左缘起**；行标题 16 / 副标题 11–12；选中态白勾或白描边胶囊；面板右缘锚在触发 icon 右缘 +12pt。
佐证：`PLAYUI/IMG_0160`（字幕玻璃面板，分隔线与行层级清晰）、`PLAYUI/IMG_0171`（倍速列表）、基线文档 `player-ui-v3-rex-replication.md` §1.3/§1.4（实测值）。

### 原则 7 · 播放器控制条量化对齐：pitch 50 / 6pt 三段轨 / 13pt 等宽时间 / 66pt 左右边距
顶栏 8 枚图标中心间距 50pt、标题 16pt；底栏进度轨道 6pt 且分「已播白 / 缓冲灰 / 未播半透明」三段；时间 13pt 等宽数字；左右 ink 边距统一 66pt。图标一律**细线线性**、同线宽、同视觉重量。
佐证：`PLAYUI/IMG_0159`（控制条基准帧）、`PLAYUI/IMG_0170`、`PLAYUI/IMG_0173`；实测表见 `player-ui-v3-rex-replication.md` §1.1/§1.2。**Emby 端同源**：`REX-EMBY/IMG_0199`（同一 pitch50 顶栏、6pt 三段轨、白字 4K/选集/倍速；仅多一枚"4K HDR Vivid/AAC"资源浮标），证明 REX 全系播放器是同一套语言，我们的播放器应与之对齐。

### 原则 8 · 浅色页统一到"一种底 + 白卡 + 无框"，强调色单点使用
设置/资源库/我的等浅色页用同一浅灰紫底 + 纯白扁平卡（圆角 20）+ 彩色只在 chip/开关/链接单点出现，其余全中性；**不同功能页不换主色**，全系统像一个 App。
佐证：`sheet_rex.png` 第 8–9 行设置页、`thumbs_rex/060`（资源库双白卡）、`thumbs_rex/040`（搜索页中性色）；基线 `settings-ui-redesign.md` §二。**Emby 库页同款**：`REX-EMBY/thumbs_emby/001`/`008`（浅灰紫底 + 白卡 + "最近更新/我的媒体/播出平台"横滑分区，绿色菱形源标只出现在底部胶囊），与 `thumbs_rex/060` 一脉相承——"全系统一种风格"在竞品里是跨端成立的。

---

## 二、问题清单（20 条，按严重度排列）

| 编号 | 维度 | 严重度 | 现象描述 | 证据（我们 vs 竞品） | 可落地改进建议 | 预估影响面 |
|---|---|---|---|---|---|---|
| P0-01 | 视觉 | P0 | **海报美术字 + 自绘标题双重/三重叠加**：电影详情海报自带"魔鬼天使"美术字，界面又盖一层黑色"魔鬼天使"；AHS 海报自带 logo 又盖白色"美国恐怖故事"；犯罪都市海报自带片名字，界面先盖白字再盖一层黑字，同一屏出现三套字 | 我们 `ui-shot/detail-hero-fix-movie.png`、`detail-hero-fix-series-ahs.png`、`smb-movie-detail-final.png` vs `REX/IMG_0082`、`REX/IMG_0110`（海报字即标题，界面零叠加） | 统一策略：海报含可读片名 → **移除自绘标题**，只在海报下方放元信息；海报无片名（纯剧照）→ 才用**单套**白字 + 阴影，字号取 28–32、颜色跟随色场。删除重复的那层 `TitleLayer` | `ui/detail/DetailHeroContent.kt`、`HeroForeground.kt`、`DetailBackdrop.kt`、`ui/design/home/HomeTitleSheet.kt` |
| P0-02 | 视觉 | P0 | **详情背景是脏灰棕/米灰，不是海报取色**：电影页是灰米色、犯罪都市是灰褐色，饱和度低显"脏"，与海报的浓烈色彩割裂，正是用户否过的"粉紫不明/脏"同病 | 我们 `detail-hero-fix-movie.png`（米灰）、`smb-movie-detail-final.png`（灰褐）、`detail-hero-fix-series-ahs.png`（灰白） vs `REX/IMG_0082`（玫红→砖红，海报同色系）、`thumbs_rex/008`/`016`/`050` | 接入已有 `PosterPalette.kt` 的主色提取，详情页背景改为 `主色@高饱和 → 同色压暗` 垂直渐变，状态栏到页底一色贯穿；取色失败退化为海报暗化版，不用中性灰。参照 `visual-polish-spec.md` §一·1 | `ui/detail/DetailBackdrop.kt`、`DetailPage.kt`、`ui/design/PosterPalette.kt`、`ui/tokens/DesignPalette.kt`（PageGradient.detail） |
| P0-03 | 视觉/UX | P0 | **详情信息区勋章/chip 堆叠 + 大空白**：一排简介灰字、"展开简介"、独立一行的 TMDB 青绿小标、"播放资源：1080P" 胶囊、下面一张 SDR 大卡（SDR 30pt + 6.0GB + 7.82Mbps + NAS + 绿勾），节点过多且中段落差 100+px 空白；剧集页还多一张"共12集"卡 | 我们 `detail-hero-fix-movie.png`、`detail-hero-fix-series-ahs.png`、`detail-resource-card-closeup.png` vs `REX/IMG_0082`（元信息+两行简介+一个评分即结束）、`REX/IMG_0110` | 砍掉独立 TMDB 行：评分并入元信息行尾（"类型 · 日期 · ★6.0"）；资源卡收进折叠的"播放资源"行，默认只显示一行摘要，点开才展开 SDR/码率；删除"SDR 30pt 大字"，降为卡内 16pt 标签。消除 >48dp 空白带 | `ui/detail/DetailPage.kt`、`DetailSections.kt`、`DetailInfoCards.kt`、`DetailResource.kt`、`TechBadges.kt`、`RatingBadges.kt` |
| P0-04 | 视觉 | P0 | **继续观看/海报墙出现纯黑、纯灰空框**：首页第一张"继续观看"是纯深灰矩形无图无占位设计，另有"A Beautiful Mind"等纯黑卡；剧照页、相似作品也大面积空紫块。REX 同类位置全部有实图或统一插画占位 | 我们 `home-nas-library-20261118.png`、`home-multisource-v2-20261118.png`、`stills-check.png`、`aggregate-posters-20261118.png` vs `REX/IMG_0082`（两张继续观看均有画面+进度叠字）、`thumbs_rex/055`（剧照/相似作品均有图） | 空态不许是裸色块：加载失败/无图统一用 `TmdbPlaceholder` + 4% 白微光 + 居中细线播放/胶片 glyph（11–12sp），与 `visual-polish-spec.md` §一·3 一致；修复图片加载链路使已缓存项不空。海报墙空位不显示或降级为紧凑占位 | `ui/design/TmdbImage.kt`、`ui/design/home/HeroCard.kt`、`ui/design/home/sections/*`、`ui/detail/DetailRelatedVideos.kt`、`ui/components/MediaCard.kt` |
| P0-05 | 一致性 | P0 | **底部胶囊在详情页深色画面上标签发灰不可读、深浅态观感割裂**：详情/首页 hero 区胶囊是深灰底 + 黑灰图标标签（"发现/我的/资源库/设置"几乎看不清），而设置/资源库页是浅灰白 pill + 蓝选中；同一 App 两种导航语言 | 我们 `home-multisource-v2-20261118.png`、`detail-hero-fix-movie.png` vs `settings-apple-icons-20261118.png`、`lib-grouped.png`；竞品 `REX/IMG_0082`（胶囊在玫红场上通透、标签清晰）、`thumbs_rex/060` | 落地 `nav-capsule-v3.md` 已定稿的深态：`navPillDark #2C2C2E` + `navIdleDark #EBEBF5@66%` + 蓝选中，**图标标签必须浅色**；禁止"深灰底配黑字"的中间态。深态只用于深色 hero，其余页统一浅态，两态只在颜色上分叉、几何完全一致 | `ui/design/CapsuleNavBar.kt`、`DesignTokens.kt`（nav chrome）、`ui/design/DiscoverPage.kt` |
| P1-06 | 视觉 | P1 | 详情片名自绘字号失控：电影页黑字约 40sp 压在明亮海报上；剧集页白字 34sp；SMB 页白字 34sp + 黑字 32sp。同级不统一，且字号远超基线 26–28sp | 我们三张 detail-hero-fix vs `REX/IMG_0082`（海报字由海报决定，界面文字仅元信息 11–12） | 与 P0-01 合并处理：自绘片名唯一规格 30sp/700 + 文字阴影，且只在海报无字时出现；元信息 13sp、简介 14sp，严格三层字阶（`visual-polish-spec.md` §一·4） | `ui/detail/DetailHeroContent.kt`、`ui/design/home/HomeTitleSheet.kt` |
| P1-07 | 视觉 | P1 | 播放主按钮过大过白：宽约 260px、纯白实心，在深色详情里抢占全部视觉重心，压过海报；REX 的播放胶囊更小且与圆钮成组 | 我们 `detail-hero-fix-movie.png`、`detail-hero-fix-series-ahs.png`、`marks-apple2.png` vs `REX/IMG_0082`（胶囊与三圆钮同一行、等高等材质） | 主按钮高度与圆钮对齐（约 46pt）、宽度按文案自适应不铺满；白底降到 92% 或改玻璃白 20% + 白描边，与次要钮形成"亮一级"而非"白对灰"的强反差 | `ui/detail/DetailHeroContent.kt`、`DetailOverlayHost.kt`、`ui/detail/actions/*` |
| P1-08 | 一致性 | P1 | 三个圆形操作钮图标语言不统一：投屏是线条方盒、收藏是心形、更多是三点，描边粗细/视觉重量各异；"更多"弹出后是白底紫勾 chip 菜单，与全局蓝/绿语言冲突 | 我们 `detail-hero-fix-movie.png`、`marks-apple2.png`（淡紫 chip + 紫勾） vs `REX/IMG_0082`（三圆钮同材质同线宽）、`sheet_rex` 设置页（中性色） | 三个钮统一为 46pt 玻璃圆底 + 17pt/1.9 线性 glyph（复用 `SettingsIcons.kt` 线风格），"更多"菜单卡片改白底 + 蓝/绿语义色，禁用紫色；勾选用 `accentGreen` | `ui/detail/DetailOverlayHost.kt`、`ui/detail/actions/*`、`KivoBackPill.kt` |
| P1-09 | 一致性 | P1 | **弹窗/浮层至少三套语言并存**：详情三玻璃弹窗（更多/季/日历）、"更多/标记"白底大圆角+紫 chip 菜单、设置页扁平白卡、播放器黑玻璃面板。用户会逐个发现"每个页弹窗都不一样" | 我们 `marks-apple2.png`、`detail-hero-fix-series-69.png`、`settings-apple-icons-20261118.png`、`p240/player-ui/device/02-panel-speed.png` vs 竞品 `PLAYUI/IMG_0160` + `sheet_rex` 设置页（各自内部统一、跨页同一逻辑） | 定义全局 `OverlaySurface` 两种规格：① 内容浮层＝白卡 radius20 + 0.5dp hairline + 彩色 chip（浅色页）；② 媒体浮层＝黑 75% 玻璃 radius16（播放器/详情深色场）。所有弹窗必须二选一，删掉紫色变体（私有紫已定位 `DetailActionMenu.kt:27-28,90`：Accent `0xFF655978`/AccentWash `0xFFF3F0F8`/active chip `0xFFE7E1F0`，未进 `DesignPalette`，与 palette 唯一紫源 `bgLavender` 不同源，REX 全系无第四套强调色 ⇒ 收敛为中性或蓝） | `ui/detail/DetailMorePopup`、`DetailSeasonMenu.kt`、`DetailCalendarSheet.kt`、`DetailConfirmDialog.kt`、**`ui/detail/actions/DetailActionMenu.kt`**、`ui/design/settings/SettingsVisuals.kt`、`ui/compose/player/PlayerGlassSurface.kt` |
| P1-10 | 视觉 | P1 | 设置页四色高饱和 chip（蓝/紫/橙/青）过于鲜艳杂乱，与"极简一种风格"相悖；无图标版反而更干净 | 我们 `settings-apple-icons-20261118.png` vs `settings-rex-noicon-20261118.png` vs 竞品 `sheet_rex` 设置页（无图标或单色线性图标） | 收敛为**单色线性 glyph + 同色 14% 淡底 chip**，彩色只保留在开关/链接/存储条；或采用无图标版。以 `settings-ui-redesign.md` §二"彩色只在 chip/链接/开关/存储条"为上限 | `ui/design/settings/SettingsIcons.kt`、`SettingsRows.kt`、`SettingsVisuals.kt`、`SettingsEntryGrid.kt` |
| P1-11 | 一致性 | P1 | 在看页列表缩略图比例/形态混乱：有方形、有横向、有写着"假"字的占位，标题 17sp 截断成"..."，进度条位置随卡片变化 | 我们 `watching-synced.png`、`watching-multi-select.png` vs 竞品 `thumbs_rex/040`（列表卡统一"海报+标题+评分"结构） | 在看页缩略图统一 1:1 或 4:3 一种比例，圆角 12；占位统一 P0-04 规范；来源（NAS/Emby）与主标题分层为 13/11sp；进度条固定贴在缩略图底边 | `ui/design/WatchingPage.kt`、`WatchingSwipeRow.kt`、`WatchingIcons.kt` |
| P1-12 | 视觉 | P1 | 剧照/演职员/相似作品占位块过大且无处理：剧照两块淡紫空矩形约 260×120、演职员两个空圆、相似作品左侧大淡紫块；顶部"演职员"标题还被返回圆钮压住 | 我们 `stills-check.png`、`stills2.png` vs 竞品 `thumbs_rex/055`（有实图或紧凑占位，标题不与返回钮重叠） | 剧照占位改 16:9 小卡 + 统一 glyph；演职员空头像用姓名首字圆形占位；修复"演职员"标题与返回钮的 z 序/inset 冲突（≥16dp 间距） | `ui/detail/DetailPageSections.kt`、`DetailRelatedVideos.kt`、`ui/detail/DetailInfoCards.kt`、`ui/design/TmdbImage.kt` |
| P1-13 | 一致性 | P1 | 聚合搜索页**主色是绿色**（"返回/搜索/类型"、筛选"媒体库·最新更新·全部"均为 iOS 绿，评分黄绿角标），而全局导航是蓝色、开关才是绿色。全系统出现绿/蓝/紫/橙四套强调色 | 我们 `aggregate-posters-20261118.png`、`lang-en.png` vs 竞品 `thumbs_rex/040`（搜索页中性灰底 + 黑字）、`sheet_rex` 设置页 | 聚合搜索改为中性：返回/筛选灰黑字，选中项用 `navActive #0A84FF` 蓝下划线或胶囊；绿色仅保留给 toggle/成功态，全局强调色收敛到 1 主（蓝）+1 语义（绿） | `ui/AggregateActivity` 相关 compose、`ui/design/RatingBadges.kt`、`ui/components/SectionHeader.kt` |
| P1-14 | 一致性 | P1 | 海报卡角标语言不统一：聚合搜索每卡左上角深灰"电视剧"胶囊 + 右下角黄绿评分圆角块，而首页/详情海报无角标；同类"电视剧"信息在剧集页又是普通文字 | 我们 `aggregate-posters-20261118.png`、`lib-grouped.png` vs 竞品 `thumbs_rex/011`（Netflix 片库海报干净无角标，信息在卡下方） | 角标统一规范：类型/评分只在**网格卡**出现，用同一种 12sp 圆角胶囊（深色 60% 底 + 白字，评分可加★），信息层级放卡下方文字区；详情/hero 大卡不加角标 | `ui/design/RatingBadges.kt`、`ui/components/MediaCard.kt`、`ui/design/home/HeroCard.kt` |
| P1-15 | UX | P1 | 播放器顶栏标题显示文件名 "shelter.mkv" 而非片名，用户无法确认在看什么；REX 显示"悬崖 第2集-周二秘密会见孙悦剑" | 我们 `p240/player-ui/device/01-fullscreen-controls.png`、`p240/player-ui/v3.1/01-launch-bars.png` vs `PLAYUI/IMG_0160`、`PLAYUI/IMG_0170`（16pt 中文片名+集数） | 顶栏标题优先取媒体库片名+集数（已有 MediaMatch 匹配结果），取不到才退化为去扩展名的文件名；16pt 常规、超长省略 | `ui/compose/player/PlayerTopBar.kt`、`PlayerStatusRow.kt`、`PlayerComposeRoot.kt` |
| P1-16 | 视觉 | P1 | 播放器进度条是一根均匀灰线，缺"已播白 / 缓冲灰 / 未播半透明"三段；拖动预览浮窗在 SMB 源为空（虽已定设计但真机仍空框） | 我们 `p240/player-ui/device/01-fullscreen-controls.png`、`v3.1/10-drag-empty-d2.png`、`v3.1/13-drag-loading-reason.png` vs `PLAYUI/IMG_0159`（6pt 三段轨）、`PLAYUI/IMG_0169` | 按 `player-ui-v3-rex-replication.md` §1.2 落地 6pt 三段轨 + 无滑块 + 按下 10pt；预览浮窗按 v3.1 §2.5 完成 mpv 第二实例出帧（fetch 有帧）并在空态画原因小字 | `ui/compose/player/PlayerSeekTrack.kt`、`PlayerSeekPreview.kt`、`PlayerTokens.kt`、`thumbnail/MpvThumbnailReader.kt` |
| P1-17 | 一致性 | P1 | 播放器顶栏图标是自造实心/文字混排（实心方块投屏、"弹"字圈、实心 PiP），线宽与视觉重量和 REX 的细线线性图标不同；8 枚图标间距视觉不均 | 我们 `p240/player-ui/v3.1/01-launch-bars.png`、`device/04-topbar-icons-ours-3x.png` vs 竞品 `device/04-topbar-icons-rex-3x.png`、`PLAYUI/IMG_0170` | 复用 `PlayerGlyphFeature.kt` 全部改细线 1.9pt、22pt 盒；按实测 pitch 50pt 对齐；`PLAYUI/IMG_0170` 为最终参照，不改语义序 | `ui/compose/player/PlayerGlyphFeature.kt`、`PlayerGlyph.kt`、`PlayerTopBar.kt`、`PlayerTokens.kt` |
| P2-18 | 视觉 | P2 | 详情页右侧滚动条是明显的深灰粗竖条，破坏沉浸感 | 我们 `detail-hero-fix-movie.png`、`smb-movie-detail-final.png`（右侧竖条） vs 竞品 `REX/IMG_0082`（无可见滚动条） | 滚动条改为 3pt 白 40% 贴右内 6pt（同 `player-ui-v3` §1.3 面板滚动条规范），或滚动停 1s 后自动隐 | `ui/detail/DetailPage.kt`、`MovieDetailActivity.kt` |
| P2-19 | UX | P2 | 简介截断后固定显示"展开简介"，两行灰字与展开文案层级接近；REX 只做两行截断、无展开按钮，靠详情页整体滚动可读全文 | 我们 `detail-hero-fix-movie.png`、`detail-hero-fix-series-ahs.png` vs `REX/IMG_0082` | 遵循 P0-03 的信息克制：去掉"展开简介"独立行，简介两行截断即可；如需全文并入"更多信息"入口，与资源卡收编同一处 | `ui/detail/DetailHeroContent.kt`、`DetailPageSections.kt` |
| P2-20 | 一致性 | P2 | 分页点数量/尺寸不统一：首页 hero 9 个点、电影详情 6–8 个点、部分页无点；且点的间距/大小随页变化 | 我们 `home-multisource-v2-20261118.png`（9 点）、`detail-hero-fix-movie.png`、`REX/IMG_0082`（8 点）对照 | 定义全局 pager 指示：超过 8 项最多显示 8 点，点径/间距固定（参考 REX 6pt 点 + 12pt 距），当前点高亮放大 1.2×，统一组件复用 | `ui/design/home/HeroCard.kt`、`ui/design/HeroCarousel.kt`、`ui/detail/DetailPage.kt` |

**合计 20 条：P0 = 5，P1 = 12，P2 = 3。维度分布：视觉 10、一致性 7、UX 3。**

---

## 三、优先级路线图（P0 → P1 → P2）

### 第一波 · P0（"最刺眼、一开屏就看到"的问题，先做，一次改完）
1. **P0-01 + P1-06 详情标题去重**：先解决"海报字 + 自绘字"双重叠加。这是所有详情页第一步，改动集中在 `DetailHeroContent.kt`/`HeroForeground.kt`，风险最低、观感提升最大。
2. **P0-02 详情背景取色**：接 `PosterPalette`，把灰棕/米灰换成海报主色饱和渐变。与 1 同一批做（都在详情 hero），一次真机取证。
3. **P0-03 详情信息减肥**：评分并入元信息行、资源卡折叠、删 SDR 大字。与 1/2 同批，详情 hero 一次成型。
4. **P0-04 空图占位统一**：`TmdbPlaceholder` 全网替换裸色块 + 修缓存加载链路。涉及首页/在看/详情/聚合多个页，单独一批并做回归。
5. **P0-05 导航深态修正**：`CapsuleNavBar` 深态标签改浅色，消除"深灰底黑字"。几何不动，只改颜色，最后做且必须五页截图确认。

> 第一波验收：详情电影/剧集/SMB 三页 + 首页 + 设置页各出 before/after 并排图，对照 `visual-polish-spec.md` §三 六问逐条打勾。

### 第二波 · P1（"用一阵才会 noticing"的问题，按页成组）
- **P1-07/08/09 详情操作组 + 弹窗语言**：和 P0 详情批衔接，把播放按钮、圆钮、更多菜单统一到 `OverlaySurface` 两规格。
- **P1-10 设置页 chip 收敛**：单色线性图标，与 `settings-ui-redesign.md` 对齐。
- **P1-11/12 在看页 + 剧照页占位**：与 P0-04 同规范延伸。
- **P1-13/14 聚合搜索去绿 + 角标统一**：整页改中性主色，收敛强调色到蓝+绿。
- **P1-15/16/17 播放器**：标题显示片名、三段轨、图标线性化。播放器已有 v3 像素基线，**只做这三处，不动 pitch50/锚点表**。

### 第三波 · P2（打磨细节）
- P2-18 滚动条、P2-19 展开简介、P2-20 分页点统一组件。

### 路线图原则
- **先"同页统一"，再"跨页统一"**：详情 hero 一屏内先把字/按钮/色场理顺（P0），再谈全系统语言（P1）。
- **一次改一族，避免反复惊动用户**：用户明确"不要逐个发现问题"，因此每波必须是**成组、成页**交付，不留"这次改一半、下次再改"的中间态。
- **每波必出并排图**：`docs/evidence/design-audit/cmp/` 下存 before/after，未并排不宣布完成。

---

## 四、一致性检查（"同一种东西多种样式"清单 + 统一方案）

| # | 不一致项 | 当前多种样式 | 统一方案 |
|---|---|---|---|
| C1 | **详情片名** | 海报自带美术字（REX 式） / 自绘黑字 / 自绘白字 / 自绘蓝字（首页 hero）四种 | 三选一规则：海报有字→不画；海报无字→一套 30sp/700 白字+阴影；首页 hero 同规则，禁用蓝字压图 |
| C2 | **详情背景色场** | 海报主色饱和渐变（REX） / 中性灰棕 / 米灰 / 灰白 | 全局接入 `PosterPalette.kt`：详情背景一律"海报主色→压暗"垂直渐变；失败退化海报暗化，不用中性灰 |
| C3 | **弹窗/浮层** | 详情玻璃弹窗 / 白底紫 chip 菜单 / 设置扁平白卡 / 播放器黑玻璃 | 全局两种规格：`OverlaySurface.light`（白卡 r20+hairline）与 `OverlaySurface.media`（黑 75% 玻璃 r16）。所有浮层强制二选一，**紫色变体删除** |
| C4 | **底部导航** | 浅灰白 pill + 蓝选中（设置/资源库/我的） / 深灰底黑字（详情/首页 hero） / 独立搜索深圆钮 | 严格执行 `nav-capsule-v3.md`：浅态/深态仅颜色分叉，几何一致；深态图标标签必须浅色；搜索钮与胶囊同色同投影 |
| C5 | **海报卡** | 无角标（首页/hero） / 深灰"电视剧"胶囊 + 黄绿评分（聚合） / 纯色空框（无图） | 角标只在网格卡出现、统一 12sp 深色胶囊+白字+★；空图统一 `TmdbPlaceholder`；hero/详情大卡不加角标 |
| C6 | **评分徽章** | TMDB 青绿矩形 / 聚合黄绿圆角块 / 播放器无 / REX 用 IMDb 小字 | 统一为一枚 12sp 圆角胶囊：深色 60% 底 + 白字 + ★，来源（TMDB/IMDb）用 10sp 灰字附后 |
| C7 | **播放/主操作按钮** | 详情超大纯白胶囊 / 设置页文字行 / 播放器文字按钮 | 定义 `PrimaryAction` 组件：高度 46pt、白底 92% 或玻璃白 20%+描边、宽度随文案；全 App 主操作统一调用 |
| C8 | **图标语言** | 详情线性细笔 / 设置彩色 chip / 播放器自造实心+文字圈 / 导航单色 | 一套 1.9pt 线性 glyph 库（复用 `SettingsIcons.kt` 风格），彩色只在 chip 底；播放器 `PlayerGlyphFeature` 共用同一线宽规范 |
| C9 | **强调色** | 蓝（导航选中） / 绿（TMDB、开关、搜索） / 紫（更多菜单勾） / 橙（设置 chip） / 青（TMDB 徽章） | 收敛为 **1 主色蓝 `#0A84FF` + 1 语义色绿 `#34C759`**，其余功能色一律用灰阶；紫色、青色、橙色从 UI 中移除。**紫色具体落点已定位**：`ui/detail/actions/DetailActionMenu.kt:27-28,90` 私有三色（Accent `0xFF655978` / AccentWash `0xFFF3F0F8` / active chip `0xFFE7E1F0`），未登记进 `DesignPalette`、且与 palette 唯一紫源 `bgLavender`（仅背景渐变三色之一）不同源。REX 全系（含 Emby）无第四套强调色 ⇒ 该私有紫**不合规，应收敛进 palette 语义色或改中性/蓝**，不得照现状保留 |

**统一落点**：在 `ui/design/` 下建立唯一来源组件集（`OverlaySurface` / `PrimaryAction` / `PagerDots` / `PosterPlaceholder` / `RatingBadge` + 线性图标库），页面只允许调用、不允许自绘。这正是"全系统一种风格、不让用户逐个发现问题"的工程保证——**风格收敛到组件，而不是靠人工逐页检查**。

**配套门禁（与架构师共识）**：上述"组件只准读 `DesignTokens`/`DesignPalette`"必须从口头约定固化为检查——给 `ui_gate` 增加一条规则：**`ui/design/` 与 `ui/components/` 下的组件文件不得出现 `Color(0x...)` 字面量**。否则新增组件极易再写裸色，改完即回退。这条门禁与架构师报告的令牌收敛（rank2）和 `KivoActionPill` 提取（rank8）是同一件事，建议一起进 Same 批次。

---

## 五、根因交叉印证（与架构师代码审计联动）

架构师 `docs/evidence/design-audit/arch-maintainability.md` 的量化证据，解释了本报告多条视觉问题为何反复出现——**视觉不一致的根因是令牌/组件没有成为唯一来源**：

| 代码侧证据 | 印证的视觉问题 | 结论 |
|---|---|---|
| `DesignTokens.kt:54-55` 的 `pageGradient` 注释明确"当前无任何调用点"；`fontStat/fontSectionTitle/elevationCard/navHeight` 各仅引用 1 次 | **P0-02**（详情背景没取海报色）、**P1-06**（字号失控） | 令牌已存在但页面没接 → 不是没设计，是**没落地**。P0-02 修法是接 `pageGradient.detail`/`PosterPalette`，不是重新设计 |
| UI 目录裸色值 176 处（全项目 316）；Top3：`PlayerTokens.kt`(25)、`SettingsVisuals.kt`(14)、`DetailActionMenu.kt`(10) | **P0-05**（导航深态）、**P1-10**（设置四色 chip）、**C9**（四套强调色） | 裸色散在三个高频视觉文件 → 改色必漏。统一方案必须先收口到 `DesignPalette` |
| `PlayerTokens.textSecondary(0xFF8A8A8E)` 与 `DesignTokens.textSub` 同值；`SettingsVisuals.blue/green` 与 `DesignPalette.navActive/accentGreen` 同值 | **C3**（弹窗三套语言）、**C9**（强调色） | 两套令牌各维护一份同值色 → 这是"同一种东西多种样式"的最直接来源 |
| `RoundedCornerShape` 出现 16 种值（20dp×5、16dp×4、12dp×4、50dp×3…），令牌只有 `radiusCard=13`/`radiusPoster=12` | **C3**（浮层形态）、**C5**（海报卡角标）、**P1-09** | 圆角无统一 → 同类卡片观感不同。需把 16 种收敛到 3 种（20 卡 / 12 图 / 16 玻璃） |
| `fontSize` 字面量硬编码 294 处 | **P1-06**（片名字号 32/34/40 混用）、**P2-20**（分页点） | 字阶三层规范写在文档里但没进代码 → 用 `DesignTokens.font*` 替换字面量 |
| 4 个主 Page（Discover/Watching/Hero/Detail）无 `@Preview`；全项目 374 Composable 仅 44 Preview（11.8%） | **为何这些视觉问题长期没被发现** | 主页面无预览 = 视觉回归无网。每个主 Page 至少补 1 个 `@Preview`，才能让"全系统一种风格"可检查 |

> 结论：本报告 §四 建议建的 `ui/design/` 唯一来源组件集，与架构师的令牌收口是**同一件事的两个面**——视觉问题不能只靠改截图，必须让令牌/组件成为页面唯一可调用入口，否则改完还会回退。
> 另：历史坑当前均正确（`activeId` 已 `mutableStateOf`；`WatchingSwipeRow.kt:96`、`DetailResource.kt:85` 有 `matchParentSize`/非 `IntrinsicSize` 防坑注释），本报告涉及的视觉改动**勿回退这些修复**。

---

## 六、审计边界与披露
- 本轮只审**视觉/UX/一致性**，不修改任何代码；代码可维护性与交互逻辑缺陷由架构师、QA 另行审计。
- REX 量化值（pitch50/6pt 轨/16pt 圆角等）来自已定稿 `player-ui-v3-rex-replication.md` 的像素实测，本报告直接引用，未重复测量。
- 深态导航、深色模式色值在 `DesignPalette.Dark` 中多处标 `◐ 推断，待真机取色`；P0-05 落地时需同步真机取色，不得凭推断上线。
- **Emby 审查看法已更新**：`sheet_emby.png` 拼版渲染异常（只有 2 张），已按 team-lead 提示改用 `REX-EMBY/` 原图 23 张 + `thumbs_emby/` 逐张复核，结论：REX-EMBY 的**详情页色场、播放器控制条、库页浅色卡片**与原生 REX 完全同源（见原则 2/7/8 佐证），无独立的第四套语言。因此本报告不再把 Emby 列为"待补证"，其设计原则可直接纳入统一方案。
- Emby 播放器有一处 REX 原生没有的元素：左上"4K HDR Vivid / AAC"资源浮标（`REX-EMBY/IMG_0199`）。这佐证 P0-03 的建议——**资源信息应放在播放器浮标/折叠行，而不是详情页展开成 SDR 大卡**。
