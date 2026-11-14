# P273 · Emby 播放卡顿 —— 真机取证（K40 b6e950ee / dd137cf7）

> 采集时间：v4 = 2026-11-13 23:52–23:58；v5（取数层收口）= 2026-11-14 02:05–02:20；
> **v8–v11（起播提速收口）= 2026-11-14 03:10–05:20**；
> **v12/v13（预览浪费收口）= 2026-11-14 05:37–06:11**（设备本地时间，K40 时钟比 UTC 快 8h）
> 设备：`b6e950ee` / `dd137cf7`（Redmi K40）· APK：debug 签名 `android/keystore/kivo-debug.jks`
> 服务器：`emby.bangumi.ca`（Cloudflare 后端）· 片源：`清潭国际高中 · S01E01 贪婪`
> 文件：854,260,052 B（815 MB）· 时长 32:42 · HEVC 3840×1920 @23.976 + AAC 44.1kHz
> 规格：[`docs/design/playback/273-emby-stutter-fix.md`](../../design/playback/273-emby-stutter-fix.md)（v13 最终稿）

---

## 〇、当前结论（v13，2026-11-14 收口）

| 段 | 结论 | 证据 |
| --- | --- | --- |
| **播放中不卡** | ✅ 240 s 会话 **0 次缓冲**、0 失败帧、23.96 fps、渲染间隔 > 200 ms **0 次** | `v13/mpv-kernel.log` + `.diag` |
| **起播不慢** | ✅ 冷启动 → 首帧 **3.490 s**，51 次请求表头 max 658 ms | `v13/mpv-kernel.log` + `v13/net-diag.log` |
| **跳读不白屏** | ✅ 拖到未缓存位置 **2.5 s** 出画面（另跑 3 次复验，**0.2–2.5 s**），11 张截图近白像素 ≤ 1.7 % | `v13/drag-mpv-kernel.log` + `v13/drag-r1..r3-mpv.log` + 截图 |
| **内存** | ⚠️ **507,744 KB**（= 484.5 MiB / 507.7 MB） | `v13/meminfo-playback.txt` |
| **预览浪费** | ✅ **38.1 → 10.1 MiB**（多点扫描 13.1 MiB / 20 点），帧照出 | `v13/v13-single-net.log` + `v13/v13-scan-net.log` |
| **cues 预取** | ❌ 未做，仍要跳到文件尾读 cues | `v13/net-diag.log` |

**三段收口的因果**：
v5–v7 收的是"**连接生命周期**"（播放中别卡），
v8–v11 收的是"**连接冷 + 定位晚**"（起播别慢），
v13 收的是"**预览内核重复下载整片**"（拖动别浪费带宽）。
详见规格稿 §十（v8–v11）、§十一（v13）与 §二（结论推翻链）。

---


## 一、v5 结论（取数层收口；已被 v11 复验，见 §六）

**卡顿是取数层的问题：源站连接"老化"，不是带宽不够。**

- 需求：435 KB/s（854,260,052 B / 1962.889 s）。
- 新连接实测：**2459–7609 KB/s**（18 个连接周期，中位 3663 KB/s）。
- 老连接：**闲置后塌陷到 60–250 KB/s**，需 ~60 s 指数退避恢复。

修法三条：**闲置即回收**（空闲 ≥3 s 关连接）、**慢读即换连接**（<300 KB/s 且曾跑过 >1 MB/s）、
**重开门槛 4 MiB**（缓冲剩不够 4 MiB 不重开，免得把 1.3–1.8 s 的握手摊成停顿）。

## 二、v5 实测数据

### 2.1 判据复算（`node tools/p273_gaps.mjs`）

| 判据 | v4 | **v5** |
| --- | --- | --- |
| `Enter buffering` | 11 次 | **0** |
| `End buffering` | 17.6 s | **0** |
| `Audio device underrun` | 20 次 | **1**（起播瞬态 t=2.375 s） |
| `Opening done` | 13.9–14.3 s | **1.605 s** |
| `playback restart complete` | 5.441 s | **2.361 s** |
| 渲染帧数 / 时长 | —— | **19894 帧 / 829.7 s** |
| 实际帧率 | —— | **23.98 fps**（片源 23.976） |
| 渲染间隔 > 200 ms | 31 次（冻结 166 s） | **0**（最大间隔 155 ms，p99 44 ms） |
| 渲染失败帧 | —— | **0** |
| 源站连接数 | 2 条（1 条泄漏 13.7 MB） | **1 条**（Recv-Q 1,799,643 B = 1.8 MB） |
| 播放位置 | 断续 | **连续**（11:20 → 12:48，剩余 -19:54，见 `v5/06-position-1248.png`） |
| 进程 PSS | 843 MB | **924 MB ❌ 未达成**（见 §二.3） |

### 2.2 回收—重连循环（`v5/net-diag.log`）

连接按设计活着：**下载一段 → 空闲 3 s → 关掉 → 下次要读时重开**。

```
[play] origin release idle @78932459 after 3001ms
[play] origin open from=78932459 code=206 hdrMs=1293
[play] origin release idle @111427797 after 3002ms
[play] origin open from=111427797 code=206 hdrMs=1785
```

按周期重算（`origin open` → 下一个 `release idle` 之间传输的字节）：

| 项 | 值 |
| --- | --- |
| 周期数 | 18 |
| 单周期传输 | 15.8–75.3 MB |
| **周期速率** | **2459–7609 KB/s**（中位 3663 KB/s） |
| 重连表头延迟 hdrMs | 首连 352/409 ms；重连 **1260–1785 ms**（中位 1375） |
| 偏移连续性 | 每次重连的 `from` 等于上次 `release idle @` 的偏移 ⇒ **零字节丢失** |

### 2.3 内存分解（`dumpsys meminfo com.kivo.cinema`）

| 状态 | TOTAL PSS | Native Heap | Dalvik Heap | Graphics |
| --- | --- | --- | --- | --- |
| 播放中 | 924,482 KB | 572,116 KB | 47,148 KB | 139,920 KB |
| 退出播放后 | 152,267 KB | 21,636 KB | 10,996 KB | 52,260 KB |

⇒ 播放会话期间多出 **~550 MB native**。
主因是 `mediacodec-copy` 的 4K 帧回拷（3840×1920 NV12 = **11.06 MB/帧**），
外加**预览内核是第二个 mpv 实例**（同样解 4K、同样重复下载整片）。
**这一项未达成 A6（< 500 MB），已列入 273-B，不藏。**

