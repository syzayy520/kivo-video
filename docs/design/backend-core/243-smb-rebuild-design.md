# 243 · SMB 重构设计（v1）

> 状态：**S1/S2/S3/S4/S5 已完成并真机验证**（S5 于 2026-11-10 07:22 在两台设备上 2/2 PASS 结清）。（用户 2026-11-09 22:23 授权"先规划设计再写"，22:48 批准推进）。
> 前置：220（SMB 直连 MVP，已收口）、220b（SMB 硬解）、221（WebDAV）、224（Emby）、
> `docs/design/ui/library-rebuild/plan.md`（源管理 L4 = SMB 归并）。
> 参考实证：`docs/evidence/p243/smb-reference/`（6 台 iPhone 播放器真机截图，本机 USB 抓取）。
> 实施证据：`docs/evidence/p243/smb-rebuild/status.md`。

---

## 零、结论先行

现有 SMB 不是"有 bug"，是**形态错了三处**，且每一处都在用户每天要走的路上：

| # | 形态错误 | 用户感知 |
| --- | --- | --- |
| 1 | **要求用户填"共享名"** | 不知道填什么就永远连不上；错一个字报 `OBJECT_PATH_NOT_FOUND` |
| 2 | **不是一个"源"，是一个调试面板** | 不能保存、不能编辑、不能删除、不进资源库总览、每次重填密码 |
| 3 | **播放通道三层拼接**（SMB→Kotlin HTTP→HTTP 客户端→fd） | 起播慢、seek 卡、断网后只能退出重来 |

参照物（Rex / EplayerX / Noctra / Forward）**没有一个要求填共享名**：要么只填主机，连接后枚举；
要么一个完整地址 `smb://主机/共享名` 粘进去。本设计按这个共识重写，并把播放通道压成一条。

---

## 一、现状诊断（按代码定位，不按感觉）

### 1.1 P0 · 阻断级

**P0-1 共享名必填，且没有任何枚举能力**

- `SmbConfig.share` 是非空构造参数；`SmbActivity` 测试按钮 `enabled = share.isNotBlank()`。
- `jni_smb2_bridge.cxx:75` 直接 `smb2_connect_share(host, share, user)`——共享名错就整条失败。
- 全仓 **零** `smb2_share_enum_*` 调用（`libsmb2-share-enum.h` 已在 `third_party/` 里躺着）。
- 后果：用户在 Windows 上看到的是"共享文件夹"，在 NAS 上看到的是"共享目录/卷"，没人知道
  要填的是 SMB 的 `netname`。220 收口靠的是**测试时人工知道** `XiaoMi` 这个共享名。

**P0-2 SMB 没有接入源管理**

| 能力 | Emby（已做） | SMB（现状） |
| --- | --- | --- |
| 总览卡片（名称/地址/状态/数量） | ✅ `SourceCard` | ❌ 只有一张静态入口卡 |
| 保存 / 编辑 / 删除 | ✅ `SourceRepository` 事务 + 修订号 | ❌ 不保存 |
| 凭据加密落盘 | ✅ `SourceCipher`（Keystore AES/GCM） | ❌ 明文 JSON 走 Intent |
| 粘贴识别 | ✅ `SourceShareParser` | ❌ |
| 协议字段 | `protocol="emby"`（`SourceStore:39/54` 硬编码） | ❌ 被拒 |

`SourcesScreen.kt:95` 只写了 `StorageEntryCard("SMB", "网络共享") { navigate(onOpenSmb) }` →
跳到 `SmbActivity`。**用户眼里 SMB 和"本地影片""私密区"是同一级入口，不是一个可管理的源。**

**P0-3 两套协议实现并存，旧的一套还在被 7 个文件引用**

| 文件 | 角色 | 状态 |
| --- | --- | --- |
| `smb/SmbSource.kt`（149 行，jcifs-ng） | 220 初版，AC0 已判负 | **旧**，仍导出 `ByteSource` 接口被全仓实现 |
| `smb/Smb2Native.kt`（99 行，libsmb2） | 实际在跑 | 新，但借用旧 `SmbSource.Entry` |
| `smb/SmbSpikeActivity.kt`（268 行） | AC0 选型 spike | **残留**，`AndroidManifest.xml:29` `exported=true` |

引用 `SmbSource.ByteSource` 的文件：`WebDavSource.kt`、`FileByteSource.kt`、`LocalRangeProxy.kt`、
`CastController.kt`、`PlayerComposeActivity.kt`、`SmbActivity.kt`、`SmbSpikeActivity.kt`。
→ 想删 jcifs 就得先把这个接口搬到 `net/`（220 设计稿本来就规划了 `net/ByteSource.kt`，**从没建**）。

**P0-4 播放通道三层拼接**

```
libsmb2  →  JNI(new ByteArray)  →  Kotlin LocalRangeProxy(32 KiB/块, 全局锁)
        →  loopback TCP  →  HttpRangeFile(2×1 MiB 窗口 + 8 MiB 缓存)
        →  ProxyFileDescriptor  →  FFmpeg / MediaExtractor
```

四个具体代价：

1. **放大往返**：一个 1 MiB 的 Range 请求，`LocalRangeProxy:146` 用 32 KiB 缓冲循环 32 次，
   每次一次 SMB2 READ 往返。局域网 RTT 0.3 ms 时，仅协议开销就 ≈ 10 ms/MiB；
   高码率 4K（80 Mbps）需要 10 MiB/s → 每秒 320 次往返。
