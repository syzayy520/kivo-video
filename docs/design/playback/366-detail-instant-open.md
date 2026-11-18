# 366 · 详情页「进去就已经加载好」（对标 Rex）

> 起因：用户对照 Rex ——「rex 进去就已经加载好了，咱们的要加载好一会儿」，要求拆包看 Rex 怎么实现。
> 本文分三块：**Rex 的静态实证** → **我们实测的差距** → **落地与验收**。

---

## 一、Rex 是怎么做到的（静态拆解实证）

Rex 是 iOS **FairPlay 加密 IPA**，拿不到源码；但项目内 `docs/reference/rex-teardown/rex-package/strings/`
有 78 份文案 JSON，键名与文案本身把缓存/预加载策略暴露得很清楚。

| Rex 键 | 原文（中文包） | 说明 | 证据强度 |
|---|---|---|---|
| `appSettings.copy.detail.cache.duration` | **详情缓存时长** | 详情页有**可配时长的持久缓存**，二次进入直接出内容 | ✅ 文案实证 |
| `settings.cache.item.general` | Data Request Cache / 请求数据缓存 | 通用请求数据缓存（可开关） | ✅ 文案实证 |
| `settings.cache.item.modules` | Widgets Cache | 首页组件各自缓存 | ✅ 文案实证 |
| `settings.cache.item.danmakus` / `.subtitles` | 弹幕缓存 / 字幕缓存 | 分域缓存，互不牵连 | ✅ 文案实证 |
| `settings.cache.detailPreload.mode` | **进入详情后预加载**（Always On / Wi-Fi Only / always closed） | 详情页一打开就预取**视频数据**（减少起播等待） | ✅ 文案实证 |
| `settings.cache.detailPreload.libraries` | 仅为选中的资源库预加载 | 预加载按库粒度控制 | ✅ 文案实证 |
| `settings.aggregated.sources.metadata.footer` | 源越靠前越优先用于**补全元数据** | 元数据多源按优先级逐源补全，不吊在单一阻塞源上 | ✅ 文案实证 |
| `settings.cache.clearAll` / `.clear` | 清理全部缓存 / 清理 ● 缓存 | 缓存是可清理的实体资产 | ✅ 文案实证 |

补充（336 已记，同源证据）：首页内置 `mock-*.jpg` 占位卡 + 模块级独立 Loading（`Rex_UI结构复刻说明书.md` §6.1/§7）。

**Rex 快 = 三件事叠加**

1. **详情/数据/图片全链路持久化缓存**，且详情缓存时长可配 ⇒ 常见路径是「命中本地 → 立刻出内容」，不是每次重打源站；
2. **模块级独立异步** + 内置占位 ⇒ 先出骨架，内容各自到各自补，谁慢不拖谁；
3. **元数据多源按优先级补全** + 反代 ⇒ 不吊死在单条慢链路上。

**边界（不冒充确定）**：具体缓存载体（CoreData/SQLite/文件）、默认 TTL、并发数拿不到源码级证据，属 ◐ 合理推断。

---

## 二、我们实测的差距（真机埋点，K40 `b6e950ee`）

埋点：`files/net-diag.log` 里的 `detail-perf`（常驻诊断，不是临时打印）。
环境：Emby `nyamedia`（`emby.bangumi.ca`），TMDB JSON 已热（既有 `TmdbResponseCache`）。

### 改造前（串行累加）

| 入口 | 总耗时 | 关键路径分解 |
|---|---|---|
| 电影《读书会》 | **1332 ms** | Emby 详情 908 → TMDB 搜索 3 → TMDB 详情 1 → 图片尺寸 286 |
| 剧集《兰香如故》47 集 | **2383 ms** | Emby 详情 290 → TMDB 2 → 详情 3 → 图片尺寸 436 → **季集 1592** |

两个结论：

- **TMDB 那两跳不慢**（`TmdbResponseCache` 已生效，3 ms 级）—— 336 的缓存是对的；
- **Emby 这一侧零缓存**：详情、季、集每次进详情全重打源站；剧集最贵的是**四季并发拉集**（1.6 s）。

⇒ 与 Rex 的差距不在"网络快慢"，而在**有没有把已取到的元数据留下来**。

### 改造后

