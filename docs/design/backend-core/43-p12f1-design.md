# P12-F1 设计：`seek_to` 接入会话（G8 首帧 < 150 ms）

- 版本：V1.0
- 日期：2026-10-30
- 卡片判据：**`seek_to` 接入会话；首帧 < 150 ms（G8）**
- 关联：`42-p12e4-status.md` §七（E 组给 F 组的交接）

---

## 一、先把"首帧"定义死，否则 150 ms 可以被量成任何数

| 口径 | 起点 | 终点 | 典型值 |
| --- | --- | --- | --- |
| **本卡采用** | 进入 `seek_to()` 的第一行 | 首帧被 `releaseOutputBuffer*` **交回平台** | 待测 |
| 更严格（上屏） | 同上 | 该帧被合成器**锁存**（D5 的 `actual` 列） | +1–3 个 vsync（16–50 ms） |

**为什么本卡用"交出时刻"**：交出之后的时间不归我们管（BufferQueue + 合成器），
D5 已经证明那段只能用 `dumpsys SurfaceFlinger --latency` 单独测。若把两段混成一个数，
下一次有人优化合成器时，这里的数字会莫名其妙地变。

**所以 150 ms 必须按"交出"口径读**，且证据里写死这个定义。上屏口径要另测（F4 收口时再谈）。

---

## 二、三个必须先承认的事实（都是前面阶段量出来的）

1. **`stop()` 会清零帧计数、重锚节拍**（`start()` 里的 `counters_.reset()` + `pacer_->reset()`）。
   这不是副作用，而是 seek 需要的语义：新位置就是新时间轴。
2. **`AMediaCodec_start` 不能重复调用**：所以 seek **不能**简单用 `start()` 重新起；
   必须把"起线程"与"start codec"拆开（§三）。
3. **`PREVIOUS_SYNC` 落点 ≤ 目标时刻**：`AMediaExtractor_seekTo(..., PREVIOUS_SYNC)`
   会落到目标前的同步帧。**这是有意的**：从同步帧开始解码才能立刻出图；
   落到目标之后意味着要丢一批帧才能到目标，反而更慢。

> **顺带修一个旧 bug**：`MediaExtractorSource::seek_to` 现在 `return time_us;` ——
> 返回的是**请求值**，但头文件注释写的是"返回实际落点"。F1 改成 seek 之后读
> `AMediaExtractor_getSampleTime()`。不改的话，证据里"落点"那一栏永远等于目标，
> 一个恒真的字段比没有字段更糟（E1/E2 反复踩过的同一类问题）。

---

## 三、做法：停线程 → 定位 → flush → 起线程

```
seek_to(target):
  t0 = now()
  stop()                       // 只停工作线程；codec 保持 started
  landing = extractor.seek_to(target)      // PREVIOUS_SYNC；返回实际落点
  decoder.flush()                          // 清在途输入/输出，codec 仍是 started
  t1 = now()                               // prepare_us = t1 - t0
  begin_run()                              // 清计数 / 重锚节拍 / 重建 worker / 起线程
  → 首帧交出时 counters.first_frame_latency_us 记下（沿用 C1 的既有计数）
  total = prepare_us + first_frame_us
```

**为什么是"停-起"而不是"在线 seek"**：在线做法要在 merged 循环里插一个 seek 分支，
而"停-起"复用的是已经过 C/D 两组验证的路径（`begin_run()` 就是 `start()` 去掉
`decoder_.start()` 之后的那段）。代价是线程创建（实测几十微秒），换来的是
"seek 之后的所有不变量与起播时完全一致"。

**为什么 flush 必须在起线程之前**：反过来的话，送包线程可能往一个还没清干净的
输入队列里塞数据，那些包会被 flush 丢掉 —— 表现是"seek 后前几帧没了"，不报错。

---

## 四、判据与落点

### 4.1 判据

| 项 | 判据 |
| --- | --- |
| 首帧 | 3 次 seek，每次 `total_us < 150000` |
| 落点正确 | `landing_us >= 0` 且 `landing_us <= target_us`（PREVIOUS_SYNC 的语义） |
| 不静默 | 达不到时 `reason` 非空；没发生 seek 时也必须有 reason（"没有 seek 过"≠"达标"） |
| 不变式 | seek 之后每段仍是 `samples_submitted == frames_rendered`、`rejected=0`、`completed=true` |
| 可复算 | 证据里有 target / landing / prepare / first_frame / total / budget / pass / reason 八项 |

### 4.2 明确不做

- **音频 seek 与音画重同步**（F2）。本卡只做视频会话定位；音频 feeder 没有 seek 入口，
  强行接会得到一个"视频跳了、音频没跳"的假同步证据。
- **精确 seek（到帧而非同步帧）**：`SEEK_CLOSEST` 会牺牲出图速度，本卡的判据是时间不是精度。
- **seek 后重建 Surface**（D1 已证不需要）。
- **播放器 UI 的 seek 条**：本阶段只做后端与取证入口。

### 4.3 家族树

| 新增/改动 | 位置 | 理由 |
| --- | --- | --- |
| `seek_budget.{hpp,cxx}`（**新**） | `core/seek/` | 纯判定（总量 vs 预算 + reason），零平台依赖，桌面可单测 —— 与 `core/pace`、`core/pipeline` 同族 |
| `mediaextractor_source.cxx` | 修 `seek_to` 返回值 | 实际落点只有它知道 |
| `session/decode_session.{hpp,cxx}` | 拆 `begin_run()`；新增 `seek_to()` | 只有会话知道线程与 codec 的生命周期 |
| `session/decode_stats.hpp` | 加 `SeekFacts seek` | 事实结构来自 `core/seek` |
| `session/decode_stats_renderer.cxx` | `seek` 段，schema 升 `/11` | 证据出口 |
| JNI `nativeSessionSeek` | `android/app/src/main/cpp/jni/` | 与其它 `nativeSession*` 同族 |
| `PlaybackSession.seekTo()` | `android/.../bridge/` | 唯一入口的既有约定（D4） |
| `kivo.seek.ms` / `kivo.seek.at.ms` | `ui/PlayerActivity.kt` | 取证入口，与 `kivo.pace` 等诊断 extra 同族 |
| 单测 | `backend/tests/video/core/p12f1_seek_tests/` | 只测 `assess_seek()` 的判定分支 |

---

## 五、风险

| 风险 | 处置 |
| --- | --- |
| `stop()` 里 `receive_worker_->finish()` 会写最终墙钟，把 seek 前的段也写进去 | seek 只在"段末"调用；证据里 `seek.requests` 与 counters 都按**最后一段**解读，文档写死 |
| flush 之后 codec 需要重新收关键帧 | PREVIOUS_SYNC 正好停在关键帧上，送包从那里开始 |
| 节拍器重锚导致首帧被判"迟到" | `begin_run()` 里 `pacer_->reset()`，与起播同一条路径 |
| 落点等于目标但实际 pts 更大 | 证据里同时记 `landing_us` 与首帧 pts，两者不一致时人能看出来 |
| 从工作线程误调 seek | 接口注释写死"控制线程调用"；实现里用 `started_` 与 join 做保护（重复调用会拒绝） |