2. **全局锁**：`LocalRangeProxy::lock` 同时保护 `size()` 与 `readAt()`，**所有连接串行**。
   seek 时 FFmpeg 会新开连接，旧连接还在读 → 新连接排队等锁。
3. **两次跨界拷贝**：`nativeReadAt` 返回 `ByteArray`（JNI 分配 + 拷贝），
   Kotlin 再 `System.arraycopy` 进 socket 缓冲。
4. **缓存太小**：`HttpByteCache` 8 MiB，4K 片源一次 seek 就要重走整条链。

**P0-5 fd 契约不一致，导致"文件提前结束"**

- `SmbSource.ByteSource.readAt` 注释：`EOF 返回 -1`。
- `Smb2Native.NativeByteSource.readAt`：`return data.size`（EOF 返回 **0**）。
- `LocalRangeProxy:152`：`if (n <= 0) break` → 静默截断响应，`Content-Length` 已声明但少发。
- 播放器侧看到的是"连接被服务器关闭"，不是"读失败"。

**P0-6 错误被吞，用户只看到"失败"**

- `LocalRangeProxy:159` `catch (e: Exception) { /* 空 */ }`。
- `HttpRangeFile:203` 把一切 IO 异常压成 `ErrnoException(EIO)`，reason 只剩"网络读取失败（IOException）"。
- libsmb2 的 `smb2_get_error()` 文本（含 NT status）只到 `Smb2Exception.message`，
  没做分类，直接拼进 UI 文案。

### 1.2 P1 · 体验级

| # | 问题 | 位置 |
| --- | --- | --- |
| P1-1 | 表单 6 个字段（主机/端口/共享/路径/用户/密码）语义重叠，"路径"到底是共享内还是共享后没人知道 | `SmbActivity:161-166` |
| P1-2 | 无"名称"，保存后无法辨认；无粘贴识别；无"匿名"显式说明 | 同上 |
| P1-3 | 硬编码初值 `host = "192.168.5.1"`（开发环境 IP） | `SmbActivity:55` |
| P1-4 | 浏览页浅色底 `#F2F2F7` + emoji 图标 📁🎬，与全 app 深色主题和"禁止 emoji 当图标"规范冲突 | `SmbActivity:146, 247` |
| P1-5 | 每行两个彩色按钮（播放/下载），无空目录态、无加载态、无视频过滤、无排序 | `SmbActivity:263-280` |
| P1-6 | 每次播放新建 libsmb2 上下文；浏览一个、播放再一个，不复用 | `PlayerComposeActivity:375` |
| P1-7 | 断线后无重连；只有"连续读失败 >200 次"才报错 | `220-smb-compat-report.md §五点八` |
| P1-8 | 下载桥列表只增不减，`Source` 永不关闭（泄漏连接 + fd） | `SmbActivity:293-306` |
| P1-9 | 无局域网发现（Rex 有"可用地址"胶囊） | 全仓无 mDNS/NSD |

### 1.3 P2 · 可维护性

