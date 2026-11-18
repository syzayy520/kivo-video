# 首页多源内容聚合 · 规划设计

状态：**规划稿（待裁决，未写代码）**
范围：本地文件 / NAS(SMB) / Emby 三类源的内容进首页（最近播放 + NAS 片库）
关联：`docs/design/playback/274-startup-latency.md` §8（首屏数据编排）、`library/ContinueWatching.kt`

---

## 0. 一句话

首页现在只有「本地 MediaStore 续播」一条本地线，**Emby 与 NAS 的内容进不了首页**。本规划要把「最近播放」打通三源，并新增「NAS 片库自动识别」模块。

---

## 1. 用户需求原文拆解

> "先把无网络环境本地源信息做了，比如 NAS 影片自动识别到首页显示的模块也要规划设计，最近播放也要同步，Emby 这些最近播放也需要同步到首页"

| # | 需求 | 落点 |
|---|---|---|
| R1 | NAS 影片**自动识别**到首页显示模块 | 新增 NAS 扫描 + 片库模块（P3） |
| R2 | **最近播放**同步 | ContinueRow 支持本地 + SMB（P2） |
| R3 | **Emby 最近播放**同步到首页 | EmbyClient 拉续播列表 + 统一模型（P1） |
| R4 | "无网络环境"可用 | 海报/缩略图不依赖公网 TMDB（见 §4-D2） |

---

## 2. 现状盘点（已逐行核对源码）

| 能力 | 真实现状 | 文件 |
|---|---|---|
| 首页结构 | `DiscoverPage` 几乎全 **TMDB 驱动**（趋势/综艺/动漫/高分/人物/平台/分类/公司），仅 `ContinueRow` 是本地续播 | `ui/design/DiscoverPage.kt`（296 行） |
| 续播对齐 | **只对齐本地 MediaStore path**，注释明确"网络源 key（Emby stream?…）永远无法对齐，不会进入 UI" | `library/ContinueWatching.kt` |
| 续播存储 | `WatchStore`，key = **路径字符串**，存 position/duration/updatedAt，`recentKeys(limit)` | `stats/WatchStore.kt` |
| 本地片源 | `LocalVideoSource.query()` **只查 MediaStore**（本机视频），不含 SMB | `library/LocalVideoSource.kt` |
| 片源模型 | `MediaItem(id,title,path,durationMs,width,height,addedSec)` — **无 poster、无 source 类型** | `library/MediaItem.kt` |
| Emby 列表 | `items(auth,start,search,sort,limit,parentId,types,genres,years,maxPremiereDate,rich)` — 支持 `SortBy∈{DateCreated,SortName,ProductionYear,CommunityRating,PremiereDate}`；`rich` 含 `UserData`(PlayedPercentage/PositionTicks)；**无 `DatePlayed` / `IsResumable`** | `emby/EmbyClient.kt:80` |
| Emby UserData | 详情单请求已随带 UserData（续播点） | `emby/EmbyClient.kt:237/303` |
| SMB 目录 | `SmbSession.list(subPath)` → `nativeList(ctx,path)` **可列任意目录** ⇒ 递归扫描可行 | `smb/SmbSession.kt:44`、`smb/Smb2Native.kt:38` |
| SMB 共享 | `SmbShareLister.list()` 只列 share 根 | `smb/SmbShareLister.kt` |

**结论**：底层料齐了（SMB 能列目录、Emby 有 UserData），缺的是**统一模型 + 聚合层 + 三个取数器 + NAS 索引**。

---

## 3. 差距

| 需求 | 缺口 |
|---|---|
| R3 Emby 续播 | EmbyClient 不支持按"可续播/最近播放"筛选 → 需加 `Filters=IsResumable` + `SortBy=DatePlayed`（Emby 标准 API，需真机 4.8 验） |
| R2 最近播放 | `ContinueWatching` 只吃本地 path，`smb://` 与 `emby:` key 被丢弃 → 需统一 key 协议 |
| R1 NAS 片库 | 无递归扫描器、无视频识别、无索引存储、无首页模块 → 全新子系统 |
| 跨源展示 | `MediaItem` 无 poster/source，`ContinueRow` 只吃 `MediaItem` → 需统一条目模型 |

---

## 4. 关键设计决策

### D1 · 统一条目模型（必做）

