# P12-F2A 验收：不重启流的音频 seek（静音栅栏）—— **达成，14/14 轮全绿**

- 版本：V1.0
- 日期：2026-10-30
- 设计：`47-p12f2a-design.md`（解 `46-p12f2-status.md` 的 BLOCKED）
- 判据：**与 F2 相同，但要求 5/5 轮通过**（F2 是 3/5，间歇性失败不算过）
- 结论：**达成。** 14 轮（含前向 / 后向 / 三个目标位置）全部
  `frames_dropped_late=0`、`clock_mix.fallback_frames=0`、`catch_up_frames=0`、`gap ≤ 8 ms`。

---

## 一、两处修改，各自解决一个不同的问题

| # | 改动 | 解决的问题 | 证据 |
| --- | --- | --- | --- |
| **A** | **不重启 AAudio 流**，改用"静音栅栏"（`render()` 直接返回 0，一次不碰环） | F2 的 2/5 轮 3 秒追赶：停流让 `getTimestamp` 成片失效 → 节拍器 `kFreeRun` | `p12f2a-fence-r1..r6`：全部 `fallback=0`、`catch=0` |
| **B** | **音频追视频的落点**（视频先做 `defer_start` 定位 → 音频按该落点定位 → 一起起跑） | 后向 seek 到 500 ms 时两边差一整个 GOP：视频落 0、音频落 490 ms ⇒ **gap 490 ms** | `p12f2a-av-back-r1..r3`：gap **0** |

B 是在 A 之后跑出来的：A 把"冻住 3 秒"消掉之后，后向 seek 的落点差才暴露出来 ——
**如果只跑 5000 ms 这一个目标，B 永远不会被发现**（该位置两边都落在关键帧上，差 8 ms）。

---

## 二、14 轮数据

| 组 | 目标 | 轮数 | 视频落点 | 音频落点 | gap | 丢帧 | fallback | catch_up | total |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| fence | 5000 ms | 6 | 5 000 000 | 4 992 000 | −8 ms | 0 | 0 | 0 | 27–39 ms |
| fence | 8000 ms | 2 | 8 000 000 | 8 000 000 | 0 | 0 | 0 | 0 | 30–39 ms |
| av | 5000 ms | 2 | 5 000 000 | 4 992 000 | −8 ms | 0 | 0 | 0 | 39–47 ms |
| av | 8000 ms | 1 | 8 000 000 | 8 000 000 | 0 | 0 | 0 | 0 | 22 ms |
| av | **500 ms（后向）** | 3 | **0** | **0** | **0** | 0 | 0 | 0 | 19–60 ms |

段内帧数也闭合：5000 ms 后剩 150 帧、8000 ms 后 60 帧、500 ms 后 300 帧，全部 `completed=true`。

**`superseded/`** 里放的是 B 改动之前的两轮后向 seek（gap 490 666 µs）—— 留着是因为
"音频按用户目标跳、视频按关键帧落"这个差异本身是一个反例，删掉就没人知道为什么要改成
"音频追视频落点"。

---

## 三、实现要点

### 3.1 静音栅栏（A）

```cpp
std::int32_t render(float* out, std::int32_t n) override {
    if (muted_.load(acquire)) return 0;   // 一次都不碰环
    return ring_.read(out, n);
}
```

顺序（`AudioPlayback::seek_to`）：

```
set_muted(true) → 等 10 ms（在途回调退出）→ feeder.stop()
→ extractor.seek_to → decoder.flush() → ring.reset()     ← 此刻真的没人碰环
→ feeder.start() → wait_filled(3 bursts)
→ set_muted(false)                                        ← 先放音
→ clock.rebase(landing)                                   ← 后对表
```

**"先放音后对表"**：放音之后第一个真实样本才是 `landing` 那一段；先锚表会让时钟在静音期
继续前进，视频反而提前。这是与 F2 的另一个实质差别（F2 先锚后放，没暴露问题只是因为它更早失败了）。

