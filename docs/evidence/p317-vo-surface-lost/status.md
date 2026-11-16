# 317 · Emby 4K 片源黑屏有声（vo 初始化失败）诊断

状态：**根因已定位，A+C 已实现并验证（vo 失败场景未实测）**
设备：K40 `b6e950ee`
时间：2026-11-16 16:19–16:55
触发：用户播**第二个 Emby 源**里的一部片 —— **有声音、无画面**

## 一、片源

```
[mkv] + duration: 8700.640s          （2 小时 25 分）
[Video --vid=1] (hevc 3840x2160 23.976 fps)
size=27221193219                     （27.2 GB，直连，非转码）
url=http://nl2.lovehd.de:8096/emby/Videos/189779/stream?static=true&...
```

⇒ **4K HEVC 原盘直连**（`static=true` 表示不转码）。

## 二、根因链（四步，每步都有日志）

### ① 源站极慢

```
[play] origin chunk from=25847810 code=206 hdrMs=12698 len=4194304
```

**12.7 秒才返回 4 MiB** ⇒ **≈ 330 KB/s（2.6 Mbps）**。
对 4K HEVC（通常 20–60 Mbps）来说，**差一个数量级**。

期间还有多次超时：

```
[play] origin open fail @27220770247: timeout     ← 27.2 GB 处（片尾窗口）
[play] origin open fail @4713961879: timeout      ← 4.7 GB 处
[play] origin open fail @283277784: timeout       ← 283 MB 处
```

### ② 打开耗时 61 秒

```
[ 99.3s] Opening http://127.0.0.1:40739/1 ...
[160.2s] Opening done: http://127.0.0.1:38171/1 ...
```

**61 秒**。而且端口从 **40739 变成 38171** ⇒ 中间**重新打开过一次**（第一次可能失败/被顶掉）。

### ③ 建立视频输出时 Surface 已失效

```
[160.278][v][vo/gpu-next/opengl] Initializing GPU context 'android'
[160.279][f][vo/gpu-next/android] Failed to create ANativeWindow
[160.279][e][vo/gpu-next] Failed initializing any suitable GPU context!
[160.279][f][cplayer] Error opening/initializing the selected video_out (--vo) device.
[160.279][i][cplayer] Video: no video
```

**为什么失效**：我们的 gpu-next 路径把 **Surface 的 jobject 当 `wid`** 交给 mpv
（`mpv_session.cpp:48`），但 `wid` 只是一个引用；mpv 真正调
`ANativeWindow_fromSurface()` 是在**建 vo 的那一刻**（160.28 秒）。
**那 61 秒里 Surface 的底层 native window 已经被废弃**（系统回收 / 配置变化 / 重新打开），
于是转换失败。

### ④ 音频继续 ⇒ 黑屏有声

```
[160.413][i][cplayer] AO: [audiotrack] 48000Hz stereo 2ch float
[120.429][v][cplayer] playback restart complete @ 0.011000, audio=playing, video=eof
```

`video=eof`（视频轨被判结束）+ `audio=playing` ⇒ **用户看到黑屏、听到声音**。

## 三、★ 最严重的地方：UI 完全不知道

Kotlin 侧**没有任何** mpv 路径的 vo 失败处理（grep `no video` / `video_out` 只命中
`FfmpegPlaybackSession` 的旧软解路径）。

⇒ 用户看到的是：**黑屏 + 有声音 + 界面显示"正在播放"**，会一直等下去。
**这是静默失败，违反"不许静默降级"铁律。**

## 四、修复方案（三档，按代价排序）

| 档 | 做什么 | 代价 | 效果 |
| --- | --- | --- | --- |
| **A（必须）** | **让 UI 知道 vo 失败**：首帧后读 `video-params`，为空则报「视频输出初始化失败」 | 小（Kotlin 侧，20 行） | 用户不再干等，能看到真实原因 |
| **B** | **vo 失败后自动降级**：`gpu-next` → `gpu`（或 `mediacodec_embed`）重试一次 | 中（要重建 vo，且降级后 HDR/插值失效） | 4K 至少能出画（SDR） |
| **C** | **慢源提示**：打开超过 N 秒显示「源站响应慢（已等 61 秒）」 | 小 | 用户知道在等什么 |

