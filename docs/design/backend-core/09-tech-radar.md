# 技术选型雷达（主流性判据）

- 版本：V1.0
- 日期：2026-10-28
- 由来：用户明确要求「不要用过时技术栈，不要非主流，必须按大厂标准」。本文是该要求的可执行化。
- 用法：**任何新增技术选型，必须先在本文留下判据，否则不许落代码。**

---

## 一、判据（四条全过才算「主流」）

| # | 判据 | 不合格的样子 |
| --- | --- | --- |
| 1 | **官方支持**：平台厂商提供长期维护与文档 | 社区 fork 的小众方案 |
| 2 | **生态规模**：有大量生产环境使用，问题能被搜到 | 全网找不到三个以上真实案例 |
| 3 | **头部采用**：有知名产品在用，可作对标 | 只有个人 demo |
| 4 | **可替换性**：出问题时有人接手，不是孤岛 | 只有作者一个人懂 |

**反面教材（本项目真实教训）**：PC 端曾被建议用 Rust 写播放核心。
Rust 本身是好语言，但在**多媒体/编解码领域**生态薄弱——FFmpeg 绑定不成熟、
Android NDK 侧无成熟播放器先例、构建链（cargo-ndk）额外引入复杂度。
按判据 2/3 直接出局，后改回 C++。**语言的好坏不等于路线的合适。**

---

## 二、Android 后端技术栈

| 关注点 | 选择 | 头部对标 | 排除的替代方案及原因 |
| --- | --- | --- | --- |
| 语言（核心） | **C++23** | VLC、ijkplayer、ExoPlayer 的 native 层、所有商用播放器内核 | Rust：多媒体生态弱；Go：GC 停顿不可控，无 NDK 播放器先例 |
| 语言（壳层） | **Kotlin** | Google 官方 Android 首选语言 | Java：官方已转向 Kotlin-first；Dart/JS：需跨平台通道 |
| 构建 | **Gradle + AGP** | 官方唯一构建系统 | Bazel：Android 上非主流，只有极少数超大厂内部用 |
| 原生构建 | **CMake + NDK** | Android NDK 官方两种支持方式之一（另一为 ndk-build，已不推荐新项目） | ndk-build：Google 已不建议新项目使用 |
| 硬解 | **MediaCodec（AMediaCodec）** | ExoPlayer、ijkplayer、VLC Android、腾讯/阿里/字节播放器 SDK | 纯 FFmpeg 软解：CPU 与功耗不可接受；Vulkan Video：Android 生态尚未铺开 |
| 解封装 | **MediaExtractor**（首版）→ **FFmpeg libavformat**（演进） | ExoPlayer 默认用 MediaExtractor；VLC/ijkplayer 用 FFmpeg | 自研容器解析：格式覆盖永远追不上，投入产出比极差 |
| 渲染 | **OpenGL ES 3.0 + SurfaceTexture（OES）** | ExoPlayer GL 管线、ijkplayer SDL/GL 渲染 | Vulkan：生态已成熟但复杂度高，首版引入会把「能不能播」和「Vulkan 写得对不对」混为一谈（阶段演进见 07-p12-plan） |
| 上屏容器 | **SurfaceView** | 所有视频类 App 的标准做法 | TextureView：多一次合成，功耗与延迟更高 |
| 音频输出 | **AAudio**（API 26+） | Google 官方低延迟方案 | OpenSL ES：官方已不推荐；AudioTrack（Java）：JNI 往返，延迟更高 |
| 异步/线程 | **自研线程 + 条件变量**（核心）/ Kotlin 协程（壳层） | 播放器内核普遍不用高级调度框架 | std::async / 线程池：播放管线需要确定性时序，不适合通用调度器 |
| 依赖管理 | **Gradle Version Catalog** | Google 官方模板默认 | 手写版本号散落各处 |

---

## 三、明确排除清单（写下来避免反复讨论）

| 方案 | 为什么不用 |
| --- | --- |
| **ExoPlayer / Media3 直接套用** | 目标是「自研核心」，套壳拿不到零拷贝与自定义渲染的控制权；但它的架构可作参考 |
| **Flutter / React Native 承载播放** | 视频帧过跨平台通道，性能与延迟不可控 |
| **Rust 重写核心** | 见第一节反面教材 |
| **Vulkan 作为首版渲染后端** | 复杂度前置，违反「先跑通再优化」；第二阶段再上 |
| **自研容器解析器** | 格式长尾无穷，属于重复造轮子 |
| **任何 `Fake*` 占位实现** | 03-goals.md 红线第 2 条 |

---

## 四、版本基线（2026-10）

| 组件 | 版本 | 说明 |
| --- | --- | --- |
| JDK | 17 (Temurin) | AGP 9 最低要求 |
| Gradle | 9.6.1 | AGP 9.4 要求 ≥ 9.6.0 |
| AGP | 9.4.1 | 当前主线；内置 Kotlin |
| Kotlin | 随 AGP（2.3.x） | 不再单独声明版本 |
| compileSdk / targetSdk | 37（Android 17） | 最新稳定 |
| minSdk | 26（Android 8.0） | `AHardwareBuffer` / `AAudio` 门槛 |
| NDK | r30 (30.0.16248370) | 最新稳定 |
| Build-Tools | 37.0.0 | 与 compileSdk 对齐 |
| C++ 标准 | C++23 | 与桌面端一致 |

> 版本会随官方迭代更新，但**判据不变**：升级前先看是否仍满足第一节四条。

---

## 五、待验证项（不预设结论）

| 项 | 何时验证 | 验证方式 |
| --- | --- | --- |
| Vulkan 是否值得替换 GLES | P12-E 之后 | 同一片源双后端跑分对比，有数字才换 |
| FFmpeg for Android 交叉编译 | P12-F 之前 | 覆盖率对比：MediaExtractor 打不开的格式清单 |
| 16KB page size 兼容 | 发布前 | NDK r30 默认支持，需在 Android 15+ 设备实测 |
