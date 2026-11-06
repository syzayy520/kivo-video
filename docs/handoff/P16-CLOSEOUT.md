# P16 结题 · 后端核心加固与音频时钟

> 2026-11-06。本文件是 **P16 的索引与结论**，不重复设计细节 ——
> 每一条都指向 `docs/design/backend-core/` 下的原始设计稿。
> 接手顺序：先读本文 §一（结论），再按需点进 §二 的卡片。

---

## 一、五条结论（先看这个）

| # | 结论 | 依据 |
| --- | --- | --- |
| 1 | **A1 速率伺服在架构上不可行**。音频时钟读的是 AAudio 设备 timestamp，重采样只改"送出去的样本内容"，改不了 timestamp。实验已证伪（注入 2000 ppm + trim 2000，`phase_error` 两组都是 65.8 ms） | `199`、`200` §六 |
| 2 | **相位钳制是当前唯一手段**，且已到天花板。门从 1000–1500 ppm 开始动作；`band` 调到 20 ms 残差也只从 40 降到 25 ms，仍追不上 2000 ppm 的 70 ms 累积 | `197`、`198` |
| 3 | **A2（seek 排空）已换成显式凭据**：不再 `sleep_for(10ms)`，改为等 `in_flight()==0` + 10 ms deadline；超时带非空 reason。seek→0 实测 23.5 ms、`pass=true` | `189`、`190` |
| 4 | **burst 兜底不再静默**：`getFramesPerBurst <= 0` 时明确失败并带 reason，不再悄悄用 192 | `185` |
| 5 | **测量层齐了**：`in_flight()`、`rate_ppm`（合成/注入/默认路径三条验证）、频差账目重算。`rate_ppm` 已进 decode 证据 | `186`、`193`、`195`、`196` |

**没做的**（不是遗漏，是范围外）：字幕像素、网络流、DRM、tone mapping、倍速、轨道切换、
统一状态机、多 SoC 矩阵、取证统计移出热路径。详见 §四。

---

## 二、卡片索引（按主题）

### P0 加固（`181`）

| 项 | 内容 | 文件 |
| --- | --- | --- |
| D1 | JNI 17 个导出加函数 try 块，异常转成 rc=-1 + 非空 `last_error` | `jni/jni_call_guard.hpp` |
| D2 | `stop()` 加 worker 退出计数 + 500 ms 有界等待，超时写 reason | `decode_pipeline_gate.*`、`decode_session.cxx` |
| D3 | 全局 `operator new` 钩子：默认 ON，release flavor 传 OFF | `alloc_probe_hook.cxx`、Android `CMakeLists.txt`、`build.gradle.kts` |
| D4 | 双清单一致性校验 + CI 硬门禁 | `tools/verify_source_manifest.py` |

### A1 音频时钟（`183`、`193`、`195`–`200`）

| 卡 | 做了什么 | 关键数字 |
| --- | --- | --- |
| B1 `184` | 162 份 P13 证据重算 | 40 事件轮，**6/23 自然轮门沉默** |
| B10 `193` | `ClockRateEstimator`（纯逻辑） | 合成输入 5000 ppm ±50 |
| B11 `194` | 接进 `ClockPositionTracker::read()` | 每帧约十次浮点 |
| B12 `195` | 出版到 decode 证据（绕开 `pace_stats.*`） | `rate_ppm` 首次进证据 |
| B13 `196` | 注入验证，**发现"喂修正后位置"的缺陷** | 改前 -3123/-945 → 改后 1987/4959 |
| B14 `197` | 六组注入标定门限 | 临界 1000–1500 ppm |
| B17 `198` | `drift_band` 参数化 + 四组对照 | `max_correction` 精确 = band |
| B18 `199` | 速率伺服可行性查证 | 视频无独立速率；时钟来自 timestamp |
| B20/B22 `200` | 微调内核 + 接线 + **实验证伪** | trim 2000 无效，`phase_error` 仍 65.8 ms |

### A2 seek（`186`–`192`）

| 卡 | 做了什么 |
| --- | --- |
| B3 `186` | 兜底行为测试（`used_fallback()` 可查询） |
| B5 `188` | 采样率切换位置连续（T6 delta = 0 ns） |
| B6 `189` | `in_flight()` 只读观测（p50 1 µs / p99 2 µs） |
| B7 `190` | seek 栅栏换显式凭据 |
| B8 `191` | 失败归因分离（落点/关键帧 ≠ 解码延迟） |
| B9 `192` | 压测 8 轮，0 命中 → 记为"无法复现的偶发" |

### 交接与可移植（`187`）

`tools/verify_tools_portability.py`：P16 脚本不得硬编码本机路径；
行为测试从 `ANDROID_HOME` / `local.properties` / PATH 三级解析 SDK。

---

## 三、门禁怎么跑

