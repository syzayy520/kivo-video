# 254 · Emby 接入层重构 + 详情页（B 方案 + 详情卡）

> 闸门：本稿过目后才动代码；收口以真机证据复算为准（K40 `b6e950ee`，模拟器无效）。
> 任务对应：#17 设计 / #18 路由与契约 / #19 mpv+预览+token 离 URL / #20 死代码 / #21 真机闭环 / #22 收口 /
> #23 详情页（复用既有 TMDB 智能填充）/ #24 连接表单补全 / #25 退役旧 Emby 连接表单。
> 用户定调：① 海报单击**进详情页**再播放（正常播放器形态），版式完全参照 REX，能比复刻更好就更好；
> ② **复用既有智能填充**（TMDB 详情家族 + 粘贴识别），不另起炉灶；③ 连接表单要有智能识别/端口/http-https/名称/备注，
> 主机地址必填、其余选填；④ 两台验证服务器由用户运行时提供，**凭据不入库、不入证据、不入本稿**。
>
> **修订记录（2026-11-12 二次过闸）**：初稿 D1/D2/D10 主张「EmbyActivity 内嵌 selected 详情 + 不引 TMDB + 新暗色版式」，
> 与用户「复用智能填充」定调冲突，且 `MovieDetailActivity` 已是 REX 逐帧校准的成品（pull-zoom/滚动色场/Logo 标题/动作行/演职员/相似）。
> 故 D1/D2/D10 **作废重写**为复用方案（见下），新增 D13（表单）/D14（详情复用）/D15（旧表单退役）。
> 另更正初稿 §一 fact5 的误判：`Pages.kt` 并非零调用，死的只是 `SourcesPage`。

## 一、现状事实（坐标已核）

| # | 事实 | 坐标 |
| --- | --- | --- |
| 1 | 海报单击直接起播，无详情页 | `emby/EmbyActivity.kt:79-86` |
| 2 | defaultHome 不看协议，SMB 默认首页被误路由进 Emby | `MainActivity.kt:224-228`（onOpenSource :276-287 却看协议） |
| 3 | `api_key` 拼进流 URL → 进 mpv 日志/代理日志 | `emby/EmbyClient.kt`（**本卡已修**：streamUrl 去 api_key，token 走 `X-Emby-Token` 头） |
| 4 | URL 起播只走旧内核、**无预览帧** | `ui/compose/PlayerSessionStart.kt:25-29,91-93` |
| 5 | ~~`ui/design/Pages.kt` 189 行零调用~~ **误判更正**：死的只是 `SourcesPage`（52 行，已删）；`SettingsPage`/`ProfilePage`（MainActivity 在用）+ `relDays`/`fmtSeen`/`formatDate`（WatchingPage/DiscoverPage 共用）是活代码，**Pages.kt 保留**（现 137 行） | 全仓 grep 核过调用方 |
| 6 | `fake_emby` 骨架被桌面测试依赖 | `backend/CMakeLists.txt:44-45` + p3_behavior/emby_direct_play/emby_session 三测试 |
| 7 | SourceTypeSheet 的 Plex/Jellyfin/WebDAV/飞牛 = 禁用态「暂未接入」 | 诚实预留，非死代码 |

内核侧零改动前提已核：`MpvPlaybackSession(context, ByteSource)` 与
`MpvThumbnailReader(appContext, ByteSource, cacheDir)` 都吃 `net/ByteSource` 接口；
`LocalRangeServer` 对 `readAt` 已 `synchronized(source)` 串行化（:286）。

## 二、决策

**D1 导航（重写：复用成品详情页）**：Emby 海报墙 `onItem` → `MovieDetailActivity`（既有 REX 逐帧校准详情页），
带 Emby extras（`kivo.embyItemId/sourceId/embyToken/embyUserId/embyName/embyYear`）。详情返回 → 回海报墙
（EmbyActivity 仍在栈）；详情播放按钮 → PlayerComposeActivity（Emby 流 intent）。**不在 EmbyActivity 内嵌 selected 详情**、
不新建详情 Activity——详情壳/主题/边缘到边缘由 MovieDetailActivity 既有逻辑承担。