**建议先做 A + C**（都是小改动，直接消除"静默"），**B 单独评估**——
因为 vo 降级会让 HDR 和帧插值全部失效，属于"用画质换可用"，需要用户知情。

## 五、这不是"必现 bug"

触发条件是**三个因素同时成立**：

1. 片源是 **4K 原盘直连**（大码率、大文件）；
2. **源站极慢**（330 KB/s，打开 61 秒）；
3. 打开期间 **Surface 生命周期发生变化**。

1080p 小文件、或转码流（HLS）不会触发。**所以它是"慢源 + 大文件"下才暴露的边界**，
但一旦触发就是黑屏，体验上比"播不出来"更糟（用户会一直等）。

## 六、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/mpv-kernel.log" \
  | grep -iE "Opening|Failed to create ANativeWindow|video_out|Video: no video|audio=playing"
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/net-diag.log" \
  | grep -iE "origin chunk|origin open fail|prep"
```

## 七、证据

内核日志与 net-diag 片段（上引全部为原文）。


## 八、已实施的修复（A + C）

### A · 让 UI 知道 vo 失败

改 `playback/MpvFirstFrameWait.kt`（与"起播判定"同职责，不新建文件）：

原来的判定把 `vid=no` 一律当成"真的纯音频媒体"放行：

```kotlin
if (position > 0.0 && native.property("vid")?.trim() == "no") {
    return Result(position, false)      // ← 我们的 4K 故障也走这里，被当正常放行
}
```

**两者内核表现相同，但性质完全不同**：

| 情况 | `video-codec` | `vid` | 性质 |
| --- | --- | --- | --- |
| 真的纯音频（.mp3） | 空 | `no` | 正常 |
| **vo 初始化失败** | **hevc**（有值） | `no` | **故障** |

⇒ 新增判据：**`video-codec` 非空却 `vid=no` ⇒ 抛异常**（走既有的
`@throws IllegalStateException` 约定，UI 会显示成错误，不再黑屏干等）：

```kotlin
val codec = native.property("video-codec")?.trim().orEmpty()
if (codec.isNotEmpty() && !codec.equals("no", true)) {
    NetDiag.log("[prep] ✗ 视频输出初始化失败：片源含 $codec 视频轨，但内核未建出视频输出…")
    throw IllegalStateException("视频输出初始化失败：片源有 $codec 视频轨，但内核没能出画。…")
}
```

错误文案里带上了**用户能采取的动作**（关低码率/转码版可播），避免只报技术名词。

### C · 慢源提示

等待首帧的循环里每 **5 秒**留一条日志：

```
[prep] 仍在等首帧 … 已等 35s position=0.0 vid=no
```

62 秒的打开过程从此在日志里**可追溯**，而不是一片空白。

## 九、验证

**设备：K40 `b6e950ee`**（用户要求用手机验，平板只用于前期无设备时的探索）。

| 场景 | 期望 | 实测（K40） |
| --- | --- | --- |
| 正常视频（640×360 MP4） | 不误报 | ✅ `MPV 首帧就绪`，**无"视频输出初始化失败"** |
| 纯音频（.mp3） | 不误报 | ⚠️ 起播不了，但**不是误报 vo 失败**（见下） |
| **真实 vo 失败（4K 慢源）** | 抛异常、UI 报错 | ❌ **未实测** |

**纯音频为什么起播不了** —— 顺带发现：**我们的 mpv 构建里没有 mp3 demuxer**。

```
[v][ffmpeg] Mime-type: 'video/x-matroska'
[e][lavf] avformat_open_input() failed
[v][cplayer] finished playback, unrecognized file format (reason 4)
```

排查过程：先怀疑测试服务器把 mp3 标成了 `video/mp4`（确实标错了，已修成按扩展名给 `audio/mpeg`），
**但修好 mime 之后依然解不出** ⇒ 根因是 **`libavformat.so` 精简构建（2.5 MB）没编入 mp3 demuxer**，
与 §三十（DASH）是同一个根因。

**对本卡结论的影响：无。** A 的判据是 `vid=no` **且** `video-codec` 非空；
mp3 场景下 `vid` 始终是 `auto`（内核根本没识别出轨道），压根不进那个分支 ——
它最终会在 30 秒后报「内核起播超时」，**不会被误判成"视频输出初始化失败"**。

**C 的价值在这次验证里直接体现了**：等待期间每 5 秒一条
`[prep] 仍在等首帧 … 已等 10s position=-1.0 vid=auto`，
让人一眼能看出"它在等、而且在第几秒、卡在哪个状态"——而不是对着一个不动的界面猜。

**未实测的原因**：复现需要"4K 原盘 + 330 KB/s 慢源 + 打开耗时 60 秒以上"，
本地没有这样的源；靠代码逻辑保证（`video-codec` 来自解封装报出的轨道，与 vo 是否成功无关），
**等下次真机遇到时验证**。

## 十、没做的

**B 档（vo 自动降级 `gpu-next → gpu`）未实施**：降级会让 **HDR 和帧插值全部失效**，
属于"用画质换可用"，需要单独评估并让用户知情。当前只做到"报错而不是黑屏"。


## 十一、后续实测：属性可读性 + 两台 Emby 服务器对比

### 11.1 ★ A 的判据被证明有效（属性真的读得到）

在第一个 Emby 服（`https://emby.bangumi.ca`）播放 4K HEVC 时，317 加的取证日志：