> ⚠️ **本节是 v5 的历史读数，"回拷"这一条已在 v13 之前被推翻**：
> 代码已换成零拷贝 `hwdec=mediacodec`（真机日志 `Using hardware decoding (mediacodec)`），
> 同口径 native 337 MB → **278 MB**、PSS 737 MB → **544 MB**。
> v13 实测 TOTAL PSS **507,744 KB**，剩下的构成是
> **显存侧**（EGL mtrack 93 MB + Graphics 101 MB）+ native 侧的 demux 缓存与预览实例。
> 详见规格稿 §六 L1 续。

### 2.4 v5 归档文件

| 文件 | 内容 |
| --- | --- |
| `v5/mpv-kernel.log` | mpv 内核日志 1353 行（0 缓冲 / 1 欠载 / Opening done 1.605 s） |
| `v5/mpv-kernel.log.diag` | 渲染逐帧诊断 19894 帧（间隔复算源） |
| `v5/net-diag.log` | 取数层 48 行（开/关/表头延迟） |
| `v5/01-library.png` … `v5/06-position-1248.png` | 库 → Emby → 详情 → 播放 → 控制条 → 位置推进 |

---

## 三、v4 现场（历史取证；结论部分已被 v5 修正，保留原始数据）

> **注意**：本节 §3.1 与 §3.3 的结论已被 v5 推翻（见规格稿 §二 推翻链）。
> 保留的是**原始现场数据**，不是当前结论。

### 3.1 有效速率一路衰减到 16 KB/s（`net-diag.log`，v4 版）

| 累计 | 平均速率 | 时刻 |
| --- | --- | --- |
| 8 MiB | 2837 KB/s | 起播 +5 s |
| 20 MiB | 933 KB/s | +7 s |
| 36 MiB | 466 KB/s | +12 s |
| 60 MiB | 200 KB/s | +23 s |
| 80 MiB | 28 KB/s | +75 s |
| 84 MiB | **16 KB/s** | +115 s |

⇒ v4 当时判为"带宽衰减"，v5 改判为**连接老化**（同一根连接从未被回收）。

### 3.2 缓冲与音频欠载（v4 `mpv-kernel.log`）

```
Enter buffering (buffer went from 100% -> 0%)        × 11
End buffering (waited 44.313520 secs)                ← 最长一次卡 44 秒
End buffering (waited 38.524611 secs)
End buffering (waited 19.952053 secs)
Audio device underrun detected                        × 20
```

### 3.3 起播 5.44 秒，其中 3.5 秒是 MKV cues 反复 seek（v4）

```
[1.402] mkv Seeking to 854195684 to read header element   ← 跳到文件尾读 cues
[3.374] mkv Seeking to 854258019 to read header element   ← 再跳一次
[3.374] ffmpeg stream level seek from 854260052 to 6072  ← 回到开头
[4.901] Opening done
[5.403] AO: [audiotrack] 44100Hz stereo 2ch float
[5.441] playback restart complete @ 0.000000
```

v5 实测（`v5/mpv-kernel.log`）同场景：

```
[1.605] Opening done: http://127.0.0.1:39087/1?reconnect=1&...
[2.361] playback restart complete @ 0.000000, audio=playing, video=playing
```

### 3.4 预览实例在重复下载同一个文件（v4）

```
[play]  open from=0        len=854260052   →  net 80 MiB
[thumb] open from=0        len=854260052   →  net 32 MiB
[thumb] open from=524288   len=853735764   ← 预览又开了一条从头读的顺序流
```

**v5 仍未修**（预览内核仍重复下载，见规格稿 §六 L2），列入 273-B。

### 3.5 连接泄漏：13.7 MB 数据躺在内核缓冲里没人要（v4）

```
Recv-Q Send-Q Local Address:Port        Peer Address:Port
13785361 0  [::ffff:192.168.5.152]:42188  [::ffff:172.67.212.223]:443   ← 13.7 MB 未读
       0 0  [::ffff:192.168.5.152]:43794  [::ffff:104.21.45.99]:443    ← 主播放，Recv-Q 0
```

v5 同一命令：`Recv-Q 1,799,643`（1.8 MB）⇒ 泄漏量降一个数量级。

### 3.6 SenPlayer 对照（v4 实测 + 拆解结论）

| 维度 | 我们（v5） | SenPlayer |
| --- | --- | --- |
| 播放内核 | libmpv 0.39.0（mpv-android） | 定制 libmpv，产物名 `libFFmpegKit.so` |
| URL 走向 | mpv → 本地代理 → 自建 ByteSource → 源站 | **URL 直进 FFmpeg 的 http/smb 协议** |
| 到源站的连接数 | 1 条（v4 也是 1 条，但从不回收） | 实测 6 条并行（104.21.45.99 ×6） |
| 预览 | 独立 mpv 实例重复下载整片 | 无此重复下载 |
| 本地代理 | 每源一个 HTTP 服务 | 无自建代理 |

SenPlayer 播放同一片源时本机 wlan0 瞬时只有 92 KB/s 仍然流畅，
说明**流畅来自多连接聚合 + 客户端本地缓存**，不是源站给单人更多带宽。

---

## 四、工具限制（必须明示）

1. **REA 在本机不可用**：native manifest 缺失（落到 npm `rea-agents@6.8.0`），
   而 REA 还要求 `REA_JADX_MCP_JAR` → `jadx-headless-mcp 0.7.1` 的 jar，
   npm 上只发布 0.7.0（无 jar）。SenPlayer 的拆解因此改用**二进制字符串提取**，
   结论（`libFFmpegKit.so` / 直连 URL）来自该路径，**未经反编译逐行确认**。
2. **HyperOS 过滤应用 logcat**：`adb logcat --pid=<pid>` 看不到本应用的 `Log.i` 输出。
   诊断改为写入应用私有目录 `files/net-diag.log` / `files/mpv-kernel.log`，
   用 `adb exec-out "run-as com.kivo.cinema cat files/<name>"` 取回。
3. 屏幕方向会变（详情页竖屏、播放页横屏），**注入坐标必须按当前截图分辨率换算**，
   否则滑动/点击全部落空。
4. **本机没有 Python 解释器**（`python`/`python3` 是 WindowsApps 占位，退出码 49；
   `py -3` 报 "No Installed Pythons Found!"）⇒ 证据复算工具一律用 Node：
   `tools/p273_gaps.mjs`。

## 五、复算方法

