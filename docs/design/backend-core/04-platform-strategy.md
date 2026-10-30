# 平台战略与后端分层（Android 优先）

- 日期：2026-10-28
- 决策依据：用户明确目标顺序 **Android → iOS → 桌面**。
- 影响：**Windows D3D11 不再是"核心"，它只是核心的第一个后端实现。**

---

## 一、核心判断

一个"自研播放核心"的可移植性，取决于**它把什么藏在接口后面**。

- ❌ 错误做法：核心代码里到处是 `ID3D11Texture2D*`、`AVCodecContext*`、`ANativeWindow*`。
- ✅ 正确做法：核心只认"帧、时钟、队列、策略"，平台 API 全部退到后端实现里。

现行契约层（`video_plane_gpu/`）已经做对了：所有 GPU 类型都是 `void*` 不透明句柄，
`HardwareDecodeBackend` 是纯虚接口，头文件里没有一行 `<d3d11.h>` 或 `<libavcodec/>`。
**这套契约可以直接给 Android 用**，这是本次转向最大的一笔资产。

---

## 二、三平台后端对照

| 关注点 | Android（先） | iOS（次） | Windows（后） |
|---|---|---|---|
| 硬解 | **MediaCodec**（`AMediaCodec`，NDK） | **VideoToolbox** | **D3D11VA**（已实现） |
| 输出纹理 | 硬解输出到 **Surface / OES 纹理** | CVPixelBuffer → **Metal 纹理** | D3D11 Texture2D |
| 上屏 | **Vulkan**（首选）或 GLES3 | **Metal** + CAMetalLayer | flip 交换链 |
| 音频 | **AAudio**（低延迟），后退 OpenSL ES | AVAudioEngine / AudioUnit | WASAPI（共享/独占） |
| 主时钟 | 音频时钟（AAudio 时间戳） | 音频时钟 | 音频时钟（QueryPerformanceCounter 校准） |
| 零拷贝要点 | 硬解直接渲染到 `Surface`，不经过 ByteBuffer | `CVPixelBuffer` 零拷贝到 Metal | 纹理直出，不回读 |

**共同点**：三者都是「硬解 → GPU 纹理 → 着色器直接采样 YUV → 合成上屏」。
所以核心的队列、时钟、节拍、同步、seek 逻辑**三端完全共用**。

---

## 三、目录结构（目标形态）

```
backend/src/video/video_plane/
├── core/                      # 可移植核心，禁止出现任何平台 API
│   ├── source/                # 解封装：文件/网络 → 包
│   ├── packet/                # 包队列与背压
│   ├── frame/                 # 帧队列（有界、可归还）
│   ├── clock/                 # 主时钟（音频驱动）
│   ├── pace/                  # 帧节拍器：等/现/丢
│   ├── sync/                  # A/V 同步策略
│   └── session/               # 播放会话状态机、seek 编排
├── backends/
│   ├── d3d11/                 # Windows（当前实现，作为参考后端）
│   ├── mediacodec/            # Android（待建）
│   └── videotoolbox/          # iOS（待建）
└── probe/                     # 跨平台探针与证据产出
```

铁律：`core/` 下任何文件 include 到 `<windows.h>` / `<d3d11.h>` / `<libav*>` /
`<android/*>` / `<CoreVideo/*>` 一律视为架构违规，由守卫脚本拦截。

---

## 四、为什么先做 Windows 后端

不是因为 Windows 重要，而是因为**这台机器只能验证 Windows**：

- 已安装：MSVC 14.51、CMake 4.3.3、FFmpeg 7.1（含 d3d11va）、Qt 6.8.3
- **未安装**：Android SDK / NDK / JDK、adb、gradle（2026-10-28 检查，全部为空）

因此 Windows 后端的角色是**参考实现 + 核心逻辑的验证台**：
先在能跑的地方把队列、时钟、同步、零拷贝做对并测出数字，
再把同一份 `core/` 搬到 MediaCodec 上，只换后端，不重写逻辑。

---

## 五、Android 落地前置条件（需要用户提供）

| 项 | 用途 | 状态 |
|---|---|---|
| Android Studio / SDK（platform 34+） | 编译 APK | ❌ 未安装 |
| NDK r26+ | 编译 C++ 核心 | ❌ 未安装 |
| JDK 17 | Gradle 构建 | ❌ 未安装 |
| 一台真机或模拟器 | 验证硬解与零拷贝 | ❓ 未知 |

**没有 NDK，核心可以在桌面写完并测透，但无法产出 Android 可执行证据。**
这一点不会用"看起来能编译"来冒充。

---

## 六、对当前进度的处置

| 已完成 | 处置 |
|---|---|
| `video_plane_gpu/` 契约（5 头） | **保留，三端共用**，确认无平台类型泄漏 |
| `decode_gpu/device|hwaccel`（10 文件） | 迁移到 `backends/d3d11/`，作为 Windows 后端 |
| get_format 策略单元测试 | 保留；策略本身是跨平台的（只认硬件格式，拒绝软解） |

后续新增代码的归属规则：
- **跨平台逻辑** → `core/`
- **平台 API 调用** → `backends/<platform>/`
- **契约** → `backend/include/kivo/cinema_engine/video_plane_gpu/`（保持零第三方类型）
