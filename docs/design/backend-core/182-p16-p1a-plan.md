# P16-P1A · P0 加固防回退门禁 + 第二台设备 APK 回归

- 日期：2026-11-06
- 定位：本文是 **P1-A 的唯一设计事实来源**。门闸顺序：构思 → 规划设计 → 自然家族树 → 实现 → 单文件单职责 → 禁止平铺。
- 上位：`181-p16-p0-hardening-plan.md`（P0 四项加固与收口记录）、`08-p12-family-tree.md`、`03-goals.md`。
- 工作目录：`C:\kivo video` 既有仓库；**不新建项目包、不新建平行源码目录**。
- 设备纪律：真机操作**只针对 `dd137cf7`**；`b6e950ee` 不碰；**禁止 `adb uninstall`**；签名固定在仓库内，`install -r` 可覆盖。

---

## 一、构思：P0 之后还缺什么

P0 已把四个缺陷修掉并编译/真机探针验证过，但留下两个"会随时间失效"的洞：

| # | 洞 | 后果 |
| --- | --- | --- |
| H1 | P0 的四条加固没有任何自动化守护 | 后续有人新增一个 JNI 导出却忘了接异常边界、把 hook 默认改成 OFF、删掉 stop 的有界等待、给 CI 加回 `continue-on-error`，全都不会有人发现 —— 这正是本项目反复踩过的"恒错字段"形态 |
| H2 | L3 只做到"设备端探针跑通"，**没验证过 `libkivo_player.so` 在 APK 里的播放链路** | D1/D2 改的是 JNI 与 stop 路径，探针不覆盖；用户已提供第二台 K40，应当补上 |

### 实现期更正（保留 `181` 第九节原文）

`181` 第九节写「OFF flavor 不得产出 G3 证据这条约束只写在注释里，建议 P1 给 `verify_p14a_metrics.py` 加前置断言」—— **这句是错的，本卡不改那个文件**。
`tools/verify_p14a_metrics.py:9-10` 的 **M3 已强制 `alloc.hook_installed = true`**，OFF 构建必然判红；约束早就被守住了。
重复加一条既是造第二个真相，又会碰到他人主线（P15-C）正在用的文件。本卡改为**在文档里把这条链写清**，并让防回退校验器检查"M3 仍然存在"而不是复制它。

---

## 二、规划设计：四条取舍

1. **不碰他人主线。** 当前指针是 `P15-B / P15-C / 字幕接线`（`CURRENT.md` 顶部）。本卡不改 `backend/CMakeLists.txt`、`CURRENT.md`、`TASKS.md`、`docs/evidence/*`、`tools/verify_p12_*`、`tools/verify_p14*`。
2. **只做加法。** 新增校验器只读仓库文件，不改任何被检查对象；CI 只多一个 step。
3. **守护的是"形状"不是"数值"。** 不判断性能数字、不判断证据内容；只钉住"机制还在不在"，避免与 P15 的取证判据抢口径。
4. **APK 回归错峰。** 先确认没有活跃的 Gradle 原生构建再动手；若与他人的 `.cxx` / `build` 冲突，如实降级，不强推。

---

## 三、自然家族树（文件落位与单一职责）

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/182-p16-p1a-plan.md` | 新增 | 本卡规划、家族树、验收与更正 | 不复制 `181` 全文 |
| `tools/verify_p16_p0_gates.py` | **新增** | 钉住 P0 四条加固不被悄悄回退；自带变异自检 | 不 import 项目代码、不判断性能数字、不改任何文件、不替代 `verify_source_manifest.py` |
| `.github/workflows/p2-ci-build-test.yml` | 改 | 在既有 `hard-gates` job 增加该校验器一步 | 不动其它 job、不加 `continue-on-error` |

**新增 2 个文件，全部归位既有目录（`docs/design/backend-core/`、`tools/`）；改动 1 个既有文件。无新目录。**

### 为什么 `verify_p16_p0_gates.py` 不并入 `verify_source_manifest.py`

`tools/` 下现有 25 个校验脚本，一卡一职责。`verify_source_manifest.py` 守的是"源文件有没有注册"，
本卡守的是"加固机制有没有被删"。合并会让一个文件同时回答两个问题，也让 `--self-test` 的变异语义混在一起。

### 五条判据（G1–G5，全部只查形状）

| 判据 | 检查内容 | 为什么 |
| --- | --- | --- |
| G1 JNI 边界覆盖 | `jni_player_bridge.cxx` 中 `JNIEXPORT` 数 == `KIVO_JNI_BEGIN` 数 == `KIVO_JNI_END_*` 数，且每个导出函数都成对 | 新增导出最容易漏掉异常边界 |
| G2 分配钩子默认 | `option(... ON)` 存在；release flavor 传 `OFF`；hook 源码里有 `#if !defined(KIVO_DISABLE_GLOBAL_ALLOC_HOOK)`；默认构建不带 OFF 定义 | 防止把取证口径悄悄改成产品口径（或反过来） |
| G3 停止可观测 | `decode_session.cxx` 里 `wait_for_workers` 出现在 `join()` **之前**，且超时分支的 `failure_.mark` 带非空字面量；`gate` 的 `reset_for_start` 清零计数 | 防止"有界等待"被删或计数不复位导致 seek 后误报 |
| G4 CI 硬门禁 | `hard-gates` job 存在、无 `continue-on-error`、包含 manifest / core / p0 三个检查、push 分支含 `codex/**` | 防止门禁退回"只打印不失败" |
| G5 文件纪律 | `jni_player_bridge.cxx` < 300 行；`jni_call_guard.hpp` 存在；M3 仍要求 `hook_installed=true` | 家族树 §四 与 OFF-flavor 约束的下游链条 |

