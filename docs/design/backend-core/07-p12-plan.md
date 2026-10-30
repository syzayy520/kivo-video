# P12 Android 后端核心 · 阶段计划

- 版本：V1.0
- 日期：2026-10-28
- 上游：`00-master-plan.md`、`03-goals.md`、`04-platform-strategy.md`
- 定位：本文是 P12 的**唯一验收口径**，与 `03-goals.md` 的 G1–G10 一一对应。

---

## 一、P12 要解决什么

P11 在 Windows 上证明了「硬解 + GPU 帧」这条路走得通（501fps / P010 / CPU 7.66%）。
P12 要把同一套**可移植核心**搬到 Android，并让它在真机上产出同样的证据。

关键约束：**核心逻辑一行都不重写，只换后端。**

若出现"Android 版另写一套队列/时钟/同步"，即为架构失败，必须回退重做。

---

## 二、技术选型（逐项给理由，不给偏好）

### 2.1 构建形态

| 项 | 选择 | 理由 |
| --- | --- | --- |
| 工程位置 | `android/`（仓库根下，与 `backend/` 平级） | 不污染既有 CMake 树 |
| 构建 | **Gradle 9.6.1 + AGP 9.4.1** + `externalNativeBuild` | Android 唯一标准路径；版本线以 2026-10 官方主线为准 |
| Kotlin | **AGP 9 内置 2.3**（不再 apply kotlin 插件） | AGP 9 硬变更，重复 apply 会 duplicate extension |
| C++ 来源 | 直接 add_subdirectory 引用 `backend/` 源码 | 避免复制粘贴，杜绝双份真相 |
| minSdk | **26** | `AHardwareBuffer`/`AAudio` 门槛；覆盖 95%+ 在用设备 |
| targetSdk | **37** | 与 compileSdk 对齐 |
| compileSdk | **37** | 已安装（android-37.0） |
| NDK | **30.0.16248370（r30）** | 与 AGP 9.4 匹配 |
| ABI | 仅 `arm64-v8a` | 真机 64 位；`armeabi-v7a` 留作后续 |

> 版本线在 P12-A 期间已整体升级一次（原 Gradle 8.11.1 / AGP 8.5 / NDK r27c / compileSdk 35 → 现版本）。
> 踩过的 AGP 9 三处硬变更见 `06-android-toolchain.md`。**不要把这里的版本改回旧值。**

### 2.2 解封装

| 项 | 选择 | 理由 |
| --- | --- | --- |
| 首版 | `AMediaExtractor`（NDK） | 零依赖、系统内置、覆盖 MP4/MKV/TS/FLV |
| 演进 | 交叉编译 FFmpeg `libavformat` 替换 | 覆盖 RMVB/冷门容器；接口不变 |

落地方式：`core/source/` 定义 `MediaSource` 纯虚接口，`backends/android/mediaextractor/` 提供实现。
**首版不引入 FFmpeg for Android**，避免在打通管线前陷入交叉编译泥潭；但接口必须一次设计到位。

### 2.3 硬解

| 项 | 选择 | 理由 |
| --- | --- | --- |
| API | `AMediaCodec`（NDK，API 21+） | 无需 JNI 反射，C++ 直连 |
| 工作模式 | **同步模式**（`dequeueInputBuffer` / `dequeueOutputBuffer`） | 与桌面端 submit/receive 三段式同构，可测、可控 |
| 输出目标 | 直接渲染到 `Surface`（`configure(format, surface, ...)`） | 这是 Android 唯一真零拷贝路径 |
| 能力探测 | `AMediaCodec_getCapabilityFor` + Java `MediaCodecInfo`（仅取 profile） | 运行时判定，禁止静态假设 |

### 2.4 渲染

| 项 | 选择 | 理由 |
| --- | --- | --- |
| 首版 | **GLES 3.0 + SurfaceTexture + `GL_TEXTURE_EXTERNAL_OES`** | Android 最成熟的零拷贝采样路径；`samplerExternalOES` 由驱动做 YUV→RGB |
| 演进 | Vulkan（K40 已支持） | 后续做 HDR / 多平面时再上 |
| 呈现 | `SurfaceView` + 独立渲染线程 | 不占主线程，与桌面端 scheduler 同构 |

**为什么首版不用 Vulkan**：Vulkan 在 Android 上要自己管 swapchain / WSI / 队列，
在管线尚未跑通前引入，会把"能不能播"和"Vulkan 写得对不对"两个问题混在一起。
先用 GLES 把零拷贝跑通并测出数字，再换 Vulkan，符合 G9「不许静默降级」——换后端要有对比证据。

### 2.5 音频与时钟

| 项 | 选择 | 理由 |
| --- | --- | --- |
| 音频输出 | **AAudio**（API 26+） | 低延迟，NDK 原生 |
| 主时钟 | 音频时钟驱动（`AAudioStream_getTimestamp`） | G7 要求；与桌面端同构 |
| 首版兜底 | 系统 monotonic 时钟（无音频时） | 保证纯视频也能播 |

---

## 三、阶段划分与闸门

