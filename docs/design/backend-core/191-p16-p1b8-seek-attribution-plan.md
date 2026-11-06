# P16-P1B8 · seek 失败归因：落点/关键帧 ≠ 解码延迟

- 日期：2026-11-06
- 定位：修 `190` 收口里 C 轮暴露的归因缺陷（`landing_us=0` 却 `video_first_pts=9.03 s`，
  reason 只说"总共超预算"）。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`190-p16-p1b7-seek-gate-design.md` §五 C 轮、`03-goals.md` G8/G9。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照；无新目录、无项目包。

---

## 一、构思

C 轮的三个事实互相矛盾：

| 字段 | 值 | 含义 |
| --- | --- | --- |
| `landing_us` | **0** | 解封装器说落在 0（PREVIOUS_SYNC，应 ≤ target） |
| `video_first_pts_us` | **9,033,333** | 首帧实际是 9.03 s 的关键帧 |
| `first_frame_us` | 9,046,810 | 于是"首帧延迟"被算成 9 秒 |

`assess_seek()` 的 reason 只写"total 9079598 us exceeds ... (prepare 32788 + first frame
9046810)"——把**落点跑到后面**说成了**解码慢**。这是归因错误，不是数字错误：
数字都对，但读证据的人会被引向错误的排查方向。

历史 42 轮里 `video_first_pts > target` **一次都没出现**（全部 ≤ target），
所以新增这条判定分支**不会重算任何历史 `pass`**。这是本卡能安全动手的前提。

---

## 二、规划设计

1. **只加一个判定分支，不改既有分支**：`video_first_pts_us > target_us` 时判
   "落点 / 关键帧间隔问题"，reason 明确写出 pts、target、landing 与差值，
   并附 `total_us` 便于对照。
2. **不新增 schema 字段、不改 `pass` 语义**（该分支命中时本就该 false）；
   历史 0 轮命中，因此历史判定逐位不变。
3. **不修根因**：为什么 flush 后要等下一个关键帧、`landing` 该不该反映真实出帧位置，
   属于解码路径改动，需要独立卡 + 多轮真机；本卡只把话说清。
4. **不碰他人主线**：`av_sync_report.cxx`（sync 证据）不出 seek 段，本卡只动
   `core/seek/seek_budget.cxx` 与 decode 侧 reason 文案，零 schema 变更。
5. 门禁加在**既有** `verify_p16_seek_gate.py`（S4），不新增脚本。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/191-p16-p1b8-seek-attribution-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `190` 全文 |
| `core/seek/seek_budget.cxx` | 改 | `assess_seek` 增加"首帧 pts 晚于 target"归因分支 | 不改既有分支顺序与阈值、不改 `SeekFacts` 结构 |
| `tools/verify_p16_seek_gate.py` | 改 | 增加 S4 判据与 M5 变异 | 不改 S1–S3 |

新增 **1** 个文件、改 **2** 个既有文件；**无新目录、无 schema 变更**。

### 判据

| 判据 | 内容 |
| --- | --- |
| S4 | `seek_budget.cxx` 有 `video_first_pts_us > target_us` 的归因分支，且 reason 含 `keyframe` 与 `not decode latency` |
| M5 | 删掉该分支后，`verify_p16_seek_gate.py --self-test` 必须 CAUGHT |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | 六个门禁 self-test + 正式跑 | 全 PASS；CI `hard-gates` 7 步 |
| L2 | NDK clang `-fsyntax-only` + 双 flavor 增量构建 | 0 错；`libkivo_player.so` 链接成功 |
| L3 | `dd137cf7` 重跑 `--ei kivo.seek.ms 5000` | reason 变为"首帧 pts 晚于 target / 关键帧间隔"，`pass` 仍 false（**不宣称通过**） |
| L3 | 同参数 seek→0 | 仍 `pass=true`、reason 为空（归因分支不误伤正常轮） |

**结论边界**：本卡只修归因，不修"落点 0 却出帧 9 s"的根因；
后者需要动解码送包/丢弃策略，另开卡。

---

## 五、收口（2026-11-06）

### 交付

新增 **1** 个文件（`191` 文档），改 **2** 个既有文件（`seek_budget.cxx`、
`verify_p16_seek_gate.py` 加 S4/M5）。无新目录、**无 schema 变更**、CI `hard-gates` 仍 7 步。

### 实现

`assess_seek()` 在 `total_us` 计算之后、预算判定之前加一条：
`video_first_pts_us > target_us` 时判"落点 / 关键帧间隔"问题，reason 写明
pts / target / landing / 差值 / total，并注明 `not decode latency`。
历史 42 轮里 `video_first_pts > target` 为 **0 轮**，因此不重算任何历史 `pass`。

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | 六个门禁 self-test + 正式跑全 PASS；S4 变异 M5 **CAUGHT**；CI 7 步 |
| L2 | clang `-fsyntax-only` 0 错 0 警；双 flavor 增量构建 6/6 步、`libkivo_player.so` 链接成功；Gradle **32 s** |
| L3 | `dd137cf7` 两轮：**seek→5000 `pass=true`**（landing 0、`video_first_pts` **0**、首帧 5.8 ms、total **41.8 ms**）；seek→0 `pass=true`（25.9 ms） |

### 如实说明（重要）

- **`190` C 轮的 9 秒异常本轮没有复现**：同一片源、同一参数，`video_first_pts` 回到 0、
  `total 41.8 ms`、`pass=true`。说明那 9 秒是**偶发**，不是稳定缺陷。
- 因此**归因分支没有在真机上被触发过**，只由 S4 静态门禁 + 编译验证守护。
  **不宣称"行为已验证"**。
- 归因分支的价值是防御性的：一旦再出现 `pts > target`，reason 会直接指向关键帧/落点，
  而不是把人引向"解码慢"。
- 要定因需要专项复现：连续多轮 seek（不同 target × 不同片源）压测，抓 `pts > target` 的轮次
  并保留 trace —— 这是下一张卡的输入，不是本卡的成绩。

### 未做

不修解码送包/丢弃策略（根因）、不改 schema、不改阈值、不碰他人主线、
不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不改 `SeekFacts` 结构、不升 schema、不改 `kDefaultBudgetUs`；
- 不修改 `decode_submit_worker` / 送包过滤逻辑（根因另开卡）；
- 不碰他人主线、不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 不碰。