- `Smb2Native.Source.opened` 声明后从未赋值（死字段）。
- `Source.close()` 不追踪已打开的 `fh`，依赖调用方自觉。
- `jni_smb2_bridge.cxx:103` 手写 JSON 转义只处理 `"` 和 `\`，文件名含控制字符会产出非法 JSON。
- `SmbActivity` 324 行，同时承担表单/校验/连接/浏览/下载 5 个职责 → 违反单文件单职责。
- 零测试（无 instrumented / 无 JVM 单测）。

---

## 二、参考实证（真机截图，本机 USB 抓取）

抓取方式：`pymobiledevice3 developer dvt screenshot`（iPhone13,3 / iOS 27.2 / USB）。
证据目录 `docs/evidence/p243/smb-reference/`。

### 2.1 Rex（`tv.rexnow.app` 1.1）

`rex-add-smb.png`：底部弹层"添加资源库"，标题栏 `×` / 居中标题 / `保存`。

| 元素 | 实测内容 |
| --- | --- |
| **可用地址** | 胶囊 `192.168.5.197` + 第二行 `N1`（IP + 主机名合并展示）= **局域网自动发现** |
| 服务器 | 单行，占位"服务器" |
| 端口 | 单行，占位"端口" |
| 用户名 / 密码 | 各单行，密码带眼睛 |
| 测试连通性 | 居中文字按钮（弱化，不是主按钮） |
| **共享** | **没有这个字段** |
| 脚注 | "添加后会自动接入 Drivemby 生成海报墙：只上传文件夹与文件名，不上传文件内容和密码。" |
| 底部分段 | 连接 / 信息 / iCloud / 高级 |

→ 关键：**发现优先于输入；共享在连接后由系统枚举**。"信息"是名称/备注，"高级"是协议细节，
默认折叠。

#### 2.1.1 Rex 拆解包的硬参数（`C:\Users\Administrator\Pictures\REX\rex\rex\rex`）

来源：`Rex_拆解成果_落地包/strings/zh-Hans__Library.json`（460 键）+ `en__Library.json` 对照 +
`Rex_UI结构复刻说明书.md §6.2` + `Rex_动效参数表.csv`。**不是推测，是 App 内真实字符串。**

**表单键（SMB 实际只用前 5 个）**

| 键 | 中文 | SMB 用 |
| --- | --- | --- |
| `library.form.server` | 服务器 | ✅ 主字段 |
| `library.form.host` / `.hostPlaceholder` | Host / 服务域名或 IP 地址 | ✅（与"服务器"二选一展示，视协议） |
| `library.form.port` | 端口 | ✅ 缺省 445 |
| `library.form.username` | 用户名 | ✅ 留空 = 匿名 |
| `library.form.password` | 密码 | ✅ |
| `library.form.name` / `.namePlaceholder` | 资源库名称 / 资源库名称（自动获取） | ✅ 空则自动 |
| `library.form.remark` | 线路别名 | ⬜ 本卡不做 |
| `library.form.pathPlaceholder` / `.webDAVPathPlaceholder` | 例如 emby / 例如 dav | ❌ **SMB 不用**（只有 WebDAV/Emby 有路径） |
| `library.form.backupServers` / `.backupName` / `.autoSyncLines` | 备用服务器 / 名称 / 自动同步线路 | ⬜ 本卡不做（架构留位） |

→ **决定性证据**：Rex 的 SMB 表单**没有任何"共享/share"键**。SMB 走的是
`服务器 + 端口 + 账号`，共享在连接后由系统列出。与本设计 D1 完全一致。

**状态与文案（可直接复用为 Kivo 文案基线）**

| 键 | 原文 |
| --- | --- |
| `library.form.testingConnectivity` | 正在测试… |
| `library.form.validating` | 正在验证服务器与账号… |
| `library.form.connectivityResult` | 线路连通性 |
| `library.form.unreachableRoute` | 无法连接 |
| `library.form.duplicate` | 该资源库已存在：相同服务器与账号的资源库只能添加一次。 |
| `library.form.savedRouteSwitched` | 已切换至线路：%@ |
| `library.form.savingSettings` | 正在保存设置… |
| `library.error.connectionFailed` | 无法连接服务器，请检查地址与网络。 |
| `library.error.untrustedCertificate` | 不安全的证书 |
| `library.error.saveSettingsFailed` | 保存失败，原配置未被改写。 |
| `library.validation.invalidSMBServer` | 请输入有效的 SMB 服务器地址，端口可选。 |
| `library.validation.invalidURL` | 请输入有效的 http 或 https 服务器地址。 |
| `library.file.loading` / `library.file.match.folderLoading` | 正在读取目录… |
| `library.copy.this.directory.has.no.subfolders.or.video.files.supported.by` | 这个目录中没有子目录或 Rex 支持的视频文件。 |
| `library.empty.title` / `.subtitle` | 还没有资源库 / 添加媒体服务器后，可以在这里集中浏览。 |
| `library.action.retryConnection` | 重试连接 |
| `library.drivemby.shareMissing` | 需要先在本设备添加这个共享，才能播放这个片源。 |

**三条能直接抄的产品规则**

1. **去重键 = 服务器 + 账号**（`library.form.duplicate`）——不是"名称"，也不是"地址字符串"。
   与 `SourceRepository.sameAccount(address, username)` 同构，可直接复用。
2. **粘贴识别的目标是"线路信息"，不是"地址"**：`library.form.smartRecognitionPlaceholder`
   = "粘贴包含链接、用户名和密码的线路信息" → 解析器要能从一段文本里同时抠出
   host / port / user / password，而不只是 URL。
3. **共享是独立实体**（`library.drivemby.shareMissing`）：Rex 允许"这个共享还没在本设备添加"
   这种状态存在。→ 我们的数据模型里 `share` 必须是源记录的一等字段，不能塞进 URL 字符串里。

**动效硬参数（表单 sheet）**

| ID | 场景 | 时长 | 缓动 | 其它 |
| --- | --- | --- | --- | --- |
| M05-S01 | Sheet 升起 | 0.35 s | `spring(0.35, 0.85)` | 遮罩 0→0.4 同步 |
| M04-S03 | 分组展开收起 | 0.3 s | `easeInOut` | 子项 stagger 0.02×i |

**库类型枚举**（说明书 §6.2）：Emby / Jellyfin / WebDAV / **SMB（`AMSMB2.framework`）** /
百度网盘 / 阿里云盘 / Drivemby / 本地文件 / Trakt —— SMB 与其它来源**平级**，走同一个"添加"入口。

### 2.2 EplayerX（`com.snowlee.eplayerx`）

`eplayerx-smb-form.png`：底部弹层，`←` / 居中 `SMB` / 右上 `连接`。

| 元素 | 实测内容 |
| --- | --- |
| 名称 | 单行 |
| 备注 | 占位"可选" |
| **地址** | 单行，占位 **`smb://nas.local/video`** ← **一个字段装下主机+共享+路径** |
| 用户名 | 占位"可选" |
| 密码 | 单行 |
| 脚注 | "可省略协议前缀。WebDAV 默认使用 HTTPS，未填写路径时访问服务器根目录。填写的路径保持不变。匿名访问可留空用户名，可分别设置各客户端的 iCloud 同步。" |

背景（列表页）：卡片 `本地播放 · 未观看 · 文件`、`予初 Emby · 1个月前 · 主线路 · 电影: 0 剧集: 0`，
右上三图标：链接、`+`、锁。

→ 关键：**URL 即配置**；匿名 = 用户名留空（不是开关）；卡片带"线路"与分类计数。

### 2.3 Noctra（`cn.zijiu522.noctra`）

`noctra-add-smb.png`：全屏页，大标题「添加 SMB」，副标「连接局域网共享文件夹」。

| 分组 | 字段 |
| --- | --- |
| 来源信息 · **选填** | 名称（显示名称）、备注（用途或其他说明） |
| 共享地址（必填） | **完整地址**，占位 `smb://主机/共享名` |
| 访问账号 · **选填** | 用户名（占位"留空表示匿名访问"）、密码（访问密码，眼睛） |
| 主按钮 | 添加 SMB |
| 脚注 | "文件来源不提供影片资料，观看进度保存在本机。" |