**D2 详情数据（重写：复用智能填充，双层）**：
① **Emby 真值层**：`GET /Users/{uid}/Items/{id}?Fields=Overview,People,Genres,Studios,MediaSources,
MediaStreams,ProductionLocations,PremiereDate,CommunityRating,RunTimeTicks,ImageTags`（`EmbyClient.details` 已实现）。
取媒体信息（容器/容量/码率/视频/音频/字幕 `DisplayTitle`）、收藏 `IsFavorite`、已看 `Played`、简介/演职员/类型兜底。
② **TMDB 智能填充层**：`TmdbClient.searchId(name, year)`（新增，复用 `ratingFor` 的 search/movie 请求抽公共 helper）→
命中则 `TmdbDetail.details(false, tmdbId)` 取富详情（幕布/Logo/演职员头像/评分/相似/预告/剧照/分级）。
**无 key/无匹配 → Emby 字段映射成 `TmdbDetail.Details` 文本兜底**：图 URL 留空（DetailBackdrop 显「暂无封面」+ 色场），
cast `profileUrl=null`（灰底圆，不编图）。Emby 图需 token 而详情图加载器（TmdbImageCache）不带 token，
故兜底层**不挂 Emby 图 URL**（诚实文本态，不假装能加载）。评分：TMDB 命中显 TMDB 徽章（既有 RatingBadges）；
兜底显 CommunityRating 一枚；都缺 → 隐藏徽章行（不显 0，G9 一致）。

**D3 标题**：优先 Logo 图（REX 详情艺术字标题即此类资源，实测有 Logo tag）；无 Logo 退文字大标题。

**D4 动作行（复用 DetailActionRow，Emby 覆盖收藏/已看）**：
- 主胶囊「播放」：Emby 条目自带流资源，`playable=true`（不像 TMDB 本地匹配可能空）；onPlay → Emby 流 intent。
- 心形收藏 = **Emby 服务端**：`POST/DELETE /Users/{uid}/FavoriteItems/{id}`，初值 `UserData.IsFavorite`。
- 「…」菜单「已观看」= **Emby 服务端**：`POST/DELETE /Users/{uid}/PlayedItems/{id}`，初值 `UserData.Played`。
- 「待看」无 Emby 原生对应 → 保留**本地 FilmMarks**（真实本地功能，非假状态）；长按替换主操作延期。
- 实现：`EmbyResource.actions()` 包 `rememberDetailActions(FilmKey)`，`marks.copy(favorite=服务端, watched=服务端)`，
  toggle 拦截 FAVORITE/WATCHED 走 Emby（乐观更新 + 失败回滚 + 非空 reason），WATCHLIST 透传本地。
  FilmKey：TMDB 命中用 tmdbId，否则 `embyItemId.hashCode()`（稳定本地标记键）。
- 「在 TMDB 查看」：`tmdbId>0` 才开外链；无匹配 → Toast「未匹配到 TMDB 条目」（诚实抑制，不开假 URL）。

**D5 播放链路**：新增 `net/HttpByteSource.kt`：
- `size()`：`GET Range: bytes=0-0` → `Content-Range: bytes 0-0/TOTAL` 取 TOTAL；拿不到抛 reason；
- `readAt`：Range GET，**必须 206**；200 = 服务器无视 Range → 抛「服务器不支持 Range，无法播放」；
- token 走 `X-Emby-Token` 头，**永不进 URL**；代理 URL 仅 localhost，mpv 日志只见 localhost；
- 每次读独立连接、流读完关闭不 disconnect → 复用 JVM keep-alive 池；代理层已串行化，双保险；
- 主播放与预览帧**两个独立实例**（与 SMB 同构，不互顶）。

