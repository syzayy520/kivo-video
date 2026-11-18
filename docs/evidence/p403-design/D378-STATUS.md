# D378 本地片库展示改造：点海报进详情 + 剧集聚合 + 标题清洗 + 片名跟随系统语言

## 一、用户报的问题（4 条）

1. 首页点本地片库海报**直接起播**，看不到详情/选集；
2. 剧集是"**一集一张卡**"（一部 47 集的剧把整行刷满）；
3. 补充：**所有分类**（电影/剧集/综艺/动漫/演唱会/纪录片）都要修，不能只修剧集；
4. 补充：很多片名是英文的，要求**片名跟随系统语言**。

## 二、根因

| 现象 | 根因 |
|---|---|
| 点海报直接播 | `MainActivity.openHomeEntry` 的 `Kind.Smb` 分支直接 `startActivity(PlayerComposeActivity)`；且详情页**只有** TMDB/本地文件/Emby 三个入口，没有 SMB |
| 一集一张卡 | `NasLibrarySource.entries()` 是 `videos.map{}` —— 按**文件**粒度映射，索引里一个文件 = 一张卡 |
| 只修剧集不够 | 聚合必须与分类解耦（`sections()` 遍历全部 6 个分类） |
| 片名英文 | `zh-CN` 写死在 4 个文件 10+ 处（TmdbHome/TmdbDetail/TmdbClient/TmdbGenres/TmdbPeople/TmdbProviders/TmdbStudios） |

## 三、改动

### 1. 剧集聚合（两级分组，分类无关）

新增 `library/NasSeries.kt`（模型）+ `library/NasLibraryGrouping.kt`（算法）：

- **第一级按目录**：NAS 上一剧一目录（或一季一目录）、一集一文件，目录是最可靠的自然边界；
- **第二级按剧名跨目录合并**：多季剧是 `剧集/某剧.S01.../`、`S02.../` 多个目录，
  只按目录聚合会出现 4 张同名卡（真机实测 `American Horror Story` S01–S04）。
  键 = `(源, 共享, 父目录, 剧名)`，季归到 `NasSeries.seasons`；
- **算"剧"的下限**：目录里能解析出集号的文件 **≥ 2**。这条是必要的——
  电影目录的"正片+花絮"没有集号 ⇒ 保持单片，不会被并成"2 集的剧"；
- **集号识别覆盖四类命名**：中文 `第01集/第3话/第12期` → latin `S01E01/E05/EP01` → 日期 `20240101`（综艺）；
  认不出**显式返回 null**，绝不伪造 index=1。

`NasLibrarySource` 改为消费聚合结果；卡片副标题：单季 `共 6 集`，多季 `4 季 · 共 51 集`。

### 2. 片库卡片 → 详情页

- 新增 `ui/detail/SmbDetail.kt`：SMB 单片/剧集 → `DetailContent`（第四个来源，与 TMDB/本地文件/Emby 并列）；
- `MovieDetailActivity` 加 SMB 入口（`EXTRA_SMB_*`），复用既有 `DetailPage` 的季集节
  （`EmbyShows.Season/Episode` 是通用数据类，不是 Emby 专属 ⇒ 零新 UI）；
- 点击分流：`DiscoverPage` 新增 `onOpenNasItem`，只有**片库分类行**走详情页；
  **「继续观看」仍保持"接着看"直接播**（那是它的语义，不能改）；
- 详情页海报复用首页同一套 `resolvePoster`（库内图 → TMDB → 空），避免"首页有图、点进去反而空"。

### 3. 标题清洗

`MediaTitleParser.TAGS` 补：`AMZN / Hulu / DSNP / HMAX / ATVP / PCOK / iTunes / CHDWEB / CHD`、
`h 264 / h 265`（带空格写法）、声道 `5.1 / 2.0`、`DDP`。

真机效果：
- `Cold Case Gleen AMZN DDP 2 0 H 264 CHDWEB` → **`Cold Case Gleen`**
- `A Murder at the End of the World ... Hulu DDP 5 1 H 265 CHDWEB` → **`A Murder at the End of the World Homme Fatal`**

### 4. 片名跟随系统语言

新增 `tmdb/TmdbLocale.kt`，替换全部写死的 `zh-CN`：

- 中文区分地区（`zh-CN`/`zh-TW`/`zh-HK`），其它语言用两字母码（`en`/`ja`/`ko`…）——
  硬凑 `en-GB` 这类组合时 TMDB 可能返回空标题；
