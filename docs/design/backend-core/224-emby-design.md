# 224 · Emby/Jellyfin 对接 —— 设计稿

> 状态：**设计完成，实现待开工**。M2 最后一张卡。
> 卡片原文：Emby/Jellyfin 对接（学 Rex Drivemby：扫描/入库/webhook 通知）——账号 + 媒体库浏览 + 流播。
> 测试服务器（用户真实环境）：`https://emby.bangumi.ca:443`，Emby **4.8.0.62**「nyamedia」，
> 账号 `syzayy`（空密码）。公开端点 `/emby/System/Info/Public` 已从 PC 实测 200（2026-11-08）。

---

## 一、一句话目标

**填服务器地址 + 账号 → 登录 → Emby 海报墙（真服务器真条目）→ 点条目硬解直连流播——
复用 220b 网络管线与 222 海报墙，M2 网络源矩阵收官。**

## 二、参照证据（禁止推断，逐条落点）

| 来源 | 证据 | 本卡落点 |
| --- | --- | --- |
| Rex 拆解 §6.2（复刻说明书 L236） | 库类型枚举：**Emby / Jellyfin** 列首位（与 WebDAV/SMB 并列为一等库类型） | Emby 是客户端功能，不是 Drivemby 那种托管服务 |
| Rex 拆解 §6.2 | Drivemby = Rex 自家云端整理服务（email 绑定 + webhook 通知 + `progress.waiting/recognizing/uploading/done`）——**不是 Emby 客户端协议** | 扫描/整理/webhook **不属本卡**（见 §六明确不做） |
| SenPlayer 拆解 03（L96-100） | MLibKit 的 `dolbyVisionProfile/Title` 等元数据字段为 **Emby 风格 API** ——SenPlayer 消费 Emby 服务器的间接实证 | 元数据字段名对齐 Emby REST |
| Emby 官方 REST（服务器 4.8.0.62 实测钉死） | `POST /Users/AuthenticateByName` → `AccessToken/User.Id`；`GET /Users/{uid}/Items`；`/Items/{Id}/Images/Primary`；`GET /Videos/{Id}/stream?static=true` 原生支持 HTTP Range | 协议骨架 |
| 220b 收口（`NetworkPlaybackSession`） | MediaExtractor(http URL) → MediaCodec(Surface) 硬解，WebDAV 221 已验证同管线 | D3 起播零新增解码代码 |
| 222 收口（`MediaLibraryActivity`） | Rex 2:3 海报墙 + 时长徽标 + LazyVerticalGrid(3) | D4 浏览态复用同布局 |

## 三、设计决策

### D1 鉴权：AuthenticateByName，零第三方
`POST /emby/Users/AuthenticateByName`，头 `X-Emby-Authorization: MediaBrowser Client="kivo", Device="kivo-video", DeviceId="<build.fp>", Version="1.0"`，体 `{"Username":…,"Pw":…}`（**空密码 = `Pw:""`**，用户服务器即此形态）。响应取 `AccessToken` + `User.Id`，存 Activity 内存（v1 不持久化）。

### D2 浏览：Items → 海报墙
`GET /emby/Users/{uid}/Items?IncludeItemTypes=Movie&Recursive=true&SortBy=DateCreated&SortOrder=Descending&Limit=60&Fields=ProductionYear&ImageTypeLimit=1` → `Items[]`（`Id/Name/ProductionYear/ImageTags.Primary`）。海报 `GET /emby/Items/{Id}/Images/Primary?maxWidth=320&tag={tag}`。布局 = 222 的 Rex 海报墙（2:3 卡，Grid 3 列），标题 + 年份徽标。

### D3 起播：直连流喂 220b
`GET /emby/Videos/{Id}/stream?static=true&api_key={token}`（Emby 原生 HTTP Range，直连 = DirectPlay）。URL 直接进 `NetworkPlaybackSession`（同 221 WebDAV）。`PlaybackInfo` 协商 v1 不做——`static=true` 直连失败时**报 reason 不静默降级**（G3 红线：转码回退禁止）。

### D4 UI：Rex 0114 表单规格（220 同款）
`EmbyActivity`：FORM 态（服务器地址/用户/密码三行 + 测试连接按钮，白圆角行浅色底，与 SmbActivity 同规格）；连上即进海报墙态。错误 reason 用户可见（G9）。

### D5 HTTPS 与明示边界
manifest `usesCleartextTraffic` 已开（220b）；HTTPS 走系统信任链。**自签证书 v1 不做绕过**（用户服务器为有效证书域）；失败报 reason 明示。

## 四、家族树（未实现，先树后码）

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `emby/EmbyItem.kt` | **新** | 条目数据类（id/name/year/posterUrl/type） |
| `emby/EmbyClient.kt` | **新** | REST 客户端：login/items/streamUrl（HttpURLConnection，零第三方） |
| `emby/EmbyActivity.kt` | **新** | FORM 态 + 海报墙态 + 起播转发 |
| `AndroidManifest.xml` | 旧（小改） | 注册 EmbyActivity |
| `library/MediaLibraryActivity.kt` | 旧（小改） | "Emby" 入口按钮（与 SMB 按钮同款，S135/L159 对位） |
| `ui/compose/PlayerComposeActivity.kt` | 旧（**零改动**） | 复用现成 `kivo.playurl/kivo.label/kivo.routing` 分支（L203-209）——设计时预估要小改，实测零改 |

## 五、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 登录 | 真服务器 443 AuthenticateByName → 200 + AccessToken（空密码场景实测） |
| AC2 | 浏览 | 海报墙渲染真实条目（名称/年份/海报位图），≥10 项 |
| AC3 | 流播 | 点条目 → 硬解直连流起播（首帧可见 + pts 节拍 1.00×） |
| AC4 | 证据 | `verify_p224_emby.py` 校验证据链 + 变异自检 |

## 六、明确不做（v1）

剧集树/详情页（Rex §6.4 留 v2）· 观看状态回写（Watched/Favorite）· 字幕/音轨多选 ·
转码协商与码率切换 · webhook/整理/上传（**Rex Drivemby 属其自家云服务，非 Emby 协议，永久排除出本卡**）·
Jellyfin 独立验证（协议同源，实测算 bonus）· 持久化账号（明文存储风险，v2 随私密区一起做）。
