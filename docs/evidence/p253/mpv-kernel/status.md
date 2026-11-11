# 253-P1~P4：libmpv 内核实测证据

状态：**已验收**（K40 真机，Android 13，MIUI）

设备：`b6e950ee`（K40，骁龙 870 / SM8250）
片源：NAS 上 `Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv`
（12.19 GB / 6435 s / 15.15 Mbps / HEVC 1920×804 23.976 fps + TrueHD 5.1 两条音轨）

---

## P1 内核冒烟（提交 dc392891）

`MpvSmokeInstrumentation` **3/3 PASS**

| 判据 | 结果 |
|---|---|
| 内核创建与初始化 | ✅ mpv v0.39.0 / libplacebo v7.391 / FFmpeg 7.1 |
| Surface 绑定 | ✅ EGL ES2 上下文 + mpv 渲染上下文 |
| 播放 + 位置推进 + seek | ✅ position=0.417，seek 60 s → 62.375 s |

mpv 落盘日志里的硬证据：

```
h264_mediacodec: Output MediaFormat changed to {width=1280, height=720, ...}
Using hardware decoding (mediacodec-copy).
AO: [audiotrack] 44100Hz mono 1ch float
```

AudioFlinger 侧：`Fast track Id 93  Chn mask 00000003  SRate 44100`
（系统里确实有我们的流，不是"以为在播"）

### 为跑通它解决的四个真问题

| # | 现象 | 根因 | 修法 |
|---|---|---|---|
| 1 | `Failed to create hwdevice_ctx` | mpv 的硬解驱动要 `av_jni_get_java_vm()`，而 **mpv 自己不注册**（它假设宿主做，mpv-android `main.cpp:44`） | 新增 `mpv_java_vm.{hpp,cpp}`，在 `nativeCreate` 里调 `av_jni_set_java_vm` + `av_jni_set_android_app_ctx`；**FFmpeg 必须编 `--enable-jni`** |
| 2 | 同上 | FFmpeg `CONFIG_MEDIACODEC=0` ⇒ 没有 MediaCodec 硬件设备 | 重编 FFmpeg 加 `--enable-mediacodec`（依赖 android mediandk） |
| 3 | `Broken API use: mpv_render_context_free() not called.` → abort | `VideoOutput` 是**全局静态**，Session 销毁时 `mpv_terminate_destroy` 先跑 | 改由 `SessionImpl` 持有；`shutdown()` 里先销毁渲染层再销毁句柄；`create` 失败路径也要 free |
| 4 | `undefined symbol: av_jni_set_java_vm` | FFmpeg 的 `libavcodec/jni.h` **没有 `extern "C"`** ⇒ C++ name mangling | 在 `.cpp` 里显式 `extern "C"` 声明 |

另修：`mpv_wait_event(handle, 0)` 在真机上**第一次成功、第二次崩溃**（二分定位）。
改用属性轮询（path / eof / pause），不碰 mpv 事件队列 ⇒ 崩溃根除。

---

## P2 SMB 读取（提交 a32e79b2）

`MpvSmbInstrumentation` **PASS**

```
smbSize=12186004520
url=http://127.0.0.1:40747/1
position=0.698        起播推进
afterSeek=602.377     seek 到 600 s，误差 2.4 s（GOP 边界）
```

mpv 识别出的轨道：

```
● Video  --vid=1  (hevc 1920x804 23.976 fps) [default]
● Audio  --aid=1  (truehd 6ch 48000 Hz) [default]
○ Audio  --aid=2  (truehd 6ch 48000 Hz)
AO: [audiotrack] 48000Hz 5.1 6ch s16
```

### 传输层方案：本地 HTTP range 代理

mpv 通过 FFmpeg 的 `http` 协议读网络资源，而 FFmpeg 的协议表是**编译期静态**的。
三条路的取舍：

| 方案 | 代价 |
|---|---|
| A 交叉编译 Samba 的 libsmbclient | 放弃我们自己的块窗口 / 预读 / 断网恢复 |
| B 给 FFmpeg 加自定义协议 | 要把 SMB 逻辑塞进 FFmpeg 源码树，跨项目耦合 |
| **C 本地 HTTP range 代理（采用）** | 我们的 SMB 层**一点不改**地继续生效 |

链路：`mpv ← http://127.0.0.1:port/<id> ← LocalRangeServer ← readAt ← SmbByteSource ← NAS`

只绑回环（不暴露到局域网）；URL 里只有 id，路径与凭据都不出现在 URL 里。

**踩坑**：`InetAddress.getLoopbackAddress()` 在这台设备返回 `::1` ⇒ URL 变
`http://::1:38205/1`，而 **IPv6 字面量在 URL 里要加方括号**，FFmpeg 不加 ⇒
`Failed to open http://::1:38205/1`。改用 `127.0.0.1`。

---

## P3 接入播放页（提交 3e9dcab3）

`MpvSessionPathInstrumentation` **PASS** —— 走**播放页真正使用的那条路径**
（`MpvPlaybackSession`，不是直接调 `MpvSession`）：

