# P12-F2A 设计：不重启流的音频 seek（"静音栅栏"）

- 版本：V1.0
- 日期：2026-10-30
- 目的：解 `46-p12f2-status.md` 的 BLOCKED —— 方案 A
- 判据：与 F2 相同（30 帧内无花屏/错序，音画不脱节），但**必须 5/5 轮通过**
  （F2 是 3/5，间歇性失败不算过）

---

## 一、为什么是这条路

F2 的失败链已经量清楚：

```
output.stop() + start()  →  getTimestamp 成片失效（约 1.7 s）
                         →  节拍器判 kFreeRun（自生成时间轴）
                         →  真时间戳回来时整段判迟到
                         →  catch_up 连丢 91 帧（≈3 s）
```

三次"改锚点"的尝试（单次 / 二次 / 等时钟前进）都没解决，因为**病根在"停流"这个动作本身**，
不在锚的时机。方案 A 直接把病根去掉：**流不停，时间戳就不会断。**

---

## 二、必须先承认的一件事：`ring.reset()` 的线程契约

C2 定的规矩是"`reset()` 只能在两个线程都停下时调用"。方案 A 里流不停，
读者（AAudio 回调线程）仍在跑 —— 表面上违约。

所以要引入一个**显式的栅栏**，把"读者不碰环"从"线程停了"变成"读者被挡住了"：

```cpp
std::int32_t render(float*, std::int32_t n) override {
    if (muted_.load(acquire)) return 0;   // 直接返回 0，**一次都不碰环**
    return ring_.read(...);
}
```

`return 0` 的语义是"这次没数据"，由 `AaudioOutputSession` 补静音并计入 `frames_silenced`。
于是：

| 时刻 | 谁在碰环 |
| --- | --- |
| 栅栏前 | 读者读、写者写（正常） |
| 栅栏后 | **没人**：读者短路，写者已 join |
| 清环时 | 没人 —— 所以 `reset()` 合法 |
| 解除栅栏后 | 新数据，从头开始 |

**栅栏的落定时间**：`set_muted(true)` 之后必须等一个"在途回调"退出（它可能已经过了
判空那行）。K40 上 burst = 192 帧 ≈ 4 ms，取 **10 ms** 留 2.5 倍余量。
这段等待与清环时间一起计入 `prepare_us`，会出现在证据里，不藏着。

---

## 三、新顺序（与 F2 的差别只有两处）

```
ring_source.set_muted(true)     ← 新增：栅栏拉下
sleep(10ms)                     ← 新增：等在途回调退出
feeder.stop()                   ← 停写者
landing = extractor.seek_to(target)
decoder.flush()
ring.reset()                    ← 现在安全（读者被挡、写者已停）
feeder.start()                  ← 重灌
wait_filled(3 bursts)
ring_source.set_muted(false)    ← 解除栅栏（先放音）
clock.rebase(landing)           ← 再锚时钟（后对表）
```

**为什么"先放音、后锚表"**：放音之后第一个真实样本才是 `landing` 对应的音频；
先锚表会让时钟在静音期间继续前进，视频反而提前。

**与 F2 的两处差别**：`output_->stop()/start()` 消失；`rebase` 只锚一次（流没重启，
帧计数连续，不需要"锚两次吸收过渡"）。

---

## 四、判据

| 项 | 判据 | 比 F2 更严在哪 |
| --- | --- | --- |
| 稳定性 | **5/5 轮**通过 | F2 只跑 5 轮过 3 轮就收了，间歇性失败被当成噪声 |
| 无错序 | `frames_dropped_late = 0`、`catch_up_frames = 0`、`clock_mix.fallback_frames = 0` | F2 只看 `lateness`，被它骗了一次 |
| 落点 | `gap` ≤ 40 ms，视频 `total_us` < 150 ms | 同 F2 |
| 不静默 | 静音期的 `frames_silenced` 增量如实记入证据 | 新的一条：栅栏必然产生静音，不许藏 |

### 4.1 明确不做

- 不改 `AaudioOutputSession` 的接口（栅栏加在适配器上，不动会话）。
- 不做"淡入淡出"掩盖切换（那会掩盖问题）。
- 不做采样级精确 seek。

### 4.2 家族树

| 改动 | 位置 |
| --- | --- |
| `RingBufferSampleSource::set_muted()` + `muted_` | `backends/aaudio/buffer/ring_buffer_sample_source.hpp`（本就 header-only） |
| `AudioPlayback::seek_to` 改走栅栏 | `backends/avsync/session/audio_playback.cxx` |

**为什么栅栏放适配器而不是会话**：会话只认 `SampleSource` 接口，不该知道"环"的存在；
栅栏的语义是"这个数据源暂时不给数据"，正是适配器的职责。

**桌面单测做不到**：`SampleSource` 接口定义在 `<aaudio/AAudio.h>` 所在的头里，
桌面进程编不进来。这一条写进诚实清单，不用"假测试"充数。

---

## 五、风险

| 风险 | 处置 |
| --- | --- |
| 在途回调恰好在栅栏后读环 → 与 reset 竞争 | 栅栏后等 10 ms（2.5 个 burst）再 reset；这是"约定 + 时间余量"，不是锁 —— 写进诚实清单 |
| 静音期被记成欠载，看起来像故障 | 证据里单列"seek 引入的静音"，与稳态欠载分开 |
| 栅栏忘记解除 → 永久静音 | `seek_to` 单出口；解除放在所有失败路径之后统一执行 |
| 时钟不重启后帧计数是否连续 | **本轮实测确认**：若连续，一次 rebase 足够；若不连续，`clock_mix.fallback` 会立刻暴露 |
