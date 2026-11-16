# 299 · 真实 AV1 电影实测（补 275 §6.2「真实片源」缺口）

状态：**通过（1080p 10bit 软解满帧 + 5.1 直通 + 远跳 708ms）**；
设备：K40 `b6e950ee`（SM8250，A13）
时间：2026-11-16 02:34–02:40
片源：用户 02:25 新下载的真实 AV1 电影（非自造测试片）

## 一、片源（ffprobe 实测，不是文件名推断）

| 项 | 值 |
| --- | --- |
| 文件 | `[奥德赛].The.Odyssey.2026.USA.V2.BluRay.1080p.AV1.OPUS.5.1-CMCT.mkv` |
| 大小 / 时长 | **3,691,322,886 B（3.44 GiB）/ 10,355.687 s（2h52m36s）** |
| 总码率 | 2,851,629 bps（2.85 Mbps） |
| 视频 | **AV1 Main / 1920×1080 / `yuv420p10le`（10bit）/ 23.976 fps** |
| 音频 | **OPUS / 48000 Hz / 6ch（5.1）** |
| 字幕 | 4 条 `hdmv_pgs_subtitle`（PGS 位图） |
| 容器 | matroska（**cues 在片尾**：`Seeking to 3690870935 to read header element`） |

> 与 293-A 的差别：那轮是**自造 SVT-AV1 合成片**，这轮是**真实编码器 + 真实电影 + 10bit + OPUS**。

## 二、解码路径（关键：硬解被拒 → dav1d）

```
[vd]     libdav1d (av1) - dav1d AV1 decoder by VideoLAN
[vd]     av1 - Alliance for Open Media AV1
[vd]     av1_mediacodec (av1) - AV1 Android MediaCodec decoder
[vd] Looking at hwdec av1_mediacodec-mediacodec...
[vd] Trying hardware decoding via av1_mediacodec-mediacodec.
[ffmpeg/video] av1_mediacodec: Unsupported or unknown profile   ← 硬解拒绝
[vd] Using software decoding.
[vd] Selected decoder: libdav1d - dav1d AV1 decoder by VideoLAN
[vd] Decoder format: 1920x1080 yuv420p10 auto/auto/auto/limited/auto
[cplayer] VO: [gpu-next] 1920x1080 yuv420p10
```

- **不是静默降级**：K40（SM8250）本无 AV1 硬解，`av1_mediacodec` 报
  `Unsupported or unknown profile` 后由 mpv `hwdec-software-fallback` 走 dav1d，
  日志完整留痕。这是设备能力边界，不是我们的回退策略放宽。
- 10bit 全程保持：`yuv420p10` 进 gpu-next，未降到 8bit。

## 三、上屏帧率（SurfaceFlinger 口径，60 次采样 / 42.7 s）

| 指标 | 值 |
| --- | --- |
| 上屏帧率 | **60.44 fps**（源 23.976，帧插值补到 60） |
| 帧间隔 | p50 **16.7 ms** / p99 **24.8 ms** / max 49.8 ms |
| `>200ms` | **0 次** |
| 采样覆盖 | 60 块，漏采窗口 0 段 |

⇒ dav1d **软解 1080p 10bit AV1 满帧**，与 293-A 合成片的 60.53 fps 一致。

## 四、音频（OPUS 5.1 直通，未降混）

```
[af] [in] 48000Hz 5.1 6ch floatp
[ao/audiotrack] requested format: 48000 Hz, 5.1 channels, floatp
[ao/audiotrack] Channel layouts: ... 5.1 ... 7.1
AO: [audiotrack] 48000Hz 5.1 6ch float
audio ready
```

无 `Audio device underrun`；**5.1 六声道直出，未 downmix 到 stereo**。
OPUS 解码器（ffmpeg 原生 `opus`）正常，无需 libopus 外部库。

## 五、远跳 9000 s（3.44 GiB 文件 / 片尾 cues）

| 事件 | 相对时刻 |
| --- | --- |
| `seek target=9000` | 4.280 s |
| `stream level seek 117581850 → 3394055406`（跳到 3.39 GB 处） | 4.281 s |
| `mkv seek done` | 4.327 s |
| `first video frame after restart shown` | **4.988 s** |
| `playback restart complete @ 9000.033000` | 5.033 s |

- **seek → 首帧 708 ms**；**落点 9000.033 s，误差 33 ms**。
- 索引可正常读：`Parsing seek head` → 片尾 `0x1c53bb6b`/`0x1254c367` 元素 → `Parsing cues...`。
- 截图 `seek9000.png`：剧场人群画面正常，非黑屏、非错误帧。

## 六、未闭合 / 边界（不许当成已完成）

1. **1080p，不是 4K**。4K AV1 仍是 293-A 的合成片结论，真实 4K AV1 电影未测。
2. **只测两个时间点**（起播 + 9000 s），未做 30 分钟长稳；长时间热降频未知。
3. **字幕字体缺口**：日志
   `can't find selected font provider` / `fontselect: failed to find any fallback with glyph 0x0
   for font: (Noto Sans CJK JP, 400, 0)` ⇒ ASS/文本字幕缺字体；PGS 是位图不依赖字体，
   但**本轮未开字幕，PGS 是否上屏未验证**（与 275 §5.4 的 P6-2 未闭环一致）。
4. 供片装置按 4 Mbps 恒定（真实 2.85 Mbps 的 1.4 倍），**未测低带宽下的表现**。
5. K40 真 HDR PQ 输出仍不闭环（gralloc 无 10bit），与 AV1 无关。

## 七、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"
F="//NAS/12T/电影/[奥德赛].The.Odyssey.2026.USA.V2.BluRay.1080p.AV1.OPUS.5.1-CMCT/[奥德赛].The.Odyssey.2026.USA.V2.BluRay.1080p.AV1.OPUS.5.1-CMCT.mkv"

# 1) 片源事实
ffprobe -v error -show_entries stream=codec_name,profile,width,height,pix_fmt,r_frame_rate \
  -show_entries format=duration,size,bit_rate -of default=nw=1 "$F"

# 2) 供片装置（4 Mbps）
node tools/p274_local_origin.mjs --port 8133 --mbps 4 --file "$F" --size 3691322886
adb reverse tcp:8133 tcp:8133

# 3) 起播
adb shell am force-stop com.kivo.cinema
adb shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log'"
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8133/v"

# 4) 上屏帧率（层名必须带 BLAST #id）
BASE='SurfaceView[com.kivo.cinema/com.kivo.cinema.ui.compose.PlayerComposeActivity](BLAST)'
ID=$(adb shell "dumpsys SurfaceFlinger --list" | grep -F "$BASE" | head -1 | sed 's/.*#//' | tr -d '\r')
for i in $(seq 1 60); do adb shell "dumpsys SurfaceFlinger --latency '$BASE#$ID'"; echo; sleep 0.5; done > latency.txt
node tools/p274_sf_latency.mjs latency.txt

# 5) 远跳
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8133/v" --ei kivo.seekMs 9000000
```

## 八、证据文件

| 文件 | 内容 |
| --- | --- |
| `playing.png` | 起播画面（人物，非黑屏） |
| `seek9000.png` | 9000 s 落点画面（剧场人群） |
| `kernel.log` | 起播会话内核日志（解码链、AO、VO） |
| `net.log` | 起播会话供数（`underrun=false`、runway 10–18 s） |
| `latency.txt` + 输出 | 60 次 SurfaceFlinger 采样原始数据 |
| `seek-kernel.log` / `seek-net.log` | 9000 s 远跳会话日志 |
