# 92 · P13-S 设计：给「钟比墙快」加一条门

> 卡片：P13-S
> 前置：P13-R（`90-p13r-design.md` 测出满载时 `clock_ahead = +118.79 ms`）
> 结论：`jump_limit_ns` 只看单次 1 s 的跳变，管不到 1.5% 的**速率**误差；
> 新增"连续超前 60 帧 + 累积 45 ms"的漂移门
> schema：`kivo.p12c.sync/16` → `/17`

---

## 一、问题

现有两条护栏都管不到这个缺陷：

| 护栏 | 判据 | 为什么拦不住 |
|---|---|---|
| `jump_limit_ns` | 单次 `位置增量 − 墙钟增量 > 1 s` | 1.5% 的误差在 16.7 ms 帧距里只有 250 µs，跑一万帧也只是每帧 250 µs |
| 倒退钳制 | `position` 不允许变小 | 快不是倒退 |

于是真机上出现了一个"所有计数器都说正常"的状态：8 秒累积 118.79 ms（1.49%），
`jump_rebases = 0`、`extrapolated = 0`、`free_run = 0`，而 `playback_rate = 1.0151`。

---

## 二、判据设计

两条**同时**成立才复位：

1. **连续** 60 帧 `位置增量 > 墙钟增量`（一次台阶会把这条链打断）；
2. 这些帧累积的超前量 `> 45 ms`（`kAudibleThresholdNs`，与 G9/L 同一把尺子）。

复位动作：`position -= 累积超前量`，累积清零。

### 2.1 三个必须写下来的细节（都踩过）

1. **只拦"快"，不拦"慢"**。钟落后是 `stale` / `free_run` 两条路的事；
   在这里也去补，会把"时钟停了"伪装成"走得很准"。
2. **复位必须在倒退钳制之后**。减完位置会低于上一帧的位置，先减就会被钳制抬回去 ——
   第一版就是这么错的，单测打出 66 ms 的残余超前。
3. **不能"重新赋一个基准值"**。多存一个 `expected_position` 状态，它自己也会漂；
   直接减掉量到的累积量才是自洽的（`cum_drift` 按定义就是 `position − 墙钟`）。

---

## 三、字段与落位

| 文件 | 改动 |
|---|---|
| `pace_policy.hpp` | `drift_limit_ns = 45 ms` |
| `clock_position_tracker.{hpp,cxx}` | `cum_drift_ns_` / `lead_streak_` / `drift_rebases_`；`Reading::drift_rebased` |
| `pace_stats` | `drift_rebases` 计数 + `note_drift_rebase()` |
| `frame_pacer.cxx` | 每帧上报 |
| `report` | `pace.clock_health.drift_rebases`，`sync/17` |
| `p12c5_pace_tests` | S1：1.5% 快钟必须被拦住；同步钟一次都不许触发 |

---

## 四、验收

| 编号 | 判据 |
|---|---|
| S1 | 假时钟每帧多走 250 µs（1.5%）⇒ `drift_rebases ≥ 1`，且位置与墙钟的距离 ≤ 45 ms + 一帧 |
| S2 | 同步时钟 400 帧 ⇒ `drift_rebases == 0`（不误伤） |
| S3 | 真机 A/B：满载轮 `clock_ahead` 回到 ±1 ms、`playback_rate` 回到 1.000 |
| S4 | `ctest -R "kivo_p13\|kivo_p12c5"` 15/15；总收口四层通过 |
