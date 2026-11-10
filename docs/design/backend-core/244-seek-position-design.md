# 244 · 视频无音轨 seek 后进度不跳（位置时钟未 rebase）

> 状态：**CLOSED**（2026-11-10，用户确认关闭）。实施已落地、平板真机复验 4/4 PASS、音频路径零回归。
> 证据见 `docs/evidence/p244/status.md` 与 `seek-after-fix.txt` /
> `seek-before-fix-tightened.txt` / `seek-audio-regression.txt`。
> 需求来源：244 卡剩余项 #1「`lastSeekLandingUs` 与实际读位置不一致」。
>
> 关闭依据（四项同时成立）：
> 1. 判据**能变红**——收紧后修复前 `actual=5129 → FAIL`，两次复现一致；
> 2. 判据**能转绿**——修复后 `actual=27080 → 4/4 PASS`；
> 3. **对照无回归**——`mkv-audio.mkv` / `h264-audio.mp4` 均 4/4 PASS、仍为 `pace=audio_timestamp`；
> 4. **改动面**仅视频-only seek 读数，三处文件职责各一，无跨层扩散。
>
> 遗留项（**不阻塞关闭**，见 `status.md` 末节）：
> - **K40 复验已完成**（2026-11-10 07:00）：`actual=27106 → 4/4 PASS`，`RESUME 27106 → 30106`。
>   但两台 `ro.board.platform` 都是 `kona`（高通 SM8250 家族）、同一解码器 `OMX.qcom.video.decoder.avc`，
>   所以这是**跨设备 + 跨 ROM + 跨系统版本**的复现，**不是跨 SoC 厂商**；跨厂商证据仍缺。
> - **SMB 链路已验**（2026-11-10 07:02）：共享只是从 `XiaoMi` 改名为 `视频`，服务一直是好的。
>   同一份文件上传后经 SMB 走 `RangeFd`：无音轨 `actual=27012`、带音轨 `10498`，均 4/4 PASS。
>   原先担心的 `asyncInput=true` 落点差异未出现——修复挂在共用 `session_seek` 上，与输入方式无关。
> - 唯一剩余：跨 SoC 厂商复验（需非高通设备），以及共享上 `N:/kivo-test/`、`smbprobe.txt` 待手工清理。

## 一、现象（真机，可复算）

平板 MatePad `CFE6R21B12000179`，`SmbPlaybackSeekInstrumentation -e localPath local-seek.mkv`
（H.264 + MKV + **无音轨**，60 s）：

```
P243 START   codec=OMX.qcom.video.decoder.avc dur=60000000 audio=false
PASS 起播
P243 POSITION 0 ms → 3006 ms
PASS 位置推进
P243 SEEK    target=30000 ms landing=25000 ms actual=5129 ms
FAIL seek 未生效：target=30000 ms landing=25000 ms actual=5129 ms（允许 ±5 s）
```

`logcat` 的 `positionUs` 轨迹是**墙钟线性**、seek 前后完全连续：

```
11.406  session seek ok: target=30000000 us
11.443  positionUs rc=3066749     ← seek 后 37ms，位置≈3.07s（并未跳 25s）
11.546  rc=3167403
...
13.594  rc=5229624                ← 5.23s，按墙钟匀速前进
```

即：demux 落点 25 s 是真的（`FfmpegDemuxSource::seek_to` 读回关键帧 pts），
**但 `positionUs` 从不跳到 25 s，而是按墙钟从起点匀速走**。

## 二、根因（代码级）

1. 视频无音轨 → `session_attach_pacing`（`jni_session_handle.cxx:376-398`）在没有音频时钟时
   创建 `SystemClockSource` 并作为 `setup.clock` 传下去；证据里 `pace=system` 即此。
2. `SystemClockSource::now()` 的位置 = `steady_now_ns() - origin_ns_`
   （`system_clock_source.cxx:19`）——它是**墙钟绝对量**，不是媒体 pts。
3. seek 时 `DecodeSession::seek_to` → `resume_after_seek` → `begin_run` → `pacer_->reset()`
   （`decode_session.cxx:417-419`）。`FramePacer::reset()` 只复位 `ClockPositionTracker`
   （`position_ns_=0`、`anchored_=false`），**从不碰 `SystemClockSource` 的 `origin_ns_`**。
4. 于是 seek 后的第一次 `read()`（`clock_position_tracker.cxx:89-97`）又执行
   `position_ns_ = snapshot.position_ns` = `now - origin`（≈ seek 前的墙钟值 ~3 s），
   之后 `position_ns_ = snapshot.position_ns + offset` 继续按墙钟线性外推。**位置因此永远不反映 landing。**
5. 对比音频路径：`AudioPlayback::seek_to` 明确调了 `clock->rebase(landing * 1'000)`
   （`audio_playback.cxx:289`），所以音频 seek 位置会跳。**系统时钟没有这一步 → 非对称缺口。**

`audio_clock_source.hpp:24-31` 的注释原文：