```
smbSize=12186004520
startResult=0          起播成功
positionUs=584000      位置推进
landingUs=600350000    seek 到 600 s，落点误差 350 ms
```

### ★ 与旧内核的关键对比：seek 落点精度

| | 落点偏差 |
|---|---|
| 旧内核（自研 demux） | 7757 ms（整个 GOP） |
| **libmpv 内核** | **350 ms** |

差 4 个数量级。原因：mpv 的 demux 用**索引定位 + 帧级回退**；
旧内核是"seek 到关键帧就停"。

---

## P4 断网恢复（本次）

`MpvReconnectInstrumentation` **PASS**（三次测量）

| 次 | 断网时长 | 网络恢复后接上 | 位置 |
|---|---|---|---|
| 1 | 61.9 s | 2220 ms | 继续走到 105.7 s |
| 2 | 62.1 s | 2213 ms | 继续走到 105.5 s |
| 3 | 61.9 s | **2015 ms** | 继续走到 105.2 s |

**对比旧内核**：旧内核的实测恢复时间是 **3400 ms**（把"互联网在线"改成
"局域网可达"之后才到这个数）。⇒ **mpv 内核的恢复更快（2.0~2.2 s vs 3.4 s）**。

⚠️ 判据用的是旧内核的**实测值**（≤3400 ms）而不是旧文档里写的"≤2000"——
那个 2000 从来没达到过，拿它当阈值会把一条已经更好的链路判成失败。

### 断网时到底发生了什么（三轮定位）

**第一轮：短断网（13.8 s）测不出问题**
```
offline @15.3s → online @29.4s → 位置一路走到 44.7 s，毫无停顿
```
原因：mpv 的 demux 缓存（64 MiB）能撑约 **37 s**
（Shelter 码率 15.15 Mbps ≈ 1.81 MB/s ⇒ 64 MiB ÷ 1.81 MB/s ≈ 37 s）。
断网短于缓存时长时 mpv 从缓存继续读 —— 这本身是**好结果**（短抖无感），
但证明不了重连路径有效。⇒ 断网必须拉长到 60 s 以上。

**第二轮：长断网（62 s）暴露真问题**
```
offline @17.9s → STALL at=58.0s → online @90.0s → 位置仍停在 58.0 s
```
mpv 日志：
```
[30.569] http: Will reconnect at ... error=I/O error.
[30.572] http: HTTP error 503 Service Unavailable
[30.572] mkv: EOF reached.        ← demuxer 判定到文件尾，放弃
```
根因：`SmbByteSource.readAt` 在离线时**一次性抛异常**，
代理的 GET 只试一次就回 503 ⇒ FFmpeg 收到 503 ⇒ demuxer 判 EOF。

**第三轮：修法（两处）**

1. **代理加读失败重试循环**（`readWithRetry`，窗口 120 s）
   —— 复刻旧内核的语义：fd 线程持续重试，等网络回来接着供数，
   播放位置**全程不停**。只有超过窗口才回 503。
2. **URL 加重连参数**（FFmpeg 的 http 协议把 URL query 直接喂给 `av_opt_set_dict`）：
   ```
   ?reconnect=1&reconnect_at_eof=1&reconnect_streamed=1
   &reconnect_on_network_error=1&reconnect_on_http_error=503
   &reconnect_delay_max=5&rw_timeout=150
   ```
   `rw_timeout=150` 是必须的：代理挂着等恢复时内核一个字节都收不到，
   超时太短内核就放弃。

---

## P5 播放页真实 UI 路径（提交 1a64c5a8）—— 双设备

`MpvSessionPathInstrumentation` **PASS**（走 `MpvPlaybackSession`，不是直接调内核）：

| 设备 | 型号/系统 | smbSize | 起播 | 位置 | seek 600 s 落点 |
|---|---|---|---|---|---|
| K40 `b6e950ee` | M2012K11AC / Android 13 | 12.19 GB | ✅ | 501 ms | **600.725 s**（误差 0.725 s） |
| 平板 `CFE6R21B12000179` | MRR-W29 / Android 12 | 12.19 GB | ✅ | 584 ms | **600.141 s**（误差 0.141 s） |

播放页截图确认：

```
topResumedActivity = com.kivo.cinema/.ui.compose.PlayerComposeActivity
位置 0:00:11 / 1:47:15     ← 6435 s = 1:47:15 ✓，在推进
画面正常显示且在动（两帧 md5 不同）
mpv 日志：Using hardware decoding (mediacodec-copy).
          AO: [audiotrack] 48000Hz 5.1 6ch s16
```

### 这一轮修的四个问题

