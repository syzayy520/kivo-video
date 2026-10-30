# P12-C6 · 送收分线程流水线 · 设计与家族树

- 版本：V1.0（设计定稿，实现前）
- 日期：2026-10-29
- 上游：`11-p12b-status.md` §四（瓶颈判定）、`17-p12c5-design.md`、`18-p12c5-status.md`（C5 欠的债）
- 下游：`20-p12c6-status.md`（验收）、C7（`releaseOutputBufferAtTime`）

---

## 一、目标与边界

**目标（只有一条）**：把「送包」与「收帧」拆到两个线程，让解码器在呈现侧等待时**不停工**。

这条卡的起点不是"多线程更快"这种直觉，而是 P12-B 的一处硬测量：

| 片源 | 像素量 | 帧数 | fps | 每帧耗时 |
| --- | --- | --- | --- | --- |
| H.264 1080p 8bit | 1× | 180/180 | 169.8 | 5.89 ms |
| HEVC 1080p 10bit | 1× | 180/180 | 175.3 | 5.70 ms |
| HEVC 4K 10bit | **4×** | 120/120 | 127.4 | 7.85 ms |

4K 的像素是 1080p 的 4 倍，每帧却只慢 1.37 倍 → **2ms/帧 随像素增长，5.2ms/帧 与像素无关**。
与像素无关的那 5.2ms，只能来自每帧固定发生的往返：`dequeueInputBuffer` / `queueInputBuffer` /
`dequeueOutputBuffer` / `releaseOutputBuffer` 这四次调用与它们之间的串行关系。

**明确不做**（做了就越界，会让后续卡片无处落位）：

| 不做 | 因为 |
| --- | --- |
| 改 `due_at` 公式或节拍阈值 | C5 已验收，改一个数就让 G6 的历史证据失效 |
| `releaseOutputBufferAtTime` | C7 的卡 |
| A/V 偏差统计 | C8 的卡 |
| 自建帧队列（`core/frame/`） | 平台输出槽本身就是队列，见 §五 |
| 改解码器选型 / 改批量大小掩盖问题 | `18-p12c5-status.md` §七 明令禁止 |
| 音频链路 | C2–C4 已闭环，C6 一行不碰 |

---

## 二、拆成哪两段

```
                 ┌──────────────────────── submit 线程 ────────────────────────┐
  AMediaExtractor│  read_next_sample → submit(0) → [输入满] 等 room → 重送同一包  │
      (独占)     └──────────────────────────────────────────────────────────────┘
                                        │  平台输入队列（深度由 codec 定）
                                        ▼
                                  ┌───────────┐
                                  │ MediaCodec│
                                  └───────────┘
                                        │  平台输出槽（深度由 codec 定）
                 ┌──────────────────────── receive 线程 ───────────────────────┐
                 │  receive(0) → FramePacer::arrive(pts) → release(render) → 通知 room │
                 └──────────────────────────────────────────────────────────────┘
```

两段各自是一个**只做一件事**的循环：

| 阶段 | 唯一职责 | 独占的资源 |
| --- | --- | --- |
| submit | 把压缩包送进解码器，读完发 EOS | `MediaExtractorSource`（非线程安全，见其头注释） |
| receive | 把解好的帧按节拍交出去或丢掉 | `FramePacer`（非线程安全，见其头注释） |

`DecodeSession` 退化为**编排者**：open / start / stop / snapshot，不再包含任何循环体。

### 2.1 一段一轮，循环在外（两种模式共用同一份 worker）

两个 worker 都只暴露 `pump(allow_wait) → StageStep{kProgress, kIdle, kDone}`：

| 模式 | 谁在循环 |
| --- | --- |
| `split` | 每个 worker 各自的线程里 `run()` 循环 |
| `merged` | 编排者单线程：连送一批（≤ 8 包，与 P12-B 的 `kMaxSubmitBatch` 同值）→ 收一帧 → 再连送 |