```bash
# 取数层：每次重开/回收一行
adb exec-out "run-as com.kivo.cinema cat files/net-diag.log"

# 内核侧：缓冲/欠载/起播
adb exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" | \
  grep -E "Enter buffering|End buffering|underrun|Opening done"

# 渲染间隔：重算 p99 与 >200ms 次数
adb exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > /tmp/diag.log
node tools/p273_gaps.mjs <kernel.log> /tmp/diag.log

# 内存与连接
adb shell "dumpsys meminfo com.kivo.cinema | grep -E 'TOTAL PSS|Native Heap|Dalvik Heap'"
adb shell "ss -tn state established '( dport = :443 )'"
```

---

## 六、v8–v11 起播提速收口（2026-11-14）

### 6.1 为什么还要继续收

v7 收完"播放中不卡"之后，用户视角还剩一个等待：**点开到出画面要 5 秒量级**。
拆开看是两个延迟叠加，与带宽无关：

| 延迟源 | 真机实测 |
| --- | --- |
| **连接冷**（TCP + TLS 握手） | 新建 **983–1301 ms** / 复用 **266–362 ms** |
| **定位晚**（续播位在起播之后才 seek） | 起播后还要多付一整轮"读 cues + 找关键帧" |

### 6.2 版本对照链（全部可从各版本日志复算）

| 版本 | 改动 | 起播场景 | 起播完成 | 表头 max | 结论 |
| --- | --- | --- | --- | --- | --- |
| v7 | — | 从 0 起播 | 2.548 s | 1076 ms | 播放中不卡，起播慢 |
| v8 | 一次性 keep-alive 预热 | 续播 500 s | 3.257 s | 695 ms | 有效，但会被抢 |
| v9 | `loadfile` 带 `start=` | 续播 582 s | 3.152 s | 467 ms | 定位晚，已修 |
| v10 | **串行** keeper | 续播 318 s | **4.628 s** | **1165 ms** | **倒退**（keeper 抢连接） |
| **v11** | **并行** keeper（2 线程） | 续播 531 s | **3.217 s** | **362 ms** | **成立** |

> v7 是从 0 起播、v8–v11 是续播到片中段，**两者不可直接比**。
> 跨版本的硬指标是**表头延迟 1076 → 362 ms**，与场景无关。

### 6.3 v11 实测（240 s 会话 + 拖动会话）

**起播时间线**（`v11/mpv-kernel.log`）：

```
[1.401] Opening done: http://127.0.0.1:42755/1?reconnect=1&...
[1.571] queuing seek to 531.155000
[1.859] seek done
[3.201] first video frame after restart shown
[3.217] playback restart complete @ 531.156000, audio=playing, video=playing
```

**渲染与缓冲**（`node tools/p273_gaps.mjs v11/mpv-kernel.log v11/mpv-kernel.log.diag`）：

```
Enter buffering 次数: 0
End buffering 次数: 0
Audio device underrun 次数: 1        ← 起播瞬态，t=3.218 s
Opening done 时刻(s): 1.401
playback restart 时刻(s): 3.217
渲染帧数: 5641
渲染时长(s): 235.4
实际帧率(fps): 23.96
渲染间隔 >200ms 次数: 0
渲染间隔最大(ms): 194
渲染间隔 p99(ms): 43
渲染失败帧数: 0
```

**取数层**（`v11/net-diag.log`，35 次请求全部是复用，无一次握手级延迟）：

```
表头 hdrMs 升序：266 267 269 269 273 274 274 274 276 277 277 278 283 284 285
                286 287 287 289 289 293 296 297 299 300 302 306 309 318 321
                323 335 335 350 362
```

min 266 / p50 287 / p99 362 / max **362** ms。
对照：新建连接的握手级延迟是 **983–1301 ms**（v7/v10 实测）——
**max 362 ms ⇒ 全程零新建连接**。

**跳读（拖到未缓存位置）**（`v11/drag-mpv-kernel.log`）：

```
[ 15.345] queuing seek to 1857.255000
[ 15.639] seek done
[ 16.785] first video frame after restart shown        ← 1.44 s，无白屏
```

跳读那一跳的表头也是**复用**（≤ 334 ms）；v7 同场景要 2.2 s、且跳读表头 1170 ms（新建）。

**内存**（`v11/meminfo-playback.txt`）：

| 项 | KB | 对照 v5 |
| --- | --- | --- |
| Native Heap | 301,148 | 572,116 → **−47.4 %** |
| Graphics | 100,900 | 139,920 |
| EGL mtrack | 93,056 | —— |
| Dalvik Heap | 30,908 | 47,148 |
| **TOTAL PSS** | **503,061** | 924,482 → **−45.6 %** |

> 判据 "< 500 MB"：**按 MiB 折算 491.3 MiB 成立；按 MB 折算 503.1 MB，差 0.6 %**。
> 记为"逼近"，不记"达成"。渲染侧（EGL + Graphics ≈ 194 MB）留给 273-B。

**预览浪费（未修）**（`v11/thumb-net-diag.log`）：

```
$ grep '\[thumb\] origin chunk' v11/thumb-net-diag.log | wc -l
20
$ ... | awk '{s+=$1} END {printf "%.1f MiB\n", s/1048576}'
38.1 MiB
```

取一帧缩略图读了 **38.1 MiB**（v7 是 36.1 MiB，v5 是 64 MiB）。列入 273-E。

### 6.4 v8–v11 归档文件

| 目录 | 内容 |
| --- | --- |
| `v8/` | 一次性 keep-alive 预热（19 次请求的日志，表头 max 695 ms） |
| `v9/` | `+ start=` 续播位直开（32 次请求，表头 max 467 ms） |
| `v10/` | **串行 keeper 倒退现场**（52 次请求，表头 max 1165 ms，max 775/1165 = 抢到的死连接） |
| `v11/` | **并行 keeper 收口现场**：240 s 会话 + 拖动会话 + 内存 + 预览 + 8 张截图 |

`v11/` 文件清单：

| 文件 | 内容 |
| --- | --- |
| `mpv-kernel.log` / `.diag` | 240 s 会话内核日志 + 逐帧渲染诊断（5641 帧） |
| `net-diag.log` | 取数层 35 次请求（表头延迟分布） |
| `drag-mpv-kernel.log` / `drag-net-diag.log` | 拖到未缓存位置的会话 |
| `meminfo-playback.txt` | 播放中 `dumpsys meminfo` |
| `thumb-net-diag.log` / `mpv-thumb.log` / `thumb-evidence.json` | 预览内核的取数与取证 |
| `01-playing.png` … `07-drag.png` | 播放中与拖动过程截图 |

### 6.5 v11 复算命令