---

## 四、实现顺序（门闸）

1. 本文落盘；
2. `tools/verify_p16_p0_gates.py`（含 `--self-test` 变异自检）；
3. CI `hard-gates` 接入；
4. 本地全量复验（校验器、双 flavor 增量构建、行数）；
5. `dd137cf7` 的 APK 级播放回归；
6. 收口记录（写回本文第九节 + workspace memory）。

---

## 五、APK 回归口径（L3）

| 步 | 命令要点 | 判据 |
| --- | --- | --- |
| 1 | `./gradlew assembleDebug`（先确认无并行原生构建） | APK 产出；构建失败如实记录，不改签名/ABI 配置 |
| 2 | `adb -s dd137cf7 push <片源> /sdcard/kivo_p1a.mp4` | `PlayerActivity` 支持 `--es kivo.media` 任意路径 |
| 3 | `adb -s dd137cf7 install -r <apk>`；HyperOS 提示按 `347 2111` 点继续 | **不 uninstall**；装完确认 `pm list packages` 有 `com.kivo.cinema` |
| 4 | `am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media ... --es kivo.evidence p16p1a.json --es kivo.audio off` | 进程不崩、不 ANR |
| 5 | `run-as com.kivo.cinema` 读 `files/evidence/` | 证据可写、帧数 > 0、`error` 为空、`last_error` 为空 |

**降级条件**：Gradle 与其它助手的 P15 取证冲突、HyperOS 安装弹窗无法确认、或片源/Activity 参数与文档不符 →
退回"独立 CMake 产物真机探针已验证"（`181` 第九节 L3），本卡如实写 `not_measured` + reason，**不伪造证据**。

---

## 六、明确不做

- 不改 `verify_p14a_metrics.py`（M3 已覆盖）；不碰 P15-B/C/D、字幕、网络流、DRM、tone mapping 等主线卡；
- 不做 D2 的平台调用级超时与看门狗（那会改线程取消语义，需独立卡 + 真机长稳）；
- 不把取证统计移出解码热路径（会改所有真机证据口径，必须与 P15-C 合并推进）；
- 不加全仓 `-Werror` / ASan / TSan / 覆盖率（需重建测试基线）；
- 两台 K40 同型号，本卡**不宣称**多 SoC 矩阵。

---

## 七、收口记录（2026-11-06）

### 交付

- 新增 `tools/verify_p16_p0_gates.py`（235 行，单职责：钉住 P0 五条加固形状），G1–G5 + 7 条变异自检；
- CI `hard-gates` 接入该校验器，现为 3 步：清单一致性 / P0 加固门禁 / core 零平台头，**无 `continue-on-error`**；
- 本文（规划设计 + 家族树 + 收口）。**新增 2 个文件、改 1 个既有文件，无新目录、无项目包。**

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | `verify_source_manifest.py` self-test + 正式 PASS（豁免 0）；`verify_p16_p0_gates.py` baseline **0 问题**、7 个变异**全 CAUGHT**、正式 PASS；`core/` 平台头 0 命中 |
| L2 | 独立 CMake 双 flavor 增量构建均 `ninja: no work to do`（改动已在上轮 104/104 编译过）；Gradle `assembleDebug` **BUILD SUCCESSFUL**（4m40s，`buildCMakeDebug[arm64-v8a]` 重编原生库并打进 APK） |
| L3 | `dd137cf7` 两轮 APK 级播放回归，见下 |

### L3 两轮真机回归

**轮 1 · 诊断路径（不是播放成功，但验证了错误链）**
`/sdcard/` 根目录片源 → `stage=open, rc=-1, "ParcelFileDescriptor.open failed"`（Android 13 作用域存储，
Kotlin 侧拿不到 fd）。改用应用私有 `/sdcard/Android/data/com.kivo.cinema/files/media/` 后能打开，
但 `test_h264_aac_35s.mp4` + 默认参数（无节拍 / `surface_at_time`）得到
`outcome {completed:false, end_reason:"failed", error:"dequeueOutputBuffer returned -10000"}`、
4 包 0 帧、6.6 ms 退出。
**价值**：非空 reason 从 `DecodeFailureSlot` → `DecodeStats::error` → 证据 `outcome.error` 全程可见，
没有静默降级；`stop()` 正常收尾、进程存活、证据落盘 —— P0 加固守的正是这条链。