新增 `ui/compose/PlayerUrlStart.kt`（镜像 PlayerSmbStart）：IO prepare（size 探针）→
主线程**引擎分支前**挂预览（第二实例）→ MPV：`startMpvPlayback`；
LEGACY：`RangeFd(HttpByteSource)` → `startNetworkPlayback`（引擎开关 parity，不静默丢能力）。

**D6 预览帧**：`MpvThumbnailReader` 零改动；`mountSmbThumbnails` 泛化改名
`mountNetworkThumbnails(source: ByteSource)`；`startMpvPlayback` 参数泛化 `ByteSource`。

**D7 intent 契约**：`+kivo.sourceId +kivo.embyToken`；**删 `kivo.routing`**（写读两端）；
playurl 不含 api_key。`PlayerSourceOrigin` 不动（explicitName 优先，REX 0169 语义）。

**D8 投屏**：`smbPlayback=true` 走现有诚实阻断；文案「SMB 片源投屏暂未开放」泛化为
「网络片源投屏暂未开放」（`PlayerCastBridge.kt:43-47`）。Emby 投屏（渲染器带不了 token 头）延期。

**D9 defaultHome**：新增 `sources/SourceOpen.kt` `intentFor(context, source)` 协议分支，
defaultHome 与 onOpenSource 共用（MainActivity 不增肥）。

**D10 详情页版式（重写：复用 MovieDetailActivity，不新做暗色沉浸）**：
直接复用既有 REX 逐帧校准的详情编排——固定幕布 + 连续滚动色场（暗→浅，`DETAIL_SHEET_RAMP_DP=390`）+
pull-zoom 头 + Logo/大标题 + 元信息行「日期 · 地区 · 片长」+ 动作行 D4 + 简介(2 行点按展开) + 评分徽章 +
资源卡 + 演职员(Ø55 圆) + 家长指南 + 类型 chips + 预告 + 剧照 + 相似横排 + 圆形返回。
**Emby 与 TMDB 共用同一套节组件**（DetailHeroContent/DetailSections/DetailBackdrop），差异只在
「资源槽 + 播放 intent + 收藏/已看来源」。资源槽：Emby 媒体卡（分辨率/容量·码率/容器/来源名+绿勾）+
**可展开**媒体信息（视频/音频/字幕 `DisplayTitle` 真值，点「展开媒体信息」折叠区）。
剧照/预告：TMDB 命中才有（Emby RemoteTrailers 多 YouTube 播不了，兜底层不挂）。

**D11 动效**：复用详情页既有 pull-zoom + 滚动色场 + 按压反馈；收藏切换沿用 DetailActionRow 的 heartScale 弹性。
不新增转场（MovieDetailActivity 既有 push 语义）。

**D12 凭据**：验证服务器 A = 224 稿既有（历史事实可引用）；服务器 B（带密码）仅运行时输入，
地址/账号/密码一律不写进仓库任何文件。证据 JSON 只留脱敏字段。

**D13 连接表单补全（#24，对照 243 §表单 + library-rebuild/plan.md:40）**：
workspace 的 `SourceConnectionContent`（Emby/SMB 共用）补齐——
- 首屏：智能识别（粘贴板 → `SourceShareParser.parse` → `SourcePastePreview` → `applyPreview`，**复用既有**，按钮文案「智能识别」对齐 REX）/
  名称（选填）/ 完整地址（**必填**，唯一权威地址态）/ 账号 / 密码；
- 渐进展开「来源信息 · 选填」组：http/https 分段选择 + 端口数字输入 + 备注（选填）。
  **端口/协议不另立地址态**：从完整地址派生显示，编辑后写回同一地址串（plan.md 明禁两套互相矛盾的地址状态），
  归一化复用 `SourceAddress.normalizeHttp`（默认端口剥离/路径越界守卫）；
