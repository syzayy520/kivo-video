# P12-D1 · Surface 重建 · 设计

- 版本：V1.0
- 日期：2026-10-29
- 上游：`07-p12-plan.md`（P12-D 红线）、`25-p12c8-status.md`（C 组收口）
- 卡片：`docs/handoff/TASKS.md` → `P12-D1`

---

## 一、今天的行为（先量，再改）

`PlayerActivity.onSurfaceLost()` 现在做的是 **`teardownSession()`** ——
把整个 `SessionHandle` 销毁、fd 关闭、`playbackRequested = false`。
切后台再回来，`surfaceCreated` 触发 `maybeStartPlayback()`，
于是**从第一帧重新打开、重新解码、重新起播**。

也就是说：今天不是"重建后恢复得慢"，而是**根本没有恢复，是重播**。
卡片写的"3 帧内恢复 / 不黑屏超 500ms"在这个基础上无从谈起，先得让它有"恢复"这回事。

---

## 二、判据（卡片原文 + 可测量的拆解）

卡片：**重建后 3 帧内恢复，不崩、不黑屏超 500ms。**

"黑屏"要拆成两段，否则会把用户手动切走的时间算到我们头上：

| 量 | 定义 | 谁的账 |
| --- | --- | --- |
| `rebuild_gap_ms` | `detach` 调用 → `rebind` 调用 | **用户的**（切后台多久） |
| `resume_ms` | `rebind` 调用 → 重建后第一帧真正交出 | **我们的**，判据 ≤ 500 ms |
| `frames_lead` | `detach` 到重建后首帧之间被交出去的帧数 | 我们的，判据 ≤ 3 |
| 不崩 | 进程存活、`completed = true`、帧数仍闭合 | 硬性 |

`resume_ms` 与 `frames_lead` 都从 `releaseOutputBuffer` 的**真实调用**处取，
不从上层的"我以为交了"处取 —— C6B 已经吃过一次"交出时刻 ≠ 上屏时刻"的亏。

---

## 三、机制：热换输出面，**不重开会话**

```
ANativeWindow* 变了  →  AMediaCodec_setOutputSurface(codec, new)
```

- 这是 Android 上换面的标准做法（`MediaPlayer` / `ExoPlayer` 的
  `setOutputSurface` 都走这条路），API 23+，`minSdk 26` 覆盖。
- **不 stop/start 会话。** 因为 `DecodeSession::start()` 里有 `counters_.reset()`
  与 `pacer_->reset()`：走一趟就是"帧计数归零 + 时间轴重锚"，
  前者让 C8 那套帧数闭合不变量失效，后者让"3 帧内恢复"退化成"从头追帧"。
- 换面时**已在队列里的帧会被丢掉**，这是平台语义，不是 bug。
  它们本来就要显示在已经消失的旧面上。

### 3.1 为什么必须有个互斥量

`release_frame*()` 在收帧线程，`set_output_surface()` 在调用线程（JNI 线程）。
两者同时碰同一个 `AMediaCodec*`，而 NDK 没有为"换面 vs 交帧"给出并发保证。

因此 `MediaCodecVideoDecoder` 内部加一把 `surface_mutex_`，**只护这两件事**：

| 加锁 | 不加锁 | 理由 |
| --- | --- | --- |
| `set_output_surface`、`release_frame`、`release_frame_at` | `submit`、`receive`、`flush`、`stop` | 输入侧与取帧侧都不碰窗口。给它们也加锁只是让热路径更慢，不增加安全性 |

热路径代价：每帧一次无竞争 lock/unlock，几十纳秒，而 `releaseOutputBuffer` 本身是微秒级。
**不为此改成原子指针**：窗口换的是平台句柄，不是我们自己的状态，原子量挡不住平台侧的竞争。

### 3.2 面消失时先"摘面"，**但不碰平台**