→ 关键：**"选填"直接写在分组标题上**，用户扫一眼就知道哪些可以跳过。

### 2.4 Forward（`flux.inchmade.app`）

`forward-add-source.png`：弹层「添加资料库」，副标"Forward 可同时添加多种资源库，并进行内容聚合。"

列表（图标 + 名称整行）：123 云盘 / 光鸭云盘 / OneDrive / 阿里云盘 / **WebDAV / Alist** / **SMB** /
本地文件夹 / 飞牛 / **从其他设备导入**。

→ 关键：**SMB 与其它来源平级**，走同一个"添加"入口；有"从其他设备导入"（迁移能力）。

### 2.5 缺口（诚实记录）

- Vidora 的添加弹层只有 Plex / Emby / Jellyfin / MP，**本轮未找到 SMB 入口**（用户确认"有些没有 SMB"）。
- DenPlayer 停在"我的"页，未进到来源管理。
- **未抓到的部分**：连接成功后的"共享列表"页、目录浏览页、播放页的实际布局。
  这三页需要在实现前补一次真机观察（每台 30 秒，见 §六 S0 闸门）。

### 2.5.1 补充实证：共享枚举在本机网络里真的能work（2026-11-09 22:52）

用户环境里有两台 SMB 服务器，端口都在线（TCP 445 OPEN）：

| 服务器 | 端口 | 共享枚举结果 |
| --- | --- | --- |
| `192.168.5.1`（小米路由） | 445 OPEN | ✅ **6 个磁盘共享** |
| `192.168.5.197`（Rex 显示主机名 `N1`） | 445 OPEN | ⚠️ `net view` 报 1707（网络地址无效）→ 该实现不走 SMB1 的 ShareEnum，需 SMB2 `IPC$` 路径验证 |

小米路由实测输出（`net view \\192.168.5.1`，Windows 11）：

```
在 \\192.168.5.1 的共享资源
Share from router
共享名      类型  使用为  注释
TDDOWNLOAD  Disk
下载        Disk
共享        Disk
图片        Disk
文档        Disk
视频        Disk  N:
音乐        Disk  O:
命令成功完成。
```

**这条输出就是 P0-1 的铁证**：服务器一直愿意把 6 个共享名告诉客户端，
而我们让用户手打其中一个（220 的验收就是这么过的——测试者事先知道叫「视频」）。
用户打错一个字就 `OBJECT_PATH_NOT_FOUND`，且错误文案不会告诉他"其实有 6 个"。

→ D1 不是"优化"，是把服务器已经给的信息显示出来。S0 只剩"在 K40 上用 libsmb2 复算"
这一件事（同一台路由，`IPC$` + `smb2_share_enum_sync`）。

### 2.6 与既有拆解的一致性

| 来源 | 结论 | 与本设计的关系 |
| --- | --- | --- |
| SenPlayer（`03-senplayer-teardown.md`） | libsmb2 6.2 + 本地 HTTP Range 代理喂 FFmpegKit | 库选型一致；**代理层不复刻**（见 D3） |
| REX（`04-product-blueprint.md` S3） | 验收假设"1 GB 起播与本地差 ≤ 2 s" | 沿用为 AC |
| Noctra README | "多服务器、多账号；快速添加自动识别剪贴板地址；配置导入导出" | 采纳：粘贴识别、多源、导入导出后置 |

---

## 三、目标设计

### 3.0 一句话架构

**一个 SMB 源 = 一条连接策略 + 一次枚举出来的共享 + 一个能直接喂 fd 的随机读源；
UI 只认"源"，不认协议。**

```
             ┌──────────────────── 源管理层（已有，扩协议） ─────────────────┐
用户操作 ──► │ SourcesScreen → SmbSourceForm → 验证 → 枚举共享 → 保存(加密)   │
             └──────────────────────────┬────────────────────────────────────┘
                                        │ SourceRecord(protocol=smb, address=smb://host/share)
                                        ▼
             ┌──────────────────── 协议层（重写） ───────────────────────────┐
             │ SmbSession：连接/枚举/浏览/打开（独占线程，串行）             │
             │ SmbByteSource：pread + 1 MiB 预读窗口（实现 net.ByteSource） │
             └──────────────────────────┬────────────────────────────────────┘
                                        │ readAt(offset,len) / size()
                                        ▼
             ┌──────────────────── 传输层（抽取，复用） ─────────────────────┐
             │ RangeFd：ByteSource → ParcelFileDescriptor（8 MiB 共享缓存） │
             └──────────────────────────┬────────────────────────────────────┘
                                        │ fd
                                        ▼
                         FFmpeg（软解）/ MediaExtractor（硬解）
```

**删除**：`LocalRangeProxy` 在 SMB 播放路径上的位置（保留给 DLNA 投屏，见 D6）。

### 3.1 决策

**D1 · 共享枚举取代"共享名输入"（对应 P0-1）**

- 表单只收 `主机`（或完整 `smb://host`），不要求共享名。
- 连接流程：`smb2_connect_share(ctx, "host:445", "IPC$", user)` →
  `smb2_share_enum_sync(ctx, SMB2_SHARE_INFO_1)` → 过滤 `type & 1 == 0`（磁盘共享）→
  列出 `netname` + `remark` → 用户选 → 断开 IPC$ → 连目标共享。
- 若用户直接粘了 `smb://host/share/path`，**跳过枚举**直奔该共享（少一次往返）。
- **枚举失败或列表为空时，保留"直接输入共享名"入口**（见下方修订）。

