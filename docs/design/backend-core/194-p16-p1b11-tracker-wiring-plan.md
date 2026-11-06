# P16-P1B11 · 频差估计器接线（只算、不出版）

- 日期：2026-11-06
- 定位：把 `193` 的 `ClockRateEstimator` 接进 `ClockPositionTracker` 的每帧读取，
  **只算、不出版、不改任何判定**。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`193-p16-p1b10-rate-estimator-plan.md`、`183` §四 方案 C。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**；无新目录、无项目包。

---

## 一、构思

`193` 的估计器只有行为测试在跑，真实播放里一次都没运行过。
但**接线不必碰 `pace_stats.*`** —— `ClockPositionTracker`（`core/pace/`）与它的 `Reading`
都不在他人占用的文件里，可以在类内部持有估计器、把结果挂到 `Reading` 上。
出版（进证据）才需要 `pace_stats`，那一步留到让出后。

接线的价值：让它在真实负载下每帧运行，为将来换速率伺服攒数据；
同时把"每帧多算十次浮点"这个成本实测一次。

---

## 二、规划设计

1. **只算不改判**：`read()` 末尾喂样本、写两个 `Reading` 字段；`position_ns_`、门、
   三条推进分支一行不动。行为与接线前逐位一致。
2. **`reset()` 同寿命**：换片源 / seek 后估计器必须清零，否则"上一段快多少"会污染下一段。
3. **`Reading` 只加字段**：默认 `rate_ppm=0`、`phase_error_ns=0`，既有读者不受影响。
4. **不出版**：`pace_stats.*` 被他人主线占用，本卡不碰；证据里不会出现新字段。
5. **成本如实说明**：每帧约十次浮点 + 一次分支，未做 G6 性能回归，只在真机上确认播放正常。
6. 只操作 `dd137cf7`；`b6e950ee` 不碰。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/194-p16-p1b11-tracker-wiring-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `193` 全文 |
| `core/pace/clock_position_tracker.hpp` | 改 | `Reading` 加两个观测字段；私有持有估计器 | 不改既有字段语义、不改构造签名 |
| `core/pace/clock_position_tracker.cxx` | 改 | `read()` 末尾喂样本；`reset()` 清零 | 不改三条推进分支、不改门、不改 `position_ns_` |
| `tools/run_p16_burst_behavior_test.py` | 改 | SOURCES 加 tracker/ledger/now；CPP 加 T10/T11 | 不改 T1–T9、不改白名单 |

新增 1 文件、改 3 个既有文件；**无新目录、无 schema 变更**。

### 判据

| 判据 | 内容 |
| --- | --- |
| T10 | 用 `RampClock`（位置每步快 0.5%）驱动 tracker 200 帧：`Reading.rate_ppm` 落在 **5000 ± 100** |
| T11 | 同轮 `Reading.phase_error_ns > 0` |
| 回归 | 既有 T1–T9 全部仍 PASS（估计器不得影响 `AaudioClockSource` 的行为） |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | `--compile-only` | 12 个 TU 编译 0 错 |
| L2 | `--device dd137cf7` | T1–T11 **二十项断言全 PASS** |
| L2 | NDK clang `-fsyntax-only`；独立 CMake 双 flavor | 0 错；`libkivo_player.so` 链接成功 |
| L3 | `dd137cf7` 无 seek 播放一轮 | `completed=true`、`error=""`、1050 帧、`hook_installed=true`（证明热路径未坏） |
| L3 | 六个门禁 | 全 PASS；CI 7 步 |

---

## 五、收口（2026-11-06）

### 交付

新增 **1** 个文件（`194` 文档），改 **3** 个既有文件（`clock_position_tracker.hpp/.cxx`、
`run_p16_burst_behavior_test.py`）。无新目录、无 schema 变更。

### 实现

- tracker 私有持有 `ClockRateEstimator`；`read()` 末尾 `note({mono_ns, position_ns_})`，
  写 `Reading.rate_ppm` / `phase_error_ns`；`reset()` 同寿命清零。
  **三条推进分支、漂移门、`position_ns_` 一行未动。**
- 成本：每帧约十次浮点 + 一次分支。

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | `--compile-only` **12 个 TU** 编译 0 错 |
| L2 | `dd137cf7` **T1–T11 二十项断言全 PASS**（`T10 rate_ppm≈5000`、`T11 phase>0`） |
| L2 | clang `-fsyntax-only` 0 错；双 flavor **11/11 步**、`libkivo_player.so` 链接成功；Gradle **36 s** |
| L3 | 真机完整播放：`completed=true`、`error=""`、**1050 帧 / 0 丢帧**、`speed 0.998378`、`hook_installed=true`、`clock_source=audio_timestamp` |
| L3 | 六个门禁全 PASS、CI 7 步 |

### 结论与边界

- 估计器**已接线**并在真实播放中每帧运行；**未出版**，证据里没有新字段。
- 真机这轮证明**热路径没被破坏**（帧数闭合、0 丢帧、speed 0.998）。
- 但**没有做 G6 性能回归**：没有前后对照的 CPU / 丢帧率基线，
  "十次浮点不显著"目前只是量级判断，不是实测 —— 如实标注，不冒充。
- 控制律未改：漂移门、`PacePolicy`、`frame_pacer` 一行未动。

### 未做

不出版（`pace_stats.*` 被他人主线占用）、不实现速率伺服、不碰他人主线、
不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

### 提醒

`tools/run_p16_burst_behavior_test.py` 已 **379 行**，接近 400 行硬线；
下次再加断言应先拆出 `clock` / `seek` 两组，别让它变成第二个"什么都测"的脚本。

---

## 六、明确不做

- 不出版到证据、不改 `pace_stats.*` / `av_sync_report.cxx`；
- 不改 `PacePolicy`、不改漂移门、不改 `frame_pacer`；
- 不实现速率伺服控制律（等 `183` §七 Q1/Q2）；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`。