```bash
# 1) 渲染间隔 / 帧率 / 缓冲
node tools/p273_gaps.mjs \
  docs/evidence/p273-emby-stutter/v11/mpv-kernel.log \
  docs/evidence/p273-emby-stutter/v11/mpv-kernel.log.diag

# 2) 起播时间线
grep -E "Opening done|queuing seek|seek done|first video frame|playback restart complete" \
  docs/evidence/p273-emby-stutter/v11/mpv-kernel.log

# 3) 表头延迟分布
grep -oE "hdrMs=[0-9]+" docs/evidence/p273-emby-stutter/v11/net-diag.log | sed 's/hdrMs=//' | sort -n

# 4) 内存
grep -E "TOTAL PSS|Native Heap|EGL mtrack|Graphics|Dalvik Heap" \
  docs/evidence/p273-emby-stutter/v11/meminfo-playback.txt

# 5) 预览浪费
grep '\[thumb\] origin chunk' docs/evidence/p273-emby-stutter/v11/thumb-net-diag.log \
  | grep -oE "len=[0-9]+" | sed 's/len=//' | awk '{s+=$1} END {printf "%.1f MiB\n", s/1048576}'

# 6) 拖动到未缓存位置
grep -E "queuing seek|seek done|first video frame" \
  docs/evidence/p273-emby-stutter/v11/drag-mpv-kernel.log
```

---

## 七、v12/v13 预览浪费收口（2026-11-14）

### 7.1 问题

拖动进度条时浮窗里那一帧，由**独立的第二个 mpv 实例**（`MpvThumbnailReader`）去源站取。
v11 实测：**为了取一帧下载 38.1 MiB**（`[thumb]` 20 次请求）。

### 7.2 v12 · 先证伪"mpv 参数能治"

7 组参数全部用**运行时下发**（`--es kivo.thumbOpt`）跑完（`v12/*-net.log`）：

| 组 | 参数 | 预览下载 |
| --- | --- | --- |
| v0 | 默认 | 34.1 MiB |
| v1 | `demuxer-readahead-secs=0` | 42.1 MiB |
| v2 | `cache=no` | 30.1 MiB |
| v3 | `cache=no` + `demuxer-readahead-secs=0` | 30.1 MiB |
| v4 | `cache-secs=0.2` + `demuxer-max-bytes=64KiB` | 30.1 MiB |
| v5 | `demuxer-max-bytes=1MiB` + `demuxer-max-back-bytes=0` | 36.1 MiB |
| v6 | `cache=no` + `demuxer-max-bytes=64KiB` + `demuxer-readahead-secs=0` | **16.1 MiB** |

**结论：mpv 侧治不了**（最好还剩 16.1 MiB，v1 反而涨到 42.1 MiB）。

### 7.3 根因与修法

FFmpeg 收到的是**开口区间**（`Range: bytes=N-`），**出帧之后内核还会继续往前读**
（实测 9 s / ≈24 MiB）。mpv 的 `cache` / `cache-secs` / `demuxer-max-bytes` /
`demuxer-readahead-secs` 控的是**缓存多大**，不是**这次读要不要停**。

⇒ 只能在**供给侧**截断：

| 修法 | 内容 |
| --- | --- |
| ① 单次响应上限 | 代理侧非零偏移的响应截到 **4 MiB**；**偏移 0 豁免**（FFmpeg 在那里靠 `Content-Length` 推整个文件大小，截了会让所有后续 seek 失效） |
| ② 预览不许在 EOF 重连 | 预览 URL 带 `reconnect_at_eof=0`，否则 FFmpeg 到截断点会重连继续拉 ⇒ 截断白做 |
| ③ 预览环收小 | `THUMB_CAPACITY_BYTES` 4→2 MiB、`THUMB_CHUNK_BYTES` 2→1 MiB（每连接浪费 ≈ 环 + 块，3 条连接 ⇒ 18→9 MiB） |

### 7.4 v13 实测

**单点预览（475 s）**（`v13/v13-single-net.log`）：

```
[v13-single] originTotal=10.1MiB afterSeek=9.1MiB chunks=11
              proxyGets=3 proxyBytes=4.1MiB | frame:YES
```

代理层三行日志就是截断在起作用（`cap=4194304` = 4 MiB 上限）：

```
GET range=bytes=854195684- bytes=64368    cap=0          ← 播放代理，未截
GET range=bytes=854195684- bytes=64368    cap=4194304   ← 预览代理，cues 尾（本来就短）
GET range=bytes=266455794- bytes=4194304  cap=4194304   ← 预览代理，命中上限
```

**多点拖动扫描（20 点）**（`v13/v13-scan-net.log`）：

```
[v13-scan] originTotal=13.1MiB afterSeek=12.1MiB chunks=14
            proxyGets=3 proxyBytes=4.1MiB | frame:YES
GET range=bytes=463010741- bytes=4194304 cap=4194304
```

最后一行是本段最关键的一条：**截断产生一个"干净的 EOF"，
FFmpeg 的 `http_seek` 会带新 Range 重开连接** ⇒ 截断不会让后续拖动失灵。

**下载量对照**：

| 版本 | 单点预览 | 说明 |
| --- | --- | --- |
| v7 | 36.1 MiB | 预览独立预读，但仍从头下载 |
| v11 | 38.1 MiB | 未修 |
| v12 v0 | 34.1 MiB | 同环境基线 |
| **v13** | **10.1 MiB** | 供给侧截断 + 预览环收小 |

### 7.5 播放主链路回归（v13）

播放代理的 `cap=0`（不截），`LocalRangeResponder` 对不截断路径与 v11 的旧代码**逐字节等价**。

```
$ node tools/p273_gaps.mjs v13/mpv-kernel.log v13/mpv-kernel.log.diag
Enter buffering 次数: 0
End buffering 次数: 0
Audio device underrun 次数: 1        ← 起播瞬态
Opening done 时刻(s): 2.226
playback restart 时刻(s): 3.49
渲染帧数: 5620
渲染时长(s): 234.5
实际帧率(fps): 23.96
渲染间隔 >200ms 次数: 0
渲染间隔最大(ms): 190
渲染间隔 p99(ms): 43
渲染失败帧数: 0
```

| 判据 | v11 | **v13** |
| --- | --- | --- |
| 缓冲 / 失败帧 | 0 / 0 | **0 / 0** |
| 帧率 | 23.96 fps | **23.96 fps** |
| 渲染间隔 > 200 ms | 0（max 194，p99 43） | **0**（max 190，p99 43） |
| 音频欠载 | 1 | **1** |
| `Opening done` | 1.401 s | 2.226 s |
| `playback restart complete` | 3.217 s | 3.490 s |
| 表头 max | 362 ms | 658 ms（51 次请求，中位 330） |
| TOTAL PSS | 503,061 KB | 507,744 KB |

