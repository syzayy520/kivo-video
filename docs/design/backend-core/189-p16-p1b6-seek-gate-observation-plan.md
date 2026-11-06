# P16-P1B6 · seek 栅栏在途窗口的只读观测

- 日期：2026-11-06
- 定位：把 `183` §二 A2 里「`sleep_for(10ms)` 够不够」从注释里的猜想变成可测数据。
  门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`183-p16-p1b-audio-core-design.md` §二 A2 / §七 Q3·Q4、
  `186-p16-p1b3-behavior-evidence-plan.md`（行为测试范式）、`08-p12-family-tree.md`。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照；无新目录、无项目包。

---

## 一、构思

`audio_playback.cxx:210-212` 的 seek 静音栅栏写的是：

```cpp
ring_source_->set_muted(true);
std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 注释依据："K40 burst 192 帧 ≈ 4 ms，取 10 ms 留余量"
```

`ring_buffer_sample_source.hpp:40-41` 自己也承认风险：
"调用方负责在拉下之后等一个 burst（**在途回调可能已经过了判空那行**）"。

也就是说：正确性挂在一个**从未被测过**的时序假设上。直接把它换成显式同步
（ticket / generation）会引入 Q3/Q4 那个"谁发 complete"的实时线程归属分歧，
评审门闸还关着，本卡不碰。本轮只做**测量工具**：先让"在途"这件事可见、可量化。

---

## 二、规划设计

1. **计数位置是关键**：必须放在 `render()` 的**最前面**，早于 `muted_.load()`。
   若放在判空之后，那些"已经过了判空那行、还没读环"的回调不计入 —— 恰恰是注释点名的危险窗口。
   所有出口都要递减；`relaxed` 递增、`release` 递减，与 SPSC 环的 atomic 索引同类，
   无锁、无分配、无日志，符合 `aaudio_output_session.hpp:23-26` 的实时线程约定。
2. **不改栅栏逻辑**：`AudioPlayback::seek_to()` 一行不动（仍 `sleep_for(10ms)`）。
   本卡不替换 sleep、不引入 ticket/generation，也不改实时线程的**语义**。
   如实说明：实时路径多两条 atomic 指令，但无阻塞、无分配、行为逐位不变。
3. **不改证据 schema**：观测由独立的设备侧脚本产出，不往 decode/sync 证据里加字段
   （避免碰他人占用的报表与 `verify_p12_all.py` 口径）。
4. **测量脚本不注册 ctest、不进 CI**：`backend/CMakeLists.txt` 被他人主线占用，
   且脚本需要真机。C++ 源内嵌在脚本里，产物落既有 `build/`，零新增仓库目录。
5. **只操作 `dd137cf7`**，`b6e950ee` 不碰、不 `uninstall`。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/189-p16-p1b6-seek-gate-observation-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `183/186` 全文 |
| `tools/run_p16_seek_gate_probe.py` | **新增** | 编译 + 设备运行，量化 `set_muted` 到「在途归零」的耗时 | 不改产品代码、不产证据 JSON、不替代 ctest |
| `backend/include_private/video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp` | 改 | 增加 `in_flight()` 只读观测 | 不改判空/读环逻辑、不加锁、不分配 |

新增 2 文件、改 1 个既有文件，全部落在既有族；**无新目录**。

### 判据（O1–O4）

| 判据 | 内容 |
| --- | --- |
| O1 | `in_flight_` 的递增早于 `muted_.load()`，且每个出口都递减（覆盖"已过判空"窗口） |
| O2 | 并发反复 `render()` 后 `in_flight() == 0`（计数不漏账） |
| O3 | 真机重复采样，输出从 `set_muted(true)` 到在途归零的 p50/p99/max，与 10 ms 对照 |
| O4 | 五个既有门禁 self-test + 正式跑全 PASS；CI `hard-gates` 6 步不变 |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | NDK clang `-fsyntax-only` / `--compile-only` | 0 错；测试 TU 交叉编译成功 |
| L2 | `python tools/run_p16_seek_gate_probe.py --device dd137cf7` | O1/O2 PASS，p50/p99/max 有实测数字，退出码 0 |
| L3 | 五个门禁全量复跑 | 全 PASS |

