# 305 · 补 `network_recovery_ms` 与 `thermal_context`（275 §6.1 指标收口）

状态：**两个指标已落地并通过验证**；同时**暴露一个 SMB 重连的真实问题**（不是指标逻辑问题）
设备：K40 `b6e950ee`
时间：2026-11-16 03:52–04:12

## 一、`thermal_context`（热状态上下文）

### 实现
`playback/ThermalSnapshot.kt`（新文件，单职责）：
1. **系统热状态**（API 29+）：`PowerManager.currentThermalStatus`
   → `NONE/LIGHT/MODERATE/SEVERE/CRITICAL/EMERGENCY/SHUTDOWN`；
2. **最高温区温度**：扫 `/sys/class/thermal/` 下每个 `thermal_zoneN/temp`
   （单位是毫摄氏度，按量级自适应；取所有 zone 的**最大值**，避免只看到主板温度）。

不注册监听（一次性查询，与 `NetworkLinkMonitor` 同样的取舍：无生命周期、无泄漏面）。
读不到写 `not_measured: <原因>`，不写 0。

### 真机证据（两次独立会话）

```json
"thermal_context": {"status":"NONE","code":0,"max_temp_c":67.4}
"thermal_context": {"status":"NONE","code":0,"max_temp_c":61.6}
```

- 热状态 `NONE` = 系统未限频；
- 温度从 sysfs 实读（67.4 ℃ / 61.6 ℃，两次会话差 6 ℃，符合"第一次刚跑完压测"的预期）。

## 二、`network_recovery_ms`（网络恢复 → 媒体时钟再次前进）

### 为什么不能只看"离线/在线"
275 的定义是「**网络恢复 → 媒体时钟再次前进**」。网络回来但内核还在等表头时，用户看到的仍是卡住。
所以状态机要**三道闸门同时成立**：

```
位置在走 ──(位置停 ≥3s 且系统报离线)──> 离线中
   ↑                                      │
   └──(位置重新增长，记 recovery)─────── 网络回来了（记 onlineAt）
```

- 只有"位置停"不够（可能是暂停/解码慢/seek）；
- 只有"离线"不够（可能正好暂停）；
- 只有"网络回来"不够（时钟可能还没恢复）。

### 实现（2 文件改动 + 1 新文件）
| 文件 | 作用 |
| --- | --- |
| `playback/NetworkRecoveryWatch.kt`（新） | 纯状态机，**不碰 Android API**（可单测）；`note(pos, paused, online, nowMs)` |
| `playback/MpvCacheDiag.kt` | 每 2 s 采样时喂一次（位置 / `pause` 属性 / `NetworkLinkMonitor.isOnline`）；`attach()` 加 context 参数并 reset |
| `playback/PlaybackMetrics.kt` | `finish()` 输出 `network_recovery_ms`，无发生则 `not_measured: 本次会话未发生断网→恢复` |

### 验证 1：状态机单测 4/4 PASS（虚拟时间戳）

| # | 场景 | 结果 |
| --- | --- | --- |
| 1 | 正常播放（位置每拍增长） | 不计恢复 ✅ |
| 2 | **暂停**（位置不动 + 系统报离线） | **不误判为断网** ✅ |
| 3 | 断网 → 恢复 → 时钟前进 | **精确记 2000 ms** ✅ |
| 4 | 位置停但**网络一直在线**（解码慢/seek） | 不计恢复 ✅ |

第 2、4 条最容易误报，专门钉死。

### 验证 2：真机端到端（第一次，成功）

起播 SMB → 播 30 s → `svc wifi disable` 70 s → `svc wifi enable`：

```
[recovery] 判定离线（位置已停 4008ms）
[recovery] 网络已恢复，开始等媒体时钟前进
[recovery] 网络恢复 → 媒体时钟前进 2002ms（离线 40058ms）
```

**自动测出 2002 ms，与 P253 手工测的 2.0–2.2 s 一致。**

### 验证 3：第二次端到端 —— 失败，但**指标是对的**

同样流程再跑一次，结果没记到恢复。日志给出原因：

```
PROXY read fail @528303793 attempt=360: SMB 连接已断开，重连失败：无法连接服务器
[ffmpeg] http: Will reconnect at 528303793 in 0 second(s), error=Connection timed out.
```

**SMB 层重试了 360 次仍未连上**，媒体时钟确实没有前进
⇒ 指标正确地**没有**记成"已恢复"（这正是 275 想要的口径：网络回来 ≠ 播放回来）。

**这是一个待查的真实问题**：`svc wifi disable/enable` 这种"接口消失再出现"的场景下，
`SmbByteSource` 的重连没有成功恢复（`attempt` 一路涨到 360）。
P253 的 62 s 断网能 2.2 s 接上，本轮 70 s 却失败 —— 断网时长或"接口消失"这一差异需要单独复现。
**不在本卡范围内**，已记入待办。

## 三、附带修正

`PlaybackMetrics` 文件头的"能测/不能测"表已更新：
`network_recovery_ms` 与 `thermal_context` 从 ❌ 改为 ✅；
`seek_total_ms` 标注为"见 seeks 明细（p50/p95/max），顶层字段仍留 not_measured"。

## 四、边界

1. **网络恢复的分辨率约 ±2 s**（采样周期 2 s），比人工计时粗；
2. 只认**最近一次**恢复，不支持一次会话多次断网的完整序列；
3. `thermal_context` 是**时间点快照**（finish 时刻），不是全程曲线
   —— 想解释"第 20 分钟掉帧"，得配合多次会话或后续加时序采样；
4. **SMB 重连失败**（attempt=360）未修，是独立问题；
5. 断网测试用 `svc wifi disable`（接口消失），与"路由器掉线/拔网线"（接口在、链路断）
   可能不是同一种失败模式，结论不可直接外推。

## 五、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
A="adb -s b6e950ee"

# 1) 状态机单测（纯逻辑，不需真断网）
cd android
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.playback.NetworkRecoveryWatchInstrumentation --offline
adb -s b6e950ee shell am instrument -w \
  com.kivo.cinema.test/com.kivo.cinema.playback.NetworkRecoveryWatchInstrumentation

# 2) 温度上下文（任意一次播放结束后）
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.media "/sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv"
sleep 9; adb -s b6e950ee shell input keyevent KEYCODE_HOME; sleep 3
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/metrics/play.json" | grep thermal_context

# 3) 网络恢复端到端（需 2 分钟；先把 auto_pip 关掉，否则 HOME 只进画中画不触发 onStop）
adb -s b6e950ee shell svc wifi disable; sleep 70
adb -s b6e950ee shell svc wifi enable;  sleep 25
adb -s b6e950ee shell input keyevent KEYCODE_HOME; sleep 5
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/net-diag.log" | grep recovery
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/metrics/play.json" | grep network_recovery_ms
```

## 六、证据文件

`metrics.json`（第一次端到端会话的完整 JSON）、`net.log`（recovery 三行日志）。