- 备注落地：`SourceRecord +note`（默认 ""）、`SourceStore` 读写加 `note`（`optString`/`put`，**schema 仍 2，加字段不升版**）、
  `SourceManagerModel` draft +note（edit/startNew/clearDraft/draftDirty/save 贯通）；
- 分组展开动效复用 243 M04-S03（0.3s easeInOut，stagger 0.02×i），不硬切。

**D14 详情页复用（#23，反堆砌核心）**：
- **不新造 Emby 详情 Activity/Screen/Sections**，复用 `MovieDetailActivity` 全套 REX 校准节组件。
- **资源卡只做一套**：把既有 `MediaCard` 泛化为数据驱动 `ResourceMedia(resolution,sizeBytes,bitrateBps,sourceLabel,streams?)`，
  本地 `MediaMatch.Matched` 与 Emby `EmbyMediaInfo` 各映射成 `ResourceMedia` 喂**同一个卡**（「同一个东西的多份数据，不是多个文件」）；
  `ExpandMediaInfoRow` 升级为**可点击展开**（一套，两模式共用），展开内容 = 视频/音频/字幕真值。
- **按变化原因拆超线文件**（铁律 ≤300）：
  - `DetailSections.kt`（现 410）→ 把「播放资源」族（PlaySourceHeader/MediaCard/ExpandMediaInfoRow/EmptyResourceBlock/format*）
    拆到新 `ui/detail/DetailResource.kt`（独立语义：播放资源呈现），DetailSections 回落 ≤300；渲染保持像素不变（仅搬家+泛化签名）。
  - `MovieDetailActivity.kt`（现 325）→ 把 `DetailPage` 编排抽到新 `ui/detail/DetailPage.kt`，Activity 只剩壳+intent 解析，回落 ≤300。
- 新 `ui/detail/EmbyResource.kt`（桥接，≤300）：按 sourceId 读地址建 `EmbyClient`+`Auth(token,userId)`；
  `loadDetails()` = Emby 真值 + `searchId`→TMDB 富详情 / 无匹配走 `EmbyFallback` 文本映射；
  `actions()` 包 `rememberDetailActions` 覆盖收藏/已看（D4）；`playIntent()`；`ResourceSlot()` 调 `DetailResource` 同一套卡。
- 新 `ui/detail/EmbyFallback.kt`（≤80）：`EmbyDetails → TmdbDetail.Details` 纯映射（图 URL 留空、cast profileUrl=null、similar 空）。
- `TmdbClient +searchId(title,year)`：抽 `searchFirst` 公共 helper，`ratingFor` 与 `searchId` 共用一次 search/movie（不重复请求逻辑）。
- `EmbySessionModel +accessUserId`（详情需 userId；播放只需 token）；`EmbyActivity.onItem` → `MovieDetailActivity.embyIntent(...)`。
- **删孤儿 `emby/library/EmbyDetailModel.kt`**（Handler/pool 范式与详情页协程范式不合，且全仓仅自引用；逻辑折进 EmbyResource）。
  `emby/EmbyDetails.kt` 数据类**保留**（EmbyClient.details 返回类型 + 兜底映射消费）。

**D15 退役旧 Emby 连接表单（#25）**：
- 删 `emby/ui/EmbyConnectionScreen.kt`（140 行旧三字段表单，与 workspace 表单重复 = 堆砌）。
- `EmbyActivity` 无 sourceId 分支 → 跳 `MainActivity` 带 `kivo.addSource=emby`（CLEAR_TOP|SINGLE_TOP + `onNewIntent` + activity 级请求态）；
  `SourcesScreen +initialAddProtocol/onAddConsumed` → ready 后 `model.startNew(protocol)` 直接展开 workspace 添加流。
- 连接入口从此**单一**：资源库 workspace 表单（D13），不再有第二套 Emby 专用表单。

## 三、家族树

> 状态：✅=本会话已落地（未提交）；🔜=待落地。预算为行数上限（铁律 ≤300）。