新增跨源模型，**不动 `MediaItem`**（它被大量本地逻辑依赖，改它风险高）：

```kotlin
enum class HomeSourceKind { Local, Smb, Emby }

data class HomeEntry(
    val key: String,          // 统一续播键：/path | smb://sourceId/share/path | emby:itemId
    val title: String,
    val kind: HomeSourceKind,
    val posterRef: String?,   // 见 D2
    val progress: Float?,     // 0f..1f，null=无续播
    val lastPlayedMs: Long,
)
```
> 单文件单职责：`HomeEntry` 只描述"首页要展示的一条内容"，不含任何 IO/UI。

### D2 · 海报来源 —— 离线优先（**核心裁决**）

用户强调"无网络环境"。TMDB 取海报在离线/局域网时**不可用**，所以：

| 源 | 海报策略 |
|---|---|
| Emby | 直接用 Emby 服务器自带图片（局域网可达，不依赖公网） ✅ |
| 本地/SMB 视频 | **本地抽帧缩略图**：播放过的用 mpv/ffmpeg 抽关键帧缓存；未播放的用文件首帧抽一张，落盘缓存 |
| 兜底 | 抽帧失败 → 纯标题卡（**不放假图、不空白**） |
| TMDB 增强 | 仅公网可用时，用文件名匹配 TMDB 换更精美的海报（可后置，P3+） |

> 这条是"无网络也能有画面"的关键。默认不依赖 TMDB。

### D3 · NAS 扫描策略（防全盘扫 12T）

- 只扫**用户指定共享 + 指定根目录**（如 `12T/电影`、`12T/剧集`），不默认全盘。
- 最大深度 3 层；单次最多 N=2000 个视频文件，超限截断并记录。
- **后台增量**：首次全扫建索引，之后按 mtime 增量；不清空重建。
- 索引落盘（`NasIndexStore`，JSON 或轻量 SQLite），首页读索引不实时扫。

### D4 · 首页模块形态

- 现有 `ContinueRow`（继续观看）→ 升级为**跨源**（本地+SMB+Emby 合并、按 lastPlayed 排序、去重）。
- 新增一行 **「本地片库 · 最近添加」**（P3），展示 NAS 扫描出的影片（按 addedSec 倒序），点开直接播。
- 两者并列，不合并（前者是"在看"，后者是"有什么"）。

> 待你确认：片库行叫「NAS 片库」还是「本地片库」？（若以后接多个 NAS，建议「本地片库」）

### D5 · 待你拍板

1. **"五网络环境"** = "无网络/局域网环境" 吗？若是，D2 的离线优先就是硬要求。
2. NAS 扫描根目录具体指哪几个？（`12T/电影`+`12T/剧集`？还是要全盘？）
3. 片库行命名（见 D4）。

---

## 5. 文件家族树（单文件单职责，全部现有目录，不新建包/applicationId）

### P1 · Emby 最近播放进首页（最小闭环）
```
emby/EmbyClient.kt              [改] items() 增 filters/sortBy 参数(IsResumable/DatePlayed)
emby/Source/EmbyResumeSource.kt [新] 拉 Emby 可续播列表 → List<HomeEntry>（含 image url）
home/HomeEntry.kt               [新] 跨源统一条目模型（D1）
home/HomeFeedAggregator.kt      [新] 聚合 本地+SMB+Emby 续播，按 lastPlayed 排序去重
MainActivityHomeFeed.kt         [改] refreshContinue() 改走 Aggregator
ui/design/home/sections/ContinueRow.kt [改] 入参 MediaItem→HomeEntry（poster 支持远程/本地）
```

### P2 · 本地 + SMB 最近播放打通
```
library/ContinueWatching.kt     [改] 支持 smb:// key 对齐（保留 MediaStore 路径）
library/SmbPlaybackKey.kt       [新] smb://<sourceId>/<share>/<path> 构造/解析（若已有则复用）
```

