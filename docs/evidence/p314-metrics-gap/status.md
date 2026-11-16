# 314 · 275 §6.1 指标收口（最后两项补上）

状态：**已修复并在平板真机验证**
设备：MatePad `CFE6R21B12000179`（1600×2560，A12）
时间：2026-11-16 12:55–13:40

## 一、出发点：275 §6.1 还剩两项标「⚠️ 部分」

| 指标 | 275 记录的状态 |
| --- | --- |
| `frames_dropped` | mpv `vo-drop-frame-count` 未接，只能看渲染帧数差 |
| `seek_total_ms` | 有落点钩子，但没有「请求→落点」自动计时 |

**先实测再改**——结果发现两项的实际情况都和文档记录不同。

## 二、实测（平板，本地 HTTP 源，2 分钟 640×360）

修复前的 JSON：

```json
"buffer_runway_ms": 20387,
"frames_dropped_vo": "not_measured: mpv 属性读不到",
"frames_dropped_total": 2,
"seek_total_ms": "not_measured: 尚未埋点",
"seeks": { "count": 1, "total_ms_p50": 401, ... },   ← 明明有真数据
"thermal_context": {"status":"NONE","max_temp_c":61.0}
```

**两条结论**：

1. **`seek_total_ms` 早就在采了**（`recordSeek()` 280 就接了，p50=401 ms 是真实值），
   但 `metrics.seek_total_ms` 这个字段写死成「尚未埋点」—— **字段没跟上传导链**。
   ⇒ 这是「报未测但其实有数据」，比缺数据更危险：**会让人以为没做**。
2. **`frames_dropped_vo` 真的读不到**，原因不是没写代码，而是**属性名错了**（见下）。

## 三、修复 1：`vo-drop-frame-count` 这个属性根本不存在

查 mpv 源码 `player/command.c`：

```c
{"decoder-frame-drop-count", mp_property_frame_drop_dec},   // 解码层丢帧
{"frame-drop-count",         mp_property_frame_drop_vo },   // VO 丢帧
```

⇒ 我们代码里写的 **`vo-drop-frame-count` 不存在**，所以恒为 `not_measured`；
而 `frame-drop-count` 被我们当成了「total」。

**改法**：

```kotlin
val dropVo  = prop("frame-drop-count")            // VO 丢帧（真正的 frames_dropped）
val dropDec = prop("decoder-frame-drop-count")    // 解码丢帧（另一个维度）
```

JSON 字段随之改为 `frames_dropped_vo` / `frames_dropped_decoder`（语义准确，不再叫 total）。

## 四、修复 2：`seek_total_ms` 接上 seeks 汇总

```kotlin
sb.append("    \"seek_total_ms\": ")
if (seeks.isEmpty()) sb.append(q("not_measured: 本次会话未发生 seek"))
else sb.append(seeks.map { it.totalMs }.sorted()[seeks.size / 2])   // p50 作为代表值
```

明细与 p95/max 仍在下面的 `seeks` 区（不重复）。

## 五、验证（修复后，同源同片）

```json
"frames_dropped_vo": 5,
"frames_dropped_decoder": 0,
"seek_total_ms": 126,
"buffer_runway_ms": 37593,
"seeks": { "count": 1, "total_ms_p50": 126, "total_ms_max": 126 }
```

| 指标 | 修复前 | 修复后 |
| --- | --- | --- |
| `frames_dropped_vo` | not_measured | **5** |
| `frames_dropped_decoder` | （字段不存在） | **0** |
| `seek_total_ms` | not_measured | **126 ms** |

## 六、275 §6.1 的完整状态（11 项）

| 指标 | 采集路径 | 状态 |
| --- | --- | --- |
| `startup_total_ms` | `stages_ms.startup_total` | ✅ 957 ms（实测） |
| `open_ms` | `stages_ms.source_ready`（近似） | ⚠️ 语义不完全等同「父 m3u8 打开」 |
| `first_decode_ms` | — | ❌ mpv 未对外暴露解码完成事件 |
| `first_present_ms` | — | ❌ 首帧回调本身就是「已上屏」，无法再分段 |
| `read_latency_ms` | `OriginLatencyStats` | ✅ 在取数层，**不在 metrics JSON 里** |
| `buffer_runway_ms` | `demuxer-cache-time` | ✅ 37593 ms |
| `render_gap_ms` | `tools/p274_sf_latency.mjs`（SurfaceFlinger） | ✅ 外部量具 |
| `frames_dropped` | `frame-drop-count` + `decoder-frame-drop-count` | ✅ **本卡修复** |
| `seek_total_ms` | seeks p50 | ✅ **本卡修复** |
| `network_recovery_ms` | `NetworkRecoveryWatch` | ✅ 已接（本次会话未触发） |
| `thermal_context` | `ThermalSnapshot` | ✅ 61.0 °C |