- **标题兜底**：目标语言无译文时按 `original_title` / `original_name` 回落，
  否则英文系统下会出现空白卡片；
- 验证开关：`adb shell setprop debug.kivo.tmdbLang en-US`（读 `android.os.SystemProperties`，
  不是 JVM 的 `System.getProperty` —— 两者是两套东西）。

## 四、真机验证（K40 b6e950ee）

### 聚合（首页）
| 观察点 | 改造前 | 改造后 |
|---|---|---|
| `69两头勾` | 4 张重复卡 | **1 张 · 共 6 集** |
| `American Horror Story` | 4 张同名（12/13/13/13） | **1 张 · 4 季 · 共 51 集** |
| `A Murder at the End of the World` | 2 集刷 2 张 | **1 张 · 共 2 集** |

### 详情 + 选集
- 点剧卡 → `MovieDetailActivity`：标题、2023、播放按钮、`第1季·1集`、季选择、`共6集`、集列表（S01E01/E02…）；
- 点第 2 集 → 播放器控制栏显示 **`S01E02` / 1080P**（证明播的是点的那一集，不是第 1 集）；
- 点 `第 1 季` → 菜单列出 **第 1/2/3/4 季**；选第 4 季 → `共13集`、集列表 `S04E01/E02`；
- 点电影卡（犯罪都市4）→ 详情页：标题、2024、播放按钮（不再是"暂无可用资源"）。

### 语言
| 覆盖 | 结果 |
|---|---|
| 默认（中文系统） | 奥德赛 / 美国好声音 / 辛普森一家 / 实习医生格蕾（中文） |
| `setprop debug.kivo.tmdbLang en-US` | The Odyssey / The Voice / The Simpsons / Grey's Anatomy / The Hunger Games…（全英文）|
| 清空属性后 | 恢复中文 |

### 回归
| 项 | 结果 |
|---|---|
| `assembleDebug` | BUILD SUCCESSFUL |
| 单测 `--rerun-tasks` | **126 例 / 0 失败**（新增 8 例聚合 + 3 例解析） |
| `tools/ui_gate.py` | PASS（MainActivityShell 压回 300 行、DiscoverPage 压回 300） |

## 五、补：SMB 详情的 TMDB 增强（不阻塞首屏）

首版 SMB 详情只有文件事实（标题/年份/播放/选集），没有简介/评分。补上增强，关键是**不能被网络拖住**：

- `SmbDetail.enrich()`：`searchId`（片名+年份）→ `TmdbDetail.details`，两次请求都有 12h 磁盘缓存；
- **硬预算 2.5s**：`Future.get(2500ms)` 包住，超时直接放弃增强、退回文件事实
  （`TmdbClient` 自身 connect/read 是 8s/15s，不包预算会把详情页拖十几秒 —— D376 的教训）；
- 首屏本来就有 `DetailPreview` 先渲染标题，所以"先出页、后补详情"是天然成立的；
- 偏好类型先搜剧/影，搜不到再换另一种（NAS 分类目录名不一定准）。

真机结果（同一台 K40，均 1s 内完成增强）：

| 样本 | 增强后 |
|---|---|
| American Horror Story（剧） | `2011年10月5日 · 美国 · 145集` + 中文简介 + TMDB **8.1** |
| 犯罪都市4（电影） | `2024年4月24日 · 韩国 · 1小时50分钟` + 中文简介 + TMDB **7.2** + 播放资源 **6.0 GB / 7.82Mbps** |

码率用「文件体积 ÷ TMDB 时长」估算；算不出（缺时长）给 0，不伪造。

## 六、诚实边界

- **SMB 详情的增强有 2.5s 硬预算**：超时只显示文件事实（标题/年份/播放/选集），不显示简介/评分。
- **"文件名 → TMDB 搜索"有误配风险**：同名不同片时可能显示另一部片的简介。本地文件详情也是同一机制。
- **集缩略图显示"暂无缩略图"**：SMB 无缩略图接口，占位文字是既有季集 UI 的行为，未改 UI。
- 语言只验了中文 ↔ 英文；其它语言路径未逐个验（逻辑同一分支，风险低）。
- 平板未验（无设备在线）。
- **跨季合并靠"剧名"**：若同一父目录下真有两部同名不同剧，会被并成一部（NAS 上罕见）。

## 七、补二：片名重复与风格统一（用户反馈"一个白色一个 TMDB 的"）

### 现象
SMB 详情页 hero 上出现**两个片名**：橙色 TMDB logo + 海报底部烧死的艺术字，叠成重影。