| 动作 | 文件 | 预算 | 职责 | 状态 |
| --- | --- | --- | --- | --- |
| 新 | `net/HttpByteSource.kt` | ≤140 | HTTP Range 随机读 + token 头（206-only） | ✅82 |
| 新 | `ui/compose/PlayerUrlStart.kt` | ≤130 | URL 起播分发：预览+引擎分支 | ✅58 |
| 新 | `sources/SourceOpen.kt` | ≤35 | 协议→浏览页 intent 单一入口 | ✅12 |
| 新 | `emby/EmbyDetails.kt` | ≤70 | Emby 详情数据类（Details/Person/MediaInfo） | ✅38 |
| 新 | `ui/detail/DetailResource.kt` | ≤160 | 播放资源呈现族（数据驱动 ResourceMedia + 卡 + 可展开 + 空态），本地/Emby 共用一套 | 🔜 |
| 新 | `ui/detail/DetailPage.kt` | ≤260 | 详情编排（幕布/色场/pull-zoom/节序），从 MovieDetailActivity 抽出 | 🔜 |
| 新 | `ui/detail/EmbyResource.kt` | ≤220 | Emby 桥接：建 client/auth、loadDetails（TMDB 填充/兜底）、收藏/已看覆盖、playIntent、ResourceSlot | 🔜 |
| 新 | `ui/detail/EmbyFallback.kt` | ≤80 | EmbyDetails→TmdbDetail.Details 纯映射（文本兜底，不编图） | 🔜 |
| 改 | `emby/EmbyClient.kt` | ≤250 | +details/favorite/played/similar；streamUrl 去 api_key；fetchBytes +token；conn +UA | ✅236 |
| 改 | `emby/EmbySessionModel.kt` | ≤180 | +accessToken/accessUserId；poster 泛化 image(url) | ✅138（+userId 🔜） |
| 改 | `emby/EmbyActivity.kt` | ≤130 | onItem→MovieDetailActivity.embyIntent；无 sourceId→MainActivity addSource；删旧表单引用 | 🔜 |
| 改 | `tmdb/TmdbClient.kt` | ≤160 | +searchId；抽 searchFirst 公共 helper（ratingFor 共用） | 🔜 |
| 改 | `ui/detail/MovieDetailActivity.kt` | ≤130 | 只剩壳+intent 解析（TMDB/Emby 双入口）；DetailPage 抽出 | 🔜（现 325→拆） |
| 改 | `ui/detail/DetailSections.kt` | ≤300 | 移出播放资源族 → 回落 ≤300；渲染像素不变 | 🔜（现 410→拆） |
| 改 | `ui/detail/actions/DetailActionRow.kt` | ±2 | 图标 import 随拆分调整（若图标搬家） | 🔜 |
| 改 | `sources/ui/workspace/SourceConnectionContent.kt` | ≤300 | 智能识别文案 + 展开组（http/https/端口/备注），单一地址态 | 🔜 |
| 改 | `sources/SourceRecord.kt` | +2 | +note 字段 + withControls 透传 | 🔜 |
| 改 | `sources/storage/SourceStore.kt` | +4 | 读写 +note（schema 仍 2） | 🔜 |
| 改 | `sources/SourceManagerModel.kt` | +8 | draft +note 贯通 | 🔜 |
| 改 | `sources/ui/SourcesScreen.kt` | +6 | +initialAddProtocol/onAddConsumed → startNew | 🔜 |
| 改 | `MainActivity.kt` | +12 | defaultHome/onOpenSource→SourceOpen（✅）；+kivo.addSource/onNewIntent（🔜） | ◐ |
| 改 | `ui/compose/PlayerSessionStart.kt` | −8 | 分发 PlayerUrlStart；删 startUrlPlayback/routing 读 | ✅ |
| 改 | `ui/compose/PlayerSmbStart.kt` | ±10 | 两签名泛化 ByteSource；mount 改名 | ✅ |
| 改 | `ui/compose/PlayerCastBridge.kt` | 1 行 | 投屏阻断文案泛化 | ✅ |
| 改 | `ui/design/Pages.kt` | −52 | 删 SourcesPage（死）；保留 SettingsPage/ProfilePage/时间助手 | ✅137 |
| 删 | `emby/library/EmbyDetailModel.kt` | 92 | 孤儿（仅自引用），逻辑折进 EmbyResource | 🔜 |
| 删 | `emby/ui/EmbyConnectionScreen.kt` | 140 | 旧三字段表单，与 workspace 重复 | 🔜 |
| 留 | MpvPlaybackSession / MpvThumbnailReader / LocalRangeServer / NetworkPlaybackSession / fake_emby / SourceTypeSheet 预留 / Pages.kt 活代码 | — | 内核零改动；旧内核=LEGACY 分支；fake_emby=桌面测试依赖 | — |