**轮 2 · 项目标准参数（完整播放）**
`kivo_h264_aac_35s_golden.mp4` + `--ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced`：

| 字段 | 值 |
| --- | --- |
| schema / outcome | `kivo.p12c.decode/19` / `{completed:true, end_reason:"completed", error:""}` |
| 帧数闭合 | submitted **1050** / received 1051 / released 1051 / rendered **1050** / dropped_late **0** |
| 时间 | `wall_us=35,030,603`、`decode_fps=29.97`、`speed_vs_realtime=0.999126`、首帧 63.082 ms |
| 度量 | `alloc.hook_installed=true`（**D3 默认 ON 在 APK 里生效**）、`calls_steady=178`、`buffers.in_flight=0 / leaked=0` |
| 时钟 | `pace.clock_source=audio_timestamp` |
| 同步证据 | `kivo.p12c.sync/43`：`audio.opened/started=true`、48 kHz / 2ch / pcm_float / low_latency / burst 192、`completed=true`、`error=""` |

**结论**：APK 级播放链路在 `dd137cf7` 上跑通；P0 的 JNI 边界、stop 有界等待、分配钩子默认 ON、CI 门禁
均未破坏播放与证据产出。

### 证据路径（可复算）

设备上（**保留未删**）：`run-as com.kivo.cinema cat files/evidence/p16p1a-golden.json`，
同目录还有 `p16p1a-golden-sync.json`、负向轮 `p16p1a-decode.json`、路径失败轮 `p12b-failure.json`。
本机临时副本：`build/p16p1a-*.json`。

**刻意不归档到 `docs/evidence/`**：那里有他人主线的 `verify_p12_all.py` 全量扫描，塞入新文件会让
总校验器口径变红。本卡选择「设备可复算 + 设计文档记数字」，不制造跨卡污染 —— 这不是省事，是避坑。

### 设备纪律

只操作 `dd137cf7`；`b6e950ee` 全程未碰；**没有 `uninstall`**；APK 用仓库固定 debug keystore 签名；
临时文件（`/sdcard/kivo_p1a.mp4`、`kivo_ui.xml`、`/data/local/tmp` 探针）已清理，
应用私有证据与 media 片源保留。两台同型号，只互证、不是多 SoC 矩阵。

### 残留风险

- CI `hard-gates` 仍未在 GitHub 真实跑过，本轮是本地等价复算；
- 轮 1 的 `-10000` **未做 A/B 基线，不断言与 P0 改动无关**：只能说同一二进制轮 2 完整成功，
  且 P0 改动不触碰 MediaCodec 解码器路径。定因需单独一张卡（片源 × 参数矩阵）；
- 音频侧三处待专业输入，见第八节。

---

## 八、音频核心：需要外部专业输入的三处

用户自研过音乐播放器核心，2026-11-06 主动提出可提供音频核心支持。
Kivo 的音频侧不是"没做"——AAudio 低延迟输出、SPSC 环、音频主时钟、单调守卫、漂移门、
静音栅栏都已落地并有真机数字——**问题是靠经验阈值补洞，没有成体系的时钟恢复设计**。
以下三处最需要成熟音频核心的对照：

| # | 现状（证据） | 想请教的问题 |
| --- | --- | --- |
| A1 主时钟恢复 | `ClockPositionTracker` 的漂移门历经 P13-S→X→Z 多次改判据形状（连续 60 帧 / 累积 45 ms / 30 ms 中台阶 / 启动期豁免 / 只减超出部分）；8 路满载真机仍出现 `clock_ahead +165.58 ms`、`playback_rate 1.021` 而门不响；靠 `ClockOffsetSource` 注入才确定性触发 | 是否应改成 **PLL / 伺服式时钟恢复**（相位误差 + 平滑速率校正），而不是一层层加阈值门？自研音乐播放器核心通常怎么处理"输出钟比墙钟快"？ |
| A2 seek 栅栏 | `audio_playback.cxx:210-212` 用 `sleep_for(10ms)` 等"在途回调退出"，依据写死 "K40 burst 192 帧 ≈ 4 ms"；`stop()` 还用 1 ms 轮询排空 | 想换成**显式在途计数 / generation 栅栏**去掉时序假设；对方在音频核心里用的哪种无缝切换/排队排空方案？ |
| A3 输出协商与设备矩阵 | `aaudio_clock_source.cxx:8` 的 `kDefaultFramesPerBurst=192` 写死，应运行时取 `AAudioStream_getFramesPerBurst`；xrun / 欠载 / 音频焦点 / USB·蓝牙输出切换 / 第二 SoC 矩阵未覆盖；iOS AudioUnit 与桌面 WASAPI 只有接口位 | 跨设备输出协商、xrun 归因、设备切换的成熟做法是什么？哪些常量绝对不能写死？ |

**协作方式**：等音频卡设计稿出来，请对方过一眼 A1/A2 的判据；或现在先给一份自研核心的
「时钟 / 缓冲 / 输出协商」要点做对照。**不要求对方直接改本仓库代码** —— 当前有 P15 主线在跑，
避免撞车。