**结论边界（必须如实写）**：本卡测的是**本进程内 `render()` 临界区**的在途窗口，
不是 AAudio 真实回调被平台调度延迟的窗口 —— 后者在这台设备上造不出来。
所以 p99 远小于 10 ms 只能说明「`render()` 本身不需要 10 ms」，
**不能证明 10 ms 覆盖平台调度抖动**；Q4（drain 超时该按什么定）仍需评审。

---

## 五、收口（2026-11-06）

### 交付

新增 **2** 个文件（`189` 文档、`tools/run_p16_seek_gate_probe.py`），改 **1** 个既有头文件
（`ring_buffer_sample_source.hpp`）；另把新脚本加入 `verify_tools_portability.py`
的 P16 白名单（现 **7** 份）。无新目录、无项目包；C++ 源内嵌，产物落既有
`build/p16-seek-gate/`。

### 实现

- 计数放在 `render()` **最前面**（早于 `muted_.load()`），两个出口都递减；
  `relaxed` 递增 / `release` 递减，无锁、无分配、无日志。
  **不改判空/读环逻辑、不改 `AudioPlayback::seek_to()`、不改证据 schema。**
- 测量脚本复用 `187` 的 SDK 三级解析（`importlib`，不重复实现），
  O1 静态形状检查 + 500 轮真机采样。

### 实测（`dd137cf7`）

```
O1 counter before muted check / 2 exits / getter : PASS
O2 in_flight() == 0 after join                  : PASS
rounds=500  p50=1 us  p99=2 us  max=690 us  budget=10000 us
O3 p99 < 10ms                                  : PASS
RESULT: PASS
```

### 结论与边界

- 高频压力下，本进程 `render()` 临界区 **p50 1 µs / p99 2 µs / max 690 µs**，
  比 10 ms 小 3–4 个数量级。也就是说「取 10 ms 留余量」不是由 `render()` 长度决定的，
  它买的是**平台回调调度**的余量。
- **不能证明 10 ms 覆盖平台调度抖动**：真实 AAudio 回调由平台调度，
  这台设备造不出被延迟的回调。Q4（drain 超时按固定值还是设备周期换算）仍等
  `183` §七 答复；本卡只提供"render 侧不需要 10 ms"这个事实。
- 计数目前**只做观测、未用于同步**。用它替换 sleep 还需要解决 Q3（谁发 complete）
  与"计数和 `muted_` 伪共享"（若做同步需 `alignas(64)`），留给评审后的实现卡。

### 实现期更正

O1 初版用全文 `find("muted_.load")` 比较位置，先命中了注释里
「若放在 `muted_.load()` 之后」那句，把判据自己判红。
收窄到 `render()` 函数体内比较后 PASS —— **判据形状问题，不是代码缺陷**。

### 验证

- 五个门禁 self-test + 正式跑全 PASS；CI `hard-gates` 6 步、无 `continue-on-error`。
- `--self-check` / `--compile-only`（2 个 TU）均 OK；真机 O1/O2/O3 全 PASS。
- 白名单仍拒绝 `b6e950ee`；临时可执行已清理，应用私有证据保留。

### 未做

不替换 `sleep_for(10ms)`、不实现 ticket / generation、不改证据 schema、不碰他人主线、
不碰 `F:\kivo`、不 `uninstall`。

---

## 六、明确不做

- 不替换 `sleep_for(10ms)`、不实现 ticket / generation 栅栏（Q3/Q4 待 `183` §七 答复）；
- 不改 `AudioPlayback::seek_to()`、不改任何证据 schema、不调阈值；
- 不碰他人主线（`backend/CMakeLists.txt`、`av_sync_report.cxx`、`pace_stats.*` 等）；
- 不实现 `183` §五 的实现卡；不碰 `F:\kivo`、不新建项目包。