> **2026-11-09 23:05 真机修订（必须遵守）**：K40 + 小米路由实测，匿名账号枚举**只返回 1 个共享**
> （`XiaoMi`），而服务器实际有 6 个；`net view` 用 PC 的已认证会话才看到全部。
> 说明共享级 ACL 会按账号裁剪枚举结果。所以"枚举"不能**取代**手填，只能**优先**：
>
> - 枚举出 ≥1 个 → 列表选择为主，底部保留"手动输入共享名"；
> - 枚举出 0 个 → 直接落到手动输入，文案说明"服务器没有列出共享，请直接填写共享名"；
> - 用户粘贴了完整地址 → 不走枚举。
>
> 证据：`docs/evidence/p243/smb-rebuild/s0-enum-xiaomi-r4.txt`。
> 同一轮还发现 `share=视频` 匿名被拒（220 时期可连），属路由器权限变更，不是代码回归
> （`s0-enum-video-r3.txt`）。

**D2 · 地址是一个字段，不是一个表单（对应 P1-1/P1-2）**

采纳 EplayerX/Noctra 形态，字段集与 Rex 的 `library.form.*` 对齐：

```
名称（选填，空则自动：share → host）
服务器（必填）  host / host:port / smb://host[:port][/share][/path]
用户名（选填，留空 = 匿名）
密码
```

- 解析器 `SmbAddress`（纯函数，不联网）：接受 `smb://host/share/path`、`host/share`、
  `\\host\share`；去掉末尾 `/`；`#`/`?`/空格原样保留在 path 里（SMB 文件名可含）。
- 端口缺省 445；非 445 才在字段里显式显示。
- 粘贴识别覆盖**线路信息**（Rex `smartRecognitionPlaceholder`）：一段文本里同时有
  `smb://…`、`user`、`password` 时，三样一起填；只识别不改其它字段，不自动连接。

**D2b · 去重与命名（Rex `library.form.duplicate` 同构）**

- 去重键 = **规范化服务器地址 + 用户名**（不是名称、不是原始输入串）。复用
  `SourceRepository.sameAccount`，只在 `protocol` 相同时比较。
- 重名允许（不同服务器可以有同名资源库），靠副标题（地址）区分。
- 同服务器不同共享 = **不同的源**？不是。本卡：**一个源 = 一台服务器**，共享在源内切换
  （`SmbSession` 持有共享列表，浏览时选）。这样"添加一次，看所有共享"。


**D3 · 播放通道压成一条：SMB 直接产 fd，不过 HTTP（对应 P0-4/P0-5）**

`HttpRangeFile` 已经把 fd 语义做对了（非阻塞、短读填满、共享缓存、可取消），
它缺的只是"数据从哪来"。把它拆成两层：

| 新文件 | 职责 |
| --- | --- |
| `net/RangeFd.kt` | `ByteSource → ParcelFileDescriptor`：fd 生命周期、`ProxyFileDescriptorCallback`、短读填满、取消、错误分类（从 `HttpRangeFile` 抽取，行为不变） |
| `net/HttpRangeFile.kt` | 保留：`RangeFd` + HTTP Range 传输（Emby/WebDAV 用） |
| `smb/SmbByteSource.kt` | 实现 `net.ByteSource`：libsmb2 `pread`，**1 MiB 预读窗口 + 顺序读命中** |

为什么不再需要 `LocalRangeProxy`：

- 它的存在理由是"FFmpeg 只会读 path/fd，硬解只能吃 fd"（220 D2）。**fd 我们直接给**，
  不需要伪装成 URL。
- SenPlayer 需要它，是因为 FFmpegKit 不带 `libsmbclient`；我们有自己的 JNI。

收益（可测）：

| 指标 | 现状 | 目标 |
| --- | --- | --- |
| 每次读的 SMB2 READ 往返 | 1 次 / 32 KiB | 1 次 / 1 MiB（预读） |
| 跨界拷贝 | 2 次 | 1 次（JNI 直接写 ByteArray 由 fd 层复用） |
| 并发 | 全局锁串行 | 读与浏览分上下文，互不阻塞 |
| seek 后首帧 | 重新走整链 | 窗口命中直接返回 |

**D4 · 一个源 = 一个 `SmbSession`，浏览与播放各自连接（对应 P1-6）**

libsmb2 的 `smb2_context` 不是线程安全的。策略：

- `SmbSession` 持有一条**控制连接**（浏览/枚举），独占单线程串行。
- 每次播放另开一条**数据连接**（`SmbReader`），fd 回调线程独占。
- 两条连接共享同一份 `SmbCredential`（不可变），不共享 context。
- 关闭顺序：先 `cancel()` 打断阻塞读 → 再 `destroy` → 最后放 fd。

**D5 · 接入源管理，`protocol` 字段落地（对应 P0-2）**

- `SourceStore` schema 从 1 升到 2：`protocol ∈ {emby, smb}`；**读 v1 记录按 emby 解释**（向后兼容，
  不丢用户已有配置）。
- `SourceRecord.username` 允许为空（SMB 匿名）；Emby 侧保持"必填"校验（在校验层，不在构造器）。
- `SourceAddress` 增 `smb` 分支：允许 `smb://host[:port][/share][/path]`，仍拒绝 query/fragment/userinfo。
- 凭据复用 `SourceCipher`（Keystore AES/GCM，`noBackupFilesDir`）——**不再让密码进 Intent**。
  播放页只收 `sourceId`，自己去存储里取。
- 卡片状态：`已保存` / `连接中` / `在线（N 项）` / `离线（原因）`。不做假在线。

**D6 · `LocalRangeProxy` 只服务 DLNA 投屏（对应 P0-3）**

