# 336 · 全局页面加载流畅度（去等待，不止 Emby）

> 2026-11-16 19:00 用户：「要全量优化，不止是 emby 页面」「还是好几秒才真正显示」「每个影片详情页都要 3 秒钟左右」。
> 335 只解决了 Emby 首页 hero/resume 的串行，体感没变 —— 说明瓶颈在更底层的公共资源层。

## 一、真机取证（K40 b6e950ee，curl + net-diag，非猜测）

1. **TMDB 直连慢**：设备 `tmdb_proxy.txt` 不存在（无代理），`api.tmdb.org` 实测
   - 首次 `connect=0.229s total=2.609s`，后续 `0.89s / 0.70s`；
   - 对照百度 `connect=0.051s total=0.152s`。
   - ⇒ 跨境 TLS + DNS 冷启动 ≈ 2.6 s，稳态仍 0.7 s/请求。
2. **JSON 零缓存**：
   - `TmdbDetail.details()` 每次进详情页都发一次
     `/movie/{id}?append_to_response=videos,credits,similar,images,release_dates`（大响应），零缓存；
   - `TmdbHome.get()`（首页 now_playing/trending/on_the_air 等）零缓存，只有图片 URL 有 15min 内存 memo；
   - 图片有 `TmdbImageCache` 磁盘缓存，**JSON 响应没有** —— 这是缺口。
   - ⇒ 每次进详情页 = 一次 2.6 s 级跨境请求；返回再进 = 又一次 0.7 s。
3. **Emby hero 被评分卡死**：`EmbySessionModel.heroItems()` 里
   8 条目 × 2 请求（`searchId` + `details`）= 16 个 TMDB 请求，`futures.map { it.get() }` 全部完成才返回。
   net-diag 铁证：`[emby] hero 数据 n=8` 的时间戳已到，但首屏仍是空位 ——
   `heroes = h.await()` 被评分拖住。Emby 数据早到了，是 TMDB 评分让界面迟迟不显示。

## 二、根因归纳（全 App 共性）

| 页面 | 慢在哪 | 性质 |
|---|---|---|
| 影片详情页 | `tmdb.details()` 每次新请求，2.6 s 冷 / 0.7 s 热 | JSON 无缓存 |
| App 首页 | `TmdbHome.get()` 多个列表每次新请求 | JSON 无缓存 |
| Emby 首页 | hero 数据已到，但 16 个 TMDB 评分请求全等完才显示 | 评分阻塞首屏 |
| Emby 评分本身 | 每次重新 searchId + details | JSON 无缓存 |

⇒ 两件事：**给 TMDB JSON 加磁盘缓存**（一次拉取全 App 复用），**hero 评分移出首屏关键路径**。

## 二·B、Rex 是怎么快的（静态 IPA 文案实证，docs/reference/rex-teardown/rex-package）

Rex 是 iOS FairPlay 加密 IPA，拿不到源码，但 78 份文案 JSON 把它的缓存/数据源策略暴露得很清楚：

| Rex 键 | 原文 | 说明 |
|---|---|---|
| `settings.cache.item.general` | Data Request Cache | **请求数据缓存**（可配开关） |
| `appSettings.copy.detail.cache.duration` | Detail cache duration | **详情页缓存时长**（可配）—— 二次进详情走缓存，这正是我们缺的 |
| `settings.cache.item.modules` | Widgets Cache | 首页组件缓存 |
| `settings.cache.section.video` / `.libraryData` / `.subtitles` | Video/Resource library/Subtitle Cache | 各类数据分域缓存 |
| `settings.aggregated.posterLanguage.cost.message` | "…each title makes one extra TMDB image request when its poster appears on screen, and the chosen poster is **cached on this device**" | 海报选完落本机缓存，不重复拉 |
| `settings.aggregated.sources.metadata.footer` | "Drag to reorder. Sources nearer the top are preferred when enriching media information" | 元数据多源聚合 + 优先级，非单一阻塞源 |
| `settings.reverseProxy.*` | Reverse Proxy / Media Server Lines | 直连不可用时走反代 |

**结论**：Rex 快的本质 = **详情/数据/图片全链路持久化缓存 + 可配缓存时长 + 元数据多源非阻塞 + 反代**。
我们 336 的 JSON 磁盘缓存 + hero 评分非阻塞，与 Rex 同源；差别是 Rex 把缓存时长做成用户开关，
我们按铁律「不改 UI/不加设置项」用保守固定 TTL（详情 7d、列表 12h）。

### 实现层再深挖（UI 结构说明书 §6 + 资产清单，回答「怎么实现」）

| 机制 | Rex 做法 | 证据 | 性质 |
|---|---|---|---|
| 首页数据编排 | `LazyVStack` 模块流，每个 Section 有**独立** Loading/Refresh/错误重试状态位，模块各自异步加载、互不等候 | `Rex_UI结构复刻说明书.md` §6.1 line 225–230 | ✅ 静态实证 |
| 首屏占位 | `RexHome.bundle` 内置 20+ `mock-*.jpg` 占位卡 + `ShowcaseMediaLibrary.json`；先出骨架卡再填真实数据 | §7 line 313；`main.json` 资产 | ✅ 资产实证 |
| 数据源 | `RexScript.bundle` 的 `rex-tmdb.js / bangumi.js / nlsearch.js` + cheerio：首页模块数据来自 **JS 脚本**，可解析 HTML/聚合，非单一阻塞 REST | §7 line 317 | ✅ 资产实证 |
| 元数据丰富化 | 多源按拖拽优先级「nearer the top preferred」，逐源补全而非一次全等 | `settings.aggregated.sources.metadata.footer` | ✅ 文案实证 |
| 反代 | 直连不可用时给路由绑反代，绕开慢链路 | `settings.reverseProxy.*` | ✅ 文案实证 |
| 详情缓存时长 | 用户可配 `Detail cache duration`；二次进详情走本地缓存 | `appSettings.copy.detail.cache.duration` | ✅ 文案实证 |