这样"拆线程"与"不拆线程"跑的是**同一份判定逻辑**，唯一的差别是线程数。
把批大小留在 merged 一侧是公平性要求：合并模式若少了批量灌包这一层，
拆线程的收益就会被虚报（那部分收益 P12-B 已经吃过一次了）。

### 2.1 为什么节拍必须在 receive 段，不能留在"中间"

C5 的结构是 `drain → submit 批量 → drain`，节拍器的 `arrive()`（内含睡到 `due_at`）
夹在同一个循环里。30fps 时每帧睡 33ms，60fps 时 16ms —— 这段时间解码器拿不到新包。

把 `arrive()` 移到 receive 段之后：睡眠发生在**平台调用之间**，submit 段照常灌包，
解码器在"该睡的那 16ms"里继续解码。这是本卡全部的收益来源，也是为什么拆完必须用
**no-pace 全速档**去量吞吐（开了节拍 fps 被源帧率钉死，量不出任何东西）。

---

## 三、线程模型与所有权（这一节是安全性的全部依据）

### 3.1 平台调用的单写者约束

`AMediaCodec` 的线程安全性**没有任何官方文档承诺**。所以不赌"平台内部有锁"，而是自己定规矩：

| 平台函数 | 唯一调用者 |
| --- | --- |
| `AMediaCodec_dequeueInputBuffer` / `queueInputBuffer`（经 `submit`、`signal_end_of_stream`） | submit 线程 |
| `AMediaCodec_dequeueOutputBuffer` / `releaseOutputBuffer`（经 `receive`、`release_frame`） | receive 线程 |
| `AMediaCodec_start` / `stop` / `delete` / `flush` | 调用 `start()`/`stop()` 的线程，**且只在两个 worker 都已 join 之后** |
| `AMediaExtractor_*` | submit 线程 |

**不设全局限调用锁。** 设了就等于把两段重新串行化，本卡直接白做；而且锁的持有期会覆盖
`releaseOutputBuffer`（最慢的那一次调用），正是我们要重叠掉的部分。

这条约束之所以成立，是因为 AOSP 的 `ACodec` 把输入与输出放在**两个独立的 reply 通道**上
（`kWhatDequeueInputBuffer` / `kWhatDequeueOutputBuffer`），二者互不占位。
它是实现事实，不是文档承诺，所以 §八 专门有一条"平台若不许并发会怎么炸"的可证伪点。

### 3.2 回退模式：为什么保留单线程

平台并发这条线只在 K40 上验证过。为它保留一个"两段跑在同一线程"的模式，
成本只有编排层的十几行分支（worker 本身不区分模式），收益是：

1. 真机上任何一次异常（`dequeueOutputBuffer` 返回未预期负值、帧数不闭合）都有退路；
2. **A/B 取证必须在同一个二进制上做**（见 §六），编译期开关做不到"交替跑"。

模式只影响 `start()` 起几个线程，不影响任何判定逻辑 —— 这也是它能活下来的原因。

### 3.3 停止与 EOS 的时序

```
stop()：request_stop(写标志 + notify_all) → join(submit) → join(receive) → 计算 wall → 采最后一次节拍
```

- 两个 worker 的等待全部有界：等空位 500µs、平台收帧阻塞 2000µs、
  节拍器的长睡按 250ms 切块且每块检查停止标志（C5 已有），所以 join 不会卡住不动；
- `FramePacer` 的 `stop_flag` 指向闸门里的同一个原子量 —— 少一个停止量就少一处要同步的状态；
- 自然 EOS：submit 段送完 EOS 包即退出；receive 段收到带 EOS 标志的帧后退出并置 `completed`。

**join 顺序固定为 submit 在前。** 反过来的话，submit 可能正卡在"等 room"上，
而 room 的唯一来源（receive 的 release）已经退出了 —— 靠超时也能醒，但那是运气，不是顺序。

---