DLNA 渲染器要的是**真 HTTP URL**，那里必须保留一个 HTTP 服务。
但它应当：直接持有 `ByteSource`（不经 SMB 播放路径）、按需绑定 `0.0.0.0`、
只处理 `GET/HEAD`、不参与播放。

**D7 · 错误分段，文案面向用户（对应 P0-6）**

`SmbError` 分类 → 文案：

| 阶段 | 判据（libsmb2 / errno） | 文案 |
| --- | --- | --- |
| 地址解析 | 解析失败 | 请输入有效的 SMB 地址，例如 smb://192.168.1.10/视频 |
| 连接 | `ECONNREFUSED` / 超时 / DNS | 找不到服务器，请检查地址和网络 |
| 认证 | `STATUS_LOGON_FAILURE` / `STATUS_ACCESS_DENIED`（会话阶段） | 账号或密码不正确 |
| 权限 | `STATUS_ACCESS_DENIED`（树连接阶段） | 这个共享不允许当前账号访问 |
| 共享不存在 | `STATUS_OBJECT_PATH_NOT_FOUND` | 共享「X」不存在，请重新选择 |
| 目录 | `smb2_opendir` 失败 | 无法读取文件夹「X」 |
| 播放中断 | 连续读失败 | SMB 连接中断，已暂停；恢复网络后可继续 |

原始 NT status 进日志与证据 JSON，**不进**用户文案（可展开"详情"看到）。

**D8 · 局域网发现用系统 NSD，不引第三方库**

`NsdManager` 浏览 `_smb._tcp`，回调得到 `host / port / 服务名` → 渲染成"可用地址"胶囊
（对齐 Rex）。同时保留手工输入。发现结果**不自动连接**，只做候选。

### 3.2 家族树（自然结构，单文件单职责）

**工作目录约束（不可协商）**

- 一律在 `C:\kivo video` 这一个仓库内改动，**不新建 Gradle module、不新建 source set、
  不新建项目包、不把任何文件写到仓库外**。
- 不新增空目录；目录只因为要有文件才存在。
- 每个文件只承担下表列出的**一个**职责；一个文件说不清职责就是拆分信号。
- 行数预算：新建文件 ≤ 220 行；存量文件被改后不得比改动前更长（除明确标注的迁移例外）。
  超过就拆，不允许"先堆着以后再说"。

```
android/app/src/main/java/com/kivo/cinema/
  net/                            # 传输层：只认 ByteSource，不认 SMB
    ByteSource.kt         新  ≤40   随机读源契约（size/readAt/close）；唯一跨协议接口
    RangeFd.kt            新  ≤200  ByteSource → ParcelFileDescriptor（fd 生命周期/短读填满/取消/错误分类）
    HttpRangeFile.kt      改        = RangeFd + HTTP Range 传输（Emby/WebDAV 用），行为不变
    HttpByteCache.kt      不改      8 MiB 共享缓存
    LocalRangeProxy.kt    改  ≤150   仅 DLNA 投屏；直接持 ByteSource；异常外抛（不再空 catch）

  smb/                            # 协议层：只认 libsmb2，不认 UI
    SmbAddress.kt         新  ≤120  纯解析：字符串 ↔ host/port/share/path（无网络、无 Android 依赖）
    SmbEntry.kt           新  ≤40   目录条目值对象（name/isDir/size/mtime）
    SmbShare.kt           新  ≤30   共享条目值对象（name/remark/isDisk）
    SmbError.kt           新  ≤120  libsmb2/errno → 分类 → 用户文案（D7 表）
    SmbSession.kt         新  ≤200  控制连接：连接/枚举共享/浏览/关闭（单线程串行，独占 context）
    SmbReader.kt          新  ≤160  数据连接：open/size/pread/close（每条播放一个，独占线程）
    SmbByteSource.kt      新  ≤180  SmbReader + 1 MiB 预读窗口 → 实现 net.ByteSource
    SmbCredential.kt      新  ≤40   不可变凭据（host/port/user/pass/domain），不 toString 明文
    native/Smb2Native.kt  改  ≤140  JNI 声明（唯一 native 入口；补 shareEnum/stat/close）
    ui/
      SmbSourceForm.kt    新  ≤220  添加/编辑表单（名称/服务器/账号 + 测试连通性 + 粘贴识别）
      SmbSharePicker.kt   新  ≤140  枚举结果 → 选择共享（含"手动输入共享名"兜底）
      SmbBrowser.kt       新  ≤220  目录浏览 + 视频过滤 + 点开起播 + 上一级 + 空态
      SmbBrowserModel.kt  新  ≤180  浏览状态机（路径栈/加载/错误/取消），与 Compose 解耦

  sources/                        # 源管理层（已有，扩协议）
    SourceRecord.kt       改        + protocol 字段；username 允许空（校验下沉到表单）
    SourceAddress.kt      改        + smb:// 分支（仍拒绝 query/fragment/userinfo）
    storage/SourceStore.kt 改       schema=2；读 v1 按 emby 解释，写 v2
    ui/SourceTypeSheet.kt 改        SMB 与 Emby 并列，走同一"添加"入口

  ui/compose/
    PlayerComposeActivity.kt 改     只收 sourceId；删 kivo.smb 明文 Intent；删本地代理分支

android/app/src/main/cpp/jni/
    jni_smb2_bridge.cxx   改        + share_enum_sync；+ stat；错误码透出；去掉手写 JSON

android/app/src/main/
    AndroidManifest.xml   改        删 SmbSpikeActivity / SmbActivity 条目

（删除）
    smb/SmbSource.kt               jcifs 旧后端            → S1 删
    smb/SmbSpikeActivity.kt        268 行 spike 残留       → S1 删
    smb/SmbActivity.kt             324 行调试面板          → S4 由新表单/浏览页替换后删
```

