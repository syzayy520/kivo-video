# 220 · SMB 直连 —— 设计稿

> 状态：**设计完成，实现未开工**。M2 第一张卡。
> 卡片原文：SMB 直连（smbj/JCIFS-NG 选型）——浏览/起播/断点。依赖 211 软解兜底 ✓（已收口）。
> 参照：Rex 用 **AMSMB2.framework**（Mac 拆解实证，06 号文档 §二）；Rex 表单规格 05 号文档 §6.5；
> Rex 有"预缓冲到本地文件（Beta）"独立开关（本卡不做）。

---

## 一、一句话目标

**真机经 SMB（局域网）浏览共享目录、点开片源直接播放、seek 正常、
断网/权限失败有非空 reason（G9）——M2「生态与内容」的第一块地基。**

## 二、设计决策

### D1 协议库：jcifs-ng（codelibs fork）主选，smbj 备选

- **jcifs-ng**（`codelibs:jcifs`）：纯 Java、Android 实战最多、SMB1/2/3、
  `SmbRandomAccessFile` 天然带 offset 读（seek 就是协议能力）；
- smbj 备选（若 jcifs-ng 在 API 26 上有依赖冲突再切）；
- AMSMB2 是 Swift 侧答案——iOS 期直接对位复用结论，本卡不引入 KMP
  （协议逻辑 KMP 化推迟到 iOS 期一起定，避免现在为了"将来"拆两遍）。
- **AC0 选型 spike 先行**：设备上连接→列目录→offset 读 1MB 各跑通才定库。

### D2 播放通道：FFmpeg 软解 + SMB avio 适配（native 会话一行不动）

SMB 没有 fd，而 211 的软解会话吃 `fd+length`。两条路里选：

| 方案 | 内容 | 取舍 |
| --- | --- | --- |
| **B1 avio up-call（本卡）** | FFmpeg 自定义 avio 的 read/seek/size 回调经 JNI 调 Kotlin `SmbSource.readAt` | 真·流式随机读；native 会话零改动；只覆盖软解通道 |
| B2 预缓冲到本地 | SMB→cache 文件→复用 fd 通道（硬解也能吃） | 起播要等拷贝；1GB 差 ≤2s 的口径基本到不了；是 Rex 的"预缓冲"开关，**列独立后续卡** |

本卡 **B1**。证据发布 `decoder_backend=ffmpeg` + reason 写明 SMB 通道（G9：不是静默降级，
硬解-over-SMB 需要 MediaDataSource→MediaExtractor 的 Kotlin 侧解封装管线，**明确不做**，列后续卡）。

### D3 avio 回调的 JNI 形态

- native avio 持 `jobject`（SmbSource 全局引用）+ `JavaVM`；
- 回调线程（FFmpeg worker）首次调用时 `AttachCurrentThread`，线程退出 Detach；
- read 粒度 32KB；seek = jcifs offset read；`AVERROR_IO`/`AVERROR(EINTR)` 映射；
- 连接断开 → avio 返回错误 → 会话 `lastError` 非空 reason（G9）；错误文案带主机与阶段（connect/read/seek）。

### D4 浏览与表单 UI（Compose，照抄 Rex 规格）

- 表单（05 §6.5 实测）：取消/保存玻璃胶囊导航 + 全宽圆角输入行（高 ~59pt）
  + **测试连通性**文字按钮 + 脚注（隐私话术：只上传文件夹与文件名）；
- 字段：主机 / 端口(默认 445) / 共享名 / 路径 / 域(可选) / 用户名 / 密码（可见性切换）；
- 浏览页：目录列表（文件夹/文件双形态、文件名+大小），点文件 → `PlayerComposeActivity`
  传 `kivo.smb`（SmbConfig JSON extra）；
- 入口 Activity：`SmbActivity`（横屏同播放页）。

### D5 凭据与状态

- SmbConfig：host/port/share/path/domain/user/pass（data class + JSON 序列化）；
- 本卡存内存 + SharedPreferences 单源槽位（重进回填表单）；多源管理归 222。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `smb/SmbConfig.kt` | **新** | 连接描述 data class + JSON 序列化（无协议逻辑） |
| `smb/SmbSource.kt` | **新** | jcifs 封装：connect / browse(list) / readAt / size / close（唯一协议访问点） |
| `ui/compose/SmbActivity.kt` | **新** | 表单 + 浏览列表宿主（Compose，Rex 规格） |
| `backend/include_private/video/video_plane/runtime/local_media/ffmpeg_smb_callbacks.hpp/.cxx` | **新** | avio 适配：jobject 全局引用 + JavaVM up-call（read/seek/size） |
| `jni/jni_ffmpeg_bridge.cxx` | 旧（小改） | `nativeFfmpegOpenSmb`（收 jobject 代替 fd） |
| `bridge/NativePlayer.kt` | 旧（小改） | 对应 external 声明 |
| `bridge/FfmpegPlaybackSession.kt` | 旧（小改） | `openSmbVideo(config, surface)` 变体 |
| `ui/compose/PlayerComposeActivity.kt` | 旧（小改） | `kivo.smb` extra 分支 |

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC0 | 选型 spike | 选定库在 K40：连接→列目录→offset 读三步全通 |
| AC1 | 浏览 | 真机连 PC SMB 共享：列目录树 ≥2 级、文件/目录数与服务器一致 |
| AC2 | 起播 | SMB 上 1080p 片源点开即播；证据 `decoder_backend=ffmpeg` + reason 含 SMB |
| AC3 | seek | SMB 播放中 seek 落点精确（211 同口径），seek 后零丢帧 |
| AC4 | 错误路径（G9） | 断网 / 错密码 / 共享不存在：reason 非空且点名阶段（connect/read/seek），无静默 |
| AC5 | M2 口径【待验证】 | 1GB 片源 SMB 起播 vs 本地起播差 ≤2s；超差 → 结论写进状态稿，触发预缓冲卡 |

## 五、测试环境

- 服务端：本机 Windows SMB 共享（Administrator 建共享 + 专用账户只读），
  K40 经 OpenWrt WiFi 访问 PC IP；共享根放 1080p 参考片源 + 一级/二级测试目录。
- 对照：同一片源本地拷贝一份（AC5 用）。

## 六、明确不做

硬解-over-SMB（MediaDataSource 管线，后续卡）· 预缓冲到本地（Rex 同款，后续卡）·
多源管理 UI（222）· KMP 化（iOS 期定）· SMB1（只 SMB2/3）· 发现协议（mDNS/WS-Discovery）。
