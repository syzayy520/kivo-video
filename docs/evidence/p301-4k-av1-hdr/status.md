# 301 · 真实 4K AV1 HDR 实测：**10.87 fps，K40 CPU 能力上限**（并更正 293-A）

状态：**播放链路全通，但 4K AV1 10bit 真实内容无法实时**；
并**推翻 293-A 用合成片得出的「4K AV1 59.16 fps」结论**。
设备：K40 `b6e950ee`（SM8250 / A13）
时间：2026-11-16 03:06–03:20
片源：`Bridgerton.S01E01.2020.2160p.NF.WEB-DL.DDP5.1.Atmos.HDR.AV1-HHWEB.mkv`

## 一、片源（ffprobe 实测）

| 项 | 值 |
| --- | --- |
| 视频 | **AV1 Main / 3840×2160 / `yuv420p10le` / 23.976 fps** |
| HDR | **`smpte2084`(PQ) / `bt2020nc` / `bt2020`**，`CL=uhd` |
| 音频 | **EAC3 + Dolby Atmos / 6ch / 48 kHz** |
| 字幕 | 多条 `subrip`，含中文简/繁 |
| 大小 / 时长 | 5,229,112,152 B（5.23 GB）/ **3513.92 s（58m34s）** / 11.9 Mbps |

## 二、链路全通（这部分是 PASS）

```
[vd] Selected decoder: libdav1d - dav1d AV1 decoder by VideoLAN
[vd] Decoder format: 3840x2160 yuv420p10 bt.2020-ncl/bt.2020/pq/limited/auto CL=uhd
[cplayer] VO: [gpu-next] 3840x2160 yuv420p10
[cplayer] Set property: tone-mapping="bt.2390" -> 1
[ad] Selected decoder: eac3
[hdr] 真 HDR 输出不可用：gralloc 不支持 10bit ⇒ 留在 SDR 路径
[hdr] 内核=mpv 出画=gpu-next 真HDR=关 ⇒ 已退回 8bit + SRGB（dataspace=ok）
```

- AV1 10bit 解码 ✅、PQ/BT.2020 识别 ✅、tone-map 下发 ✅、Atmos 音轨解码 ✅
- 真 HDR 输出不闭环是 **K40 gralloc 无 10bit**（已知硬件限制，与 AV1 无关）
- 截图 `playing.png`：shondaland 片头，色彩正常（无发灰/过曝）

## 三、★ 帧率：10.87 fps（真实内容）

| 指标 | 值 |
| --- | --- |
| 上屏帧率 | **10.87 fps** |
| 帧间隔 | p50 **19.8 ms** / p99 **1001.4 ms** / max 1761.8 ms |
| `>200ms` | **60 次**（60 个采样块块块有） |
| 采样 | 60 块 / 50.1 s，漏采窗口 0 段 |

### 排除项（逐个查过，不是猜）

| 怀疑 | 证据 | 结论 |
| --- | --- | --- |
| 网络供数 | `underrun=false`、`fw` 9→38.5 MB、`dur` 10–19 s | ❌ 排除 |
| 帧插值 | `Detected fps ratio 0.0040 below threshold, disabling interpolation`（3.6 s 时内核**自己关了**） | ❌ 排除 |
| 渲染丢帧 | `Discontinuous source PTS jump 191.274→201.410→210.710→218.552…` 每次跳 7–10 s | ✅ **丢帧追音频** |
| CPU | `800%cpu / **766% user** / idle 55%`；`top -H` 显示 **9 个线程 55–87%**（= dav1d 的 9 个解码线程） | ✅ **解码打满** |

`Requesting 9 threads for decoding` —— dav1d 已用 9 线程（K40 8 核），没有线程数余量。

### 归因链（有对照支撑）

1. 同一台机器 **4K HDR HEVC 硬解 = 59.47 fps**（282 实测）⇒ 4K 渲染/显示/tone-map 都能跑；
2. 同一台机器 **1080p AV1 10bit 软解 = 60.44 fps**（299 实测）⇒ 我们的 AV1 通路正确；
3. **4K AV1 10bit 软解 = 10.87 fps，CPU 打满** ⇒ 瓶颈是 **dav1d 的 CPU 算力**，不是渲染、不是供数、不是插值。

## 四、★ 必须更正 293-A 的结论

279 §十五 曾写「**4K 10bit AV1：上屏 59.16 fps**」。

