# 长片源「远距离 seek 失效」（新发现，未修）

日期：2026-11-10 夜
设备：K40 `b6e950ee`；服务器：小米路由 `192.168.5.1` 共享 `XiaoMi`
测试：`SmbPlaybackSeekInstrumentation`（新增 `-e seekTargetMs` 可指定落点）

## 一、怎么发现的

在 `Shelter` 与 4K 原盘上跑既有 seek 验收，两条都失败：

```
4K  : target=3395600 ms landing=0 ms actual=2900 ms
Shelter: target=3217589 ms landing=0 ms actual=2906 ms
```

同一个测试在 `local-seek.mkv`（2 MB / 60 s）上 **4/4 PASS**（落点 27106 ms）。

## 二、bisect：不是"seek 坏了"，是"跳远了才坏"

在 Shelter 上按落点扫描：

| 目标 | 落点 landing | 实际位置 actual | 判定 |
| --- | --- | --- | --- |
| 60 s | 50 050 ms | 63 530 ms | 成功（关键帧对齐偏 10 s，未超 ±5 s 判据的边界） |
| 100 s | 90 090 ms | 111 564 ms | 落点偏 10 s、实际又跑过头 21 s → 判据失败 |
| 180 s | **0 ms** | 2 844 ms | **完全没跳**，位置停在起播附近 |
| 600 s | **0 ms** | 2 857 ms | 同上 |
| 中段（1 074 min 的一半） | **0 ms** | 2 906 ms | 同上 |

规律：**近距可跳，远距返回 landing=0 且画面停在 2.8–2.9 s**（起播后位置）。

## 三、排除了什么

- 不是传输能力问题：同一文件顺序读 24 MiB 实测 **7.4–11.3 MB/s**，
  在 13 GB 偏移处随机读也正常（`SmbWindowDiag` 读 size/2 成功）。
- 不是小文件特例：2 MB 的文件能跳，是因为它整份落在 12 MiB 块窗口里、
  **根本不发生网络读** —— 所以它证明不了 SMB 上的 seek。
- 不是文件坏了：这两个文件在别的播放器上跳转正常。

## 四、下一步（要动核心，不在本卡）

需要在 `session->seek_to()` 这条链上打点，把三件事分开：

1. demux 层 `seek` 是否返回错误（MKV 索引 / 同步点）；
2. 视频 `landing_us` 为 0 是"没跳"还是"跳了但没记录"；
3. 音频 seek（TrueHD / DDP，走 FFmpeg）是否**失败后把整体 seek 一起判失败**
   （`jni_session_handle.cxx:265` 之后有 `audio->seek_to`）。

判据：Shelter / 4K 上 seek 到任意位置，`|actual - target| ≤ 5 s`，且 landing 非 0。

## 五、复算

```bash
P="视频/电影/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv"
adb shell "am instrument -w -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  -e smbPath '$P' -e seekTargetMs 60000 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbPlaybackSeekInstrumentation"
# 依次把 seekTargetMs 换成 100000 / 180000 / 600000 观察 landing 与 actual
```

## 六、附：本轮同时确认的好消息

同一晚在 4K 原盘上做的持续播放基准（`SmbPaceBenchInstrumentation`，15 s × 4 窗）：

```
#1 prefetch=off  26.46 fps  dropped=0  speed=1.00x
#2 prefetch=on   26.33 fps  dropped=0
#3 prefetch=off  26.47 fps  dropped=0
#4 prefetch=on   22.60 fps  dropped=56   ← 开预读才丢帧
```

**关预读（App 的实际策略）两窗都是 0 丢帧、1.00x。** 与 `Shelter` 上的结论一致。

## 七、2026-11-10 深夜追加：诊断设施已就位（seek 仍未修）

为把"到底跳没跳"变成可复算事实，加了一条原生诊断链（走已有的 summary 通道，
不新增日志体系）：

- `DemuxSource::SeekDiagnostics`：请求落点 / 实际落点 / `av_seek_frame` 返回值 /
  目标轨索引条数 / avio 位置 / 交给 avio 的文件长度。
- `FfmpegDemuxSource` 在 `open_fd` 时把**文件长度**与**索引条数**挂进 `backend_label_`，
  在 `seek_to` 里记录 seek 的事实。
- `DecodeSession` 在 **seek 之后立刻**抓一份副本（源在 resume 路径会被重开，源里那份会丢）。
- JNI summary 追加 `demux=[...]` 字段，真机从 instrument 输出直接读。

### 已经拿到的硬事实