| 阶段 | 内容 | 闸门（可量化） | 状态 |
| --- | --- | --- | --- |
| **P12-A** | 工程骨架 + 能力探测 + 最小硬解上屏 | 编译通过 + 装机运行 + 产出真机能力 JSON | ✅ 已闭环（117 条记录 / 0 失败） |
| **P12-B** | 真实硬解 + 零拷贝 Surface 上屏 | G5（Android 侧）+ 零丢帧不变量 | ✅ 已闭环（三片源 4.25×~5.84× 实时） |
| **P12-C** | AAudio 音频输出 + 音频主时钟 + 呈现节拍 + A/V 同步 | G6 + G7 | ⬅ **当前** |
| P12-D | 分线程流水线 + Surface 重建 + 多实例 + 长稳 | G3 + G4（Android 侧） | 待启 |
| P12-E | 10bit / HDR 链路 | G9 之 Android 部分 | 待启 |
| P12-F | Seek 与容错 + 证据收口 | G8 + G10 | 待启 |

> **C 组内部的细颗粒任务卡在 `docs/handoff/TASKS.md`**（C1–C8），本文只定阶段边界。
> 任务状态以 `TASKS.md` 为准，阶段状态以本文为准，两者不冲突时不必同步。

### P12-C 额外前置（为什么先做音频）

G7 的分母是音频时钟。没有第二个时钟，"什么时候该显示这一帧"只能拿系统时钟硬凑，
那不是 A/V 同步，做完也无法验收。所以 C 组的实现顺序锁死为：
**AAudio 输出 → 音频解码供 PCM → 音频主时钟 → 节拍器 → 分线程 → 精确呈现时刻 → 证据。**
不许跳过前四步直接写 `due_at` 计算。

### P12-A 验收口径（本阶段唯一标准）

必须同时满足，缺一不可：

1. `gradlew assembleDebug` 成功，产出可安装 APK；
2. `adb install` 成功，App 启动不崩溃；
3. 真机运行能力探测，输出结构化 JSON 到 `artifacts/p12/p12a/<serial>.json`；
4. JSON 必须含：设备型号、API 级、支持的 codec 列表、每个 codec 的 profile/level/位深/分辨率上限；
5. 能播放一段自备 MP4，画面正确显示（肉眼可见），且 `logcat` 报告实际协商到的 codec 与输出格式；
6. 全程**不得出现软解回退**：若硬解不可用，必须报错并给出非空 reason（G9）。

> 不满足第 5 条就宣称"管线打通"属于红线第 5 条违规。

---

## 四、Android 端与桌面端的对应关系

| 能力 | 桌面（已实现） | Android（P12） | 是否共用核心 |
| --- | --- | --- | --- |
| 设备/能力探测 | `dxgi_adapter_selector` + `d3d11_decode_capability_probe` | `mediacodec/device/` + `capability_probe` | 契约共用，实现各异 |
| 硬解 | `ffmpeg_d3d11va_decoder` | `mediacodec/decode/` | 契约共用 |
| 表面/纹理 | `dxgi_surface_pool` | `surface/surface_texture_pool` | 契约共用 |
| 帧队列 | `gpu_frame_queue` | 同一份 `core/frame/` | **完全共用** |
| 节拍 | `frame_pacer` | 同一份 `core/pace/` | **完全共用** |
| 同步 | `av_drift_monitor` | 同一份 `core/sync/` | **完全共用** |
| 呈现 | `dxgi_flip_swapchain` | `present/gles_presenter` | 契约共用 |

**判断标准**：`core/` 下出现任何 `#include <android/*>` 或 `<GLES*>` 即为违规，由守卫脚本拦截。

---

## 五、已知风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| 厂商 ROM 篡改 MediaCodec 行为 | 某些 codec 声明支持但实际失败 | 能力探测以**实测 configure 成功**为准，不信 XML 声明 |
| `SurfaceTexture` 首次帧延迟 | 首帧黑屏 | 探测 `onFrameAvailable` 后再算首帧时间 |
| 10bit 走 Surface 路径被降 8bit | G9 违规 | 显式查询 `COLOR_FormatYUVP010`，协商不到即报 reason，不静默接受 NV12 |
| NDK 版本与 AGP 不匹配 | 构建失败 | 固定 NDK r30 (30.0.16248370) + AGP 9.4.1，见 `06-android-toolchain.md` |
| 真机权限（读 SD 卡视频） | 无法测试 | 首版用 assets 内置样片，后续走 SAF |

---

## 六、红线（与 `03-goals.md` 对齐，Android 追加两条）

1. 不许为了让测试变绿而放宽断言。
2. 不许用 `Fake*` 类型充当实现。
3. 不许在呈现路径引入任何形式的像素回读。
4. 不许把多个职责塞进一个文件（单文件 > 400 行即需拆分评审）。
5. 不许在没有实测数字的情况下宣称达标。
6. **新增**：不许在 `core/` 出现任何 Android / GLES 头文件。
7. **新增**：不许用「模拟器能跑」代替「真机能跑」——模拟器无硬件 MediaCodec，不产生有效证据。