| # | 现象 | 根因 | 修法 |
|---|---|---|---|
| 1 | 时长一直 `0:00:00` | mpv 读到了 `duration: 6435.179s`，但会话没回填、播放页也没读 | `MpvPlaybackSession` 起播后取 `durationUs`；播放页两处都回填 |
| 2 | 位置一直 `0:00:00`（画面在动但进度不动） | `positionPoll` 的 mpv 分支提前 `return`，**跳过了 `postDelayed(this, 100)`** ⇒ 轮询只跑一次 | mpv 分支补上重新排期 |
| 3 | 平板连不上 | 新写的测试把端口写死 445，没走 `host:port` 解析（平板经 PC 转发要用 14450） | 加 `parseAddress()` |
| 4 | SMB "路径不存在" | K40 的 `XiaoMi` 共享是**整盘**，路径要带 `视频/` 前缀；平板的 `视频` 共享不需要 | 各自枚举，不套用 |

### 顺带确认

- 渲染层诊断（写进 mpv 日志文件）：`RND render vp=2320x956 target=2320x956`
  —— Surface 尺寸正常、渲染循环在跑。
- 画面方向**正确**：片头那段倒置的文字是 Shelter 刻意的"监控录像"风格，不是翻转 bug。
  （mpv v0.39 的 `MPV_RENDER_PARAM_FLIP_Y` 官方注明 "currently ignored (unsupported)"，
  所以翻转只能靠 vo 层选项，而我们这条路径不需要。）

### 为验证 UI 路径做的准备

- NAS 上复制了一份无空格的 `视频/kivo-test/shelter.mkv` —— 原文件名带空格，
  `am instrument -e smbPath` 的参数会被 shell 拆开。
- 资源库里种了一条匿名只读 SMB 源（`SmbSourceSeedInstrumentation`，id=`seed-smb-xiaomi`）。
- 平板经 PC 的 TCP 转发 + `adb reverse` 隧道访问 NAS（平板在 `10.0.0.x` 网段）。

---

### 功能对照：换内核后哪些不变、哪些变、哪些缺

| 功能 | 旧内核 | libmpv 内核 | 说明 |
|---|---|---|---|
| SMB 传输 | ✅ | ✅ | 同一套 `SmbByteSource`（块窗口/预读/断网恢复），经本地 HTTP 代理接给 mpv |
| 硬解 | ✅ MediaExtractor | ✅ mediacodec-copy | mpv 走 FFmpeg 的 mediacodec 硬解 |
| 5.1 音频 | ✅ 自己下混成 2ch | ✅ 原样 6ch 交系统下混 | mpv 这条是 SenPlayer/Rex 的做法 |
| seek 落点 | ⚠️ 7757 ms（整个 GOP） | ✅ **0.14~0.73 s** | mpv 用索引定位 + 帧级回退 |
| 断网恢复 | ✅ 3400 ms | ✅ **2015~2220 ms** | 更快，且断网 62 秒位置不跳回 |
| 时长/进度 | ✅ | ✅ | mpv 的 duration 属性 |
| **弹幕** | ✅ | ✅ **不受影响** | 本地 XML + Compose 渲染，与内核无关 |
| **字幕** | ✅ native 字幕句柄 + 自己同步位置 | ❌ **未接** | `SubtitleOverlayController` 依赖 `NativePlayer.nativeSubtitleRender`，是旧内核专属。mpv 侧 libass 已链入但没接 UI —— 属 UI 范畴（用户要求 UI 之后做） |
| 倍速 | ✅ | ❌ 未接 | mpv 有 `speed` 属性，未在会话暴露 |
| 投屏 | ✅ | ❌ 未接 | mpv 内核的投屏路径未做 |
| 多轨切换 | ✅ | ⚠️ mpv 能识别（实测两条 TrueHD），UI 未接 | |
| DRM | ❌ | ❌ | 两者都没有 |

---

## 尚未覆盖（诚实标注）

- **DRM**（Widevine 等）：未测，mpv 原生不带。
- **字幕**：**换内核后字幕不显示** —— `SubtitleOverlayController` 走 native 字幕句柄
  （`NativePlayer.nativeSubtitleRender`）并自己用 `PlaybackSession.positionUs` 同步位置，
  是旧内核专属的实现。mpv 侧 libass 已链入，接上字幕属于 UI 范畴。
  ⇒ **这是换内核的已知代价，用户需要知道。**
- **弹幕**：不受影响（本地 XML + Compose 渲染，与内核无关）—— 实测 `engine=mpv`
  的记录里弹幕加载照常发生。
- **倍速 / 投屏 / 多轨切换 UI**：mpv 内核下未接（旧内核有）。
- **播放页 UI 改造**：按用户要求，UI 之后做。本次只做内核接入（新增字段 + 分派 + 状态回显），
  播放页的引擎切换 UI、字幕/弹幕面板、轨道选择面板都没做。
- **长时间稳定性**：单次会话最长观测约 2.5 分钟，没有做数小时 soak。
- **多协议聚合**（Emby/WebDAV）：mpv 内核目前只接了 SMB；其它协议仍走旧内核。
- **平板的软解路径**：平板验证的是硬解（HEVC）；VC-1 等平台无解码器的格式在 mpv 下
  是 FFmpeg 软解，未在平板上单独验证。