### P3 · NAS 片库扫描 + 首页模块（最大工程）
```
smb/scan/VideoFileFilter.kt     [新] 扩展名/隐藏目录/样本文件过滤
smb/scan/SmbLibraryScanner.kt   [新] 递归遍历（深度≤3、数量上限），产出文件清单
library/NasIndexStore.kt        [新] 索引持久化 + 增量更新（按 mtime）
library/ThumbnailExtractor.kt   [新] 本地抽帧缩略图（离线兜底，落盘缓存）
library/NasLibrarySource.kt     [新] 索引 → List<HomeEntry>（按 addedSec 倒序）
ui/design/home/sections/NasLibraryRow.kt [新] 首页「本地片库」横滑行
```

> 每个文件 ≤300 行、单一职责；`SmbLibraryScanner` 只管遍历、`NasIndexStore` 只管存取、`ThumbnailExtractor` 只管抽帧。

---

## 6. 分期与验收

| 期 | 内容 | 真机验收判据 |
|---|---|---|
| **P1** | Emby 续播进首页 | 首页 ContinueRow 出现 Emby 在看的片（带 Emby 海报+进度条）；点开续播到正确位置 |
| **P2** | 本地+SMB 续播 | 播过 SMB 片后回首页，ContinueRow 出现该片（SMB 抽帧缩略图+进度） |
| **P3** | NAS 片库 | 首页出现「本地片库」行，展示 NAS 电影/剧集（抽帧缩略图），点开可播；重启 App 索引仍在（不重扫） |

每期：`ui_gate` PASS + `assembleDebug` BUILD SUCCESSFUL + K40 真机截图/日志。

---

## 7. 不做的事（诚实边界）

- 不改 `MediaItem`（避免波及本地播放/详情链路）。
- 不默认全盘扫 NAS（12T 全扫会拖垮首页）。
- 不在无网络时调 TMDB（D2 离线优先）。
- 不做"自动刮削/整理文件"（只读索引，不移动/改名 NAS 文件）。
- P3 前不承诺"海报和 Emby 一样精美"——NAS 抽帧缩略图是画面来源，非精修海报。

---

## 8. P3 实现记录（2026-11-18 定稿）

### 8.1 分类（用户要求：必须分类，且可开关）

| 分类 | NAS 根目录映射 | 真机数量 |
|---|---|---|
| 电影 Movie | 电影 / REMUX / 原盘 | 378 |
| 剧集 Tv | 剧集 / 电视剧 / 美剧… | 1331 |
| 综艺 Variety | 综艺 | 0（该共享无此目录） |
| 动漫 Anime | 动漫 / 动画片 / 番剧 / 动画 | 155 |
| 演唱会 Concert | 演唱会 / 音乐 / MV | 22 |
| 纪录片 Documentary | 纪录片 / 记录片 | 126 |

- **分类由目录名决定，不做内容识别**（猜错分类比按目录归类更糟）；认不出的目录直接跳过，不塞"其他"。
- 首页每分类一行，**空分类不渲染**。
- 设置 → 「首页本地片库」→ 每分类一个开关（默认开电影/剧集），**切换即时生效**（`onCategoryChanged` 回调，不依赖 onResume）。

### 8.2 封面三级链路

1. **库内同目录图**（真机 7% 命中：CMCT 组 `片名.年份.jpg`、CHD 组 `cover.jpg`）
2. **TMDB 兜底**：`完整名 → 纯中文段 → 英文名` × `电影/剧集`，共 6 路候选
3. **渐变 + 片名首字**占位（都失败时，不放假图）

### 8.3 稳健性要点（踩过的坑）

| 坑 | 表现 | 对策 |
|---|---|---|
| 裸连接不走代理 | 图片全取不到 | 必须 `TmdbClient.openConnection`（有代理走 SOCKS） |
| 图片 CDN 时断 | `SocketException: Connection reset` | 3 次重试 + 8s 超时 + **磁盘缓存** `cacheDir/nas-poster`（成功一次永久可用） |
| MIUI 过滤日志 | `logcat` 只有系统日志 | 诊断走 `files/net-diag.log`（NetDiag 通道） |
| 剧集名带季集 | `S01E01` 让 TMDB 搜不到 | `MediaTitleParser` 剥 `S01E01/E05/Season 2/1x03` |

### 8.4 已知限制（诚实边界）

- TMDB 图片在国内网络**不稳定**，部分卡片仍可能落占位；成功过一张就永久缓存。
- 无 `tmdb_key.txt` 时只有库内图（7%）+ 渐变占位。
- NAS 影片**不刮削**（不移动/改名文件，只读索引）。
