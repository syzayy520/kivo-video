# P16-P0 播放核心加固 · 规划设计与文件家族树

- 日期：2026-11-06
- 定位：本文是 **P0 加固的唯一设计事实来源**。门闸顺序：构思 → 规划设计 → 自然家族树 → 实现 → 单文件单职责 → 禁止平铺。**家族树未定不得落代码。**
- 上位来源：`181` 之前的评审结论（后端六维度评估）；`03-goals.md`（G9 失败必须带 reason）、`08-p12-family-tree.md`（家族与职责）、`.codex/memory/kivo-video-iron-gate.md`。
- 工作目录：`C:\kivo video` 既有仓库。**不新建项目包、不新建平行源码目录、不另起一套体系。**
- 验证设备：`dd137cf7`（第二台 K40）。**主测试机 `b6e950ee` 全程不碰** —— 其他助手可能正在其上取证；且严格遵守"不许为了装 APK 去 `adb uninstall`"。

---

## 一、构思：要消灭的四个具体缺陷

| # | 缺陷（评审证据） | 不修的后果 |
| --- | --- | --- |
| D1 | `jni_player_bridge.cxx` 全部 `extern "C"` 导出无 `try/catch`，C++ 侧仍会 `throw`（`spsc_audio_ring_buffer.cxx:34`、`alloc_probe_hook.cxx:67`） | 异常穿过 JNI 帧 → `std::terminate`；HyperOS 上 logcat 取不到，故障表现为"进程静默消失" |
| D2 | `decode_session.cxx:434-447` 三个 `join()` 无超时，注释自承"靠超时也能醒，但那是运气" | 平台调用挂住时 `stop()` 永久阻塞，且没有任何可读原因 |
| D3 | 全局 `operator new` 钩子默认编进产品 APK（`android/app/src/main/cpp/CMakeLists.txt:39-44`），进程级、多实例计数污染 | release 包也背着一个取证用的分配器拦截，G3 的账还读成"解码循环内的分配" |
| D4 | Android 与 `backend/CMakeLists.txt` 两份源文件清单并列，注释三处自承"漏注册 → 桌面全绿、arm64 链接炸"；CI 的 gate 只打印不失败 | 漏注册只能在链接 arm64 时发现；CI 全绿不代表过闸 |

---

## 二、规划设计：四条不可协商的取舍

1. **只做加法，默认行为逐位不变。** 所有新能力默认 OFF 或默认与原路径等价；现有证据字段、schema、判据、真机命令一律不动。理由：仓库当前有他人未提交的 P15 改动，任何口径变更都会污染他们的取证。
2. **失败必须带非空 reason（G9）。** 新增的失败路径一律写进既有的 `last_error` / `DecodeFailureSlot` 通道，不新造一套错误体系。
3. **不引入新线程、不 `detach`、不改 `join` 顺序。** D2 本轮交付的是"有界等待 + 可读原因"，不是"强行打断卡死的平台调用"——后者需要平台调用级超时，属 P1。
4. **新文件只落在既有语义域。** 不新开目录；每个新文件一件事；超过 400 行必须拆（本次无）。

---

## 三、自然家族树（文件落位与单一职责）