| 场景 | 改造前 | 改造后 | 说明 |
|---|---|---|---|
| 剧集 · **二次进入** | 2383 ms | **74 ms** | 全链路 `[emby-cache] HIT` |
| 电影 · 二次进入 | 1332 ms | **67 ms** | 同上 |
| 首进新片（无缓存） | 2344 ms | 约 0.9–2.7 s（随源站抖动） | 首进仍需网络，Rex 也需；差别是**只付一次** |

热路径证据（原文节选）：

```
+     0 ms  detail-perf activity-create
+    56 ms  detail-perf begin emby=255945 series=true
+    58 ms  [emby-cache] HIT 255945?Fields=...
+    62 ms  [emby-cache] HIT Images
+    62 ms  [emby-cache] HIT Seasons?...
+    64 ms  [emby-cache] HIT Episodes?...
+    74 ms  detail-perf load-done ok=true
```

---

## 三、落地（文件家族树）

```
emby/EmbyResponseCache.kt   [新] Emby 慢变元数据响应磁盘缓存
                                 key=SHA256(url)，TTL 分级；只写 2xx；原子写；.key 伴生索引
emby/EmbyClient.kt          [改] details()/imageWidths() 走缓存；Fields 加 ProviderIds；
                                 userDataWrite/reportStopped 后失效
emby/EmbyShows.kt           [改] seasons()/episodes() 走缓存
emby/EmbyDetails.kt         [改] 新增 tmdbId（服务器刮削值）
ui/detail/EmbyResource.kt   [改] Emby 详情 ∥ TMDB ∥ 图片尺寸 ∥ 季表 四路并行；
                                 有 ProviderIds 就不搜索；图片尺寸 800ms 上限
playback/EmbyPlaybackReporter.kt [改] 带系列 id，停止后失效剧集详情缓存
ui/detail/DetailPage.kt / MovieDetailActivity.kt [改] detail-perf 常驻计时埋点
```

### 3.1 缓存什么、不缓存什么

| 端点 | TTL | 理由 |
|---|---|---|
| `Users/{uid}/Items/{id}?Fields=…`（详情） | 6 h | 慢变元数据；含 UserData，写操作会失效 |
| `Items/{id}/Images`（图片尺寸表） | 30 d | 只与图片 tag 有关，几乎不变 |
| `Shows/{sid}/Seasons` | 6 h | 慢变 |
| `Shows/{sid}/Episodes?SeasonId=` | 6 h | 含每集进度 ⇒ 停止播放后失效 |
| `Items/Resume`、`Items/Counts`、`Items` 列表、`Playing|Played|Favorite` 写 | **不缓存** | 快变/有副作用，缓存会让首页续播与已看状态定格 |

### 3.2 一致性（不做静默降级）

- 收藏/已看写成功 → `invalidateItem(itemId)`；
- 播放停止（服务器刚写入续播位）→ 失效**所属剧集**（按单集 id 命不中，故带 `kivo.embySeriesId`）；
- 缓存目录被系统回收 → 写入前 `mkdirs()`，否则会静默写失败、永远 MISS（**真机踩过**）；
- 只写 2xx 正文；失败照旧回源并把错误抛给原有 UI 逻辑。

### 3.3 并行与预算

- 四路并行（Emby 详情 / TMDB / 图片尺寸 / 季表），关键路径从"累加"变"取最慢"；
- 服务器已刮削 `ProviderIds.Tmdb` 时**免掉一次 TMDB 搜索**（更准，且少一跳）；
- 图片尺寸表只用于"库图 vs TMDB 图谁更清晰"，**上限 800 ms**，超时按空表走（库图兜底），不拖整页。

---

## 四、已知边界（如实）

1. **首次**进入一部从未打开过的片，仍需网络（Rex 同样）。本改造保证的是"**只付一次**"。
2. Emby 源站单请求抖动大（实测 290 ms–1.8 s），冷路径总耗时受它支配，非本地代码可控。
3. 首页/「继续观看」进入详情时 `preview=false`（没有列表快照）⇒ 冷路径期间仍是转圈；补骨架属 UI 改动，本轮未动。
4. 缓存 TTL 固定，没有做 Rex 那种"用户可配缓存时长"（加设置项属 UI 变更，未做）。
5. 进度类数据（已看/续播）依赖失效策略；若第三方客户端改了进度而我们没收到写事件，最多陈旧 6 h。
