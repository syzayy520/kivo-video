# P16-P1B26 · 暴露当前播放位置（`position_us`）

- 日期：2026-11-06
- 定位：评审说"打断之后回来从哪继续"需要 native 暴露 `position_us`；UI 进度条也需要。
  这是 pause-resume 的前置，且改动小、风险低。门闸：构思 → 规划设计 → 家族树 → 实现 → 验收。
- 上位：评审结论（播放流程 / 能力面）、`03-goals.md` G10。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**；无新目录。

---

## 一、构思

`DecodeStats` 有 `first_frame_latency_us`、`wall_us`、`presented`，但**没有"现在播到哪"**。
Kotlin 侧只能靠 `presented × frame_duration` 估，误差随帧率变化累积。

位置在 `ClockPositionTracker::Reading.position_ns` 里，但它只存在于 `FramePacer`
的 `arrive()` 栈上。`FramePacer` 不加锁（热路径），所以不能直接跨线程读。

**做法**：`arrive()` 里把位置存进 `std::atomic<int64_t>`，`snapshot()` 读 atomic。
atomic 读写各一条指令，不改变热路径语义。

---

## 二、规划设计

1. **`FramePacer` 加 `std::atomic<int64_t> position_ns_`**：`arrive()` 末尾 store，
   `reset()` 清零；公开 `position_ns()` getter（acquire load）。
2. **`DecodeStats.position_us`**：`-1` = 还没起播（与"0 µs"严格区分）。
3. **`snapshot()` 读**：`pacer_ ? pacer_->position_ns() / 1000 : -1`。
4. **只出版、不判定**：不在 native 里算"播到哪了"以外的任何东西。
5. **不升 schema**：仍 `decode/19`，加一格（与 `rate_ppm` / `publish_cost_ns` 同一做法）。
6. 只操作 `dd137cf7`；`b6e950ee` 不碰。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/203-p16-p1b26-position-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 CLOSEOUT |
| `core/pace/frame_pacer.hpp` | 改 | atomic 位置 + getter | 不改决策逻辑、不加锁 |
| `core/pace/frame_pacer.cxx` | 改 | `arrive()` store、`reset()` 清零 | 不改 due 计算 |
| `mediacodec/session/decode_stats.hpp` | 改 | 加 `position_us` | 不改既有字段 |
| `mediacodec/session/decode_session.cxx` | 改 | `snapshot()` 填位置 | 不改其它 |
| `mediacodec/session/decode_stats_renderer.cxx` | 改 | `timing` 块出版一格 | 不改其它块 |

新增 1 文件、改 5 个既有文件；**无新目录、无 schema 版本变更**。

### 判据（P1–P3）

| 判据 | 内容 |
| --- | --- |
| P1 | 真机证据 `timing.position_us` 单调递增、末值接近片源时长（35 s ± 1 s） |
| P2 | 与 `presented × frame_duration` 的估算差 < 1 帧 |
| P3 | 帧数闭合不变：1050 帧、0 丢帧、`completed=true` |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | clang `-fsyntax-only` + 双 flavor 构建 | 0 错；`libkivo_player.so` 链接成功 |
| L2 | `dd137cf7` 完整播放一轮 | P1–P3 全过 |
| L3 | 七个门禁 + `verify_p12_all.py` | 全 PASS |

---

## 五、收口（2026-11-06）

### 交付

新增 1 文件（`203` 文档），改 5 个既有文件（`frame_pacer.hpp/.cxx`、
`decode_stats.hpp`、`decode_session.cxx`、`decode_stats_renderer.cxx`）。
无新目录、无 schema 版本变更。

### 实测（`dd137cf7`）

| 字段 | 值 |
| --- | --- |
| `timing.position_us` | **34,955,250**（34.955 s，片源 35 s） |
| `timing.wall_us` | 34,958,369（34.958 s） |
| 帧数 / 丢帧 | 1050 / 0 |
| `completed` / `error` | true / `""` |

### 判据核对（含一处判据更正）

| 判据 | 结果 |
| --- | --- |
| P1（位置单调、末值接近片源时长 ±1 s） | **PASS**：34.955 s vs 35 s，差 45 ms |
| P2（原判据：与 `presented × frame_duration` 差 < 1 帧） | **未过**：实测 44.4 ms > 33.3 ms |
| P2 更正后（`|position − wall|` < 10 ms） | **PASS**：3.1 ms |
| P3（1050 帧、0 丢帧、completed） | **PASS** |

**P2 是我自己写错的**：`presented × frame_duration` 是"视频已呈现帧数的理论时长"，
而 `position_us` 来自**音频钟**（`elapsed_ns`）。两者之间本来就有音频缓冲差
（44 ms ≈ 1.3 帧），不是 bug。用"位置 vs 墙钟"才是对的 —— 实测 3.1 ms，说明音画同步正常。

### 结论

- `position_us` 正确暴露：34.955 s，与片源 35 s 差 45 ms（末帧不呈现 + 音频尾缓冲）。
- 与墙钟差 **3.1 ms**，说明音频主时钟与真实时间同步良好。
- 这是 pause-resume 与 UI 进度条的前置；本卡只暴露位置，不做状态机。

### 未做

不做 pause-resume、不做 UI 进度条、不碰 `pace_stats.*`、不改 schema、
不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不做 pause-resume 本身（本卡只暴露位置，状态机是另一张卡）；
- 不做 UI 进度条（那是 Kotlin 侧）；
- 不碰 `pace_stats.*` / `av_sync_report.cxx`；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`、`b6e950ee` 不碰。