```
[vo] 首帧后属性 vid=1 codec=H.265 / HEVC (High Efficiency Video Coding) conf=true
     params={"pixelformat":"mediacodec","w":3840,"h":1920,...}
```

- `vid=1`（有视频轨）
- **`codec` 有完整值** ⇒ 「`codec` 非空却 `vid=no`」这个判据**不是盲的**
- `conf=true`（首帧确认上屏）

⇒ 317-A 的检测逻辑可行。**但"vo 真的失败时 `codec` 是否仍有值"仍未实测**
（要靠下次真机遇到；理论上 `video-codec` 来自解封装报出的轨道，与 vo 成功无关）。

### 11.2 ★ 两台服务器的差别是客观的

| 服务器 | 读取耗时（4 MiB / hdrMs） | 结论 |
| --- | --- | --- |
| `emby.bangumi.ca`（443） | 1086 → 1168 → 1093 → 1069 → 1278 | **稳定 ~1.1 s（≈3.6 MB/s）** |
| `nl2.lovehd.de:8096` | 1101 → 592 → 1148 → 3172 → **6713** | **持续恶化至 6.7 s** |

且用户当晚实测：**同一时段别的播放器也播不动 8096 那台**。

⇒ **8096 那台是服务器自身问题**（不是我们的取数层 bug）。
**这一条纠正了我此前的两次误判**：
① 先归因"源站慢"（被"别的播放器快"推翻）；
② 又归因"我们的连接退化"（被"今天别的播放器也播不动"推翻）。

**教训（第三次同类）**：**"别的播放器快"和"我们的慢"必须同一天同一时段测**，
否则拿到的是两个不同时间点的服务器状态，不能作为对照。

### 11.3 顺带确认的能力

```
Loading hwdec driver 'aimagereader'
Decoder format: 3840x1920 [0:1] mediacodec bt.709/... 
```

4K HEVC 走 **mediacodec 零拷贝**（`aimagereader`）——这是相对 SenPlayer（只有
`mediacodec-copy`）的真实优势项。

### 11.4 HLS 选档仍未验证

用户播的是 `?static=true`（**原盘直连**），没有 `master.m3u8` ⇒ **选档逻辑未被触发**。
要验选档需让 Emby **转码播放**（客户端选较低码率或强制转码）。
