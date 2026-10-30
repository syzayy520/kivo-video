# P11 视频平面 自然家族树

- 版本：V1.0
- 日期：2026-10-28
- 关联：`docs/design/backend-core/00-master-plan.md`
- 命名约定：目录 = 语义域，文件 = 单一职责动词/名词，`snake_case`

---

## 一、设计原则

1. **同一家人住一起**：设备相关只进 `device/`，纹理相关只进 `surface/`，呈现相关只进 `present/`。
2. **长出新职责才分家**：不预先造空目录，某子域文件数 ≥ 3 且职责可再分时才开子目录。
3. **一个文件一件事**：文件名即职责说明，读文件名应能推断内容。
4. **契约与实现分离**：`include/` 只放契约，`src/` 只放实现，契约不得反向依赖实现。

---

## 二、契约族：`backend/include/kivo/cinema_engine/video_plane_gpu/`

```
video_plane_gpu/
├── device/
│   ├── gpu_device_contract.hpp          # GPU 设备契约：类型、适配器信息、生命周期
│   └── gpu_decode_capability.hpp        # 解码能力契约：codec/profile/位深/分辨率上限
├── surface/
│   └── gpu_surface_contract.hpp         # 表面契约：不透明纹理句柄、格式、尺寸、子资源
├── frame/
│   └── gpu_frame_contract.hpp           # GPU 帧契约：表面引用 + pts + 时长 + 色彩信息
├── decode/
│   └── hardware_decode_backend.hpp      # 硬解后端接口：配置/送包/收帧/刷新/重置
└── present/
    └── present_contract.hpp             # 呈现契约：交换链描述、呈现结果、呈现统计
```

### 文件职责表

| 文件 | 唯一职责 | 明确不做 |
|---|---|---|
| `gpu_device_contract.hpp` | 定义 GPU 设备的抽象描述与不透明设备句柄 | 不包含任何 D3D11 类型 |
| `gpu_decode_capability.hpp` | 描述设备/解码器支持的能力集合 | 不做能力探测（探测在实现层） |
| `gpu_surface_contract.hpp` | 描述一块 GPU 纹理（句柄/格式/尺寸/子资源） | 不管理生命周期 |
| `gpu_frame_contract.hpp` | 描述一帧 GPU 画面及其时间戳与色彩属性 | 不持有池、不做队列 |
| `hardware_decode_backend.hpp` | 定义硬解后端纯虚接口 | 不含 FFmpeg 头文件 |
| `present_contract.hpp` | 定义呈现面描述与呈现统计 | 不含 DXGI 类型 |

**铁律校验**：契约层零 `#include <d3d11.h>`、零 `#include <libavcodec/...>`。第三方类型只能以 `void*` 不透明句柄穿越边界。

---

## 三、实现族：`backend/src/video/video_plane/`

```
video_plane/
├── decode_gpu/                          # 硬件解码平面（P11-A）
│   ├── device/
│   │   ├── dxgi_adapter_selector.cpp    # 枚举/挑选适配器（优先独显、排除 WARP）
│   │   └── d3d11_video_device.cpp       # 创建带 D3D11_CREATE_DEVICE_VIDEO_SUPPORT 的设备
│   ├── hwaccel/
│   │   ├── ffmpeg_hwdevice_binding.cpp  # AVHWDeviceContext ↔ ID3D11Device 双向绑定
│   │   ├── ffmpeg_get_format_policy.cpp # get_format 回调策略：优先 D3D11，显式拒绝静默回退
│   │   └── ffmpeg_d3d11va_decoder.cpp   # 送包/收帧主循环，输出 GpuFrame
│   ├── surface/
│   │   ├── dxgi_surface_pool.cpp        # 纹理池：按 (宽,高,格式) 复用，上限保护
│   │   └── surface_lifetime_ledger.cpp  # 表面借出/归还账本，泄漏检测
│   ├── frame/
│   │   ├── gpu_frame.cpp                # GpuFrame 构造与校验
│   │   └── gpu_frame_queue.cpp          # 有界 GPU 帧队列 + 背压信号
│   └── capability/
│       └── d3d11_decode_capability_probe.cpp  # 查询 codec profile 硬解支持
├── present/                             # 呈现平面（P11-C）
│   ├── swapchain/
│   │   ├── dxgi_flip_swapchain.cpp      # flip-model 交换链创建/呈现/resize
│   │   └── swapchain_present_policy.cpp # 呈现参数策略（vsync/tearing/buffer 数）
│   ├── pacing/
│   │   ├── frame_pacer.cpp              # 单帧节拍判定：等/现/丢
│   │   └── presentation_scheduler.cpp   # 呈现调度循环，串联 pacer 与 swapchain
│   ├── shader/
│   │   └── yuv_to_rgb_ps.cpp            # 像素着色器：NV12/P010 → RGB（保 10bit）
│   └── hdr/
│       └── dxgi_color_space_policy.cpp  # DXGI 色彩空间选择与 HDR10 metadata 送出
├── sync/                                # A/V 同步（P11-D）
│   ├── video_clock_consumer.cpp         # 从 P6 音频时钟取时基
│   └── av_drift_monitor.cpp             # 漂移统计与告警
└── runtime/
    ├── local_media/                     # 【既有】软解回退路径，不改契约
    └── hardware_media/                  # 硬解运行时（P11-A 末尾接通）
        ├── hardware_video_runtime.cpp   # 硬解播放运行时：串起 decode→queue→present
        └── hardware_video_stats.cpp     # 运行时统计：帧数/丢帧/耗时/后端标识
```

