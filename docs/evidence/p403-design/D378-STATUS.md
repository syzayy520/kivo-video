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

## 五、诚实边界

- **SMB 详情页暂无 TMDB 增强**（简介/评分/演员为空）：本地文件详情也是同样取舍（等一次搜索会把首帧拖到 8–15s）。
  要做的话应该是"先出页、后台补"，属下一轮。
- **集缩略图显示"暂无缩略图"**：SMB 无缩略图接口，占位文字是既有季集 UI 的行为，未改 UI。
- 语言只验了中文 ↔ 英文；其它语言路径未逐个验（逻辑同一分支，风险低）。
- 平板未验（无设备在线）。
- **跨季合并靠"剧名"**：若同一父目录下真有两部同名不同剧，会被并成一部（NAS 上罕见）。