**Rex 快 = 三件事叠加**：① 持久化缓存（详情/数据/图片，可配时长）；② 首屏内置占位 + 模块级独立异步加载（不等全量）；③ 数据源多源/脚本化/反代（不吊在单条慢链路上）。
FairPlay 加密 ⇒ 具体缓存实现（CoreData/文件/SQLite）、TTL 默认值、并发数拿不到源码级证据，属 ◐ 合理推断，不冒充确定。

**对照我们的落地**：
- ① 已落地：`TmdbResponseCache`（详情/列表/搜索 JSON 磁盘缓存）+ 既有 `TmdbImageCache`（图片）。
- ② 部分落地：Emby hero 评分已移出首屏（hero 数据先出）；**首页/详情的骨架占位因铁律「不写 UI」未加** —— 这是与 Rex 的客观差距，登记不隐瞒。
- ③ 未落地：多源聚合/反代属于新架构，不在本次范围；设备当前走 TMDB 直连（实测 2.6s 冷/0.7s 热），缓存命中后不影响体验。

## 三、设计约束（铁律）
- 不改任何 UI 视觉；只缩短等待。
- 单文件单职责，Kotlin ≤300 行；先规划家族树；不堆砌、不留死代码。
- 不静默降级：缓存只写成功响应，失败仍回源并保留原错误信息。
- 必须真机验证 + 证据；编译只看 `BUILD SUCCESSFUL`。

## 四、文件家族树（本次）

```
tmdb/
├── TmdbResponseCache.kt   [新] 单一职责：TMDB JSON 响应磁盘缓存
│                                key=SHA256(url)；正文原文；TTL 分级（详情/评分 7d，列表 12h）
│                                只写 2xx；原子写；失败回源
├── TmdbClient.kt          [改] 构造时 ensure(cacheDir)；searchFirst 走缓存
├── TmdbDetail.kt          [改] get() 走缓存（详情页 3 秒主凶）
└── TmdbHome.kt            [改] get() 走缓存（首页列表，短 TTL）
emby/
├── EmbySessionModel.kt    [改] heroItems() 拆为 heroRaw() + enrichHeroRatings()
└── ui/EmbyBrowseScreen.kt [改] 两阶段：Emby 数据先显示，评分后台补
```

- **不新建平行实现**：缓存是全局基础设施，`TmdbClient/TmdbDetail/TmdbHome` 各自只加几行读写缓存，
  不各自造缓存轮子；图片继续用既有 `TmdbImageCache`，两者职责分明（JSON vs 图片）。
- **不写 UI**：`EmbyBrowseScreen` 只调数据顺序，卡片/布局/颜色零改动。

## 五、实施顺序
1. 新建 `TmdbResponseCache`；接入三个 TMDB 请求口。
2. 拆 `heroItems` → `heroRaw` + `enrichHeroRatings`，Screen 两阶段显示。
3. 编译 → 装机 → 真机复测：详情页二次进入耗时、Emby 首屏 hero 出现时机、日志无异常。
4. 证据与诚实边界写 `docs/evidence/p336-page-load/`。

## 六、实施中发现的第二层根因（录屏实证，必须记录）
缓存上线后录屏抽帧（`docs/evidence/p336-page-load/detail_in_grid.png`）发现：
**即使二次进入走缓存 HIT，仍在 1.2–1.6 s 出现白屏 + 转圈**（约 0.4–0.6 s）。
采样该帧中心像素 = `RGB(243,242,247)` ≈ `#F2F2F7` —— 不是 window（黑 `#050506`），
而是 `DetailBackdrop` 在 `details == null` 时铺的 `DesignTokens.pageBackground` 浅色底。
⇒ 「白屏」有两层：① 网络等待（缓存已解决）；② **加载期首帧的浅色占位底**（本节解决）。
修法（`DetailBackdrop` + `DetailPage`，不改最终视觉）：加载期占位底由浅改黑、转圈由深改白；
数据到位后仍走原海报/吸色逻辑，最终态像素级不变。

## 七、诚实边界（先写，防止自欺）
- **首次**进入详情页仍要一次真实 TMDB 请求（≈2.6 s 跨境），缓存解决的是「返回再进/同片重进」；
  除非后续加「列表页后台预热详情」，本轮不承诺首次秒开。
- 加载期首帧仍有极短（<1 帧）重组开销，无法归零；改深色后观感与最终态一致，不再"白闪"。
- TMDB 宽带/跨境是客观上界；本地缓存命中时为磁盘读取（毫秒级）。
- 缓存不缓存失败响应；密钥/代理配置不变（设备当前无代理，走直连）。