> **起播与表头比 v11 差是环境方差，不是本次改动**：播放代理 `cap=0`，
> 截断逻辑不在播放路径上；v13 那次会话源站拉了 **200.1 MiB**（v11 是 136.1 MiB），
> 源速/边缘节点状态不同 ⇒ 建连与取数都慢。**既不记"回归"，也不记"达成"。**

### 7.6 跳读不白屏（v13，4 次）

| 次 | 目标位置 | `queuing seek` → 首帧 | 缓冲 |
| --- | --- | --- | --- |
| 主 | 1865.119 s | 15.261 → 17.766 = **2.505 s** | 1 次 0.641 s |
| r1 | 1857.255 s | 12.307 → 14.130 = **1.823 s** | 0 |
| r2 | 1871.409 s | 12.311 → 12.543 = **0.232 s** | 0 |
| r3 | 1844.675 s | 12.475 → 14.474 = **1.999 s** | 0 |

**像素层面复算"不是白屏"**（`node tools/p273_png_avg.mjs`）：

```
01-playing.png     2400x1080 avgRGB=34,44,54   nearWhite=0.1% ✅ 有内容
02-drag.png        2400x1080 avgRGB=54,50,46   nearWhite=0.0% ✅ 有内容
04-drag.png        2400x1080 avgRGB=82,88,88   nearWhite=0.0% ✅ 有内容
07-drag.png        2400x1080 avgRGB=97,105,106 nearWhite=0.2% ✅ 有内容
drag-r1.png        2400x1080 avgRGB=118,113,109 nearWhite=0.4% ✅ 有内容
drag-r3.png        2400x1080 avgRGB=119,114,110 nearWhite=0.3% ✅ 有内容
```

11 张截图近白像素全部 ≤ **1.7 %**（白屏会接近 100 %）⇒ **没有白屏**。

### 7.7 v12/v13 归档文件

| 目录 | 内容 |
| --- | --- |
| `v12/` | 7 组 mpv 参数 A/B 的取数日志（每组一个 `*-net.log` + `*-thumb.log`）+ `long-net.log` |
| `v13/` | **收口现场**：单点预览 + 20 点扫描 + 240 s 会话 + 拖动会话 + 8 张截图 + 3 次拖动复验 |

`v13/` 文件清单：

| 文件 | 内容 |
| --- | --- |
| `v13-single-net.log` / `v13-single-thumb.log` | 单点预览（475 s）的取数与内核日志 |
| `v13-scan-net.log` / `v13-scan-thumb.log` / `v13-scan-thumb.json` | 20 点拖动扫描 |
| `mpv-kernel.log` / `.diag` | 240 s 播放会话（5620 帧渲染诊断） |
| `net-diag.log` | 播放取数层 51 次请求（表头延迟分布） |
| `drag-mpv-kernel.log` / `drag-net-diag.log` | 拖到未缓存位置的会话 |
| `drag-r1..r3-mpv.log` / `drag-r1..r3.png` | 拖动复验 3 次 |
| `meminfo-playback.txt` | 播放中 `dumpsys meminfo` |
| `01-playing.png` / `0*-drag.png` | 播放中与拖动过程截图 |

### 7.8 v13 复算命令

```bash
# 1) 预览下载量（按版本对照）
node tools/p273_preview_bytes.mjs \
  docs/evidence/p273-emby-stutter/v12/v0-default-net.log \
  docs/evidence/p273-emby-stutter/v13/v13-single-net.log \
  docs/evidence/p273-emby-stutter/v13/v13-scan-net.log

# 2) 截断是否生效（代理层每次响应多少字节）
grep "GET range=" docs/evidence/p273-emby-stutter/v13/v13-single-net.log

# 3) 播放侧回归闸门
node tools/p273_gaps.mjs \
  docs/evidence/p273-emby-stutter/v13/mpv-kernel.log \
  docs/evidence/p273-emby-stutter/v13/mpv-kernel.log.diag

# 4) 跳读时间线
grep -E "queuing seek|seek done|first video frame" \
  docs/evidence/p273-emby-stutter/v13/drag-mpv-kernel.log

# 5) 截图是不是白屏
node tools/p273_png_avg.mjs docs/evidence/p273-emby-stutter/v13/0*-drag.png
```

---

## 八、v14–v17 预取与预热收口（2026-11-14）

设计稿全文：[`docs/design/playback/273-emby-stutter-fix.md`](../../design/playback/273-emby-stutter-fix.md)
（§十三 v14 · §十四 v16 · §十五 v17）

### 8.1 版本链一句话对照

| 版本 | 改了什么 | 关键路径还剩几跳回源 |
| --- | --- | --- |
| v14 | 起播瞬间抓尾窗口（cues） | **被打回** —— 窗口在起播时才发，尾段冷，窗口大就晚到 |
| v15 | 尾窗口改为随源走（源对象持有） | 尾命中有了，但窗口到达仍晚于起播 |
| v16 | **详情页预热**：进程级登记处 + 片尾窗口 + 保温池延后 | ①片头 ②cues ③恢复点 ⇒ 干掉 ② |
| **v17** | **片头窗口（4 MiB）**：尾缓存泛化成多窗口缓存 | ①②③ ⇒ 恢复点在窗口内时**三跳全灭** |

### 8.2 v17 A/B 实测（`v17/ab1/`）

复算命令：

```bash
node tools/p273_prewarm_ab.mjs docs/evidence/p273-emby-stutter/v17/ab1
```

复算输出：

| 轮 | 路径 | 闸门等待 | 诚实首帧 | 片头命中数 | 尾命中字节 | 尾回源 | 关键路径源站往返 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| c1 | 无预热 | 1047 ms | 2161 ms | 0 | 0 | 0 | 1 |
| c2 | 无预热 | 1125 ms | 4389 ms | 0 | 0 | 1 | 2 |
| c3 | 无预热 | 1030 ms | 3614 ms | 0 | 0 | 1 | 2 |
| p1 | 详情页预热 | 107 ms | **690 ms** | 26 | 130,494 | 0 | **0** |
| p2 | — | 轮次无效（导航到别的片源，无播放） | | | | | |
| p3 | 详情页预热 | 100 ms | 3926 ms | 17 | 130,494 | 0 | 1 |

- **诚实首帧中位 3614 → 2308 ms（−1306 ms，−36.1%）**；闸门等待中位 1030 → 103 ms。
- **p1 是三跳全灭的理想态**：整 4 MiB 片头 **21 ms** 本地供出（`[head] cache hit` × 26）、
  cues **1 ms** 命中，源站第一次分块直接从 `from=4194304` 起（无 `from=0`、无 cues 回源）。
