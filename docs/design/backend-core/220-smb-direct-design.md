# 220 · SMB 直连 —— 设计稿 v2（实证版）

> 状态：**设计完成，实现未开工**。M2 第一张卡。v1（f3dd844）被用户叫停——
> "功能参照 SenPlayer 来，UI 参照 Rex"。v2 每条决策都挂在两个拆解源的证据上。
> 依赖 211 软解兜底 ✓（已收口）。

---

## 零、参照物实证（本稿的地基）

### 0.1 SenPlayer 侧（功能参照）—— 本轮深拆新证据

拆解产物 `build/rea-senplayer/raw/`（dex 三表 + libMLibKit.so 符号/字节串扫描，2026-11-07）：

| # | 实证 | 证据 |
| --- | --- | --- |
| S1 | SMB 是**独立 native 模块**：`com.mlibkit.smb.SMBClient`（dex 单类壳，实现经 RegisterNatives 在 libMLibKit.so） | dex class_defs + `Java_com_mlibkit_smb_*` 导出符号 |
| S2 | **API 全功能面**：`create / invoke / fileRead / fileWrite / readBlock / writeBlock / progressCore / cancelCore`——**含写、块读（随机访问）、进度回调、可取消** | libMLibKit.so 导出符号全列 |
| S3 | **播放通道 = 本地 HTTP Range 代理**：libMLibKit.so 内嵌 `HTTP/1.1 1xx/2xx` 响应串 + `Content-Range` + `Accept-Ranges`；FFmpegKit.so 含 `127.0.0.1`。FFmpegKit **无** libsmbclient（smb:// 不可用）→ SMB 字节必经本地代理喂给播放内核 | 字节串扫描 |
| S4 | 下载是**独立模块 + Service**：`DownloadClient` + `DownloadService` + `DownloadJobService`（后台续传）——对应 kivo 的 225 卡 | dex 类表 + 导出符号 |
| S5 | 协议栈不可复用：自研 native SMB + 脚本总线（加密脚本串 `s1:` 前缀）——**学架构，不学实现** | 03 拆解 §二/§五 |

**S3 是本轮最重要发现**：本地 HTTP Range 代理是 SenPlayer 一切远端源（SMB/网盘/Emby）
的统一网关，也是硬解的唯一可行通道（`MediaExtractor.setDataSource("http://127.0.0.1:port/…")`
原生支持 Range 流）。kivo 的 221 WebDAV / 224 Emby 将直接复用这个代理。

### 0.2 Rex 侧（UI 参照）—— 既有证据 + 本轮补抽

| # | 实证 | 证据 |
| --- | --- | --- |
| R1 | 添加资源库表单：取消/保存玻璃胶囊导航 + **可用地址胶囊**（自动发现的地址候选）+ 服务器/端口/用户名/密码输入行 + 测试连通性 + 隐私脚注 + 底部 连接/信息/iCloud/高级 分段 | 用户截图 `IMG_0114.PNG`（真机实拍） |
| R2 | 表单字段语义全集：`名称（自动获取）/ Host（服务域名或 IP）/ 端口 / 路径（例如 dav）/ 用户名 / 密码 / HTTPS 开关 / 粘贴并识别 / 备用线路（主+备）/ 测试状态（正在测试…→可连接 reachable）` | `rex-package/strings/zh-Hans__Library.json` `library.form.*`（460 键 Library 模块） |
| R3 | SMB 专属验证文案：`invalidSMBServer=请输入有效的 SMB 服务器地址，端口可选`、`missingUsername`、`connectionFailed=无法连接服务器，请检查地址与网络`、`unable.to.read.folder=无法读取文件夹`、`folderLoading=正在读取目录…` | 同上 `library.validation.*` / `library.error.*` / `library.file.match.*` |
| R4 | 空目录态：`这个目录中没有子目录或 Rex 支持的视频文件` | 同上 |
| R5 | 浏览语义：`文件浏览`（fileBrowser）+ `上一级`（back）——目录树两级导航 | 同上 `library.drivemby.*` / `library.scrapeFolder.*` |

---

## 一、一句话目标

**真机经 SMB 浏览共享目录（Rex 表单/浏览 UI）、点开片源经本地 Range 代理流式播放
（SenPlayer 架构）、seek 正常、错误路径全部非空 reason（G9）——M2 生态地基。**

## 二、设计决策（每条挂证据）

### D1 SMB 客户端：jcifs-ng（codelibs），API 面对齐 SenPlayer S2

- Android 无 AMSMB2（Swift 专用）；SenPlayer 自研 native 栈不可复制（S5）；
- jcifs-ng（纯 Java，`SmbRandomAccessFile` offset 读 / `SmbFileOutputStream` 写 / list 浏览）
  对位 SenPlayer API 面 S2 一一映射：

| SenPlayer SMBClient | kivo SmbSource |
| --- | --- |
| `create` | `connect(config)` |
| `invoke`（浏览等） | `list(path): List<SmbEntry>` |
| `fileRead` / `readBlock` | `readAt(offset, buf): Int`（ByteSource 接口） |
| `fileWrite` / `writeBlock` | `writeAt`（本卡只读路径不触，接口预留） |
| `progressCore` | `onProgress` 回调（播放/后续下载卡共用） |
| `cancelCore` | `cancel()` |
| —（销毁） | `close()` |

- smbj 备选（jcifs-ng 在 API 26 冒烟失败才切）；**AC0 spike 先行**。

### D2 播放通道：本地 HTTP Range 代理（照 SenPlayer S3 架构）

```
SmbSource(ByteSource) ──► LocalRangeProxy(127.0.0.1 随机端口)
                              │  HTTP/1.1 + Range: bytes=x-y + Content-Range/Accept-Ranges
                              ▼
            软解：FFmpeg avformat open "http://127.0.0.1:port/…"（本卡）
            硬解：MediaExtractor.setDataSource(http://…)（后续卡，唯一远端硬解通道）
```