## 四、闸门：一个停止标志 + 一个条件变量

跨线程只需要传三件事，多一件都是耦合：

| 信号 | 谁发 | 谁等 | 语义 |
| --- | --- | --- | --- |
| `input_room` | receive（每次 `release_frame` 之后） | submit（送包返回 `TRY_AGAIN` 之后） | 输出槽腾出来了，输入槽大概率也有了空位 |
| `submit_finished` | submit（EOS 包送出之后） | receive（收不到帧时） | 不会再有大包了 —— 用它把"还没解出来"和"解码器卡死"分开 |
| `stop` | 任一方失败 / `stop()` | 双方 | 一起退出 |

**为什么只有"输入空位"一个条件变量，没有"输出可能有帧"这个信号：**
收帧段收不到帧时直接在平台上阻塞（`dequeueOutputBuffer(timeout=2ms)`），
平台自己就是最准的通知源。再加一层我们的信号，就多一条会丢的路径，
而它一次也帮不上忙 —— 平台从"没帧"变"有帧"的那一刻，它比我们先知道。

等待用 `wait_for` 而不是 `wait`：通知可能早于等待到达（送包先 `TRY_AGAIN`、
收帧后 `release` 这一对在时序上完全可以反过来），丢一次通知就死等是最难查的一类 bug。
超时（500µs）兜底，所以即使通知丢了也只是多一次重试。

---

## 五、背压：不建队列

送包段的背压由**平台输入队列**提供（`TRY_AGAIN` = 满），收帧段的背压由**平台输出槽**提供
（槽被占满时解码器自己停止吐帧，进而输入队列不再被消费）。

所以整条流水线的在途深度 = 输入槽数 + 输出槽数（K40 上约 8 + 8 帧），
由平台决定，不由我们猜。**不引入 `core/frame/` 的中间队列**，理由：

1. 多一层队列就多一层"帧的归属"问题（谁归还、何时归还），而现在的归属是"谁 `receive` 谁 `release`"；
2. 队列深度由我们写死，就等于替平台决定了它自己的缓冲策略 —— P12-B 已经证明这决定不了；
3. 真要更深，调 `AMediaFormat` 的 `max-output-buffer-count` 比自建队列诚实。

代价是：在途帧数不受我们控制，`first_frame_latency` 的构成里会混入平台缓冲深度。
这一点在证据里如实写，不解释成"我们的延迟"。

---

## 六、测量口径（这一节决定数字能不能被信）

### 6.1 必须同二进制 A/B

`docs/evidence/p12b/*.json` 是**另一个二进制**测的。跨二进制比较已经被证明不可靠：
同一个 1080p 源，P12-B 是 169.8 fps，C5 的 no-pace 对照组是 177.6 fps —— **差 4.6%**，
而本卡预期的提升量级是 10–30%。用 4.6% 的噪声去论证 15% 的提升，等于没证。

所以取证方式固定为：

```
同一 APK，同一片源，交替跑 merged / split / merged / split …
```

- `merged`：两段在同一线程里交替泵（等价 C5 结构，且共用同一份 worker 代码）
- `split`：两段各一个线程

交替而不是"先全跑 merged 再全跑 split"，是为了让**热漂移**（K40 连续硬解会降频）
落在这两种模式上等量。

### 6.2 判据

| 项 | 判据 | 说明 |
| --- | --- | --- |
| 4K 提升 | `gain(4K) ≥ gain(1080p)` | 卡片原文；增益必须随像素走，否则说明拆错了地方 |
| 正确性 | `samples_submitted == frames_received == frames_rendered == 轨道总帧数` | P12-B 的不变量，一条不许破 |
| 缓冲归属 | `frames_released == frames_received` | 新加：漏一个 release 就是漏一个输出槽 |
| 稳定性 | 6 轮 `error == ""`、`completed == true` | 平台并发若不被允许，会在这里炸 |

