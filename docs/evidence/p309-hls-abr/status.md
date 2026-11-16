# 309 · HLS / ABR 能力评估（严格对照 275 §1.4 / §5.3 / §6.2）

状态：**能力已实测；结论是「HLS 直通 + 手动切档可用，ABR 完全不具备」**
设备：K40 `b6e950ee`
时间：2026-11-16 04:41–05:20
测试流：本地 3 档 HLS（`testsrc2`，360p/720p/1080p，24 s，master + 每档 3 个 4 s 分片）

## 一、275 的原文要求（先摆出来，逐条对）

| 出处 | 原文 |
| --- | --- |
| §1.4 | 「HLS / DASH：现有可追溯记录没有证明其 manifest、分片和变体切换链路 —— **待验证**；不得宣称支持或支持 ABR」 |
| §1.4 | 「自适应码率：没有证据指出吞吐估计、buffer rule、切换阈值、升降档迟滞或切换时机 —— **未还原**」 |
| §5.3 | 「**先验证所选引擎实际支持相应 HLS/DASH 变体，再实施 ABR；不能因为 APK 中有 FFmpeg 命名库就宣称支持。**」 |
| §5.3 | ABR 目标算法：每段采样有效吞吐 → EWMA 抑制单段波动；可选 representation 峰值带宽必须低于安全吞吐预算；同时满足 **decoder profile / 屏幕分辨率 / buffer runway**；升档需**连续多窗口**有余量；降档在 **runway 逼近下限**时快速执行；设**最短驻留时间**与**升降档不同阈值**；切档原因/前后 representation/估计带宽/buffer 变化**必须可观测** |
| §6.2 | 「多 representation 切换**无 ABR 振荡**；切换**无明显黑帧**；验证 **manifest、segment、鉴权、seek 与网络恢复**完整链路」 |

## 二、实测对照（每条都给出证据或明确标注未测）

| 275 条目 | 我们的实测 | 判定 |
| --- | --- | --- |
| **manifest 解析** | `Found 'hls' at score=100`，三个子清单 `v0/v1/v2.m3u8` 全被打开 | ✅ **通过** |
| **segment 拉取** | `HLS request for url '.../v0_000.ts', offset 0, playlist 0`，分片经我们的 `LocalRangeServer` 转发 | ✅ **通过** |
| **多 representation 识别** | 3 档暴露为 **3 个独立 video track**：`--vid=1` 640x360 900k / `--vid=2` 1280x720 2700k / `--vid=3` 1920x1080 5200k | ✅ **通过** |
| **默认选档** | 启动即选 `--vid=3`（1080p）= mpv 文档的「Default: max」 | ✅ **与文档一致** |
| **变体切换（手动）** | `Set property: vid="1"` → 104 ms 后新解码器就绪 → **293 ms** 完成 `VO reconfig` | ✅ **可用** |
| **切换无明显黑帧** | 录屏逐帧 YAVG：切档窗口（8.0–8.2 s）稳定 32.7，**无一帧掉到 16（黑）**；仅 8.84→8.94 s 有约 100 ms（≈3 帧）缺口 | ✅ **基本达标**（有帧缺口，无黑屏） |
| **鉴权** | 未测（本地无鉴权 HLS） | ❌ **未测** |
| **seek** | **未测** —— URL 路径没有 seek 钩子（`kivo.seekMs` 只在 SMB 路径生效），HLS 又走不了 SMB | ❌ **未测** |
| **网络恢复** | 未测（HLS 分片级断网恢复与 SMB 不同，本轮时间不够） | ❌ **未测** |
| **ABR（自适应）** | **完全不具备**（见第三节） | ❌ **无** |
| **ABR 无振荡** | 不适用 —— 没有 ABR 就没有振荡 | — |

## 二之二、★ 补测：**结构 B（带 `#EXT-X-MEDIA`）根本切不了**

上表的「多档识别 ✅ / 手动切档 ✅」**只对结构 A（我自建的简单 master）成立**。
用 Apple 官方多码率流复测后，结论要改写：

| | 结构 A（无 `#EXT-X-MEDIA`） | 结构 B（有 `#EXT-X-MEDIA`，**Apple / Netflix / Emby fMP4 全是这种**） |
| --- | --- | --- |
| master | 我自建 3 档 | Apple 官方：**24 个 `EXT-X-STREAM-INF` + 5 个 `EXT-X-MEDIA`** |
| FFmpeg 解析 | 1 program + N video stream | **N 个 program**（`ffprobe -show_programs` → `program_id=0..23`，带 `variant_bitrate`） |
| mpv 暴露 | N 个 track | **1 个 track**（只当前 program：`--vid=1` 960x540） |
| 运行时切档 | ✅ 293 ms、无黑帧 | ❌ **不能** |