CI `hard-gates` 7 步（`.github/workflows/p2-ci-build-test.yml`），本地等价复算：

```sh
python tools/verify_source_manifest.py --self-test && python tools/verify_source_manifest.py
python tools/verify_p16_p0_gates.py --self-test && python tools/verify_p16_p0_gates.py
python tools/verify_p16_rate_events.py --self-test && python tools/verify_p16_rate_events.py
python tools/verify_p16_burst_gate.py --self-test && python tools/verify_p16_burst_gate.py
python tools/verify_tools_portability.py --self-test && python tools/verify_tools_portability.py
python tools/verify_p16_seek_gate.py --self-test && python tools/verify_p16_seek_gate.py
```

每个都带 `--self-test`（变异注入，抓不到缺陷即 exit 2）。
真机行为测试（需 `dd137cf7`）：

```sh
python tools/run_p16_burst_behavior_test.py --device dd137cf7    # T1–T15，18 项断言
python tools/run_p16_seek_gate_probe.py --device dd137cf7        # 在途窗口 p50/p99
```

**注意**：`hard-gates` 尚未在 GitHub 真实跑过（只做了本地等价复算）。

---

## 四、明确没做（范围外，不是遗漏）

| 缺口 | 现状 |
| --- | --- |
| 字幕 | 74 文件语义层，零像素产出，没在 Android 验过 |
| 网络流 | `providers/http` 144 行、无 socket |
| DRM | 只有一句 `MediaCrypto` 提及 |
| tone mapping | `core/hdr` 不存在 |
| 倍速 / 轨道切换 / pause-resume | 都没有 |
| 统一状态机 / 统一 demuxer | 没有 `PlayerState` |
| 多 SoC 矩阵 | 两台同型号 K40，只互证 |
| 取证统计移出热路径 | 仍侵入解码线程（`pace_stats.*` 被他人占用） |

---

## 五、接手注意（会踩的坑）

1. **`F:\kivo` 是用户的音乐播放器项目，正在开发中，只读** —— 不写、不复制源码；
   引用它的结论标"引用"不标"移植"。
2. **`b6e950ee` 是主测试机，别人在用**；P16 全程只用 `dd137cf7`。
3. **`backend/CMakeLists.txt`、`pace_stats.*`、`av_sync_report.cxx` 被 P15 主线占用** ——
   P16 一行没碰。`rate_ppm` 出版就是绕开它们做的。
4. **JNI 改签名是四处联动**：Kotlin 调用方 / `NativePlayer.kt` external 声明 /
   bridge 实现 / native 实现。漏一处就 `Too many arguments`。
5. **压测前先 `svc power stayon usb`**，否则息屏会把 `sleep` 和播放一起冻住
   （`192` 上过一次当：19 分钟只跑 2 轮）。
6. **批量 seek 实验用 `--es kivo.finalize on_stop` + HOME**，45 s/轮 → 6 s/轮。
7. **`adb reconnect` 会让两台设备同时消失**（USB 重新枚举），约 1 分钟自动恢复；
   不要接着 `kill-server`。

---

## 六、性能画像（从 811 份证据重算，2026-11-06）

| 指标 | 预算 | 实测 | 结论 |
| --- | --- | --- | --- |
| 丢帧率 | < 0.5%（G6） | **0.0876%**（1,605 / 1,831,234 帧） | 达标 |
| seek 首帧 | < 150 ms（G8b） | n=45，p50 **36.0 ms**，max **59.6 ms** | 全部达标 |
| 冷启动首帧 | < 300 ms（G8a） | **`/19`：n=72，p50 29.1 ms，p99 128.8 ms** | 达标 |
| 冷启动（旧 `/13`） | — | n=174，p50 86.5 ms，p99 378.6 ms，**3 份超 300** | 历史轮次，非当前 |

- 3 份超预算全是 `/13` 旧证据：`p13r-4k60-r3` 548 ms、`p13s-4k60-r1` 379 ms、
  `p13k-130s-r1` 311 ms，都是 4K 或 130 秒长片源。
- 当前 `/19` 的冷启动比 `/13` 好 **3 倍**（p50 29.1 vs 86.5、p99 128.8 vs 378.6 ms）。
- `speed_vs_realtime` 的 p99 = 9.1× **不是退化**：那是 seek 后剩余片段 / 短片的快进轮次，
  看 `wall_us` 与 `rendered` 能分辨。

**结论**：当前性能在预算内，且比 P13 时期明显更好。

**补（同日）**：评审说的"没有性能门禁"现在不成立了 ——
`tools/verify_p16_perf_baseline.py`（CI `hard-gates` 第 8 步）把上述三条判据变成门禁：
超预算未登记即红。它守的是"仓库里的证据数字"，不是"代码改动"—— 新证据仍要真机跑。