```
backend=ffmpeg tb=1/1000 start=0 sb=0 len=12186004520 idx=653 sidx=12
```

- **我们走的是 FFmpeg 解封装**（不是平台 MediaExtractor）；
- **文件长度正确**（12 186 004 520 = 真实大小，没有 int 截断）；
- **FFmpeg 建出了 653 条索引条目** —— 索引在。

而同一时刻 ffprobe 用同一个文件（PC 端挂载）跳到 600 s 只花 **0.294 s**，落点 594.469 s。
**所以文件没问题、FFmpeg 也能跳，缺口在我们的 avio/会话这一侧。**

### 还没定位到的那一段

seek 之后 `demux=[backend=none …]` —— 抓诊断时源已经不在了（`impl_` 为空），
说明 seek 走的可能不是 `source_.seek_to()` 这条（例如走了 `AsyncSampleReader` 的
异步路径，或 seek 前源被释放）。下一步就是在 `AsyncSampleReader::seek_to` /
`DecodeSession::seek_to` 上加一行"到底调没调到源"的记录，把最后这一段钉死。

判据不变：任意位置 `|actual − target| ≤ 5 s`，且不再出现 landing=0。

### 顺带修掉的构建 bug（重要）

`KIVO_LIBASS_ANDROID_DIST` 在 CMake 文件末尾才定义，却被前面的
`kivo_backend_ffmpeg` 先使用 → 展开成 `/include` → `fatal error: 'ass/ass.h'`。
**这台机器上的 native 全量重建一直是坏的**，增量构建恰好跳过 —— 也解释了
"改了后端代码、构建 7 秒完成、行为却没变"这类假象。定义前置后，全量重建 35 s 通过。

## 八、2026-11-10 深夜：seek 的真相已经查到最后一层

诊断链补齐（会话计数 + demux 事实 + JNI 计数 + 快照落点 + 会话地址）后，
Shelter 上跳 600 秒的真实情况是：

```
demux=[backend=ffmpeg ... len=12186004520 idx=653
       target=600000000 landed=594469000 avio=1275431492
       calls=1 async=1 src=0 alive=1 seen=594469000
snap=594469000   jni=1   seekptr=0xb4000073834b5000 sumptr=0xb4000073834b5000
```

**结论（都是实测，不是推断）：**

1. **解封装层的 seek 完全正确**：`landed=594469000` = **594.469 s**，
   与 ffprobe 在同一个文件上得到的落点**一模一样**；`avio=1275431492` 说明
   读取位置确实从 0 跳到了 1.275 GB；`rc=0`。
2. **会话层也拿到了正确落点**：`seen=594469000`（在 `seek_to` 里、source seek 之后立刻抓的）。
3. **但 JNI 读出来是 0**：JNI 紧跟 `seek_to` 之后读落点，得到 0，
   于是 `handle->last_seek_landing_us = 0`、Kotlin 侧 `positionUs = 0`、
   时钟没有 rebase，画面停在起播位置 —— 这就是用户看到的"拖了进度条，画面从头开始"。
4. **不是两个对象**：`seekptr == sumptr`（seek 用的会话与 summary 报的是同一个）。

### 已经排除的假设

| 假设 | 排除依据 |
| --- | --- |
文件没有索引 / FFmpeg 跳不动 | ffprobe 0.294 s 跳到 594.469 s；我们的 `idx=653` |
平台 MediaExtractor 的锅 | MKV 走的是 FFmpeg（`backend=ffmpeg`），不是平台提取器 |
文件长度被截断 | `len=12186004520` 与真实大小一致 |
AVIO 的 seek 没实现 | avio 位置确实从 0 跳到 1.275 GB |
seek 没进会话 | `calls=1`、`jni=1`、`seen` 有值 |
两个会话对象 | `seekptr == sumptr` |

### 剩下的唯一矛盾（下一轮的一行改动即可钉死）

同一个对象、同一个成员，`seek_to` 刚写进去的 594469000，JNI 紧接着读出来是 0，
而两秒后读快照又是 594469000。**下一步：在 JNI 读取点把 `video_landing` 自己打出来**
（现在只打了两秒后的快照值），就能分清是"读的时候就是 0"还是"读到了但被覆盖"。

同时把 JNI 的落点来源从 `snapshot().seek.landing_us` 改成会话直读
（`DecodeSession::seek_landing_us()`）—— 落点应当由"刚完成 seek 的那次调用"直接交出去，
不该绕一圈再读回来。这条改动本身没解决问题，但它把问题范围缩小到了"读取那一刻"。