- **p2 是无效轮**：固定点击坐标在库列表顺序变化后点到了别的片源，无播放 ⇒ 不计入对照。
- **p3 是诚实的局限**：恢复点 8,490,718 B **超出 4 MiB 窗口** ⇒ 第 ③ 跳（恢复点簇）仍回源，
  中段冷（0.34 MB/s）⇒ 该跳独占了约 3.1 s。**下一卡 v18 的靶心。**

### 8.3 口径说明

- **诚实首帧** = 渲染层真正 swap 过一帧（`files/mpv-kernel.log.diag` 里首个 `RND +Nms post #0`），
  **不是**应用锚点 `MPV 首帧就绪`（播放时钟起跳，比画面早 1.5–2.8 s）。两种口径在复算工具里并排打印。
- **关键路径源站往返** = 起播窗口内 `tag=play` 且 `from=0` 或 `from=恢复点偏移` 的源站分块数。
- 设备：Redmi K40 `dd137cf7`，片源 233908，Emby 源 `emby.bangumi.ca`。

### 8.4 v14–v17 归档文件

| 目录 | 内容 |
| --- | --- |
| `v14/ab/` | v14 尾窗口方案 A/B（**已被 v16 判定为设计错误并作废**） |
| `v15/` | v15 尾窗口随源走 + diag 采样（ab / diag / fix） |
| `v16/ab4/` | **v16 收口 A/B**（3 轮 C + 3 轮 P），诚实首帧中位 4551 → 2170 ms |
| `v16/soak2`、`soak3/` | v16 长稳会话（120 s） |
| `v17/ab1/` | **v17 收口 A/B**（3 轮 C + 3 轮 P，本节 §8.2 的来源） |

---

## 九、v18 · 恢复点定向预取（2026-11-14）

设计稿：[`docs/design/playback/273-emby-stutter-fix.md`](../../design/playback/273-emby-stutter-fix.md) §十六

### 9.1 靶心

v17 留下的尾巴：**片头窗口（4 MiB）盖不住续播落点**（恢复点在 8,490,718 B，
差 4.3 MB），于是"续播那一跳"独占约 3.2 s。

v18 的做法：**在详情页阶段**读 MKV 的 cues 表 ⇒ 算出恢复点所在簇的**绝对字节偏移**
⇒ 提前把那一段抓下来。cues 表 v16 起就已经下好了，**不多下任何一个字节**。

### 9.2 独立验证：cues 算出来的偏移 == 内核自己 seek 的偏移

| 来源 | 值 |
| --- | --- |
| 我们的 cues 解析（`MkvCueIndex`） | 4244 条，21,063 ms → **8,490,718** |
| mpv 自己的 seek（内核日志） | `stream level seek from 1935985234 to` **8490718** |

两边**独立算出同一个字节** ⇒ cues 解析没有错位。
（这是本卡的关键自证：**猜错的后果只是"没命中"，绝不会把错字节喂给内核**。）

### 9.3 A/B 实测（`v18/ab1/`，同机、同片源、同续播位）

复算命令：

```bash
node tools/p273_prewarm_ab.mjs docs/evidence/p273-emby-stutter/v18/ab1
```

| 轮 | 路径 | 闸门等待 | **诚实首帧** | 片头命中 | 尾命中 | 恢复点命中 | 恢复点回源 | **关键路径源站往返** | 缓冲 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| c1 | 无预热 | 953 ms | 4352 ms | 0 | 0 | 0 | 0 | **2** | 0 |
| c2 | 无预热 | 921 ms | 6563 ms | 0 | 0 | 0 | 0 | **2** | 1 |
| c3 | 无预热 | 1049 ms | 4251 ms | 0 | 0 | 0 | 0 | **2** | 0 |
| **p1** | 详情页预热 | 76 ms | **620 ms** | 17 / 4,194,304 B | 2 / 130,494 B | 17 / 4,128,768 B | **0** | **0** | 0 |
| **p2** | 详情页预热 | 134 ms | **832 ms** | 17 / 4,194,304 B | 2 / 130,494 B | 17 / 4,128,768 B | **0** | **0** | 0 |
| **p3** | 详情页预热 | 61 ms | **622 ms** | 8 / 1,900,544 B | 2 / 130,494 B | 17 / 4,128,768 B | **0** | **0** | 0 |

⇒ **诚实首帧中位 4352 → 622 ms（−3730 ms，−85.7%）**；闸门等待中位 953 → 76 ms；
**关键路径源站往返 2 → 0**；缓冲事件 0。

### 9.4 已知边界（不藏）

1. **详情页得停够久**：恢复点窗口在冷源上要 1.7–9.2 s 才落地（p1 = 1.7 s，p2 = 9.2 s）。
   用户点得太早 ⇒ 那一跳仍回源（但预取连接已建好，不是从零开始）。
2. **只对"有续播位"生效**：`resumeMs <= 0`（从头播）⇒ 不预取 —— 从头播时片头窗口已经盖住。
3. **只对 MKV 生效**：cues 是 MKV 的结构；容器解析不出来 ⇒ 放弃预取（照常回源）。

### 9.5 归档文件

| 目录 | 内容 |
| --- | --- |
| `v18/ab1/` | **v18 收口 A/B**（3 轮 C + 3 轮 P；每轮 `c*/p*-net.log`、`c*/p*-kernel.log`、`c*/p*-diag.log`、`p*-prewarm-net.log` + 4 张截图） |
| `v18/soak1/` | v18 长稳会话里的拖动截图（`drag{1,2,3}-{back,fwd}.png`）—— 拖动后的落点回归，见 §十一 |

---

## 十、v19 · 起播期音频欠载的判决（2026-11-14）

设计稿：见 [`docs/design/playback/273-emby-stutter-fix.md`](../../design/playback/273-emby-stutter-fix.md) §十七
（缺口清单在 §十九，G-音频 条目）

### 10.1 问题

内核日志在**起播瞬间**报欠载：

```
[   0.635][v][cplayer] playback restart complete @ 21.063000, audio=playing, video=playing
[   0.638][w][cplayer] Audio device underrun detected.
[   0.640][v][cplayer] restarting audio after underrun
```

拖动 seek 之后还会再来一次（t=35.376 / 36.122 / 36.142）。

### 10.2 为什么不能只信内核

`Audio device underrun detected.` 是 **mpv 自己报的**（`[cplayer]` 级别），
它说的是"我的音频环被读空了"，**不等于设备上真的断音**。
设备侧另有两个**独立**计数器：

- `dumpsys media.audio_flinger` 音轨表里的 `Underruns` / `Flushed` 两列（按进程 PID 过滤）；
- 混音器原始计数 `Normal mixer raw underrun counters: partial=… empty=…`。

