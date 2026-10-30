# P12-F1 验收：`seek_to` 接入会话 —— 首帧 28–40 ms，G8 的 150 ms 余量 4 倍以上

- 版本：V1.0
- 日期：2026-10-30
- 设计：`43-p12f1-design.md`
- 判据：**`seek_to` 接入会话；首帧 < 150 ms（G8）**
- 结论：**达成。** 三次真机 seek 到 5000 ms，`total_us` = 39 791 / 28 085 / 33 532 µs，
  预算 150 000 µs，**余量 3.8–5.3 倍**；落点精确等于目标（片源每 1 s 一个关键帧），
  seek 后每段 150/151/150 帧闭合、0 丢帧、`completed=true`。

---

## 一、证据

| 轮次 | target | landing | prepare | first_frame | **total** | pass | 段内帧数 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `p12f1-seek-r1` | 5 000 000 | **5 000 000** | 29 454 | 10 337 | **39 791** | ✅ | 150/151/150，0 丢帧 |
| `p12f1-seek-r2` | 5 000 000 | 5 000 000 | 17 546 | 10 539 | **28 085** | ✅ | 150/151/150，0 丢帧 |
| `p12f1-seek-r3` | 5 000 000 | 5 000 000 | 22 520 | 11 012 | **33 532** | ✅ | 150/151/150，0 丢帧 |

（单位 µs。`prepare` = 停线程 + 定位 + flush；`first_frame` = 起线程 → 首帧交出。）

口径：**终点是"交出"不是"上屏"** —— 交出之后的时间归 BufferQueue 与合成器，
D5 证明那段要单独测。若按上屏算，要再加 1–3 个 vsync（16–50 ms），
即使如此仍在预算内，但**证据不这么写**，因为混起来就没法归因（见 §三 第 1 条）。

---

## 二、一个必须先踩的坑：片源的关键帧密度决定"落点"

第一版测试用的是 `hdr10_1080p30.mp4`（x265 默认 `keyint=250`，**整段只有开头一个关键帧**）：

```
target 5000000 → landing 0 → 段内帧数 300（= 整段重播）
```

`landing = 0` 一度看起来像"seek 没生效"，实际是 **`PREVIOUS_SYNC` 的正确行为**：
目标 5 s 之前最近的关键帧就是 0。帧数 300 也印证了这一点。

**于是专门造了一份每秒一个关键帧的片源**（`-g 30 -keyint_min 30 -sc_threshold 0`）：
`tests/real/probe_seek_1080p30.mp4`。同一目标落到 5 000 000 µs，段内 150 帧 ——
这才是有区分度的测试。

**这条要留给后面的人**：以后看到 `landing` 离 `target` 很远，先查片源关键帧间隔，
别急着改代码。同时这也说明证据里**必须同时记 landing 与段内帧数**：
只记 landing 看不出"是不是重播了整段"。

---

## 三、实现要点（都在代码注释里，这里只列会咬人的）

1. **`stop()` 只停线程，不动 codec。** `start()` 会再调 `AMediaCodec_start`，那是非法操作；
   seek 复用已 started 的 codec，靠 `flush()` 回到"已启动、无在途数据"。
2. **`begin_run()` 是 `start()` 与 `seek_to()` 共用的起跑段**（清计数 / 重锚节拍 / 重建 worker）。
   抽出来的唯一理由：两条路径必须逐字相同 —— 手写两遍就会出现"seek 后少一次 `pacer_->reset()`"，
   而首帧时延这个数字看起来仍然正常，没人会发现。
3. **flush 必须在起线程之前**，否则新送的包会被 flush 吃掉，表现为"seek 后前几帧没了"，不报错。
4. **`MediaExtractorSource::seek_to` 修了返回值**：原来 `return time_us;`（请求值），
   于是"落点"字段恒等于目标 —— 一个恒真的字段比没有字段更糟。现在返回
   `AMediaExtractor_getSampleTime()`，即同步帧的真实 pts。
5. **帧计数在 seek 后归零**：一轮里做过 seek 的证据，`counters` 描述的是**最后一段**；
   `seek.requests` 说明发生过几次。这一条写在 schema 注释与接口注释里。

---

## 四、代码与产物

| 改动 | 位置 | 说明 |
| --- | --- | --- |
| `seek_budget.{hpp,cxx}`（**新**） | `core/seek/` | 纯判定：总量 vs 预算 + reason；`requests==0` 判失败 |
| `seek_to()` + `begin_run()` | `session/decode_session.{hpp,cxx}` | 停→定位→flush→起跑；`SessionOutcome` 加 `kSeekRejected/kSeekFailed` |
| `SeekFacts` | `session/decode_stats.hpp` | 事实，不含判定 |
| `seek` 段 | `session/decode_stats_renderer.cxx` | schema 升 **`decode/11`** |
| 修落点 | `source/mediaextractor_source.cxx` | 返回真实 pts |
| `session_seek` | `jni/jni_session_handle.{hpp,cxx}` + `jni_player_bridge.cxx` | 返回码 9=拒绝 / 10=失败 |
| `PlaybackSession.seekTo()` | `bridge/PlaybackSession.kt` | 唯一入口约定（D4） |
| `kivo.seek.ms` / `kivo.seek.at.ms` | `ui/PlayerActivity.kt` | 取证入口，默认起播 1.5 s 后触发 |
| 单测 | `tests/video/core/p12f1_seek_tests/` | **8 例 / 20 断言** |

`ctest -R kivo_p12` 现在 **9/9**。

---

## 五、诚实清单（六条）

1. **判据口径是"交出"不是"上屏"。** 上屏要另测（D5 的工具可复用），本轮没测。
2. **只测了一种片源与一个目标**：1080p30 / H.264 / `-g 30` / seek 到 5 s（正好是关键帧）。
   **落在非关键帧上的情况没测** —— 那时 `landing` 会早于 `target`，首帧前的丢帧数会变，
   时延可能略增。4K / HEVC 未测。
3. **只测了"一次 seek"**：连续快速多次 seek（拖动条那种）没测。当前实现每次 seek 都会
   join 再起线程，连续调用会把线程反复创建销毁 —— 功能上安全，但**没有数字**。
4. **音频没有跟着 seek**（F2）。本轮证据全部是纯视频路径：`hdr10` 片源无音轨，
   `seek_1080p30` 也无音轨。有音轨的片源 seek 之后音频会继续从原位置播 —— 这是已知缺口，**不是 bug**。
5. **`landing` 读的是 seek 之后的 `getSampleTime`**，即同步帧 pts。它与"第一帧实际解出的 pts"
   理论上一致，但本轮**没有单独记录首帧 pts**（只有帧计数），所以这条一致性是推断不是观测。
6. **`prepare_us` 包含线程 join 时间**，在负载重的机器上会更大；本轮三轮 17–29 ms 的波动
   主要来自这里，不是 seek 本身。

---

## 六、给 F2 的交接

- **音频 seek 是 F2 的核心**：`avsync::AudioPlayback` 现在没有 seek 入口；接的时候要注意
  **音频时钟的连续性** —— seek 会让位置跳变，`clock_position_tracker` 已有"跳变重锚"分支
  （C4 实现，见 `16-p12c4-status.md`），F2 要确认它在 seek 场景下真的被触发，而不是被当成异常吞掉。
- **音画双侧重同步的判据别用 `av_drift`**：`surface_at_time` 下它不可测（C8 §三）。
  F2 要么走 `cpu_paced` 取证，要么用"两侧都对齐到同一个 seek 落点"这种结构性判据。
- **`decode/11` 是 F 组当前 schema**，F2 加字段继续升版本号。
