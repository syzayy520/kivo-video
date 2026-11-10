# 252：「正在连接SMB…」永远不动 —— 起播前的探测把整片读了一遍（已修）

日期：2026-11-10 夜
设备：K40 `b6e950ee`（Android 13 / kona）
服务器：小米路由 `192.168.5.1` 共享 `XiaoMi`（匿名只读）
片源：`Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD`（12.19 GB，1920×804，2×TrueHD + PGS）

## 一、现象

在播放页打开这条片源，状态永远停在「正在连接SMB…」，时间 `0:00:00 / 0:00:00`，
既没有错误提示，也没有解码器实例。等 13 分钟也不变。

同期：小片源（`local-seek.mkv`，720p H.264）在同一个播放页里 7 秒正常起播。

## 二、定位：不是卡死，是在把 12 GB 读完

分段计时（临时诊断，逐行落盘）：

```
A-open-start ...
A-open-done  size=12186004520 in 938 ms     ← 连接 / 打开 / fstat 都正常
B-probe-start openDescriptor + MediaExtractor
（40 秒后仍停在这一行）
```

同时测网络：

```
wlan0 rx: 16239866009 → 16321953417（5 秒）
5 秒内收到 78.28 MB，约 15.66 MB/s
```

15.66 MB/s × 13 分钟 ≈ 12 GB —— **`MediaExtractor.setDataSource` 为了解析这个 MKV，
把它从头线性读完了**。这条片源的头部没有可用索引，解析器不找到 Cues 不返回。

再细拆 probe 内部，确认卡点：

```
B1-head-read  offset=0        read=65536 in 216 ms     ← 正常
B2-tail-read  offset=12185873448 read=65536 in 121 ms  ← 正常
B3-setDataSource ...                                   ← 永不返回
```

## 三、为什么别的播放器没事

它们用 FFmpeg 做 demux，探测是**有界**的：读头部几百 KB，再按索引 seek 到文件尾。
我们拿系统 `MediaExtractor` 当"能不能硬解"的探测器，而它在这个封装上退化成了全片扫描。

**不是缺解码器**：平台有 `OMX.qcom.video.decoder.hevc`，同一个文件走我们自己的播放管线
实测 26–27 fps、0 丢帧、1.00x（见 `p251/hard-decode-prefetch`）。

## 四、修法

**网络片源不再做容器探测。**

- 默认走平台解码（历史默认，零成本）；
- 平台解不了时（真解码那一刻才知道）自动退软解，并重开一条数据源
  （硬解会话关闭时会连 source 一起关，复用原 fd 只会得到 `handle is null` —— 真机踩过）。

这样"判断成本"从**读完整片**降到**解不出第一帧**，而且不再需要猜探测预算。

顺带保留两处防御（本地文件仍在用的 `FfmpegRouting.probe`）：
4 秒截止时间 + `cancel()` 中断（不是 `close()`：关掉代理 fd 会让阻塞中的读等不到回调线程）。

## 五、验证

| 片源 | 结果 |
| --- | --- |
| `Shelter`（HEVC 10bit，2×TrueHD + PGS） | **硬解正常播放**（状态「正在播放」，无"软解"标记）；10 s 内推进、码流 2.3 MB/s |
| `[恐怖废墟]`（VC-1 remux 1080p） | 硬解失败 → 自动退软解：日志 `platform decode failed (open(network-media): extractor: unsupported codec id 70) -> ffmpeg (rebind)`，状态「正在播放 · 软解」，画面正常 |
| `local-seek.mkv`（720p H.264） | 播放页正常；`SmbPlaybackSeekInstrumentation` **4/4 PASS**，落点 27107 ms（基线 27106） |

`codec id 70` 就是 VC-1 —— 这条兜底把"探测本来要负责的那件事"接住了，而且是在真解码失败时接的。

## 六、过程里修掉的两个自身缺陷

1. **`smbTrace` 静默失败**：原来写 `getExternalFilesDir(null)`，该目录为 null 时
   `File(null, "evidence")` 直接抛异常并被 `catch {}` 吞掉 —— 排障期间"没有日志"被误读成
   "没走到那段代码"。现在写内部 `filesDir`，`run-as` 可读。
2. **探测用 fd 的关闭时机**：超时后关闭代理 fd 会把准备线程一起挂住（黑屏、零流量、
   既无错误也不起播）。改成 `cancel()` 置标志，让下一次读抛 EIO 自然退出。

## 七、这一轮暴露的测试环境问题（不是代码问题）

- 设备在本轮多次掉线（`adb devices` 里直接消失）、前台被系统/用户切走、
  进程被清，导致几次「黑屏 + 零流量」的假象，浪费了定位时间。
  判据改成：**instrument 落盘日志**（`run-as cat files/...`）+ 网络字节增量，不看截图猜。
- 4K HEVC 解码器会进坏状态（`dequeueOutputBuffer -10000`、`samples=1`），重启可恢复；
  重启后手机可能连到别的 Wi-Fi（10.0.0.x），需切回 192.168.5.x。

## 八、复算

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
export PATH="$PATH:/c/Android/sdk/platform-tools"
cd "C:/kivo video/android" && ./gradlew.bat assembleDebug

adb install -r app/build/outputs/apk/debug/app-debug.apk
# 补一条资源库记录（重装后资源库为空）
adb shell "am instrument -w -e smbHost 192.168.5.1 -e name XiaoMi \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbSourceSeedInstrumentation"

P="视频/电影/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv"
adb shell "am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  -e kivo.sourceId seed-smb-xiaomi -e kivo.smbShare XiaoMi -e kivo.smbPath '$P'"
sleep 8
adb shell "run-as com.kivo.cinema cat files/evidence/p220-status.log" | head -5
# 期望：prepared open size=12186004520 / routing=mediacodec（不再有 probe begin/end）
```