### 根因
- `TitleLayer` 在 TMDB 有 logo 时显示 logo（橙色艺术字）；
- 幕布图用的是**竖版海报**，而竖版海报本身就把片名"烧"在底部 ⇒ 两者叠在一起。
- Emby 详情之所以干净，是因为它的幕布是**无字剧照**，不是海报。

### 修复（不改任何 UI 布局）
1. **回滚"backdrop 特例"**：我一度想把 SMB 的幕布换成横版剧照（无字），但 TMDB 图片 CDN 在国内不稳，
   实测直接变成"封面暂不可用"，比重复更糟；且那是给 SMB 开特例，违背"全系统一种风格"。
2. **改为裁掉幕布底部 15%**（`Bitmap.trimTitleBand()`）：只裁不缩放，去掉海报上烧死的片名，
   保留人物构图。SMB 与 TMDB/Emby 从此走**完全相同的** `DetailBackdrop` + `TitleLayer` 路径。
3. 幕布图源仍是"库内图 → TMDB → 空"，与首页卡片同一套。

### 结果（真机 K40）
`smb-detail-trimmed.png`：hero 显示人物画面，**片名只出现一次**（TMDB 的橙色 logo），
下方日期/播放/简介/TMDB 7.2 版式与其它详情页一致。

## 八、补三：片库片名本地化（中文系统显示中文片名）

### 问题
用户反馈"很多电影名还是英文的，我手机系统是中文"。此前的 `TmdbLocale` 只解决了 **TMDB 行**（正在热映/本周趋势），
**本地片库完全没被覆盖** —— 片库标题来自 `MediaTitleParser` 解析文件名，文件名是英文就是英文，与系统语言无关。

### 修复
新增 `library/NasTitleStore`（内存 + 磁盘缓存）与 `ui/design/home/sections/NasDisplayTitle`：

- **数据**：`TmdbClient.localizedTitle(片名, 年份)` → 走 `searchFirst`（与海报兜底共用 12h 磁盘缓存），
  先按电影搜、搜不到再按剧集搜（片库目录名不一定准）；
- **渲染**：`rememberNasTitle` 首帧同步读缓存（命中即中文），未命中丢 IO 线程解析后重组；
- **限流**：`Semaphore(3)`，首页一次可见几十张卡，不限流会打爆 TMDB 配额；
- **不空转**：换不到中文就保持原文件名，绝不显示空白；
- **缓存落盘** `files/nas/nas-titles.json`，第二次冷启动起全部命中，不再发网络。

覆盖两处标题：片库分类卡（`NasLibraryCard`）与「继续观看」（`ContinueRow`）。Emby 条目不参与（服务端已按用户语言返回）。

### 真机结果（中文系统）
| 原名 | 显示 |
|---|---|
| A Murder at the End of the World H | **世界尽头的一场谋杀** |
| American Horror Story | **美国恐怖故事** |
| BANSHEE | **丧妖集聚** |
| Caged | **笼困** |
| Dangerous Encounters of the First Kind | **第一类型危险** |
| 69两头勾 6ixtynin9 The Series | **69两头勾** |
| Cold Case Gleen | 保持原名（TMDB 无中文译名，属数据限制，不编造） |

### 边界
- TMDB 没有中文译名的条目（冷门片）仍显示原名 —— 这是数据现实，**不伪造翻译**。
- 首次冷启动每张新卡会发一次搜索（有 12h 缓存与 3 并发限制），表现为标题在 1–2s 内由英文变为中文。

## 九、详情页区块全面自检（用户要求"不要一个个发现"）

### 真 bug（已修）
**所有演员头像是同一张图 / 剧照全灰 / 相似海报灰块** —— 三个症状同一个根因：

`DetailContent.loadImage` 是**一个**回调，同时服务幕布（页级大图）、演员头像、剧照、相似海报。
我把它写成"忽略 URL 一律返回本地海报"，于是所有小图都拿到了同一张。

修法：给幕布一个专属 scheme（`kivo-nas-hero://`），`loadImage` 只认它；
其它 URL 返回 `null`，交回各自的默认加载器，各取各图。

附带：剧照从 `w780` 降到 `w500`（卡宽仅 220dp，w780 收益为零且真机缓存零新增 = 完全没下下来）。