`gain` 定义为 `(fps_split − fps_merged) / fps_merged`，分子分母都取同片源两轮的中位数。

### 6.3 阶段耗时计量

只报"快了 X%"无法解释原因，所以每次平台调用都记 `(次数, 累计微秒, 峰值微秒)`：
`submit` / `receive` / `release` 三段各一组。

它同时是**判断"该不该继续拆"的唯一依据**：若 `release` 一段就占了每帧 4ms，
那么把它和 submit 重叠几乎不可能有收益，那时应当直接报"无改善"并保留 merged（卡片允许）。

---

## 七、验收与回退判据

**验收需要同时满足**：

1. 三种片源（H.264 1080p8 / HEVC 1080p10 / HEVC 4K10）各有 merged 与 split 各 2 轮数据；
2. `gain(4K) ≥ gain(1080p)`；
3. 正确性三条不变量全绿；
4. C5 的三档节拍回归（24/30/60fps）**丢帧仍为 0**、`p99` 不劣化。

**回退判据（写死，不许临场放宽）**：
若 `gain(4K) < gain(1080p)`，说明拆的地方不是瓶颈所在 —— **回退到 merged 作为默认模式**，
把 `split` 保留为可选实验路径，并在状态文档里写明"拆了但没用在瓶颈上"。

若 6 轮里出现任何一次 `dequeueOutputBuffer` 的未预期负值或帧数不闭合，同样回退，
且状态文档必须写明"平台不允许输入/输出并发"这一结论 —— 那是比性能更重要的发现。

---

## 八、可证伪点

设计如果错了，下面任一条会先炸：

1. **`gain` 接近 0 且 `release` 耗时占大头** → 瓶颈在输出侧，与送包无关，拆错地方。
2. **4K 增益小于 1080p** → 卡片的原始判据，说明被优化掉的是固定开销里与像素无关的那部分
   之外的东西。
3. **`frames_released != frames_received`** → 有输出槽没归还，几十帧后解码器会彻底停住
   （表现为 `completed=false` + 卡在某个帧数）。
4. **`dequeueOutputBuffer` 返回非 `TRY_AGAIN`/非 `INFO_*` 的负值** → 平台不接受并发，
   立刻回退 merged。
5. **split 模式下 `frames_dropped_late` 显著高于 merged**（同节拍档）→ 说明节拍器拿到的
   pts 序列被两段交错搞乱了，需要回查 receive 段是否真的只有单写者。

任何一条成立 → 停手回查，不许"多跑几轮看看"。

---

## 九、家族树

```
backend/
├── include_private/video/video_plane/core/pipeline/
│   ├── submit_types.hpp              # 送包阶段的输入/回执/步骤（纯数据）
│   └── submit_sequencer.hpp          # 送包时序状态机（纯逻辑，零平台）
├── src/video/video_plane/core/pipeline/
│   ├── submit_types.cxx              # 枚举 → 可读名（进证据/日志）
│   └── submit_sequencer.cxx
├── include_private/video/video_plane/backends/mediacodec/session/
│   ├── decode_pipeline_gate.hpp      # 停止标志 + 输入空位信号 + 送包段收工标志
│   ├── decode_stage_step.hpp         # 一段一轮的结果（两段与编排者共用的词汇）
│   ├── decode_pipeline_counters.hpp  # 热路径原子计数（纯数据，无 cxx）
│   ├── decode_stage_cost.hpp         # 单段平台调用耗时（次数/累计/峰值）
│   ├── decode_failure_slot.hpp       # 失败原因槽：首次写入为准
│   ├── decode_pace_sampler.hpp       # 节拍快照的跨线程发布
│   ├── decode_submit_worker.hpp      # 送包线程体
│   ├── decode_receive_worker.hpp     # 收帧线程体
│   └── decode_session.hpp            # 退化为编排者
├── src/video/video_plane/backends/mediacodec/session/
│   ├── decode_pipeline_gate.cxx
│   ├── decode_stage_cost.cxx
│   ├── decode_failure_slot.cxx
│   ├── decode_pace_sampler.cxx
│   ├── decode_submit_worker.cxx
│   ├── decode_receive_worker.cxx
│   └── decode_session.cxx            # 只剩 open/start/stop/snapshot 与模式分支
└── tests/video/pipeline/p12c6_pipeline_tests/
    └── submit_sequencer_tests.cpp    # C6-1 … C6-9
```

