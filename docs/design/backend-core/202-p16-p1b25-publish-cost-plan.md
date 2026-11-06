# P16-P1B25 · 量化 `publish()` 的热路径开销

- 日期：2026-11-06
- 定位：评审说"取证统计侵入解码热路径（每 30 帧一次 O(n·L) 分位数）"，但**从未实测**。
  本卡量它，决定"该不该优化"而不是凭感觉优化。门闸：构思 → 规划设计 → 家族树 → 实现 → 验收。
- 上位：评审结论（性能与并发）、`03-goals.md` G6、`P16-CLOSEOUT` §四（未做项之一）。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**；无新目录。

---

## 一、构思

`DecodePaceSampler::publish()` 每 30 帧被调一次，内部 `decode_pace_stats_from()`
要遍历直方图求分位数（O(n·L)）。评审据此说它"侵入热路径"。

但"侵入"到什么程度？如果一次只要 20 µs，30 帧（≈0.5 s）里占 0.004%，那是理论问题；
如果要 2 ms，每 0.5 s 卡 2 ms，就会直接变成丢帧。**两种结论对应完全不同的行动**，
而目前没有任何数据。

---

## 二、规划设计

1. **在 `publish()` 内部计时**，不改调用方：`monotonic_now_ns()` 前后各取一次，
   差值写进 `DecodePaceStats.publish_cost_ns`。
2. **只出版、不判定**：不在 native 里判"快不快"，让证据说话。
3. **不碰 `pace_stats.*`**：计时与出版都在 `decode_pace_sampler.*` / `decode_pace_stats.hpp` /
   `decode_stats_renderer.cxx`，这三个都不在他人占用列表里。
4. **不升 schema**：仍 `decode/19`，只加一格（与 `rate_ppm` 同一做法，`verify_p12_all.py` 已验过不会红）。
5. 只操作 `dd137cf7`；`b6e950ee` 不碰。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/202-p16-p1b25-publish-cost-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 CLOSEOUT |
| `mediacodec/session/decode_pace_stats.hpp` | 改 | 加 `publish_cost_ns` 一格 | 不改既有字段 |
| `mediacodec/session/decode_pace_sampler.cxx` | 改 | `publish()` 内部计时 | 不改转换函数、不改锁范围 |
| `mediacodec/session/decode_stats_renderer.cxx` | 改 | `pace` 块出版一格 | 不改其它块 |

新增 1 文件、改 3 个既有文件；**无新目录、无 schema 版本变更**。

### 判据（R1–R3）

| 判据 | 内容 |
| --- | --- |
| R1 | 真机证据的 `pace.publish_cost_ns` 非零且量级合理（不是 0，也不是毫秒级） |
| R2 | 与帧间隔对比：`publish_cost_ns / frame_duration_ns` < 1%（G6 余量内） |
| R3 | 帧数闭合不变：1050 帧、0 丢帧、`completed=true` |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | clang `-fsyntax-only` + 双 flavor 构建 | 0 错；`libkivo_player.so` 链接成功 |
| L2 | `dd137cf7` 完整播放一轮 | R1–R3 全过，`publish_cost_ns` 有值 |
| L3 | 七个门禁 + `verify_p12_all.py` | 全 PASS |

---

## 五、收口（2026-11-06）

### 交付

新增 1 文件（`202` 文档），改 3 个既有文件（`decode_pace_stats.hpp`、
`decode_pace_sampler.cxx`、`decode_stats_renderer.cxx`）。无新目录、无 schema 版本变更。

### 实测（`dd137cf7`）

| 轮 | `publish_cost_ns` | 占一帧 | 帧数 / 丢帧 |
| --- | --- | --- | --- |
| cost3 | 22,187 ns（**22.19 µs**） | 0.0666% | 874 / **72** |
| cost4 | 22,448 ns（**22.45 µs**） | **0.0673%** | **1050 / 0** |

### 结论

- `publish()` 一次 **≈ 22.4 µs**，在 33.33 ms 帧间隔里占 **0.067%**。
- **不是热路径瓶颈**。评审说"取证统计侵入解码热路径"是理论担忧，实际量级可忽略。
- 因此**不需要优化 `publish()`** —— 优化它会改分位数算法、引入证据口径变化，
  收益 0.067%，不划算。

### 如实说明（两个坑）

1. **cost3 的 72 帧丢帧不是 `publish` 导致**：那轮设备刚从 `Asleep` 唤醒，
   Surface/CPU 恢复造成瞬态；cost4 稳定后 1050 / 0。
2. **前两轮证据是 0 字节**：设备息屏（`mWakefulness=Asleep`）后 Surface 不可用，
   `maybeStartPlayback` 不触发。**压测前必须 `svc power stayon usb`** ——
   这个坑 `192` 记过一次，这次又踩了，说明"记在文档里"不等于"每次都会做"。

### 未做

不优化 `publish()`、不碰 `pace_stats.*`、不改 schema、
不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不优化 `publish()`（先量化，再决定；优化属下一张卡）；
- 不碰 `pace_stats.*` / `av_sync_report.cxx`；
- 不改 schema 版本、不改 `verify_p12_all.py`；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`、`b6e950ee` 不碰。