### 逐区块核对（SMB 电影详情）
| 区块 | 状态 |
|---|---|
| 幕布图 + 片名 logo | ✅ 正常，只有一个片名 |
| meta（日期·地区·时长）| ✅ |
| 播放 / 收藏 / 更多 | ✅ 渲染（SMB 无 TMDB id ⇒ 收藏为禁用态）|
| 简介 + 展开 | ✅ |
| TMDB 评分 | ✅ |
| 播放资源（体积 / 码率）| ✅ 6.0GB / 7.82Mbps |
| 演职员（11 人 + 导演）| ✅ 头像各异（本次修复）|
| 家长指南（分级 / 内容提示）| ✅ |
| 分类 | ✅ |
| 剧照 | ⚠️ w500 后开始出图，首张仍可能慢（TMDB 图 CDN 国内不稳）|
| 相似作品 | ✅ |
| 出品方 logo | ⚠️ 数据有（studioLogo 非空）但页面未见，疑图片加载失败，待查 |
| 预告片 | — 该片 TMDB 无中文预告（trailerKey 空），不显示属正常 |

### 未修 / 待定
- **出品方 logo 不显示**：`studioLogo` 有值但未渲染出图，下次单独查（不影响主信息）。
- **剧照仍可能慢**：TMDB 图 CDN 波动，属网络，不是逻辑。
- **收藏/待看对 SMB 禁用**：需要"无 TMDB id 的本地收藏"，属功能缺口，未做。

## 十、SMB 片源「收藏 / 已看」可用（补自检清单里的功能缺口）

### 问题
本地片源的收藏按钮一直是**禁用态**：收藏体系按 TMDB id 记（`FilmKey(id, isTv)`），
而 SMB 片源没有 id —— 但详情页的 TMDB 增强**明明已经搜到了 id**，用完就丢了。

### 修复
新增 `library/NasTmdbIdStore`（内存 + `files/nas/nas-tmdb-ids.json`）：
- `SmbDetail.enrich` 解析到 id 时顺手落盘（key = `片名|年份`）；
- `MovieDetailActivity` 在 NAS 分支同步读这个缓存，有 id 就构造 `FilmKey` 启用收藏/已看。

### 真机结果
- 首次进入：仍是禁用态（那一刻确实还不知道 id，**不假装可用**）；
- 退出重进：收藏按钮变为可用，点开出现「影片标记 · 待看 / 已观看」面板；
- 点「待看」后落盘：`"movie:1017163":{"watchlist":true,...}`（1017163 = 犯罪都市4）。

### 边界
- **首次进入不能收藏**：id 要等一次详情解析才有。这是诚实上限，不是 bug。
- 本地文件（手机内视频）仍无 id，保持不变（没有 TMDB 匹配就没有 id）。

## 十一、影片标记菜单：底部弹层 → 锚点弹出 + Apple 质感

### 用户诉求
1. "不要压在最底下，哪里点击往哪里展开"；
2. "质感往 Apple 上做"。

### 改动
- **形态**：`ModalBottomSheet`（底部弹层）→ `DropdownMenu`（锚点弹出），
  挂在收藏按钮的 Box 内，`DropdownMenu` 自动锚定父布局 ⇒ 点图标就从图标下方展开。
  旧文件 `DetailActionSheet.kt` 已删除（不保留被替代实现）。
- **质感**（按 iOS popover 口径）：
  - 近白底 `#FCFCFE` + 0.5dp 白色高光边框 + 22dp 大扩散阴影 + 20dp 圆角；
  - 系统灰阶层级：标签 `#1C1C1E`、次要 `#8E8E93`、分隔 `#3C3C43@8%`；
  - 图标 32dp 圆角砖（选中为淡紫底），标题 15sp Medium / 副标题 11.5sp，字距 -0.2；
  - 选中态 = 淡紫行底 + 右侧紫色对勾；**未选不画任何符号**（iOS 习惯，去掉了原来的"+"）；
  - 与按钮留 6dp 气口。

### 真机
- 点收藏图标 → 菜单在图标正下方展开，紧凑卡片，不再占满下半屏；
- 点「已观看」→ 落盘 `movie:1017163 watched:true`；菜单随即关闭。

### 顺带修
`DetailActions.toggle` 之前在某些状态下**静默 return**（点了没反应且无日志）。
现在被跳过时留一条 `[marks] toggle skipped ... ready=/saving=` 证据，避免再次成为取证黑洞。

### 诚实说明
不是真·毛玻璃（Popup 内模糊背景需要捕获下层内容，Compose 跨窗口做不到）。
用的是"近白不透明 + 高光边 + 柔和阴影"的近似方案；若要做真毛玻璃需换自定义 Popup + RenderEffect，风险与收益不成正比。