**依赖方向（单向，不许回环）**

```
ui/*  ──►  smb/*（会话/浏览）  ──►  net/ByteSource
  │                                  ▲
  └──►  sources/*（源记录/存储）      │
                              smb/SmbByteSource ┘
smb/*  不许 import sources/*；sources/* 不许 import smb/ui/*；
net/*  不许 import smb/*（除 SmbByteSource 实现 ByteSource）。
```

**为什么不是"一个大 SmbClient"**

现行 `Smb2Native.Source` 一个类同时管连接、列目录、开文件、读字节（99 行 4 职责），
`SmbActivity` 324 行 5 职责。本树按"**连接（控制）/ 读取（数据）/ 解析 / 错误 / 文案**"
切成 5 个族，每个族里再按上文单文件切。任何一处要改（比如换预读策略），只动一个文件。

**不新增的东西**

- 不新建 `smb/ui` 之外的 UI 包；不新建 `smb2/`、`smbclient/` 等平行包。
- 不引入新的第三方 SMB 库（jcifs 删掉，不自研协议）。
- 不新增 Gradle 依赖（NSD 用系统 API，JSON 用 `org.json`）。


### 3.3 与既有卡的关系

| 卡 | 关系 |
| --- | --- |
| 220 | 协议与库选型沿用（libsmb2、不做 SMB1）；**UI 与代理层被本卡取代** |
| 220b | 硬解-over-SMB 由本卡 D3 直接打通（fd 天然可喂 MediaExtractor），无需另开卡 |
| 221 WebDAV | 复用 `RangeFd`，`HttpRangeFile` 行为不变 |
| 224 Emby | 播放链路不变；源管理字段扩展需回归 |
| 222 媒体库 | SMB 源进入总览后，可作为内容来源被索引（后置） |
| library-rebuild L4 | 本卡即 L4 的落地实现 |

---

## 四、分阶段与闸门

| 阶段 | 工作 | 完成判据 |
| --- | --- | --- |
| **S0 补证** ✅ | ①参考截图（4 台 App 添加页）；②共享枚举的服务器侧实证；③K40 上复算 `IPC$` + `smb2_share_enum_sync` | 已完成：枚举出 `XiaoMi`，与服务器匿名可见范围一致（见 §2.5.1 修订） |
| **S1 契约与清理** ✅ | `net/ByteSource.kt`；7 个引用点迁移；删 `SmbSource.kt`（jcifs）、`SmbSpikeActivity.kt`、manifest 条目、jcifs/slf4j 依赖；删死字段 | 已完成：`grep SmbSource` 归零；`assembleDebug` 通过 |
| **S2 协议层** ✅ | `SmbAddress` / `SmbCredential` / `SmbShare` / `SmbError` / `SmbSession` / `SmbShareLister` / `SmbReader` / `SmbByteSource`；JNI 补 `nativeListShares`、`nativeReadInto`、`nativeNtStatus` | 已完成：地址解析 12/12、真机枚举+浏览+1 MiB 首尾读 3/3 |
| **S3 播放通道** ✅ | `RangeSource` / `RangeFd`；`HttpRangeFile` 实现 `RangeSource`；SMB 直连 fd；删 SMB 路径的 `LocalRangeProxy`。**补充**：`RangeFd` 加 20 s 耐心窗口，`SmbByteSource` 改成有退避的持续重连 | 已完成：19.5 GB HEVC 起播 + 1:1 推进 + 1 MiB 随机读；**断网 10 s 后自动接上继续播（4/4 PASS）**；seek 归因于 MKV demux 既有缺陷（本地 MKV 同样位置恒 0），另开 244 → **244 已于 2026-11-10 CLOSED**：根因是 H.264-in-MKV 缺 `h264_mp4toannexb` 转换（起播恒 0）+ 视频-only seek 时 `SystemClockSource` 未 rebase（进度不跳），证据见 `docs/evidence/p244/` |
| **S4 源管理** ✅ | `SourceStore` schema=2（兼容读 v1）；`SourceRecord.protocol`；`SmbBrowserActivity`（读 sourceId → 解密 → 浏览 → 起播）；表单双协议；删除 `SmbActivity`/`SmbConfig`/`kivo.smb` 明文通道 | 已完成：写读连 3/3、源卡→浏览→4K 播放全通；**分组归属与"点文件起播"两跳未复拍**（设备息屏） |
| **S5 发现** ✅ | `NsdManager` `_smb._tcp` 浏览 → 可用地址胶囊（`SmbDiscovery` + 表单集成） | **真机验收通过（2026-11-10 07:22）**：K40 与平板均 **2/2 PASS**，均只发现 `192.168.5.197:445 N1`；K40 早年那次三次 `Process crashed` 是**息屏**导致，唤醒后复跑即通过。小米路由不广播 `_smb._tcp`（与 PC 侧 zeroconf 结论一致），故路由器只能走枚举/手填 |
| **S6 验收** | 全量回归 + 证据归档 | 见 §五 |