> V1.0 这里写的是 `AMediaCodec_setOutputSurface(codec, nullptr)`（切到 ByteBuffer 模式）。
> **真机第一轮就被否掉了**：K40 那颗 `OMX.qcom.video.decoder.avc` 切过去之后
> 再切回 Surface 直接返回 `AMEDIA_ERROR_UNKNOWN`（status = −10000），会话只能作废。
> 下面是修正后的做法。

"摘面"是**本地行为**：会话里一个原子标志置位，收帧段看到它就一律 `render=false` ——
只把输出缓冲还回去，一次都不碰 Surface。

```
surfaceDestroyed → session_rebind_window(handle, nullptr)   // 只置标志 + 记时刻
                → （用户切回来）
surfaceCreated   → session_rebind_window(handle, newWindow) // window → window 直换
```

为什么这样是安全的：`releaseOutputBuffer(index, false)` 不接触 Surface，
所以旧 BufferQueue 已经 disconnect 也不会有事；而旧 `ANativeWindow*`
**必须继续握着**（codec 还指着它），直到新面挂上之后才能放。

为什么 window→window 直换能用：这是平台真正支持的那条转场，
`MediaPlayer` / `ExoPlayer` 换面走的就是它。

### 3.3 为什么必须摘面，而不是什么都不做

不做的话，`surfaceDestroyed` 之后收帧线程还在 `releaseOutputBuffer(render=true)`，
目标是已经 disconnect 的 BufferQueue。

而且这个项目里后果更重：`release_frame` 失败会直接让收帧段 `abort()` 整条流水线 ——
一次切后台就等于把播放打死。摘面之后这条路径根本不会被走到。

---

## 四、家族树（不新增文件，只加职责）

```
backends/mediacodec/decode/
└── mediacodec_video_decoder.hpp/.cxx   + set_output_surface()、surface_mutex_
backends/mediacodec/session/
├── decode_session.hpp/.cxx             + rebind_window()、重建计数与时刻
├── decode_stats.hpp                    + window_rebinds / rebuild_gap_ms / resume_ms / frames_lead
├── decode_receive_worker.hpp/.cxx      + 首帧回填（rebind 之后第一帧）
└── decode_stats_renderer.cxx           schema /5 → /6（新增 pipeline.surface_rebuild）

android/.../jni/jni_session_handle.hpp/.cxx  + session_rebind_window()
android/.../jni/jni_player_bridge.cxx        + nativeSessionRebindSurface
android/.../ui/PlayerActivity.kt             + 不 teardown；改挂新面
```

**为什么不新建模块**：这个能力属于"解码会话怎么处理输出目标"，
拆出去会得到一个只有两个函数、却要反过来持有 `DecodeSession` 的模块。
按铁律，它归位在 `session/`。

**为什么 `PlayerActivity` 要改而不是只改 JNI**：今天"销毁会话"这个决定
写死在 `onSurfaceLost()` 里。不改它，native 侧做得再对也不会被调用。

---

## 五、退路（先写下来，免得到时候临时改口径）

**这条退路没有被启用。** 记录它是因为"什么时候该认输"必须提前写下来。
实际走通的是 §3.2 的 window→window 直换 + 本地摘面。

若日后在别的芯片上热换面不工作（症状：换回后 `frame_count` 不再增长、
或 `setOutputSurface` 返回非 OK）：

1. 改为"**停而不销毁**"：`request_stop` → join 两个 worker → 换面 → 重建 worker；
2. 代价：`counters_` 与 `pacer_` 会被 `start()` 重置，必须额外把
   `counters_` 的累计值搬过去（新增 `carry_over` 语义），否则帧数不闭合；
3. 这条退路**改变判据**：时间轴重锚后 `frames_lead` 不再是"3 帧"，
   要在验收里明说"恢复 = 从当前 pts 继续，重新建锚"。

退路不是坏事，但**必须先说清楚它会改哪个数字**，而不是事后解释。
