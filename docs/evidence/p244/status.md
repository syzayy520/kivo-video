# 244 · MKV 本地播放黑屏 / 位置恒 0（证据）

> **状态：CLOSED**（2026-11-10）。两段症状均已修复并真机复验：起播（`h264_mp4toannexb`）+
> seek（`SystemClockSource::rebase`）。关闭依据与结转项见文末「244 关闭记录」。

> 2026-11-10 00:26–00:29，平板 MatePad MRR-W29（Android 12），本地文件播放
> （`kivo.media` 直开，走 `PlaybackSession`，不经 SMB）。

## 一、三组对照（同一套播放器、同一台设备、同一画面内容）

| 组 | 文件 | 容器 | 视频 | 音频 | 结果 | 证据 |
| --- | --- | --- | --- | --- | --- | --- |
| A | `local-seek.mkv` | MKV | H.264 720p30 | 无 | ❌ 黑屏，`0:00:00 / 0:01:00` 不动 | `mkv-h264-noaudio-black.png` |
| B | `mkv-audio.mkv` | MKV | H.264 720p30 | AAC 静音 | ❌ 黑屏，`0:00:00 / 0:00:20` 不动，状态却是"正在播放" | `mkv-h264-aac-black.png` |
| C | `h264-audio.mp4` | MP4 | H.264 720p30 | AAC 静音 | ✅ 画面正常，`0:00:12 / 0:00:20` | `mp4-h264-aac-ok.png` |

三组的**视频编码与画面内容完全相同**（同一 ffmpeg `testsrc` 参数），唯一变量是容器。

## 二、结论

1. **不是"无音轨"**：B 组有音轨，照样黑屏、位置不动。此前"无音轨→时钟不走"的假设作废。
2. **不是编码**：C 组同样是 H.264，正常。
3. **不是时钟本身**：C 组同一套 `positionUs` 轮询能正常走秒。
4. 剩下唯一变量是 **MKV 容器**：`duration` 能读对（60 s / 20 s 与文件一致），
   说明容器头解析成功；但**没有帧进入渲染与计时**。

## 三、和 SMB 的关系（重要，别混淆）

同一台 K40 上，**SMB 的 4K HEVC MKV 能正常播**（23:15 证据：`0:01:52 → 0:02:01`，位置推进）。
所以"MKV 全坏"不成立，坏的是**本地文件 + MKV** 这条路。
网络路径用 `RangeFd`（fd 输入，`asyncInput=true`），本地路径直接给文件路径 ——
嫌疑集中在两条输入链对 MKV 的处理差异（demux 读取方式 / seekable 判定 / 时间戳来源）。

## 四、下一步（244 卡内）

1. 在 `FfmpegRouting` 上打印本地 MKV 的决策（platform vs ffmpeg）与实际打开的轨道数；
2. 对比本地路径与 `NetworkPlaybackSession` 的 `configurePipeline(asyncInput=...)` 参数；
3. 若要最小复现：同一 MKV 用 `RangeFd(FileByteSource)` 喂（即绕成 fd 输入），
   若此时能播 → 直接锁定"文件路径输入"这条链路。

## 五、生成命令（可复算）

```bash
# A 无音轨 MKV
ffmpeg -f lavfi -i "testsrc=size=1280x720:rate=30" -t 60 -c:v libx264 -preset ultrafast -pix_fmt yuv420p out.mkv
# B 带音轨 MKV
ffmpeg -f lavfi -i "testsrc=size=1280x720:rate=30" -f lavfi -i "anullsrc=r=48000:cl=stereo" \
       -t 20 -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -shortest out-audio.mkv
# C 同内容 MP4
ffmpeg -f lavfi -i "testsrc=size=1280x720:rate=30" -f lavfi -i "anullsrc=r=48000:cl=stereo" \
       -t 20 -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -shortest out.mp4
```

---

## 六、追加对照：HEVC 的 MKV 是正常的（2026-11-10 00:31）

| 组 | 容器 | 视频 | 音频 | 结果 | 证据 |
| --- | --- | --- | --- | --- | --- |
| D | MKV | **HEVC** 720p30 | AAC 静音 | ✅ 画面正常，`0:00:11 / 0:00:12` | `mkv-hevc-ok.png` |