**归因（不是我们的 bug）**：
- 代理 `resolve()` 正确，**8 个 variant 全部被成功打开**
  （`hls: Opening '.../v5/prog_index.m3u8'` … `v9/v8/v7/v6/v4/v3/v2`）；
- `ffprobe` 直连也能看到全部 stream（960x540 / 1920x1080×3 / 1280x720 / 768x432）；
- **mpv 没有 `--program` 选项**（`options.c` 无此定义）⇒ 一次只能播一个 program，且运行时无法换。

⇒ **真实 HLS 流在 mpv 路径上做不了运行时 ABR。** 这条改变了 309 的最终结论，
也直接决定了 310 的设计必须绕开"切 track"这条路。

> 附带发现：公网流还出现 `Failed to open segment 1 of playlist 0` 与 38 次
> `PROXY 直通 fail: Connection reset / Broken pipe`。**未归因**（CDN 限速？代理转发？），
> 记入待办，不影响上面的 program 结论。

## 三、ABR 为什么「完全不具备」——三条硬证据

### 3.1 mpv 官方文档自己否定了运行时切档做 ABR

`DOCS/man/options.rst:935-938`：

> Although this mechanism makes it possible to switch streams at runtime,
> **it's not suitable for this purpose** for various technical reasons.
> (**It's slow, which can't be really fixed.**) In general, this option is not useful,
> and was only added to show that it's possible.

### 3.2 `--hls-bitrate` 只是**启动时的静态选择**

`DOCS/man/options.rst:5371`：

```
--hls-bitrate=<no|min|max|<rate>>
    :no:   Don't do anything special.
    :min:  Pick the streams with the lowest bitrate.
    :max:  Same, but highest bitrate. (Default.)
    Additionally, if the option is a number, the stream with the highest rate
    equal or below the option value is selected.
    The bitrate as used is sent by the server, and there's no guarantee it's
    actually meaningful.
```

- 语义是「**selected by default**」——一次性，不是持续决策；
- 默认 `max` 已在我们实测中确认；
- 源码 `options/options.c:645` 定义为 `OPT_CHOICE`（`no/min/max`），**没有吞吐输入**。

### 3.3 没有任何吞吐估计 / buffer rule / 迟滞 / 驻留时间

mpv 侧能拿到的只有：
- `demuxer-cache-state` 的 `raw-input-rate`（**估算值**，非分段测量）；
- `cache-duration`（runway）。

而 275 要的「EWMA 抑波、峰值带宽低于安全预算、连续多窗口余量才升档、runway 逼近下限快速降档、最短驻留时间、升降档不同阈值」——**一条都没有实现**，也不在 mpv 的可配置项里。

## 四、按 275 的算法要求，自研 ABR 要做什么

mpv 提供的是**两件基础件**，够搭，但要我们自己做决策层：

| 275 要求 | 我们已有的基础 | 缺什么 |
| --- | --- | --- |
| 每段采样有效吞吐 | `OriginLatencyStats`（单次请求字节数 + 耗时） | 需要**按分片**切分（现在按请求） |
| EWMA 抑波 | 无 | 新增 |
| 峰值带宽 < 安全吞吐预算 | `OriginSupplyPressure`（runway/underrun） | 需要把"预算"形式化 |
| 满足 decoder profile / 分辨率 | `Av1HardwareProbe`（302）+ `resolutionTier` | 可以做 codec×分辨率闸门 |
| 升档需连续多窗口余量 | 无 | 新增 |
| 降档快速（runway 逼近下限） | `RebufferPolicy` 已有 runway 判定 | 可复用 |
| 最短驻留时间 | 无 | 新增 |
| **切换执行** | `set vid <n>`（实测 293 ms、无黑帧） | **已有** |
| 切档原因可观测 | `NetDiag` 通道 | 需要结构化成 metric |

**结论：自研 ABR 在技术上可行**，因为最危险的"切换动作"已被实测证明可用（293 ms、无黑帧、无 ABR 振荡问题因为没有自动逻辑）。
工作量集中在**决策层**（吞吐采样 + EWMA + 迟滞 + 驻留），约 1 个独立卡。