> seek 之后重设时钟基准（P12-F2）。**默认空实现是刻意的**：`SystemClockSource` 的位置来自墙钟，
> 没有"媒体基准"可重设；让它假装能 rebase 只会制造一条永远不对的分支。

**这条前提在 seek 场景下不成立**：seek 之后我们**恰恰有**媒体基准——demux 返回的 landing。
当时没有"视频无音轨也要能 seek 到正确进度"的诉求，所以留成了空实现；244 把它暴露出来。

> 帧本身是跳对了的：`pacer` 的 `due_at` 用相对 pts 增量（`frame_pacer.cxx:85-93`），
> `reset()` 后首帧重锚，25 s 关键帧不会冻结、按 30 fps 正常出（seek 后 `frames=67/2.2s`、`dropped=0`）。
> 坏的只有 **`position_us` 这个读数**（进度条 / 证据里的位置）。

## 三、修复方案（最小面，单文件单职责）

改三处，职责各一：

| 文件 | 改动 | 职责 |
| --- | --- | --- |
| `core/audio/clock/system_clock_source.hpp/.cxx` | 新增 `rebase(media_position_ns)` override：`origin_ns_ = steady_now_ns() - media_position_ns` | 让系统钟能按**媒体基准**重设原点（与 `AudioClockSource::rebase` 契约一致） |
| `jni/jni_session_handle.cxx` `session_seek` | 拿到 `video_landing` 后、`resume_after_seek()` 前：`if (handle->system_clock) handle->system_clock->rebase(video_landing * 1'000)` | 视频-only seek 时把系统钟原点挪到 landing |
| `core/audio/clock/audio_clock_source.hpp` 注释 | 更新 §24-31 说明：seek 场景**有**媒体基准，系统钟必须 rebase | 消除"注释与实现两个真相" |

**为什么不碰音频路径**：`system_clock` 只在无音频时钟时创建（`jni_session_handle.cxx:381`），
有音频时为 `nullptr`；`if (handle->system_clock)` 天然只命中视频-only，音频钟的 rebase 仍由
`AudioPlayback::seek_to` 独占，互不重复。

**为什么 rebase 放在 `resume_after_seek()` 之前**：`begin_run()` 会 `pacer_->reset()` 并起线程，
首帧立即读钟；先 rebase 才能保证首帧读到的是 `landing`，而不是墙钟旧值（避免"先显示旧位置再跳"的抖动）。

## 四、验证（判据必须能变红）——已全部完成

| 项 | 预期 | 实测 | 证据 |
| --- | --- | --- | --- |
| 判据能变红（修复前） | `FAIL` | ✅ `actual=5129 ms`，06:45 / 06:48 两次复现一致 | `seek-before-fix-tightened.txt` |
| 判据能转绿（修复后） | `actual ∈ 25000 ± 5000 ms` | ✅ `actual=27080 ms` → **4/4 PASS** | `seek-after-fix.txt` |
| 音频对照无回归 | 行为不变、仍 `audio_timestamp` | ✅ `mkv-audio.mkv` 10010/8333/10480；`h264-audio.mp4` 10000/8333/10467，均 4/4 PASS | `seek-audio-regression.txt` |
| `RESUME` 从落点续播 | 从 25 s 起继续 | ✅ `27080 → 30151 ms` | `seek-after-fix.txt` |

`27080` 而非精确 `25000`：落点是 25 s 关键帧，测试读位置前有 2 s 等待窗口，窗口内按 30 fps 正常推进，
落在 ±5 s 判据内即为「真的跳过去了」——这正是收紧判据要表达的语义。

## 五、家族树 / 影响面

- 只影响 **视频-only** 会话的 seek 进度读数；音频会话、普通播放路径零改动。
- `ClockPositionTracker` / `FramePacer` 的漂移、节拍、丢帧逻辑不改——它们读的是同一个钟，
  rebase 只挪原点，`elapsed`、`anchor` 自洽。

## 六、与 241 契约的追溯关系（为什么起播那半不是"新 bug"）

244 的两半症状，一半是**上游契约的落地缺口**，一半是新发现。分清楚才不会修完又复发：

| 症状 | 归属 | 依据 |
| --- | --- | --- |
| 起播黑屏 / 位置恒 0 | **241 §四.2 契约的落地缺口** | 241 已写死「csd 与 sample 必须是同一种封装」，§四.1 又规定 csd 按 AnnexB 给 → 每种编码都必须挂对应的 `*_mp4toannexb`。首版只挂了 HEVC，H.264 漏掉 |
| seek 后进度不跳 | **本卡新发现**（241 未涉及） | 241 §四.3 只规定 seek 返回实际落点，没规定"落点如何喂回位置时钟"；`SystemClockSource` 缺 `rebase` 属时钟层缺口 |

已回写 241 §四.2 一条落地缺口说明。**教训**：契约写"必须配套"不够，
要按 `codec_id` 穷举、并让 `default` 分支**显式失败**而不是静默——否则漏一种编码就是全黑。

> 注：本条不是追究首版实现的责任划分，而是为了让同类缺陷下次能被契约本身拦住。