### 10.3 实测判决（`v19/audio/`）

本应用的音轨行（Client = 应用 PID 15909）：

| 阶段 | 音轨行 | `Underruns` | 混音器原始计数（全局） |
| --- | --- | --- | --- |
| 起播前（`af-0-before-play.txt`） | 尚无本应用的轨 | — | `partial=0 empty=198` |
| 起播 30 s 后（`af-1-after-start.txt`） | `672 yes 15909 … A 0 0 509.70 t` | **0** | `partial=0 empty=198` |
| 拖动 seek 后（`af-2-after-seek.txt`） | `672 yes 15909 … A 0 23056 430.10 t` | **0** | `partial=0 empty=198` |

⇒ **两个独立的设备侧计数器在起播、稳态 30 s、拖动 seek 之后全程没有增加**
（全局混音器 `empty` 恒为 198，本应用音轨 `Underruns` 恒为 0）。

### 10.4 结论（分两句，别混成一句）

1. **设备上没有真的断音**：混音器原始欠载计数与本应用音轨的 `Underruns` 全程为 0 ⇒
   用户听到的音频是连续的。这条**已判决**。
2. **但内核侧的欠载提示确实存在**，而且它自己会"重启音频"
   （`restarting audio after underrun`）。它发生在**每次 seek 落点之后**的 3 ms 内 ——
   这是 mpv 在 flush 音频设备后重建输出环的固有抖动，**不是**我们的取数层喂不上数据
   （同期源站往返 0、缓冲事件 0）。
   ⇒ 归类为 **G-音频**：**不影响听感，但要消掉这条日志噪声**，处理办法见 §十七。

### 10.5 归档文件

| 目录 | 内容 |
| --- | --- |
| `v19/audio/` | 三份 `dumpsys media.audio_flinger` 快照 + 同轮的内核/取数日志 |
| `v19/` | 同轮的整段会话（`kernel.log` / `net.log` / `thumb.log` / `meminfo.txt` + 3 张截图） |

---

## 十一、v20 · 字幕字体集最小化（2026-11-14）

设计稿：[`docs/design/playback/273-emby-stutter-fix.md`](../../design/playback/273-emby-stutter-fix.md) §十八

### 11.1 根因

`sub-fonts-dir=/system/fonts` ⇒ libass（我们编的没有 `CONFIG_FONTCONFIG`）
对目录里的**每一个条目**调 `ass_add_font()`，FreeType 的 `FT_New_Face` 把整个文件读进内存
⇒ **239 个字体文件 / 125.6 MB 全部常驻原生堆**。内核日志里是 **242 条** `Loading font file`
（239 个真实文件 + 3 个符号链接 —— libass **不跳过**符号链接）。

### 11.2 A/B 实测（同一构建体系、同一个 Emby 流）

复算命令：

```bash
node tools/p273_font_ab.mjs docs/evidence/p273-emby-stutter/v23-base \
                           docs/evidence/p273-emby-stutter/v23-v20-font
```

| 指标 | A（`/system/fonts` 全量） | B（最小字体集） | 差值 |
| --- | --- | --- | --- |
| `Loading font file` 条数 | **242** | **2** | −240 |
| 起播期原生堆峰值 PSS | 241,331 KB | **111,689 KB** | **−129,642 KB（−126.6 MB）** |
| TOTAL PSS 峰值 | 440,778 KB | **313,640 KB** | −127,138 KB（−124.2 MB） |

逐秒原生堆 PSS（KB）：

```
A: 1734 13218 13424 205572 206784 213845 224509 234707 240183 240363 240555 …
B: 1779 15294 14733  14797  15261  51874  51797  59754  62806  67358  72374 …
```

⇒ A 在第 4 秒**一步跳 +192 MB**；B 把它**摊平成 24 秒的缓坡**，累计少占约 126 MB。

B 侧的 2 个字体是：**主 CJK 字体**（`NotoSansCJK-Regular.ttc`，符号链接）+ **emoji**
（`NotoColorEmoji.ttf`，符号链接）。emoji 是**故意**留的：Noto Sans CJK 已含拉丁/标点/假名，
唯一真正的字形缺口是表情符号。

### 11.3 字幕没有被牺牲（同一构建 A/B + 本地确定性片源）

`sub-fonts-dir` 是**选项**、不是运行时属性 ⇒ 运行时改不了 ⇒ 必须靠**同构建两版**对比。
本地 HTTP 服务挂一个 ffmpeg 造的确定性 MKV（1280×720 深蓝底 + 5 条中文字幕 + 正弦音），
A 版退回整目录、B 版用最小字体集：

| 项 | A（`v20-sysfonts-local2/`，242 字体） | B（`v20-minfonts-local/`，2 字体） |
| --- | --- | --- |
| 字体匹配 | `fontselect: (Noto Sans CJK JP, 400, 0) -> NotoSansCJKjp-Regular, 0, NotoSansCJKjp-Regular` | **完全一致** |
| OSD 纹理分配 | `Reallocating OSD texture to 512x256.` → `1024x256.` | **完全一致** |
| 画面 | `band/s3.png`：「第二行：字体应当正常显示」 | `band/s5.png`：「最后一行：字幕渲染成功」 |

⇒ **中文照常显示，不豆腐块**；字族匹配与纹理分配两边的行为**逐字相同**。

`band/*.png` 是从 raw screencap 裁出来的（裁剪脚本：`tools/p273_band_crop.mjs`，
可逐字节复算 —— 已验证用归档脚本重新裁出的 PNG 与归档文件 **完全一致**）。
raw 中间件（772 MB）不入库，只留裁好的 PNG。

### 11.4 归档文件

| 目录 | 内容 | 说明 |
| --- | --- | --- |
| `v23-base/` | **A 侧**：逐秒 meminfo（`mi-1..24.txt`）+ 242 条 `Loading font file` | v23 基线 |
| `v23-v20-font/` | **B 侧**：逐秒 meminfo + 2 条 `Loading font file` | v20 收口 |
| `v20-minfonts-local/` | B 侧本地 MKV：`kernel.log` / `net.log` / `band/*.png` | 字幕渲染证明（B） |
| `v20-sysfonts-local2/` | A 侧本地 MKV：`kernel.log` / `net.log` / `band/*.png` | 字幕渲染证明（A） |
| `v20-fontfix/`、`v20-fontburst/`、`v20-fontband/` | Emby 流上的 B 侧截图与裁剪 | 诚实记录：**Emby 那个片源在采样窗口里没有可见中文字幕**，所以字幕证明改用上面的本地确定性片源 |
| `v20/` | 起播期 `dumpsys meminfo` / `smaps` 前后对照 + 3 张截图 | 内存归因现场 |
| `v20-systemfonts/`、`v20-sysfonts0/` | A 侧 Emby 流裁剪 | 探索轮，保留 |