- **为什么不是 avio 直连**（v1 方案，撤）：avio up-call 只喂软解，硬解永远开不了；
  221/224 每个源都要重做一遍适配。本地代理是统一网关，且是 SenPlayer 生产验证过的形态。
- **FFmpeg 重编译**：Android 版当前只有 file 协议（build.sh L100），需加
  `--enable-network --enable-protocol=tcp,http`（脚本化一次投资；重编译后 211 证据回归）。
- 代理自写最小实现（Kotlin `ServerSocket` + 单请求线程池；Range/HEAD/GET/404/416），
  不引 HTTP 框架——它只服务本机 127.0.0.1。
- `ByteSource` 接口：`size() / readAt(off, buf): Int / close()`——220 SMB 第一个实现，
  221 WebDAV / 224 Emby 后续逐个实现同一接口。

### D3 G9 错误路径：文案对齐 Rex R3，阶段点名

| 阶段 | reason（对齐 Rex 文案） |
| --- | --- |
| 表单校验 | `请输入有效的 SMB 服务器地址，端口可选` / `请输入用户名。` |
| 连接失败 | `无法连接服务器，请检查地址与网络。` + 主机:端口 |
| 认证失败 | `用户名或密码不正确`（Rex 无 SMB 认证专属键，用通用登录失败语义） |
| 目录读取失败 | `无法读取文件夹` + 路径 |
| 空目录 | `这个目录中没有子目录或 Kivo 支持的视频文件` |
| 播放中断连 | `SMB 连接中断：<阶段>` + 可重试 |

### D4 UI：照 Rex R1/R2 表单 + R5 浏览语义（Compose，05 号文档 token）

- 表单（R1 实拍 + R2 键）：取消/保存胶囊导航 → 可用地址胶囊（可先留空，mDNS 后续卡）
  → 名称（自动获取）→ Host → 端口(445) → 路径 → 用户名 → 密码（可见性切换）→
  测试连通性（正在测试…→可连接/失败原因）→ 保存；
- 首行输入完成后整行白圆角 59pt、左标签右值，与 0114 实拍一致；
- 浏览页（R5）：目录/文件列表（文件夹行 + 文件行[名称+大小]）、上一级导航、
  空目录态 R4、点文件起播；布局按 05 号文档毛玻璃卡片规格。

### D5 凭据存储

- SmbConfig JSON + SharedPreferences 单槽位（表单回填）；多源管理 = 222 卡。

### D6 硬解-over-SMB：不做（本卡），但代理即为其铺路

- `MediaExtractor.setDataSource(http://127.0.0.1)` + Kotlin 解封装管线 = 独立后续卡；
- 本卡证据 `decoder_backend=ffmpeg` + reason `smb via local range proxy`（G9 显式，非静默）。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `smb/SmbConfig.kt` | **新** | 连接描述 data class + JSON（无协议逻辑） |
| `smb/SmbSource.kt` | **新** | jcifs 封装 + 实现 `ByteSource`（connect/list/readAt/close，唯一协议访问点） |
| `net/ByteSource.kt` | **新** | 随机读源接口（size/readAt/close）——220/221/224 共同地基 |
| `net/LocalRangeProxy.kt` | **新** | 127.0.0.1 HTTP/1.1 Range 代理（BindSource→端口），SenPlayer S3 同构 |
| `ui/compose/SmbActivity.kt` | **新** | 表单 + 浏览列表宿主（Rex R1-R5 规格） |
| `third_party/ffmpeg-android/build.sh` | 旧（小改） | +network+tcp+http 协议，重交叉编译 |
| `backend/…/ffmpeg_local_playback_session.cxx` | 旧（小改） | 输入源新增 URL 分支（avformat_open_input(url)，fd 路径不动） |
| `bridge/NativePlayer.kt` / `FfmpegPlaybackSession.kt` | 旧（小改） | `openSmbVideo`（URL 模式） |
| `ui/compose/PlayerComposeActivity.kt` | 旧（小改） | `kivo.smb` extra 分支 |

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC0 | 选型 spike | jcifs-ng 在 K40：连接→列目录→offset 读 1MB 全通（失败则 smbj 重跑本 AC） |
| AC1 | 浏览 | 真机连 PC SMB 共享：目录树 ≥2 级、条目数与服务器一致（AC1 用 AC0 环境复算） |
| AC2 | 起播 | SMB 1080p 片源点开即播；证据 `decoder_backend=ffmpeg` + reason 含 `local range proxy` |
| AC3 | seek | SMB 播放中 seek 落点精确（211 口径），seek 后零丢帧；代理 Range 请求日志复算 |
| AC4 | 错误路径（G9） | 断网 / 错密码 / 空目录 / 停共享：reason 非空且阶段点名（D3 表） |
| AC5 | M2 口径【待验证】 | 1GB 片 SMB 起播 vs 本地差 ≤2s；超差 → 状态稿写结论，触发预缓冲卡 |

## 五、测试环境

- 服务端：本机 Windows SMB 共享（Administrator 建，专用只读账户），K40 经 OpenWrt WiFi
  访问 PC IP；共享根放 1080p 参考片源（本地同片对照）+ 两级测试目录。
- 证据：`docs/evidence/p220/` + `verify_p220_smb.py`（变异自检）。

## 六、明确不做

硬解-over-SMB（后续卡，D6）· 预缓冲到本地（Rex 的 Beta 开关，225/后续）·
多源管理 UI（222）· KMP（iOS 期）· SMB1 · 服务发现（mDNS，可用地址胶囊先手动填）·
SMB 写路径 UI（SmbSource 接口预留，无 UI）。