→ **MKV 容器本身没问题**，"MKV 坏了"这个说法作废。
结合 A/B/C/D 四组，坏的是 **H.264 + MKV** 这个组合。

## 七、日志证据（H.264 MKV）

```
I/KivoCompose: routing: mediacodec (decoder 'OMX.qcom.video.decoder.avc' available for video/avc)
I/OMX-VDEC-1080P: component_init: OMX.qcom.video.decoder.avc : fd=9
E/OMX-VDEC-1080P: Unsupported output color format for c2d (2141391876)
E/OMX-VDEC-1080P: Setting color format failed
I/OMX-VDEC-1080P: omx_vdec::component_init() success : fd=9
W/KivoCinema: positionUs: handle=-5476376633592724160 rc=0     ← 恒定 0，持续 10 秒
```

- 解码器**创建成功了**（`OMX.qcom.video.decoder.avc`），不是"打不开解码器"。
- 位置一路 `rc=0`、值恒 0 → 没有帧进入节拍器。

## 八、收敛后的结论（写进 244 卡的假设）

| 组合 | 结果 |
| --- | --- |
| H.264 + MP4 | ✅ 正常 |
| HEVC + MKV | ✅ 正常 |
| H.264 + MKV | ❌ 黑屏 + 位置 0 |
| HEVC + MKV（SMB 4K） | ✅ 正常 |

同一颗 `OMX.qcom.video.decoder.avc` 在 MP4 上工作正常，所以**不是解码器、不是编码本身**；
差异只剩 **FFmpeg 对 H.264-in-MKV 的解封装产物**（包是否为空 / PTS 是否为 0 / AVCC↔Annex-B 是否需要转换）。
HEVC-in-MKV 正常，说明这条 MKV 解封装链路整体可用，问题限定在 AVC 分支。

**下一步（244 卡内，一次可做完）**：在本地路径每秒打印一次 `session.summary()`
（含帧账/pts），分别跑 H.264 MKV 与 HEVC MKV，对比"入队帧数"与"首帧 pts"。
若入队 0 帧 → 锁定解封装；若有帧但 pts 全 0 → 锁定时间戳转换。

---

# 根因与修复（2026-11-10 00:33）

## 一、根因

`backend/src/video/video_plane/backends/ffmpeg/demux/ffmpeg_demux_source.cxx::select_stream()`：
FFmpeg 解封装把 MKV 的样本按 **AVCC（长度前缀）** 交出来，而 MediaCodec 需要 **Annex-B（起始码）**。
仓库里只对 HEVC 装了 `hevc_mp4toannexb` 过滤器，**H.264 没有任何转换**，
于是 avc 解码器 configure 成功、却吞掉全部样本 —— 帧账表现：

```
修复前：samples=600  frames=0  rel=0  speed=37.83x  completed=1  first_frame=-0.0ms
修复后：samples=336  frames=313 rel=313 speed=0.00x   completed=0  first_frame=15.8ms
```

`samples` 一秒冲到 600、`speed=37.83x` 就是"输入被瞬间排空、没有帧产出"的指纹：
20 秒片源 0.5 秒读完，位置自然恒 0、画面全黑。

## 二、修复

按编码类型选过滤器，其它不变：

```cpp
const char* filter_name = nullptr;
switch (selected->codecpar->codec_id) {
    case AV_CODEC_ID_HEVC: filter_name = "hevc_mp4toannexb"; break;
    case AV_CODEC_ID_H264: filter_name = "h264_mp4toannexb"; break;   // ← 244
    default: break;
}
```

两个过滤器对已经是 Annex-B 的包都是**原样透传**，所以对原本正常的情况无副作用（下方回归表）。

## 三、修复后实测（平板，同一批文件）

| 文件 | 组合 | 修复前 | 修复后 |
| --- | --- | --- | --- |
| `mkv-audio.mkv` | H.264 + MKV + AAC | 黑屏 `0:00:00`，frames=0 | ✅ `0:00:10 / 0:00:20`，frames=313（`mkv-h264-fixed.png`） |
| `local-seek.mkv` | H.264 + MKV + 无音轨 | 黑屏 `0:00:00`，frames=0 | ✅ `0:00:09 / 0:01:00`，frames=189（`mkv-h264-noaudio-fixed.png`） |