> 依据 `08-p12-family-tree.md` §一：`jni/` 放边界、`tools/` 放主机侧校验、`session/` 放会话编排、`core/metrics/` 放度量、`docs/design/backend-core/` 放设计口径。**无新目录。**

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/181-p16-p0-hardening-plan.md` | 新增 | 本次规划、家族树、验收口径 | 不写实现细节、不复制评审全文 |
| `android/app/src/main/cpp/jni/jni_call_guard.hpp` | **新增** | JNI 导出的异常转译：宏 + 返回类型特化 + 错误码常量 | 不含会话状态、不含解码逻辑、不写文件 |
| `android/app/src/main/cpp/jni/jni_player_bridge.cxx` | 改 | Java native 实现 + 为每个导出接上异常边界 | 仍不含任何解码/渲染逻辑；不新增第三个职责 |
| `android/app/src/main/cpp/jni/jni_session_handle.hpp` | 改 | 句柄错误通道的**写入口**声明 | 不改句柄结构、不改收尾计划 |
| `android/app/src/main/cpp/jni/jni_session_handle.cxx` | 改 | 实现该写入口（与既有 `session_last_error` 同族） | 不改 `kSessionDestroyPlan` 七步顺序 |
| `backend/include_private/video/video_plane/backends/mediacodec/session/decode_pipeline_gate.hpp` | 改 | 闸门新增"工作线程退出计数 + 有界等待"两个方法 | 不持有解码器、不碰帧数据 |
| `backend/src/video/video_plane/backends/mediacodec/session/decode_pipeline_gate.cxx` | 改 | 实现上述两个方法 | 不改既有 `wait_for_input_room` 语义 |
| `backend/src/video/video_plane/backends/mediacodec/session/decode_session.cxx` | 改 | `stop()` 接有界等待并在超时写 reason；worker 退出时报数 | 不改 join 顺序、不改证据 schema、不动 `run_merged` 循环体 |
| `backend/src/video/video_plane/core/metrics/alloc_probe_hook.cxx` | 改 | 编译开关控制是否替换全局 `operator new` | 默认 ON 时计数口径逐位不变 |
| `android/app/src/main/cpp/CMakeLists.txt` | 改 | 定义 `KIVO_ENABLE_GLOBAL_ALLOC_HOOK`（默认 ON）并下传编译定义 | 不增删任何既有源文件、不改 target 依赖方向 |
| `android/app/build.gradle.kts` | 改 | release flavor 传 `-DKIVO_ENABLE_GLOBAL_ALLOC_HOOK=OFF` | **不动签名配置、不动 ABI、不动 debug 取证配置** |
| `tools/verify_source_manifest.py` | **新增** | 校验两份 CMake 清单里的共享源文件集合 + 变异自检 | 不 import 项目代码、不判断编译结果、不改任何文件 |
| `.github/workflows/p2-ci-build-test.yml` | 改 | 新增 `hard-gates` job：清单一致性 + core 零平台头，失败即 `exit 1` | 不动 FFmpeg `BLOCKED_ENV` job、不把已知环境阻塞改成红 |

**新增文件共 3 个，全部归位既有目录；改动文件 9 个，均为精确增量编辑。**

### 为什么 D2 不新开文件

"工作线程退没退完"是闸门本来就管的第三件事（停不停 / 空位 / 收工）的同类信号，归 `decode_pipeline_gate`。
新开一个 `stop_watchdog.hpp` 会得到只有两个函数、却要反过来持有 `DecodeSession` 的空壳 —— 与 `三之七`（Surface 重建不新开模块）同一条理由。

### 为什么 D1 不新开 `.cxx`

异常原因的落点是 `SessionHandle::last_error`，它的读取口 `session_last_error` 已在 `jni_session_handle.cxx`。
写入口属于同一族，另起实现文件会造出第二个错误真相。

---

## 四、实现顺序（门闸）

1. 本文落盘（规划设计 + 家族树完成）；
2. D4：`tools/verify_source_manifest.py` → CI `hard-gates`；
3. D1：`jni_call_guard.hpp` → `jni_session_handle.{hpp,cxx}` → `jni_player_bridge.cxx`；
4. D2：`decode_pipeline_gate.{hpp,cxx}` → `decode_session.cxx`；
5. D3：`alloc_probe_hook.cxx` → Android `CMakeLists.txt` → `build.gradle.kts`；
6. 验证（下一节），再写收口记录。

---

## 五、验收与证据路径

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 校验器自检 | `python tools/verify_source_manifest.py --self-test` | 注入的假漏注册必须被抓到；退出码 0 |
| L1 清单一致 | `python tools/verify_source_manifest.py` | 共享族无差集、无指向不存在文件的路径；退出码 0 |
| L1 core 零平台头 | CI `hard-gates` 同一条规则本地复算 | `core/` 与 `core/pipeline/` 下 0 命中 `<android\|media\|aaudio\|GLES\|jni` |
| L2 NDK 独立编译 | 用 NDK 30 的 `android.toolchain.cmake`，`-B build/p0-hardening-cmake`（**独立构建目录，不碰 Gradle 的 `.cxx`**） | configure + build 全过；证明 14 个 target 链接完整 |
| L3 真机冒烟 | `adb -s dd137cf7`，仅此一台；`install -r` 不 `uninstall`；HyperOS 提示按 `347 2111` 点继续 | 起播一段本地片源，确认不崩、`last_error` 为空、证据可写 |

**L3 的前置**：两台同型号（alioth / Android 13），`dd137cf7` 只能验证"改动没破坏播放"，**不能**当作多 SoC 矩阵；矩阵仍须 P1 补异构设备。
**L3 若与其他助手的构建目录冲突，退回 L1+L2 并如实记录，不强推。**

---

## 六、明确不做（本轮边界）

- 不把取证统计移出解码热路径、不加 `-Werror` / ASan / TSan 全量门禁（需要重建全仓测试基线，属 P1）；
- 不实现 PlayerState / 统一 demuxer / 网络流 / 字幕 / DRM / tone mapping（P1–P2）；
- 不强行打断卡死的平台调用、不引入看门狗线程（D2 本轮只交"可观测"）；
- 不改 `backend/CMakeLists.txt`、`docs/handoff/*`、`docs/evidence/*`、`tools/verify_p12_all.py` 等他人正在修改的文件。

---

## 七、实现期更正：共享族的边界（保留原文，不改第二节）

**首版判据把 `backends/{mediacodec,aaudio,avsync}` 也算进"三端共享族"，这是错的。**
首次运行得到 38 条 `only-android`，逐条看全是 Android 平台专用后端——桌面清单本来就不该编它们
（`backend/CMakeLists.txt:2691` 也写明 `mediaformat_color_io` 编不进桌面）。把 38 条豁免掉换全绿，
等于用一张大豁免表把判据本身作废。

更正后的判据（**不是放宽，是缩小到正确的比较域**）：

| 域 | 真相来源 | 判据 |
| --- | --- | --- |
| `core/`（pace / audio / pipeline / seek / metrics） | 两份清单 | 差集为 0；且磁盘上的 `core/**/*.cxx` 必须被**两份**清单都注册 |
| `backends/{mediacodec,aaudio,avsync}` | **磁盘** | Android 清单必须覆盖磁盘上全部 `.cxx`；桌面清单不参与比较 |
| 重复注册 | — | **不判错**：`color_format_names.cxx` / `video_color_facts.cxx` 各出现 2 次，是 `kivo_p12e1_color_tests` 与 `kivo_p12e2_hdr_tests` 两个 target 各自编译同一份纯逻辑源，CMake 合法用法；改为逐条打印供人工看 |

这样校验器防的正是 Android 注释里反复出现的那类事故：新增 `core/pace/*.cxx` 只加到一份清单，
或新增 `backends/**/*.cxx` 忘了在 Android 清单注册。

---

## 八、实现期更正 2：异常边界用函数 try 块，不许把文件撑过 300 行

首版用普通 `try { ... } catch`，17 个导出各加两行，`jni_player_bridge.cxx` 从 302 → **322 行**，
越过家族树 §四 的 300 行线。虽然增加的全是边界样板、不是业务逻辑，但仍是破坏单文件纪律，
不接受「多几行没关系」这种解释。

改为 C++ **函数 try 块**：`KIVO_JNI_BEGIN` 贴在函数签名行末，`KIVO_JNI_END_*` 替换原来的 `}`，
一对宏不占任何一行。同时把 `to_string_vector` 归位到 `jni_string_convert.hpp`（它本就是字符串转换族）。
结果 **287 行**，比改造前还少 15 行。

编译期抓到的两个真问题（保留，别再踩）：
1. 函数 try 块的 **catch 结束即函数定义结束，不需要额外的 `}`**。首版宏多写了一个，
   NDK clang 连报 17 个 `extraneous closing brace`；
2. catch 只能看见**函数参数**，看不见函数体局部变量 —— 句柄必须由
   `KIVO_JNI_SESSION(handle)` 从 `jlong` 参数重新还原，不能引用函数体里的 `session`。

---

## 九、收口记录（2026-11-06）

| 层 | 命令 / 路径 | 结果 |
| --- | --- | --- |
| L1 | `python tools/verify_source_manifest.py --self-test` | 8 条变异全 PASS |
| L1 | `python tools/verify_source_manifest.py` | PASS，磁盘 core 36 / Android 后端 47，双清单 83 / 45，**豁免 0 条**，2 条重复注册只提示不判错 |
| L1 | CI `hard-gates` 同一规则本地复算 | `core/` 平台头 **0 命中** |
| L2 | NDK 30 / arm64-v8a / android-26 / Ninja，默认 ON | **104/104 步全过**，`libkivo_player.so` 链接成功 |
| L2 | 同一套 `-DKIVO_ENABLE_GLOBAL_ALLOC_HOOK=OFF` | **104/104 步全过**，`libkivo_player.so` 链接成功 |
| L2 | clang `-fsyntax-only -Wall -Wextra -Wpedantic -Wshadow` | 0 错误 0 警告（JNI / gate / session / alloc hook 的 ON 与 OFF） |
| L3 | `adb -s dd137cf7`，`kivo_color_probe` + `probe_static_hdr10.mp4` | 输出合法 `kivo.p12e.color-probe/1`；三次 configure 全 ok、`format_after_start=21`，与主设备历史结论一致；退出码 0；临时文件已清理 |

**设备纪律**：全程只操作 `dd137cf7`；主测试机 `b6e950ee` 一次未碰；未执行 `adb uninstall`；
未构建/安装 APK（Gradle 共享构建目录，主动与他人的 P15 取证错峰）。两台同为 alioth / Android 13，
只能**互证**，不构成多 SoC 矩阵。

**文件纪律**：新增 3 个文件，全部落在既有目录（`docs/design/backend-core/`、`jni/`、`tools/`），
**未新建任何目录或项目包**；修改 12 个既有文件，均为精确增量；`jni_player_bridge.cxx` 287 行 < 300。

**明确未做 / 残留风险**：
- CI `hard-gates` 尚未在 GitHub 上真实跑过，本轮只做本地等价复算；`push` 分支过滤已补 `codex/**`。
- 取证统计仍侵入解码热路径；`-Werror` / ASan / TSan / 覆盖率全量门禁未加（P1）。
- D2 只交「有界等待 + 可读 reason」，**不能解除卡死的平台调用**；需要平台调用级超时 + 看门狗（P1）。
- APK 级播放冒烟、PlayerState / 统一 demuxer / 网络流 / 字幕 / DRM / tone mapping、异构 SoC 矩阵（P1–P2）。
- OFF flavor 的 `hook_installed=false`，该构建**不得**用于产出 G3 证据；这条约束目前只写在代码注释与设计文档里，
  尚未由校验器钉死（建议 P1 给 `verify_p14a_metrics.py` 加一条前置断言）。
- 工作区存在他人未提交的 P15-A2/B 改动；本次未触碰 `backend/CMakeLists.txt`、`CURRENT.md`、
  `TASKS.md`、`docs/evidence/p15a/*`、`tools/verify_p12_*` 等任何一处。