> `v20-sysfonts-local/` 这一轮**作废并已删除**：它是改码前的临时 A/B 开关版本，
> 日志里 0 条 `Loading font file`（内核日志抓早了，libass 还没初始化），
> 不能用来做结论；真正的 A 侧是 `v20-sysfonts-local2/`。

## 十二、v24 · 长稳 + 拖动回归（2026-11-14）

设计稿：[`docs/design/playback/273-emby-stutter-fix.md`](../../design/playback/273-emby-stutter-fix.md) §十九

这一轮是 **v20 构建**（2 字体）的**整体复验**：120 s 逐秒 meminfo + 3 次进度条拖动，
一次跑完"内存 / 拖动 / 音频 / 缓冲"四条线。

复算命令：

```bash
node tools/p273_soak.mjs docs/evidence/p273-emby-stutter/v24-soak
node tools/p273_gaps.mjs docs/evidence/p273-emby-stutter/v24-soak/kernel.log \
                          docs/evidence/p273-emby-stutter/v24-soak/kernel-diag.log
```

### 12.1 长稳：原生堆的"爬升"不是泄漏，是 demux 缓存按步长填满

| 指标 | 实测 |
| --- | --- |
| 采样点 | 120（逐秒） |
| 原生堆峰值 | **121,857 KB**（第 120 s） |
| TOTAL PSS 峰值 | **327,200 KB**（第 120 s） |
| EGL mtrack 峰值 | 88,496 KB（第 9 s） |
| 原生堆 前 24 点均值 → 末 24 点均值 | 72,921 → 120,729 KB（**+47,808 KB**） |

原生堆**不是**连续爬升，而是**三段台阶**：

```
s=20 起  ~84.5 MB 平台
s=48    ~102.5 MB（+18.0 MB）
s=93    ~120.3 MB（+17.8 MB）
```

同一条会话里 mpv 自己的缓存状态（`demuxer-cache-state`，取数层每 2 s 采一次）也在同步台阶：

```
t= 8 s  fw=15.6M total=19.4M dur=58.3s
t=63 s  fw=14.9M total=32.9M dur=54.5s
t=112 s fw=16.6M total=47.5M dur=56.4s
```

⇒ **台阶 = mpv 的 demux 包队列在往字节上限填**（当前 `demuxer-max-bytes=64MiB` +
`demuxer-max-back-bytes=16MiB` ⇒ 上限 80 MB，末尾实测 `total=80.0M` 已经顶格）。
**不是泄漏**（没有随时间无限增长），但确实是可以拿掉的常驻内存。

### 12.2 拖动 3 次：落点准、不白屏、不断音

| 拖动 | 截图（拖前 / 拖后） | 近白像素 | 判定 |
| --- | --- | --- | --- |
| 1 | `drag1-before.png` / `drag1-after.png` | 0.6% / **0.0%** | 有内容 |
| 2 | `drag2-before.png` / `drag2-after.png` | 1.6% / **0.0%** | 有内容 |
| 3 | `drag3-before.png` / `drag3-after.png` | 1.6% / **0.0%** | 有内容 |

（判据：白屏时近白像素接近 100%；复算 `node tools/p273_png_avg.mjs <png>`。）

### 12.3 音频：拖动全程设备侧**没有**断音

内核自报 3 次 `Audio device underrun detected`（起播 1 次 + 远跳后 2 次），
但设备侧两个独立计数器全程不动：

| 快照 | 音轨 `Underruns` | 音轨 `Flushed` | 混音器 `raw underrun counters` |
| --- | --- | --- | --- |
| `af-drag1.txt` | **0** | 23056 | `partial=0 empty=198` |
| `af-drag2.txt` | **0** | 23056 | `partial=0 empty=198` |
| `af-drag3.txt` | **0** | 23056 | `partial=0 empty=198` |
| `af-final.txt` | **0** | 23056 | `partial=0 empty=198` |

⇒ 与 §十 的判决一致：**内核自报 ≠ 设备断音**，不据此改代码。

### 12.4 本轮**新发现**的缺陷：远跳落地后有 0.6 s 的缓冲停顿

内核日志（远跳到 4932 s）：

```
[ 149.763][v][ffmpeg] stream level seek from 53281274 to 1613145699
[ 152.350][v][cplayer] playback restart complete @ 4932.177000, audio=playing, video=playing
[ 152.358][w][cplayer] Audio device underrun detected.
[ 152.448][v][cplayer] Enter buffering (buffer went from 100% -> 0%) [0.000000s].
[ 152.953][v][cplayer] End buffering (waited 0.597546 secs) [3.712000s].
```

⇒ 落点耗时 **2.59 s**（seek → `restart complete`），落点**之后**又停 **0.597 s**
（缓冲从 100% 掉到 0%）。渲染侧对应 3 次 >200 ms 的间隔（最大 **1523 ms**）。

这是**用户可见**的卡顿（画面停 0.6 s），不是可接受抖动 ⇒ 记为新缺口（见 §19.2 `G-拖动`）。

### 12.5 取数侧：这一会话下载了 136.12 MiB

| 标签 | 字节 |
| --- | --- |
| `play`（主播放链路） | **136.12 MiB** |
| `thumb`（预览内核） | 12.12 MiB |
| `play-tail`（片尾窗口预取） | 0.06 MiB |

会话时长 219.5 s，末 60 s 下载 48.00 MiB（≈819 KB/s），而片源实际码率约 0.33 MB/s
⇒ **超买 ≈2.5×**。

### 12.6 归档文件

| 文件 | 内容 |
| --- | --- |
| `mi-1..120.txt` | 逐秒 `dumpsys meminfo com.kivo.cinema` |
| `mi-drag1..3.txt`、`mi-final.txt` | 每次拖动后与收尾的 meminfo |
| `af-drag1..3.txt`、`af-final.txt` | `dumpsys media.audio_flinger` 快照（音轨表 + 混音器计数） |
| `drag{1,2,3}-{before,after}.png`、`final.png` | 拖动前后截图（白屏判据） |
| `net.log` | 取数层日志（分块字节 + `[cache]` 缓存状态序列） |
| `kernel.log` | mpv 内核日志（`msg-level=all=debug`） |
| `kernel-diag.log` | 渲染层诊断（每次 swap 的时刻与结果） |
| `thumb.log` | 预览内核日志 |