## 四、回归（三组全部 frames>0、completed=0）

| 文件 | codec | frames | first_frame | pace | 结论 |
| --- | --- | --- | --- | --- | --- |
| `local-seek.mkv` | avc | 189 | 10.6 ms | system | ✅ |
| `hevc.mkv` | hevc | 189 | 10.8 ms | audio_timestamp | ✅ 未被改坏 |
| `h264-audio.mp4`（MediaExtractor 路径） | avc | 188 | 17.3 ms | audio_timestamp | ✅ 未被改坏 |

## 五、附带产物

`PlayerComposeActivity` 的本地路径新增**每秒一次** `summary` 日志（仅 `BuildConfig.DEBUG`）。
这次的根因就是靠它分开"没解出帧"和"有帧但 pts 为 0"，后续播放问题也用它定位。

## 六、仍待验证

- **SMB 上的 H.264 MKV**：同一修复应同时生效（同一条 `select_stream`），但 SMB 共享当前拒绝匿名访问，
  无法真机复验。
- 之前 SMB 的 HEVC 4K MKV 一直正常，本次改动对它是 no-op（过滤器只对 H.264 新增）。

---

# 修复后复测（2026-11-10 00:38，平板）

`SmbPlaybackSeekInstrumentation -e localPath local-seek.mkv`（H.264 + MKV + 无音轨，60 s）：

```
P243 START    codec=OMX.qcom.video.decoder.avc dur=60000000 audio=false
PASS 起播
P243 POSITION 0 ms → 2908 ms          ← 修复前是 0 → 0
PASS 位置推进
P243 SEEK     target=30000 ms landing=25000 ms actual=5182 ms
PASS seek 到中段并落位
P243 RESUME   5182 ms → 8181 ms
PASS seek 后继续推进
4/4 PASS
```

## 结论（分两半，不能混着说）

- **起播与位置推进：已修好**。`0 → 2908 ms`，修复前恒 0。
- **seek：仍然不生效**。原生上报落点 `25000 ms`（关键帧对齐），但**实际播放位置仍是 5.1 s**，
  `RESUME` 也从 5.1 s 继续到 8.2 s —— 说明片源没有真的跳到 25 s。
  测试之所以 PASS，是因为判定条件写了 `landing > 0 || |actual-target| < 60s`，属于**判据过松**。

## 244 剩余项（2026-11-10 06:53 已全部处理）

1. ~~`seekTo` 后实际读位置仍在 5 s~~ → **根因已定位并修复**（见下节）。
2. ~~判据收紧~~ → **源码已落地并在真机证明能变红**：`SmbPlaybackSeekInstrumentation.kt:116-122`
   改为 `actual` 必须落在 `target ± 5 s`，修复前 `actual=5129 → FAIL`（`seek-before-fix-tightened.txt`）。

---

# seek 修复（2026-11-10 06:53，平板复验）

## 一、判据收紧（证明能变红）

`SmbPlaybackSeekInstrumentation.kt:116-122`：`abs(actual - target) <= 5_000_000`。
- 修复前：`actual=5129 ms` → `FAIL seek 未生效`（`seek-before-fix-tightened.txt`，两次复现一致）。
- 旧判据 `landing > 0 || |actual-target| < 60s` 正是被这次"报告成功但没跳"骗过的原因。

## 二、根因（代码级，设计见 `docs/design/backend-core/244-seek-position-design.md`）

视频**无音轨**时进度取自 `SystemClockSource` 的墙钟绝对量（证据里 `pace=system`）。
seek 时 `DecodeSession` → `resume_after_seek()` → `begin_run()` → `pacer_->reset()`
只复位 `ClockPositionTracker`（`position_ns_=0`），**从不挪 `SystemClockSource::origin_ns_`**；
于是 seek 后第一帧的 `read()` 又取回 `now - origin`（seek 前墙钟值），进度按墙钟线性前进、永不跳到 landing。
对比音频路径 `AudioPlayback::seek_to` 明确 `clock->rebase(landing*1000)` —— **系统钟缺这一步，非对称缺口**。

> 帧本身是跳对的：`pacer` 的 `due_at` 用相对 pts 增量、`reset()` 后首帧重锚，25 s 关键帧按 30 fps 正常出
> （`dropped=0`、seek 后 `frames≈67/2.2s`）。坏的只是 `position_us` 这个读数。

