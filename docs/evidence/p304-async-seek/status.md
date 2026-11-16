# 304 · 异步 seek + 合并过期请求（275 §4.2 的最后一格）

状态：**3/3 PASS，gap 闭合**
设备：K40 `b6e950ee`
时间：2026-11-16 03:45–04:05
片源：本地 `mpv-test-3min.mkv`（26.6 MB / 180 s）

## 一、问题不止是"排队"

300 的同步 seek 压测把这条 gap 标成 `serial=true`。真正查下去发现**比"排队"更严重**：

- `PlayerActionRouter.performSeek()` 在 **UI 线程**执行；
- 它调的 `MpvPlaybackSession.seekTo()` 是**同步等落点**（`MpvSeekControl.awaitLanding`，上限 1.5 s）；
- 300 实测单次耗时 **100–216 ms**。

⇒ 用户拖动松手那一刻，**主线程被占住 100–216 ms**（约 6–13 帧）。
连续拖动 5 次就是 0.5–1 s 的累计卡顿，而且画面会**依次跳到 5 个中间位置**。

这也和 274 记录的「拖动是元凶：无拖动 23.87 fps · 带拖动 5.32 fps」对得上。

## 二、改法（两个文件）

### 2.1 `MpvPlaybackSession.seekToAsync()`

- 投递到会话**自己的单线程 worker**（与 `start`/`poll` 串行，不会出现"上一个 seek 还没落定、下一个已发出去"）；
- 每次调用领一张**单调递增的票**；worker 拿到任务先比票：
  **票不是最新 ⇒ 直接返回，连 seek 都不发**；
- 落点回来后再比一次票：过期结果**不许覆盖** `positionUs` / `lastSeekResult`（否则进度条会回跳）；
- 新增诊断字段 `seekExecuted`：实际执行次数（合并生效时应远小于提交次数）。

同步的 `seekTo()` **原样保留**，给探针/instrumentation 用（它们需要确定结果再断言）。

### 2.2 `PlayerActionRouter.performSeek()` 的 mpv 分支改调异步

其余三条链路（SMB 软解 / 自研内核 / 本地）**未改**——那几条的 seek 成本没有实测数据，不凭猜改。

## 三、验证：3/3 PASS（`MpvSeekAsyncInstrumentation`）

| 用例 | 实测 | 判据 |
| --- | --- | --- |
| 20 次乱序提交 | **1 ms**（同步版需 2 s+） | <300 ms ✅ |
| 实际执行次数 | **1 次**（提交 20 次） | 1–3 ✅ |
| 落点 | 165333 ms（目标 165000）**drift 333 ms** | <6 s ✅ |
| `lastSeekResult.targetUs` | 165000 ms（= 最后目标） | 相等 ✅ |
| 连发 170s→20s | 落点 **20000 ms（drift 0 ms）**，170s 被跳过，新增执行 1 次 | 未被旧目标覆盖 ✅ |
| **主线程**提交 20 次 | **1 ms**（`runOnMainSync` 实测） | <200 ms ✅ |

最后一条是关键：同步版在主线程必然占 100–216 ms/次，现在只花"投递"的时间。

## 四、行为变更（必须记录，不许静默）

1. **`kivo.seekStress` 压测的语义变了**。它原先是专门压"上一次 seek 没落位又来下一次"的
   覆盖竞争；现在中间目标被合并掉 ⇒ **不再存在覆盖竞争**。
   ⇒ 这个工具的历史数据与今后数据**不可直接比较**。
2. **`kivo.landSeekSecs` / `kivo.landSeekSec` 探针也走异步了**（它们经 `seekTo` → `performSeek`）。
   影响可忽略：投递到空闲 worker 是微秒级，而探针量的是秒级时间差（<5 ms 误差）。
   但口径上要知道"现在多了一次投递"。
3. 异步 seek 期间 `poll()`（200 ms 周期）会推迟（共用 worker）。
   落定后 `positionUs` 被落点直接覆盖，进度条不会停在旧值。

## 五、边界

1. **未做手势级体感验证**（真机用手指拖进度条看是否顺）。
   现有证据是"主线程占用 1 ms"这一硬指标；体感改善是推论，不是直接观测。
2. 只改了 **mpv 链路**。SMB 软解 / 自研内核 / 本地文件三条链路的 seek 仍是同步，
   它们的耗时**没有实测数据**，所以没动。
3. 只测 K40 + 本地片源；公网源下 seek 落点更慢（表头等待），合并收益应更明显，但未实测。
4. 异步 seek 的**落点精度**与同步版一致（同一 `MpvSeekControl`），本测试 drift 0–333 ms。

## 六、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android
./gradlew :app:assembleDebug --offline
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.mpv.MpvSeekAsyncInstrumentation --offline
adb -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s b6e950ee shell am instrument -w \
  -e mediaPath "/sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv" \
  com.kivo.cinema.test/com.kivo.cinema.mpv.MpvSeekAsyncInstrumentation
```

期望：`3/3 PASS`，`提交 20 次耗时 ≤3ms`、`实际执行=1`、`主线程提交 20 次耗时 ≤3ms`。

## 七、改动文件

| 文件 | 改动 |
| --- | --- |
| `playback/MpvPlaybackSession.kt` | 新增 `seekToAsync()` + `seekTicket` + `seekExecuted`；同步 `seekTo()` 保留 |
| `ui/compose/PlayerActionRouter.kt` | `performSeek()` 的 mpv 分支改调异步 |
| `androidTest/.../MpvSeekAsyncInstrumentation.kt` | **新增**，3 个用例 |