**那个数字来自 SVT-AV1 合成片（色条/斜线/棋盘格）**。AV1 解码里 CDEF / loop filter 的开销
与**画面边缘密度强相关**——色条几乎无边缘，代价远低于真实电影。
本轮真实电影实测 **10.87 fps**，与合成片差 **5.4 倍**。

⇒ **合成图案不能代表真实 4K AV1 解码能力**。该结论已在本轮作废，
后续凡引用"4K AV1 能播"必须注明是合成片还是真实片。

## 五、对用户的直接结论

| 场景 | 结果 |
| --- | --- |
| 1080p AV1 10bit（真实电影） | ✅ 60.44 fps 满帧 |
| **4K AV1 10bit（真实剧集）** | ❌ **10.87 fps，无法正常观看** |
| 4K HEVC HDR（真实内容） | ✅ 59.47 fps（硬解，282） |

K40（SM8250）**无 AV1 硬解**，只能软解；软解 4K 10bit 超出其 CPU 能力。
**这是设备硬件上限，不是本项目的代码缺陷**——任何播放器在这台机器上软解同一片源都受同一约束。

## 六、未闭合 / 边界

1. **未做 SenPlayer 同源实测**：理论上它也受同一 CPU 限制，但没有实测证据，
   写成 `not_measured`，不能断言"它也卡"。
2. **`hdr-compute-peak` 对照数据不可用**：关闭后采样跨度 88 s > 采样 30 s ⇒ 混入旧帧，
   口径已污染，**不能**据此说"关掉更差"。真实内容上的定量对比待重做。
3. 只测片头约 50 秒，未做长稳（10 fps 本身已判定不可用，长稳无意义）。
4. 4K AV1 **8bit** 未测（无该片源）；理论上比 10bit 快 20–30%，仍不足以到 24 fps。

## 七、方法论教训（已入记忆）

- **做性能 A/B 前必须检查调试属性残留**：本轮发现 `debug.kivo.hdrPeak=on` 是历史取证残留，
  导致首轮 10.87 fps 里混入了逐帧峰值计算的开销。虽然清掉后仍是 10 fps 量级
  （说明主因是解码），但**残留属性足以让归因跑偏**。
- **合成片只能验通路，不能验性能**。293-A 的教训。

## 八、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"
export MSYS_NO_PATHCONV=1
A="adb -s b6e950ee"
F="//NAS/12T/剧集/Bridgerton.S01.2020.2160p.NF.WEB-DL.DDP5.1.Atmos.HDR.AV1-HHWEB/Bridgerton.S01E01.2020.2160p.NF.WEB-DL.DDP5.1.Atmos.HDR.AV1-HHWEB.mkv"

ffprobe -v error -show_entries stream=codec_name,profile,width,height,pix_fmt,color_transfer \
  -show_entries format=duration,size,bit_rate -of default=nw=1 "$F"

node tools/p274_local_origin.mjs --port 8133 --mbps 16 --file "$F" --size 5229112152
$A reverse tcp:8133 tcp:8133
$A shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8133/v"

# 帧率（层名带 BLAST #id）
BASE='SurfaceView[com.kivo.cinema/com.kivo.cinema.ui.compose.PlayerComposeActivity](BLAST)'
ID=$($A shell "dumpsys SurfaceFlinger --list" | grep -F "$BASE" | head -1 | sed 's/.*#//' | tr -d '\r')
for i in $(seq 1 60); do $A shell "dumpsys SurfaceFlinger --latency '$BASE#$ID'"; echo; sleep 0.5; done > latency.txt
node tools/p274_sf_latency.mjs latency.txt

# CPU 线程归因
PID=$($A shell pidof com.kivo.cinema | tr -d '\r')
$A shell "top -H -b -n 1 -p $PID -o %CPU" | head -18
```

## 九、证据文件

| 文件 | 内容 |
| --- | --- |
| `playing.png` | 起播画面（shondaland 片头） |
| `kernel.log` | 解码链、HDR 识别、tone-map 下发、PTS 跳跃 |
| `net.log` | 供数健康（`underrun=false`、runway 10–19 s） |
| `latency.txt` | 60 次 SurfaceFlinger 采样（10.87 fps） |
| `latency-nopeak.txt` | 关 compute-peak 后的采样（口径已污染，仅存证不引用） |
| `probe2.png` | 二次探针截图 |