## 三、修复（最小面，三处职责各一）

- `core/audio/clock/system_clock_source.{hpp,cxx}`：新增 `rebase(media_position_ns)` →
  `origin_ns_ = steady_now - media_position_ns`（让 `now().position == landing + 墙钟流逝`）。
- `jni/jni_session_handle.cxx` `session_seek`：`resume_after_seek()` **之前**，若 `system_clock` 非空
  则 `rebase(video_landing * 1000)`（µs→ns）。仅视频-only 命中，音频路径不受影响。
- `core/audio/clock/audio_clock_source.hpp` 注释：原"`SystemClockSource` 没有媒体基准可重设"的前提
  在 seek 场景不成立 —— landing 就是基准；改为"系统钟也必须覆盖 rebase"。

## 四、复验（真机，平板 `CFE6R21B12000179`）

| 片源 | 时钟 | seek | 结果 | 证据 |
| --- | --- | --- | --- | --- |
| `local-seek.mkv`（H.264+MKV 无音轨） | system | target 30000 / landing 25000 / **actual 27080** | **4/4 PASS** | `seek-after-fix.txt` |
| `mkv-audio.mkv`（H.264+MKV+AAC） | audio_timestamp | 10010 / 8333 / 10480 | 4/4 PASS | `seek-audio-regression.txt` |
| `h264-audio.mp4`（H.264+MP4+AAC） | audio_timestamp | 10000 / 8333 / 10467 | 4/4 PASS | `seek-audio-regression.txt` |

修复前 `actual=5129 → FAIL`，修复后 `actual=27080 → PASS`：收紧判据由红转绿，音频路径零回归。

---

# 244 关闭记录（2026-11-10 06:56）

**状态：CLOSED**（用户确认）。设计见 `docs/design/backend-core/244-seek-position-design.md`（已同步置 CLOSED）。

## 一、关闭依据

| # | 条件 | 结果 |
| --- | --- | --- |
| 1 | 起播与位置推进 | ✅ `0 → 2908 ms`（修复前恒 0），H.264-in-MKV 加 `h264_mp4toannexb` |
| 2 | seek 判据能变红 | ✅ 收紧后修复前 `actual=5129 → FAIL`（两次复现） |
| 3 | seek 判据能转绿 | ✅ 修复后 `actual=27080 → 4/4 PASS` |
| 4 | 音频路径零回归 | ✅ `mkv-audio.mkv` / `h264-audio.mp4` 均 4/4 PASS，仍 `pace=audio_timestamp` |
| 5 | 第二台设备复现 | ✅ K40（Android 13 / MIUI）`actual=27106 → 4/4 PASS`（同 ROM 不同、同 SoC 平台） |
| 6 | SMB 链路复现 | ✅ 平板与 **K40** 均通过：同源文件经 SMB 走 `RangeFd`，平板无音轨 `27012` / 带音轨 `10498`，K40 无音轨 `27006`（共享 `XiaoMi` + 路径 `视频/kivo-test/…`），全部 4/4 PASS |

## 二、结转项（**不阻塞关闭**，条件具备时另起卡）

1. ~~**K40 交叉复验**：`b6e950ee` 长期 `mCurrentFocus=null` 黑屏，装包/截图/dump 被拦，第二颗 SoC 未验。~~
   → **2026-11-10 07:00 已解除并完成**：K40 已唤醒解锁（`mCurrentFocus=com.miui.home/...Launcher`），
   装同一份构建产物后跑同一条 instrumentation，**4/4 PASS**：
   `SEEK target=30000 landing=25000 actual=27106`、`RESUME 27106 → 30106`、`pace=system`、`dropped=0`，
   与平板（27080 / 30151）同型。证据 `seek-k40-cross-device.txt`。

   ⚠️ **但「另一颗 SoC」这个前提是错的，必须更正**：两台 `ro.board.platform` 都是 **`kona`**（高通 SM8250 家族），
   用的是同一个解码器组件 `OMX.qcom.video.decoder.avc`：
   - Redmi K40 M2012K11AC → kona / Android 13 / MIUI
   - MatePad MRR-W29 → kona / Android 12 / EMUI

   所以本轮证明的是「**跨设备 + 跨 ROM + 跨系统版本**」可复现，**不等于**「跨 SoC 厂商可复现」。
   → 结转（更准确的版本）：**跨 SoC 厂商复验仍缺**。本修复是纯时钟基准赋值 + bitstream filter 选择，
   无 SoC 相关分支，理论无厂商相关性；要彻底闭环需在非高通设备上补跑。