### 文件职责表

| 文件 | 唯一职责 | 允许依赖 |
|---|---|---|
| `dxgi_adapter_selector.cpp` | 枚举适配器并按策略挑选 | DXGI |
| `d3d11_video_device.cpp` | 创建设备并查询视频能力 | D3D11, DXGI |
| `ffmpeg_hwdevice_binding.cpp` | 建立 FFmpeg 与 D3D11 设备绑定 | FFmpeg, D3D11 |
| `ffmpeg_get_format_policy.cpp` | 决定 `get_format` 返回哪个像素格式 | FFmpeg |
| `ffmpeg_d3d11va_decoder.cpp` | 解码主循环，产出 `GpuFrame` | FFmpeg, 上述三者 |
| `dxgi_surface_pool.cpp` | 纹理分配与复用 | D3D11 |
| `surface_lifetime_ledger.cpp` | 记录借出/归还 | 无（纯记账） |
| `gpu_frame.cpp` | 帧对象构造与不变量校验 | 契约层 |
| `gpu_frame_queue.cpp` | 有界队列与背压 | 契约层 |
| `d3d11_decode_capability_probe.cpp` | 探测 codec 支持矩阵 | FFmpeg, D3D11 |
| `dxgi_flip_swapchain.cpp` | 交换链生命周期与 Present | DXGI |
| `swapchain_present_policy.cpp` | 呈现参数决策 | 无（纯策略） |
| `frame_pacer.cpp` | 单帧该等/该现/该丢 | 契约层 |
| `presentation_scheduler.cpp` | 调度循环 | pacer, swapchain |
| `yuv_to_rgb_ps.cpp` | 着色器源码与编译 | D3DCompile |
| `dxgi_color_space_policy.cpp` | 色彩空间与 HDR 元数据 | DXGI |
| `video_clock_consumer.cpp` | 读取 P6 时钟 | P6 时钟契约 |
| `av_drift_monitor.cpp` | 漂移统计 | 无 |
| `hardware_video_runtime.cpp` | 运行时编排 | 上述全部 |
| `hardware_video_stats.cpp` | 统计聚合与证据输出 | 契约层 |

---

## 四、测试族：`tests/video_plane/`

```
tests/video_plane/
├── decode_gpu/
│   ├── d3d11_video_device_tests.cpp     # 设备创建/能力查询
│   ├── ffmpeg_get_format_policy_tests.cpp # 格式策略：必须选 D3D11，不得静默回退
│   ├── gpu_frame_tests.cpp              # 帧不变量
│   └── gpu_frame_queue_tests.cpp        # 背压与容量
├── present/
│   ├── frame_pacer_tests.cpp            # 等/现/丢 判定矩阵
│   └── swapchain_present_policy_tests.cpp
└── real/
    └── hardware_decode_probe_tests.cpp  # 真实片源硬解探针（需真实媒体开关）
```

现有 `tests/video_plane/` 已有 `allocation/ capability/ evidence/ failure/ queue/ recovery/ sandbox/ stamp/ thread/ threat/ version/` 共 11 个子域，本次新增 `decode_gpu/`、`present/`、`real/`，**不修改既有子域**。

> 路径基准修正（实施时发现）：`backend/CMakeLists.txt` 的相对路径以 `backend/` 为根，
> 因此测试必须落在 `backend/tests/video_plane/`，而非仓库根的 `tests/video_plane/`。
> 仓库根 `tests/video_plane/` 的 11 个子域目前**全部为空目录**，没有任何 .cpp。

---

## 五、构建注册

每个子域一个独立静态库 target，禁止并入大 target：

| target | 源 | 说明 | P11-A 状态 |
|---|---|---|---|
| `kivo_video_plane_gpu_device` | `decode_gpu/device/` | 设备与适配器 | ✅ 已建、已编译 |
| `kivo_video_plane_gpu_hwaccel` | `decode_gpu/hwaccel/` | FFmpeg 硬解对接 | ✅ 已建、已编译 |
| `kivo_video_plane_gpu_surface` | `decode_gpu/surface/` | 纹理池与账本 | ⏳ P11-B |
| `kivo_video_plane_gpu_frame` | `decode_gpu/frame/` | 帧与队列 | ⏳ P11-B |
| `kivo_video_plane_present` | `present/` | 交换链与节拍 | ⏳ P11-C |
| `kivo_video_plane_sync` | `sync/` | A/V 同步 | ⏳ P11-D |

依赖方向单向：`frame → surface → device`，`hwaccel → device`，`present → frame`。**禁止反向依赖**。

> 内部（私有）头文件遵循项目既有约定，放在 `backend/include_private/video/...`，
> 与源码 `backend/src/video/...` 路径镜像对应；`backend/include/kivo/cinema_engine/...`
> 只放跨模块契约。**不要把私有头文件与 .cpp 并排放在 src 下。**

---

## 六、门闸自检

| 门闸 | 状态 | 说明 |
|---|---|---|
| 1. 构思 | ✅ | 见 master-plan 第二、三节 |
| 2. 规划设计 | ✅ | 见 master-plan 全篇 |
| 3. 自然家族树 | ✅ | 本文档 |
| 4. 实现 | ⏳ | 按 P11-A → P11-G 推进 |
| 5. 单文件单职责 | ✅ | 见两张职责表 |
| 6. 禁止平铺 | ✅ | 最深 4 层，无根目录散落文件 |