### 验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 零共享名也能连 | 只填 `smb://192.168.5.1` + 空账号 → 出现共享列表 ≥1 项 → 选中进目录 |
| AC2 | 匿名 | 用户名密码全空可连（小米路由） |
| AC3 | 账号 | 有密码共享可连（Windows 共享，真实账号） |
| AC4 | 持久化 | 保存后杀进程重开，不重输密码即可浏览；密码不以明文出现在任何文件/日志 |
| AC5 | 起播 | 1 GB 片源 SMB 起播 − 本地起播 ≤ 2 s（沿 220 AC5 口径） |
| AC6 | seek | 4K 片源 seek 落点正确；seek 后 3 s 内出画 |
| AC7 | 吞吐 | 顺序播放期间 SMB READ 请求中位大小 ≥ 512 KiB |
| AC8 | 断线 | 播放中断网 → 5 s 内 UI 出现"连接中断，已暂停"；恢复网络后可继续（不退出重来） |
| AC9 | 错误文案 | 错密码/错共享/服务器关机/无权限 四种场景文案各不相同且不含 NT status 原文 |
| AC10 | 无残留 | 仓库内无 jcifs 依赖、无 `SmbSpikeActivity`、无 `SmbSource`（旧） |

---

## 五、明确不做（本卡）

- SMB1（220 已定：只 SMB2/3）。
- SMB 写路径（上传/改名/删除）——只读浏览与播放。
- 海报墙/刮削（222 的职责；SMB 只提供文件）。
- 多线路/备用地址（Noctra 有，本期不做）。
- 配置导入导出（同上）。
- 后台扫描全库（222 后置）。
- 硬解-over-SMB 的**性能调优**（本卡只保证 fd 可用，不承诺 4K 硬解指标）。

---

## 五之二、后续卡（本卡发现、明确不在本卡做）

| 编号建议 | 内容 | 为什么不在本卡 |
| --- | --- | --- |
| **244** ✅ **已修** | **H.264 + MKV 黑屏 / 位置恒 0**：根因是 `ffmpeg_demux_source.cxx::select_stream()` 只给 HEVC 装了 `hevc_mp4toannexb`，H.264 缺 `h264_mp4toannexb` —— 样本是 AVCC，MediaCodec 要 Annex-B，于是 avc 解码器 configure 成功却吞掉全部样本（`samples=600 / frames=0 / speed=37.83x / completed=1`）。修复后 `frames=313 / first_frame=15.8ms / completed=0`，画面与时间码正常；HEVC MKV 与 MP4 回归通过。证据 `docs/evidence/p244/`（修复前后各两张截图 + 帧账） | 仍需在 SMB 上复验 H.264 MKV（当前匿名被拒） |
| **245** | **断网恢复的两处体验补齐**：①恢复时跳过约 16 s 内容（原生播放时钟没停，需要会话层暂停时钟）；②停顿期间 UI 仍显示"正在播放"，缺"正在重连…"缓冲态。**②已实现**：`RangeSource.stalled` → `NetworkPlaybackSession.buffering` → 播放页文案"网络中断，正在重连…"；①未做（改时钟风险高，且 `pause()` 可能与阻塞中的解码线程互等） | ①需要改 `NetworkPlaybackSession` 状态机与原生时钟；②属于本卡收口时的可见性修补 |
| **246** | **mDNS 发现真机验收**：`SmbDiscoveryInstrumentation` 在息屏后台被 MIUI 拦截（`Process crashed`），需在真实表单里看"可用地址"胶囊 | 纯验收，无代码改动；待设备解锁可用时补拍 |

---

## 六、风险

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| 小米路由的 `IPC$` 枚举被拒（220 曾见 `TREE_CONNECT` 拒绝） | 共享列表拿不到 | S0 先验；失败则降级为"手填共享名 + 明确提示"，不阻断 |
| libsmb2 预读窗口与 SMB2 大读上限（服务器可能只给 64 KiB/次） | 预读无收益 | `smb2_set_max_read_size` 协商；实测记录每服务器实际块大小 |
| `ProxyFileDescriptorCallback` 在部分 ROM 上线程模型不同 | 读阻塞 | 已有 `HttpRangeFile` 真机验证过（Emby 路径），复用同一实现 |
| schema 升级读坏老记录 | 用户 Emby 配置丢失 | v1→v2 只读兼容；写入前先读取成功才落盘；失败保留原文件 |
| 删 `SmbSource` 时漏改引用 | 编译失败 | S1 单卡单提交，`grep` 计数为 0 才收口 |

---

## 七、证据归档

- 参考截图：`docs/evidence/p243/smb-reference/`（6 张，见 `status.md` 的抓取清单）
- 服务器侧枚举：`docs/evidence/p243/smb-reference/net-view-192.168.5.1.txt`
- 真机证据：`docs/evidence/p243/smb-rebuild/`（S0/S2/S3/S4 各自 JSON + 截图）

---

## 八、实施纪律（对齐 `.codex/memory/kivo-video-iron-gate.md`）

1. **同一工作目录**：所有改动落在 `C:\kivo video`；不新建项目包、不新建 Gradle module、
   不新建 source set、不把文件写到仓库外。
2. **一卡一提交**：S1…S5 每个阶段一个提交，收口前跑门禁（`git status` / `grep` 残留 /
   `assembleDebug` / 相关 instrumented 测试）。
3. **单文件单职责**：新增文件超过 §3.2 的行数预算必须先拆；存量文件改动不得顺手塞无关逻辑。
4. **旧代码先删后加**：`SmbSource` / `SmbActivity` / `SmbSpikeActivity` 在 S1 一次删净，
   **不允许"新旧并存过渡期"**（这正是当前 P0-3 的成因）。
5. **没有证据不说完成**：每个 AC 必须有 `docs/evidence/p243/` 下的可复算产物。
6. **不静默降级**：枚举失败、预读无效、硬解不可用，都要有非空 reason 且用户可见。