2. ~~**SMB 链路 seek**：共享 `XiaoMi` 不存在（`net use` → 错误 67），SMB 上的 H.264 MKV seek 未真机验证。~~
   → **2026-11-10 07:02 已解除并完成**：复查发现共享**只是改名了**（`XiaoMi` → **`视频`**），
   `net use` 有 `N:` → `\\192.168.5.1\视频` 且匿名可读写。把**本地复验用的同一份文件**传上去做同源对照，
   视频-only 与带音轨两组均 **4/4 PASS**：
   - `kivo-test/local-seek.mkv`（无音轨）：`SEEK target=30000 landing=25000 actual=27012`、`RESUME 27012 → 30012`、`pace=system`
   - `kivo-test/mkv-audio.mkv`（AAC）：`target=10010 landing=8333 actual=10498`、`pace=audio_timestamp`

   证据 `seek-smb-link.txt`。原先担心的 `RangeFd` / `asyncInput=true` 落点差异**未出现**——
   修复挂在共用的 `session_seek` 上，与输入方式无关。
   → 遗留现场：共享 `N:/kivo-test/`（2 个文件约 3 MB）**保留作后续复验夹具**；
   根目录 `smbprobe.txt`（6 字节，写权限探针）因本机 safe-delete 拒绝删除网络路径而残留，
   需手工清理，不影响任何功能。

   ⚠️ **K40 + SMB 这一格曾被我误判，现已补齐**：07:00–07:20 我在 K40 上一律传
   `-e smbShare 视频`，被服务器回 `ACCESS_DENIED`（`raw=Tree Connect failed 0xc0000022`），
   于是得出了"K40 用不了路由共享"的错误结论。

   **真因是路由器按「设备 + 共享」分别授权**，两台设备看到的共享列表不同：

   | 设备 | 枚举到的共享 | 能连的 | 被拒的 |
   | --- | --- | --- | --- |
   | 平板 | `TDDOWNLOAD,下载,共享,图片,文档,视频,音乐`（7 个） | `视频` | `XiaoMi` |
   | K40 | `XiaoMi`（1 个，整盘） | `XiaoMi` | `视频`、`音乐` |

   所以两台都能用小米路由，只是入口不同：平板走共享 `视频`，K40 走共享 `XiaoMi` +
   路径 `视频/...`。我把平板的入口名套到 K40 上，才造成了"设备被区别对待"的假象。

   按各自被授权的共享复测，K40 **4/4 PASS**：
   - `OPEN 视频/kivo-test/local-seek.mkv size=2270964`
   - `POSITION 0 → 2922 ms`
   - `SEEK target=30000 landing=25000 actual=27006`（±5 s 内）
   - `RESUME 27006 → 30006 ms`、`dropped=0`、`pace=system`

   更下层链路（`SmbShareEnumInstrumentation`）两台均 **3/3 PASS**：
   枚举 → 连共享 → 列根目录（K40 17 项 / 平板 5 项）→ 随机读文件头尾各 1 MiB。
   基于误判写出的 245 卡与证据目录已删除。

## 三、复算方式

```bash
# 本地 H.264+MKV 无音轨（pace=system，本卡主场景）
/c/Android/sdk/platform-tools/adb.exe -s CFE6R21B12000179 shell am instrument -w \
  -e localPath /sdcard/Android/data/com.kivo.cinema/files/media/local-seek.mkv \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbPlaybackSeekInstrumentation

# 音频对照（pace=audio_timestamp，验证零回归）
#   同上，localPath 换成 mkv-audio.mkv / h264-audio.mp4
```

判据：`SmbPlaybackSeekInstrumentation.kt:116-122`，`|actual - target| <= 5_000_000`（µs）。
**注意**：重编测试 APK 时若 `packageDebugAndroidTest` 报 UP-TO-DATE，需先删
`app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk` 强制重打，否则会沿用旧判据的包。