### 单文件职责（一句一文件）

| 文件 | 做 | **不做** |
| --- | --- | --- |
| `submit_sequencer` | 由"读到什么/送出去没有"推出下一步该干什么 | 不读文件、不碰解码器、不认识 `MediaCodec` |
| `decode_pipeline_gate` | 停止标志 + 输入空位信号 + 送包段收工标志 | 不持有解码器、不做判定、不碰任何帧数据 |
| `decode_pipeline_counters` | 原子计数 | 不做统计（p99 是 `decode_pace_sampler` 的事） |
| `decode_stage_step` | 定义"一段一轮的结果"这三个词的语义 | 不含任何状态 |
| `decode_stage_cost` | 记一段平台调用的次数/累计/峰值 | 不跨段汇总 |
| `decode_failure_slot` | 记住第一个失败原因 | 不决定"要不要停"（那是会话层） |
| `decode_pace_sampler` | 把节拍器摘要锁着发布出去 | 不改节拍器、不算分位数 |
| `decode_submit_worker` | 送包循环 | **不碰** `receive` / `release` |
| `decode_receive_worker` | 收帧 + 节拍 + 释放 | **不碰** extractor / `submit` |
| `decode_session` | 编排生命周期与模式 | 不含任何循环体 |

### 依赖方向（单向，不许回指）

```
core/pipeline/submit_sequencer   ← 零依赖（只用 cstdint）
        ▲
decode_submit_worker ──> decode_pipeline_gate / counters / stage_cost / failure_slot
        │                        ▲
decode_receive_worker ───────────┘        + core/pace/frame_pacer
        ▲
decode_session（编排，两个 worker 都只被它持有）
```

`core/pipeline/` 里出现 `<media/*>` 或 `<android/*>` 即为架构违规：
iOS 的 VideoToolbox 同样是"送样本 / 收帧"两段，这套时序状态机要能直接搬过去。

---

## 十、单测口径（`p12c6_pipeline_tests`）

并发本身没法在桌面上测（没有 MediaCodec），所以把**唯一有分支的那部分**抽成纯状态机再测。
其余靠真机计数器闭合（§6.2）。

| 编号 | 用例 | 断言 |
| --- | --- | --- |
| C6-1 | 新建 | 下一步是 `kReadSample`，没有待送包 |
| C6-2 | 读到一包 | 下一步 `kSubmitPending`；送成功后才回到 `kReadSample` |
| C6-3 | 输入满（没送出去） | 下一步仍是 `kSubmitPending`，**不能**变成 `kReadSample` |
| C6-4 | 读到 EOS | 下一步 `kSignalEos`，且 EOS 用的 pts 是已送出的最大 pts |
| C6-5 | EOS 送出后 | 下一步 `kFinished`，不再读写任何东西 |
| C6-6 | EOS 被重复要求 | 只发一次（第二次仍是 `kFinished`） |
| C6-7 | 读失败/加密被拒 | 下一步 `kAbort`，且原因非空 |
| C6-8 | pts 乱序 | `eos_pts` 取最大值，不取最后一包 |
| C6-9 | 步骤名 | 五个枚举 → 可读名的拼写固定（改名字会让历史证据对不上） |

C6-3 是这张卡最重要的一条：它守的是 C3 踩过的坑 —— 送包返回 `TRY_AGAIN` 时若去读下一包，
读指针已经前进，那一包永久丢失（AAC 一包 21ms，丢几包就是可听的断续）。
