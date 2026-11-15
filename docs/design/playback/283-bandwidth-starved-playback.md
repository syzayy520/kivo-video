# 283 · "一直卡"的现场取证：手机到源站的带宽不够，不是播放器

> 时间：2026-11-15 18:19–18:35（用户："现在就会卡了，你抓下日志" → "一直调校到极致完美为止"）
> 设备：K40 b6e950ee / Wi-Fi `xiaomi-HD` / 192.168.5.152（移动数据 `mobile_data=0`，已关）
> 片源：`Dept.Q.S01E01 2160p HDR`，20.3 Mbps = **2,537,500 B/s**

## 一、现场日志（用户说卡的那一刻）

```
[cache] fw=1.1M total=42.3M dur=0.0s underrun=true  rate=0.3M/s
[cache] fw=3.4M total=45.7M dur=0.7s underrun=false rate=1.0M/s
[cache] fw=2.2M total=46.5M dur=0.0s underrun=true  rate=1.0M/s
[cplayer] Still buffering (buffer went from 35% -> 62%) [0.629667s]
[cplayer] Audio device underrun detected.   ← 6.4s / 21.6s / 25.3s / 42.4s / 43.7s / 47.0s / 50.7s / 51.0s
```

- `fw`（前向缓冲）在 **1.1–6.7 MB** 之间反复掉到 0 秒可播；
- 音频欠载 **8 次，分散在整个播放过程**（不是起播一次）；
- `Still buffering 35%→62%` 是 demuxer 缓冲见底的直接证据。

## 二、三方同时刻对照（决定性）

| 测量方 | 8 MiB 请求实测 | 说明 |
| --- | --- | --- |
| **手机 curl**（`/system/bin/curl`，同一 URL） | **953,869 / 875,059 B/s** | 0.87–0.95 MB/s |
| **PC curl**（同一 URL 同一分钟） | **3,385,975 / 3,268,768 / 3,437,834 B/s** | 3.3–3.4 MB/s |
| **App 取数层 `rate`** | 0.6–1.1 MB/s | |
| **片源需要** | **2,537,500 B/s** | 20.3 Mbps |

**结论三条**：

1. **不是我们代码浪费带宽**：App 的 0.6–1.1 MB/s 与手机 curl 的 0.87–0.95 MB/s 同量级
   ⇒ 取数层已经贴着设备网络能力的上限在跑，没有可回收的余量。
2. **不是源站对所有人慢**：PC 同一分钟能拿 3.4 MB/s。
   ⇒ 差距在**手机这条公网路径**（Wi-Fi 到路由到公网），不在源站。
3. **物理缺口**：手机侧 0.87–1.75 MB/s vs 片源 2.54 MB/s ⇒ **缺口 1.5–2.9 倍**。
   深缓冲只能延后卡顿，不能消除：入 0.9 / 出 2.54 是净失血 1.64 MB/s，缓冲吃空必卡。

## 三、这不是"SenPlayer 更强"能解释的

SenPlayer 实测连的是 **`192.168.5.1:445`（SMB 局域网）**，不走外网。
同一台机器上我们走 SMB 播 12 GB 片源是 **60.44 fps / p50 16.7 ms / 0 次 >200 ms**（282 实测）。

⇒ **"它不卡"是因为源不同，不是因为内核不同。** 把它放到同一条外网路径上，它同样要卡。

## 四、顺带修掉的一个真问题（已提交）

原先插值开启时用 `video-sync=display-resample`，真机 65 秒内打了 **33 次**
`Change display sync audio drift` + `Compensation factor` 在 0.9899~1.0149 反复横跳，
同时 `Audio device underrun` 6 次 —— 它靠**改音频采样率**追时钟，音画在持续打架。

改用 **`display-tempo`**（音频变速、保持音高）后：补偿次数 **33 → 3**。
这是实实在在的改善，与带宽问题独立。

同时 `hdr-compute-peak` 默认关（用片源静态元数据 `max-cll=1007`），
4K 的 `Render queue underrun` 从 6 → 3 次；回滚开关 `debug.kivo.hdrPeak=on`。

## 五、降码率这条路为什么走不通（已实测）

Emby 支持转码（`supportsTranscoding=true`），低码率流也能拿到：

```
GET /emby/Videos/252137/stream.mp4?...&VideoBitrate=1500000&MaxWidth=1280
→ HTTP 200, content-type: video/mp4   ✅ 服务端没问题
```

但交给内核就失败：

```
[play] origin open fail @4194304: 服务器忽略了范围请求，无法定位到指定位置
PROXY size fail: IOException
[ffmpeg] http: HTTP error 503 Service Unavailable
```

**原因**：转码流是**顺序输出、无 Content-Length、不支持 Range**，
而我们的本地代理层建立在"源可随机定位"的假设上（seek / 分段取数都要它）。

⇒ 要支持转码流，必须给代理层加一条"不可 seek 流"的通道（直通模式），不是改几个参数的事。

## 六、出路（按见效速度）

| 方案 | 效果 | 成本 |
| --- | --- | --- |
| **用 SMB 源播**（已实测 60.44 fps 满帧） | 立刻不卡 | 0（换源） |
| 手机换更快出口（试移动数据 / 换频段 / 换路由口） | 视网络而定 | 0 |
| 给代理层加"直通模式"以支持转码流 | 能降到 1.5 Mbps 播 | 中（1–2 h） |
| 等源站对手机这条路径变快 | 不可控 | — |

## 七、判据（下次再有人说"卡"，30 秒内出结论）

```bash
# 1) 手机侧带宽（App 用同一个 /system/bin/curl）
adb shell "curl -s -m 30 -o /dev/null -w '%{speed_download}\n' -H 'X-Emby-Token: <token>' -r 0-8388607 '<stream-url>'"
# 2) 与 PC 同刻对照 —— 若 PC 明显更快，就是手机侧公网路径的问题
# 3) 对比片源码率（Emby MediaSources.Bitrate / 8）
# 4) 只有"手机 curl ≈ App rate"这一条成立，才允许怀疑播放器代码
```