## 四、REX 对照与延期清单

本卡做：复用成品详情页（幕布头/Logo 标题/元信息行/播放+收藏+…/简介/评分/类型/可展开媒体信息/演职员/相似/既有动效）；
Emby 收藏/已看走服务端真值；连接表单智能识别/端口/http-https/名称/备注；旧 Emby 表单退役、入口单一。
延期（明示，不假做）：进度回写与续播、长按替换主操作、剧照浏览器（Emby 侧）、预告片（Emby 侧）、
服务器章节、服务器弹幕、STRM、多线路/反代 URL、PlaybackInfo/转码、忽略证书、QuickConnect、
TMDB 多评分聚合（仅单 TMDB 徽章）、Emby 投屏、Emby 图在详情兜底层的 token 加载（兜底走诚实文本态）。

## 五、验证计划（真机 K40）

1. 构建覆盖安装（kivoDebug 签名，禁 uninstall）；**行数自检**：本轮新增/改动的 .kt 全部 ≤300（`wc -l` 留证）。
2. 表单（#24）：workspace 添加 Emby——智能识别粘贴一条 share → 预览回填；手填主机+端口+http/https+名称+备注 →
   保存 → 重开编辑核对 note/端口/协议持久化（SourceStore schema 2 不升版取证）。
3. 旧表单退役（#25）：EmbyActivity 无 sourceId → 落 MainActivity workspace 添加流（不再出现旧三字段表单）。
4. 服务器 A：登录→海报墙 714 部→分页到底/搜索/三排序。
5. 海报→详情（#23）：TMDB 命中条富详情（幕布/Logo/演职员/评分/相似）+ Emby 资源卡（分辨率/容量·码率/容器/来源名）+
   展开媒体信息（视频/音频/字幕 DisplayTitle）；字段与 PC 侧 curl 脱敏 JSON 逐项对照；
   收藏 toggle 后 curl 复验 IsFavorite 翻转再还原；已看 toggle 复验 Played；无 TMDB 匹配条 → 诚实文本兜底（暂无封面/灰底演职员）。
6. 详情→播放：mpv 起播 + 拖动预览帧 latency（evidenceJson warm p50/p95）+
   `grep api_key mpv-kernel.log` 必须零命中（token 离 URL 取证）。
7. 服务器 B（带密码）：登录+播放一条。
8. defaultHome：SMB 源设默认首页→重启→进 SmbBrowser（误路由修复取证）。
9. `debug.kivo.engine=legacy`：Emby 走 RangeFd 旧内核可播（parity）。
10. REX iPhone 截图对照（pymobiledevice3 抓屏，用户协助点击）：详情版式 + 表单对照图。

证据目录 `docs/evidence/p240/emby-mpv/`（含 `detail/`、`form/`），status.md 收口；**无凭据**。

## 六、收口闸门

#18–#25 证据全绿 + 行数自检全 ≤300 → 新提交推送 → 交接文档更新 → 任务关闭。