⇒ **9 项直接可采，2 项（read_latency / render_gap）由别的组件采**，
**2 项明确不可采（first_decode / first_present，内核未暴露）** —— 这是 mpv 的限制，不是我们的债。

## 七、275 §6.2 验收：连续 20 次 seek 无旧请求覆盖新目标 —— **PASS**

### 7.1 先纠正我自己的一个误判

我一度认为「`kivo.landSeekSecs "20,45,70,95"` 只执行了 1 次」是缺陷。
**其实不是** —— 看 `PlayerProbeLandingSeek`：

```kotlin
val at = delayMs + index * delayMs     // 8000, 16000, 24000, 32000
handler.postDelayed({ seekTo(...) }, at)
```

4 个落点分别在**第 8/16/24/32 秒**执行，而我当时只等了 16 秒 ⇒ 自然只到 1~2 个。
**等够 40 秒后：`count = 4`，落点全部精确。**

> 教训：把"没等到"当成"没执行"，是又一种"现象可见 ≠ 原因正确"。

### 7.2 四组落点（等 40 秒）

| 请求 | 落点 | 耗时 |
| --- | --- | --- |
| 20 s | 20 s | 160 ms |
| 45 s | 45 s | 108 ms |
| 70 s | 70 s | 103 ms |
| 95 s | 95 s | 106 ms |

### 7.3 连续 20 次压测（`kivo.seekStress 20`，间隔 1.2 s）

压测模式与普通落点的差别：目标在 `[10s, 片尾-10s]` 之间**按质数步长循环**、**固定间隔连发**，
专门制造「上一次 seek 还没落位又来了下一次」的覆盖竞争。

```
count        : 20
failed       : 0
p50/p95/max  : 105 / 156 / 166 ms
error p50/max: 0 / 0          ← 请求与落点完全一致
落点偏差>500ms: 0
```

⇒ **275 §6.2「连续 20 次 seek 无旧请求覆盖新目标」验收通过**：
20 次全部成功、零失败、**落点零偏差**、最慢 166 ms。

### 7.4 仍未解决
1. ~~**`smbTrace` 的探针日志没进 `net-diag.log`**~~ ⇒ **我找错文件了**。
   `smbTrace`（`PlayerSessionBridge.kt:10`）写的是 **`files/evidence/p220-status.log`**，
   不是 `net-diag.log`。查该文件：`grep -c "seek-stress"` = **20 条全在**。
   ⇒ 275 §6.2 的验收现在有**双份证据**：metrics 的 `count=20` + 日志的 20 条探针记录。
   （`smbTrace` 这个名字有误导：它同时服务 cast / danmaku / seek 等多条链路，不只是 SMB。）
2. **`open_ms` 用 `source_ready` 近似**，语义不完全等同 275 的定义。
3. **`first_decode_ms` / `first_present_ms` 无解**，除非改内核暴露事件（成本高、收益低）。

## 八、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1

# 起一个支持 Range 的服务（Python http.server **不支持 Range**，片尾窗口会被拒）
cd /c/temp && node range-server.js &          # 监听 127.0.0.1:8202
adb -s CFE6R21B12000179 reverse tcp:8202 tcp:8202

cd android && ./gradlew :app:assembleDebug --offline
adb -s CFE6R21B12000179 install -r "C:/kivo video/android/app/build/outputs/apk/debug/app-debug.apk"
adb -s CFE6R21B12000179 shell am force-stop com.kivo.cinema
adb -s CFE6R21B12000179 shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8202/test-2min.mp4" \
  --es kivo.label "p314-fix" --es kivo.landSeekSecs "20,45,70,95"
sleep 16
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema cat files/metrics/p314-fix.json"
```

## 九、两个踩坑（操作层）

1. **`adb push` / `install` 的路径必须用 Windows 写法**（`C:/...`），
   Git Bash 的 `/c/...` 会被 Windows 版 adb 当成字面路径 ⇒ `No such file or directory`。
2. **Python `http.server` 不支持 Range** ⇒ app 报「服务器忽略了范围请求，无法定位到指定位置」。
   需要 Range 的测试（片尾窗口、seek）必须用支持 206 的服务。