### 3.2 音频追视频落点（B）

```
session.seek_to(target, defer_start=true)   // 只停线程 + 定位 + flush，不起跑
landing = snapshot().seek.landing_us        // 线程已停，读 snapshot 安全
audio.seek_to(landing)                      // 音频按**视频落点**定位
session.resume_after_seek()                 // 一起起跑（与 start() 共用 begin_run()）
session.record_audio_seek(true, audio_landing)
```

`defer_start` 这个参数存在的唯一理由就是这条编排：**先拿到视频落点，再让音频对齐它**。
没有它就只能"音频先跳"，而 F2 已经证明那样视频会先起跑、吃到旧时钟。

---

## 四、代码

| 改动 | 位置 |
| --- | --- |
| 栅栏 `set_muted()` / `muted_` | `backends/aaudio/buffer/ring_buffer_sample_source.hpp`（header-only） |
| `AudioPlayback::seek_to` 改走栅栏（不 stop 流） | `backends/avsync/session/audio_playback.cxx` |
| `seek_to(target, defer_start)` + `resume_after_seek()` | `session/decode_session.{hpp,cxx}` |
| 编排改为"视频先定位、音频追落点、一起起跑" | `jni/jni_session_handle.cxx` |

桌面单测不变：`kivo_p12f1_seek_tests` 14 例 / 38 断言；`ctest -R kivo_p12` **9/9**。

---

## 五、诚实清单（六条）

1. **"花屏"仍然只能靠人眼。** 本卡证明的是 `0 丢帧 / 0 回退 / 0 追赶 / 落点一致 / 计数闭合`，
   **没有**自动检测花屏（那需要逐帧像素比对，本架构拿不到解码后的像素）。
2. **静音期没有单独计数。** 栅栏期间 `frames_silenced` 会涨（约 10–30 ms），
   但证据里只有总计数，**没有把"seek 引入的静音"与"稳态欠载"拆开**。这是下一步该补的字段。
3. **栅栏靠时间余量，不是同步原语。** `set_muted(true)` 后等 10 ms 让在途回调退出；
   理论上仍存在一个极窄的竞态窗口（回调已过判空、尚未返回）。14 轮未观测到，但**它是约定不是锁**。
4. **只有一台设备、一个片源**（K40 / 1080p30 H.264 + AAC 48k stereo）。
   4K、HEVC、44.1k 单声道都没测；采样率不同会改变 burst 大小，10 ms 余量是否够用未知。
5. **只测一次 seek 每次运行。** 连续拖动（每秒多次）没测；每次 seek 会重建解码线程，
   连续调用会反复创建销毁 —— 功能上安全，但**没有数字**。
6. **没有听感验证。** "静音期是否有可听爆音"、"新位置起播是否有咔哒声"都只能人耳判断，
   本轮没做。要补就得有人在机器边上听，并写进证据。

---

## 六、F 组进度与交接

| 卡 | 状态 |
| --- | --- |
| F1 | `DONE`（28–40 ms，G8 余量 4 倍） |
| **F2** | **`DONE`**（经 F2A 修复：14/14 轮零丢帧、gap ≤ 8 ms） |
| F3 错误路径全覆盖 | `TODO` —— 见下 |
| F4 证据总收口 | `TODO` —— 必须等 F3 |

**给 F3 的三条**：

1. 坏文件 / 加密轨 / 无音轨 / 无视频轨，**每一条都要有非空 reason**，且落在证据里（不是只打日志）。
   现在"无音轨"由 `AudioPlayback::open` 返回失败、`clock_source=system` 承担，但**没有专门的负向证据文件**。
2. **加密轨**：`MediaCodecVideoDecoder::submit` 里已有 `is_encrypted` 拒绝，但从未真机验证过
   —— 需要一个加密样本（或至少构造一个声称加密的轨）才能证明那条分支可达。
3. 复用 F2A 的 `assess_av_seek` 模式：**判据写成纯函数 + 桌面单测**，真机只提供数字。
