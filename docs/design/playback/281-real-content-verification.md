# 281 · 真实片源验证（NAS 12T，2026-11-15 07:20–08:10）

> 编号说明：`280` 已被项目审查/本地播放/seek 指标那批工作占用，本文改挂 `281`，避免撞号。

> 触发：用户给了 NAS 上的内容位置 ——
> "nas12T那个硬盘，里面的影视 电影 剧集 REMUX 特效这几个文件夹都可以"。
> 目的：用**真实片源**（不是合成素材、不是 Emby 单一片源）验证当前默认播放链路的
> 流畅度、HDR 处理与容器/编码覆盖。

## 一、实测的片源（全部先 ffprobe 摸清参数，再上机）

| 片源 | 容器/编码 | 分辨率 | HDR | 码率 | 音轨 |
| --- | --- | --- | --- | --- | --- |
| `Dept.Q.S01E02…2160p.NF.WEB-DL…HDR.H.265` | MKV / hevc Main10 | 3840×2160 @25 | **PQ (smpte2084) / bt2020** | **20.3 Mbps（2.54 MB/s）** | EAC3 Atmos 5.1 + 多条 subrip 字幕 |
| `Awake.2007…BluRay.REMUX.AVC.DTS-HD.MA5.1` | MKV / h264 | 1920×1080 @23.976 | SDR (bt709) | **27.5 Mbps（3.44 MB/s）** | **DTS-HD MA 5.1 无损** + AC3 5.1 |
| `Squid.Game.S03…DV.HDR.H.265` | MKV / hevc Main10 | 3840×2160 | **杜比视界 Profile 8.1**（`dv_bl_signal_compatibility_id=1` = HDR10 兼容基底） | 14.1 Mbps | EAC3 Atmos |

## 二、上屏帧率（量具 = `tools/p274_sf_latency.mjs`，SurfaceFlinger 真实上屏帧）

| 片源 | 源帧率 | **上屏帧率** | 帧间隔 p50 / p99 / max | >200ms |
| --- | --- | --- | --- | --- |
| Dept.Q 2160p **HDR** | 25 | **24.99 fps** | 40.0 / 47.4 / 74.4 ms | **0 次** |
| Awake **REMUX** 27.5 Mbps | 23.976 | **23.95 fps** | 41.9 / 45.9 / 125.1 ms | **0 次** |

⇒ **两条都精确贴合源帧率、零掉帧**（掉帧口径 = 帧间隔 >200 ms 的次数）。
⇒ 说明：**K40 完全能流畅播 4K HDR 与 27.5 Mbps 的 REMUX**（经 SMB）。

## 三、HDR 处理（真实片源，不是合成彩条）

Dept.Q 2160p 内核日志：
```
[0.250][d][mkv] |  + Matrix: bt.2020-ncl
[0.250][d][mkv] |  + Primaries: bt.2020
[0.250][d][mkv] |  + Gamma: pq
[0.632][i][cplayer] ● Video --vid=1 (hevc 3840x2160 25 fps) [default]
[229.249][v][vo/gpu-next] reconfig to 1920x1080 yuv420p bt.709/bt.709/bt.1886/limited/display
```
App 侧：`[color] HDR 检出（gamma=pq primaries=bt.2020）⇒ L2 tone-map 已下发：` **全部 ok**。

设备截屏：mean 87.3 / max 255 / p95 200 ⇒ 亮度自然、高光未过曝、无偏色；
**中文字幕正确渲染**（subrip 轨）。

⇒ **L2 色调映射在真实 4K HDR 片源上有效**（此前只在合成 PQ 素材上验过）。

## 四、SDR 零影响（真实片源验证）

Awake REMUX（bt709）内核日志与 App 日志：
```
[227.823] ● Video --vid=1 (h264 1920x1080 23.976 fps) [default]
[229.249] reconfig to 1920x1080 yuv420p bt.709/bt.709/bt.1886/limited/display
[color] 非 HDR（gamma=bt.1886 primaries=bt.709）⇒ 色彩参数不动
```
⇒ **SDR 片源一个色彩参数都不下发** —— 这是 L2 实现时立的硬约束，现在在**真实 REMUX** 上兑现。

## 五、seek（4K HDR，SMB）

```
[111.035][v][mkv] queuing seek to 142.720000
[111.036][v][mkv] execute seek (to 142.720000 flags 32)
[111.505][d][vo/gpu-next/libplacebo] First frame received with non-zero PTS 127.960000
[113.482][v][cplayer] playback restart complete @ 142.720000, audio=playing, video=playing
```
⇒ 4K HDR 上 seek 到位 **2.4 秒**（执行 → restart complete），音画同步恢复。

## 六、音频（含无损轨道）

| 片源 | 选中轨道 | 输出 |
| --- | --- | --- |
| Awake REMUX | `aid=2` AC3 5.1（MKV 标为 default） | `AO: [audiotrack] 48000Hz 5.1 6ch float` |
| Dept.Q 2160p | EAC3 Atmos 5.1 | 同上（audiotrack 5.1） |

⇒ 5.1 下混到 AudioTrack 正常。**DTS-HD MA（aid=1）未单独验证**（默认轨是 AC3）。

## 七、这一轮修正的一个旧判断（重要）

旧结论（273 §28 前后）："**4K 卡成 PPT**，真因是拿错文件（Emby 485 有两份）"。
今天的数据把这句话补完整：

- 那份失败的 4K 是 **54.5 Mbps（6.8 MB/s）**，而当时量到源站单连接上限 ~3.6 MB/s
  ⇒ **再怎么优化也够不着**，与"4K"这个分辨率本身无关。
- 今天 4K HDR **20.3 Mbps** 走 SMB：**25 fps 零掉帧**；
  27.5 Mbps 的 1080p REMUX 也已经 **23.95 fps**。
  ⇒ **码率（≈3.5 MB/s 以内）与分辨率都不是问题；问题是超过通路能力的超高码率。**

⇒ 以后再说"4K 卡"，先看**码率**，不要看分辨率。

## 八、未验证 / 已知边界（诚实）

| 项 | 状态 |
| --- | --- |
| **杜比视界单独验证** | **未做**。Squid.Game S03 是 DV Profile 8.1，基底层 = HDR10，走的解码与 tone-map 路径与已验证的 Dept.Q 完全相同 ⇒ **技术风险低**，但没有截图证据。 |
| MPEG-TS 容器（`Dunkirk…REMUX…ts`，29.7 GB） | 未测（边界容器） |
| x265 10bit SDR（`特效` 目录主体） | 未测（同 HEVC Main10 解码路径） |
| DTS-HD MA 直选播放 | 未测（默认走 AC3） |
| 长时间（>10 分钟）稳定性 | 未测；本轮每条只采 30–70 秒 |
| Emby HTTP 通路的卡顿复验 | **仍需下午做**（源站吞吐随时段变化，见 273 §28.8） |

## 九、复现方式

```bash
# 上屏帧率（任意播放中）
L='SurfaceView[com.kivo.cinema/com.kivo.cinema.ui.compose.PlayerComposeActivity](BLAST)'
ID=$(adb shell "dumpsys SurfaceFlinger --list | grep -F \"$L\" | head -1" | sed 's/.*#//' | tr -d '\r')
for i in $(seq 1 40); do adb shell "dumpsys SurfaceFlinger --latency '$L#$ID'" >> lat.txt; echo "" >> lat.txt; sleep 0.5; done
node tools/p274_sf_latency.mjs lat.txt
```