### 4.1 但有两个必须先确认的前提

1. **切档会重开 demuxer**：日志显示 `Decoder format` 与 `VO reconfig` 都重建了。
   293 ms 在**本地 HTTP** 上成立；**公网/慢源**下切档要多久？未测 —— 这决定 ABR 在真实网络下是否可用。
2. **音频跟着切**：3 档各带一条 aac（`--aid=1/2/3`），切 `vid` 时音频要不要跟着换、
   会不会爆音/断音，**未测**。

## 五、结论

| 问题 | 答案 |
| --- | --- |
| 我们「支持 HLS」吗？ | ✅ **支持**：master/segment/多档识别/手动切档全部实测通过 |
| 我们「支持 ABR」吗？ | ❌ **不支持**，且**不能宣称**。275 §1.4 的要求一条都没实现 |
| mpv 能替我们做 ABR 吗？ | ❌ **不能**。官方明说运行时切档不适合此用途，`hls-bitrate` 只是静态选择 |
| 自研 ABR 可行吗？ | ⚠️ **技术可行**（切换动作已验证 293 ms/无黑帧），但**前提未验证**：公网切档耗时、音频跟随 |

**因此 275 的「HLS / DASH 支持与 ABR」这一格，现状应记为：**
> HLS **直通可用**（结构 A 的 manifest/segment/多档/手动切档已实测）；
> **结构 B（Apple/Netflix/Emby 风格）只能播到 program 0**，运行时切档不可行（mpv 无 `--program`）；
> **ABR 未实现**；DASH **完全未测**（连 manifest 都没验过）。

## 六、未测项（不许当成通过）

1. **seek**（URL 路径无钩子，HLS 走不了 SMB 路径）；
2. **鉴权**（本地无鉴权 HLS；401/403、Cookie、DRM 未测）；
3. **网络恢复**（HLS 分片级断网恢复）；
4. **DASH**（MPD / 分片模板 / 时间线，一条都没验）；
5. **公网 HLS**（本地 HTTP 的 293 ms 切档不能外推到公网）；
6. **移动端 HLS**（`#EXT-X-MEDIA`、多音轨、字幕组）；
7. **音频跟随切档**（爆音/断音）。

## 七、复算命令

```bash
# 1) 生成 3 档 HLS（testsrc2，24 s）
mkdir -p /tmp/hls-test && cd /tmp/hls-test
for spec in "v0 640 360 800" "v1 1280 720 2500" "v2 1920 1080 5000"; do
  set -- $spec
  ffmpeg -y -loglevel error -f lavfi -i "testsrc2=size=$2x$3:rate=25" \
    -f lavfi -i "sine=frequency=440:sample_rate=48000" -t 24 \
    -c:v libx264 -preset ultrafast -b:v ${4}k -c:a aac -b:a 96k \
    -f hls -hls_time 4 -hls_playlist_type vod -hls_segment_filename "$1_%03d.ts" "$1.m3u8"
done
# master.m3u8 手写三档（见本目录 master.m3u8）

# 2) 静态服务 + 反代
python -m http.server 8200 --bind 127.0.0.1 &
adb -s b6e950ee reverse tcp:8200 tcp:8200

# 3) 起播（默认选 max = 1080p）
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8200/master.m3u8"

# 4) 切档（起播后 3 s 生效）
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8200/master.m3u8" --es kivo.mpvOpt "vid=1"

# 5) 黑帧判定（录屏 + 逐帧 YAVG）
adb -s b6e950ee shell "screenrecord --time-limit 12 /sdcard/sw.mp4 & sleep 1; am start ...; wait"
adb -s b6e950ee pull /sdcard/sw.mp4 .
ffmpeg -y -i sw.mp4 -an -vf "signalstats,metadata=print:key=lavfi.signalstats.YAVG:file=yavg.txt" -f null -
# YAVG<=20 即黑帧；切档窗口若稳定在 30+ 则无黑帧
```

## 八、证据文件

| 文件 | 内容 |
| --- | --- |
| `switch-360p.png` | 切到 360p 后的画面（正常，非黑屏） |
| `sw.mp4` | 12 s 录屏（含起播 + 切档瞬间） |
| `yavg.txt` | 逐帧 YAVG（526 帧） |
| `master.m3u8` | 三档 master（与本文档同目录留档） |
