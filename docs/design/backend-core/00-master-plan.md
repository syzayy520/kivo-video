# Kivo Video 后端核心设计计划书 V1

- 版本：V1.0
- 日期：2026-10-28
- 项目：`C:\kivo video`（KivoCinemaEngine）
- 基线提交：`2b5cfaa`（2026-07-01，`PASS_CONTINUOUS_PLAYBACK_PUSHED`）
- 适用范围：后端核心解封装/解码/呈现平面重建，不含 UI 视觉改动

---

## 一、定位与目标

### 1.1 产品定位

Windows 私有影院级播放后端核心。对标 Infuse / PotPlayer 的画质与流畅度，不追求功能堆砌，追求**每一帧准时、正确、零妥协**。

### 1.2 本计划要解决的问题

现有后端在 2026-07-01 已达到"能连续播放真实本地/SMB 文件"的里程碑，但**性能与画质不达标**，无法进入商业级。本计划的目标是把后端核心从"能播"推进到"播得好"。

### 1.3 量化目标（验收线，不可协商）

| 指标 | 现状 | V1 目标 |
|---|---|---|
| 1080p HEVC 10bit 帧率 | 8.6–9.9 fps | **≥ 23.976 fps，丢帧 0** |
| 4K HEVC 10bit 60fps 帧率 | 未验证（预计 < 10 fps） | **≥ 58 fps** |
| 解码+呈现 CPU 占用（1080p） | 未测（受软解限制必然高） | **< 15%**（单核口径 < 60%） |
| 首帧时间（SMB 千兆） | 未测 | **< 800 ms** |
| Seek 后首帧 | 未测 | **< 500 ms** |
| A/V 偏差 p99 | 无监控 | **< 40 ms** |
| 10bit 精度 | 转 8bit RGBA 丢失 | **全程 10bit 不落 8bit** |
| HDR10 | 假实现 | **P010 + HDR10 metadata 正确送出** |
| 连续播放 30 分钟 RSS 增长 | 未测 | **< 50 MB，无泄漏** |

---

## 二、现状诊断

### 2.1 真实链路（已核实代码）

```
QML PlayerPage
  → PlaybackShellRuntimeAdapterBridge
  → PlayerRuntimeAdapter
  → P7 PlaybackSession
  → P2 自定义 IO → P5 FFmpeg 解封装
  → LocalVideoRuntime::pump_packets()
      → ffmpeg_session_->decoder.decode_video_rgba(packet)   ← 软解 + sws 转 RGBA（CPU）
      → convert_decoded_frame_to_rgba8888_into()             ← 二次转换/搬运
      → frame_buffer_.rgba8888 = make_shared<vector<uint8_t>> ← 每帧 6.2 MB 堆分配
      → d3d11_upload_.upload(frame)                          ← 上传了，但结果被 (void) 丢弃
  → Qt image surface
```

### 2.2 三个结构性缺陷

**缺陷 A：GPU 帧被迫走 CPU 内存。**

`DecodedFrame::frame_data` 是 `std::vector<uint8_t>`。任何硬件解码帧要进入这个结构，都必须从显存回读到内存——硬解的收益在此处被完全抹平。这不是实现疏忽，是**契约层面的设计缺陷**。

**缺陷 B：像素格式在 CPU 侧就被压成 8bit RGBA。**

`decode_video_rgba()` 强制 `AV_PIX_FMT_RGBA` + `SWS_BILINEAR`。1080p 单帧 sws 转换 + 6.2 MB 拷贝，在 23.976 fps 下每秒搬运 148 MB，且 10bit → 8bit 不可逆。HDR 链路在此彻底断掉。

**缺陷 C：呈现没有节拍。**

`rendered_frame_count` 在解码成功时自增，而非真正 Present 时。当前没有任何帧节拍器、没有 vsync 对齐、没有 A/V 同步消费者。`presenter_boundary_shell.cpp` 只是边界壳，不是呈现器。

### 2.3 结论

现有代码不是"写得不好"，而是**架构成型于"先证明能播"的阶段**，其契约（CPU 字节帧 + RGBA）天然无法承载硬解与 HDR。因此本计划不是优化，而是**在保留 P2/P3/P4/P6/P7 的前提下，重建 P5 视频平面**。

---

## 三、核心设计决策

以下五条为架构级决策，是整个计划的承重墙。任何实现偏离需先修订本文档。

### 决策 1：GPU 帧独立成族，绝不复用 `DecodedFrame`

新增 `GpuFrame` 契约，携带**不透明纹理句柄 + 子资源索引 + 格式 + 时间戳**，不经由 `std::vector<uint8_t>`。

- CPU `DecodedFrame` 保留，仅服务软解回退路径与音频。
- 两条路径严禁混用；类型系统层面不可隐式互转。
- D3D11 具体类型仍不出 adapter 边界（延续现有 `void*` 不透明句柄约定）。

**理由**：只要帧还必须"能变成字节数组"，零拷贝就不可能实现。

### 决策 2：后端自持 flip-model 交换链，不交给 Qt RHI

由后端创建 `IDXGISwapChain`（`DXGI_SWAP_EFFECT_FLIP_DISCARD`，3 buffer），UI 仅提供子窗口 HWND。

**理由**：
1. 帧节拍必须由解码侧掌握，交给合成器就无法做精确的丢帧/等待决策；
2. HDR 需要 `SetColorSpace1` 直接控制交换链色彩空间；
3. 现有 `artifacts/` 证据体系要求后端可脱离 UI 独立运行取证——自持交换链是前提。

**代价**：需处理 resize、device lost、全屏切换。列入 P11-C。

### 决策 3：视频严格跟随音频主时钟

音频是唯一硬时钟。视频帧按 `pts` 与主时钟比较：**早则等待，晚超阈值则丢弃**，绝不允许通过拉长帧间隔"追平"。

- 复用 P6 `audio_plane/runtime/clock` 已有时基，不新建第二时钟。
- 丢弃阈值：常态 1 帧，卡顿恢复期最多连丢至下一个 IDR。

### 决策 4：硬解失败显式降级，禁止静默

降级到现有软解路径时，必须同时：
1. 置 `hardware_decode=false` 写入证据 JSON；
2. 通过 Engine API 暴露能力位供 UI 展示；
3. 记录降级原因（adapter 不支持 / codec profile 不支持 / device lost）。

**理由**：延续项目既有铁律"no silent fallback"，且用户必须知道自己看到的是软解画质。

### 决策 5：像素格式以 GPU 原生为准，YUV→RGB 交给 shader

纹理直接用 `DXGI_FORMAT_NV12` / `P010`，色彩转换在像素着色器中完成。

- 彻底移除呈现路径上的 `sws_scale`。
- 10bit 全程保持，不落 8bit。
- 为 HDR10（PQ/HLG + mastering metadata）预留完整链路。

### 决策 6：与既有 `gpu_hw` 假契约族划清边界，只吸收设计不继承实现

仓库根 `include/kivo/video_plane/gpu_hw/` 已有一套 13 个头、619 行的 D3D11VA / P010 /
适配器拓扑契约，但核心类型为 `FakeD3D11vaHwcontext`，纯模拟，不接触 FFmpeg 与 D3D11。
其 9 个测试（p5a~p5s）全绿，**不构成硬解可用证据**。

处置原则：
1. **不删除、不修改**既有 `gpu_hw` 族与 p5a~p5s 测试，避免破坏现存构建；
2. 新平面 `video_plane_gpu` 直接以真实 FFmpeg + D3D11 实现，不复用 `Fake*` 类型；
3. 仅吸收 `gpu_hw` 的契约切分思想（跨适配器禁止、P010 采样闸、显存预算、上下文锁桥），
   在 P11-B/C/E 落地为真实现；
4. P11-G 收口时再评估：把 p5a~p5s 的验收断言重定向到真实实现，或整体退役。
   在此之前，任何"P5G 13 项 PASS"的表述都必须标注 `fake=true`，不得作为能力宣称。

**理由**：这是本项目此前最大的自欺来源——契约与测试都很齐，唯独没有一行真实硬解。
若新平面继续寄生在 `Fake*` 之上，只会复制同一个错误。

---

## 四、目标架构

### 4.1 分层视图

```
┌──────────────────────────────────────────────┐
│ UI (QML)  —— 只调 Engine API，不碰 FFmpeg/D3D11 │
└───────────────────┬──────────────────────────┘
                    │ Engine API
┌───────────────────▼──────────────────────────┐
│ PlayerRuntimeAdapter / P7 Session            │
└───────────────────┬──────────────────────────┘
                    │
┌───────────────────▼──────────────────────────┐
│ P11 视频平面（本计划新建）                      │
│  decode_gpu ──► frame queue ──► present      │
│      ▲                            ▲          │
│      │                            │          │
│   hwaccel(D3D11VA)          pacing(主时钟)    │
│                                   ▲          │
│                          P6 audio clock       │
└──────────────────────────────────────────────┘
```

### 4.2 数据流（目标态）

```
SMB/Local file
  → P2 custom IO
  → P5 FFmpeg demux (av_read_frame)
  → ffmpeg_d3d11va_decoder  ── GPU 显存 NV12/P010 纹理
  → gpu_frame_queue (有界, 背压)
  → frame_pacer (对比 P6 主时钟)
  → dxgi_flip_swapchain::present
      └─ pixel shader: NV12/P010 → RGB (10bit 保持) → HDR10 color space
```

---

## 五、分阶段实施

每一阶段独立可验证，未通过验收不进入下一阶段。

### P11-A：硬件设备与 D3D11VA 解码器（基础）

**范围**：D3D11 视频设备、D3D11VA 硬解对接、`GpuFrame` 契约、表面池最小实现。

**交付**：
- `video_plane_gpu` 契约族（device / surface / frame / decode / present）
- `d3d11_video_device`：设备创建 + 视频能力查询（支持哪些 codec profile）
- `ffmpeg_hwdevice_binding`：`AVHWDeviceContext` 绑定到 D3D11 设备
- `ffmpeg_get_format_policy`：`get_format` 回调，优先 `AV_PIX_FMT_D3D11`，拒绝隐式 `AV_PIX_FMT_NONE` 回退
- `ffmpeg_d3d11va_decoder`：送包/收帧，输出携带纹理句柄的 `GpuFrame`
- `dxgi_surface_pool`：按分辨率/格式复用纹理，池化上限防显存爆
- 探针可执行：解码 N 帧，输出设备名、codec、帧率、每帧耗时

**验收（必须实测）**：
- [ ] 1080p HEVC 10bit 解码 ≥ 100 fps（纯解码，不含呈现）
- [ ] 输出纹理格式为 `NV12` 或 `P010`，非 RGBA
- [ ] `GpuFrame` 全程无 `std::vector<uint8_t>` 像素拷贝伪影
- [ ] 设备不支持时明确返回能力缺失，不崩溃

> **实测校正（2026-10-28，Intel UHD 630）**
>
> 用本机 FFmpeg CLI 先行测得：1080p HEVC Main10 **软解已达 280 fps（9.34x 实时）**，
> 硬解 CPU 时间从 0.469 s 降至 0.016 s（**−96.6%**）。
>
> 因此两条判断需要修正：
> 1. 「≥100 fps」这条闸门**太松**——软解都能过，它无法区分硬解价值。
>    真正该守的是 **CPU 时间 ≤ 软解的 10%** 与 **显存预算**。
> 2. 「8.6 fps」的瓶颈**不在解码**，而在 `sws_scale → RGBA → QImage` 呈现路径。
>    **P11-C（零拷贝上屏）优先级上调，与 P11-B 并行推进。**

### P11-B：GPU 帧队列与背压

**范围**：有界 GPU 帧队列、表面归还、解码线程与呈现线程解耦。

**验收**：
- [ ] 队列深度可配置，满时解码侧阻塞而非丢帧
- [ ] 呈现侧释放即归还表面，池不增长
- [ ] 30 分钟播放显存占用平稳（增长 < 32 MB）

### P11-C：flip-model 交换链与帧节拍

**范围**：自持交换链、resize、device lost 恢复、按主时钟的帧节拍器。

**验收**：
- [ ] 1080p HEVC 10bit 实际呈现 ≥ 23.976 fps，丢帧 0
- [ ] 4K HEVC 10bit 60fps ≥ 58 fps
- [ ] 窗口 resize / 全屏切换无崩溃、无黑屏超 1 帧
- [ ] 拔掉/重插显示器后 device lost 可恢复

### P11-D：A/V 同步

**范围**：视频时钟消费者、漂移监控、丢帧策略。

**验收**：
- [ ] 30 分钟播放 A/V 偏差 p99 < 40 ms
- [ ] 音频被系统抢占后视频正确暂停而非狂丢帧
- [ ] Seek 后 500 ms 内音画重新对齐

### P11-E：色彩与 HDR10

**范围**：P010 全链路、PQ/HLG 传输函数、mastering display metadata、DXGI 色彩空间切换。

**验收**：
- [ ] HDR10 片源在 HDR 显示器上正确点亮 HDR
- [ ] 在 SDR 显示器上有正确 tone mapping，不灰不曝
- [ ] 10bit 灰阶过渡无 banding（对比 8bit 基准）

### P11-F：字幕平面

**范围**：文本字幕 + PGS 位图字幕渲染，叠加到呈现链。

**验收**：
- [ ] `p8_subtitle_connected=true`
- [ ] PGS 位图正确 blend，无锯齿错位
- [ ] 字幕帧与视频帧同节拍，不同步拖影

### P11-G：稳定性与性能收口

**范围**：长时播放、异常媒体、性能回归测试集。

**验收**：
- [ ] 30 部异构片源（HEVC/H.264/AV1/VC-1，8/10bit，23.976/24/25/50/60）全部可播
- [ ] 损坏文件不崩溃，错误信息可读
- [ ] 全部指标回到 1.3 节验收线

---

## 六、技术选型与理由

| 项 | 选择 | 理由 |
|---|---|---|
| 硬解 API | D3D11VA | Windows 通用，FFmpeg 原生支持，覆盖 NVDEC/Intel QSV/AMD 统一路径 |
| 纹理格式 | NV12 / P010 | GPU 原生支持，零转换；P010 承载 10bit HDR |
| 呈现 | DXGI flip-model + ALLOW_TEARING | 低延迟，可绕开 vsync 做精确节拍 |
| 色彩转换 | 像素着色器 | 消灭 sws CPU 开销，保住 10bit |
| 帧队列 | 有界 + 背压 | 防显存爆，防解码跑飞 |
| 时钟 | 复用 P6 音频时钟 | 单一权威时钟，符合"state has one owner"铁律 |

---

## 七、风险与对策

| 风险 | 影响 | 对策 |
|---|---|---|
| 部分老旧 GPU 不支持 HEVC 10bit 硬解 | 该机器回到软解 | 显式降级 + 能力位上报；保留软解路径 |
| D3D11VA 输出纹理无法直接给自持交换链 | 需一次 GPU 侧拷贝 | 优先同 adapter 同 device；必要时 `CopySubresourceRegion`（仍在显存内） |
| Qt 与后端争用 D3D11 device | 撕裂/卡死 | 后端独立 device，UI 仅提供 HWND |
| device lost（驱动重置、显示器切换） | 播放中断 | P11-C 内实现恢复状态机 |
| HDR 在窗口模式下的限制 | 无法点亮 HDR | 检测并提示切换独占全屏 |
| 与现有 P10 证据体系冲突 | 证据失效 | 新旧路径并存，证据 JSON 增加 `decode_backend` 字段区分 |

---

## 八、范围边界（明确不做）

- 不做 UI 视觉改动、不做播放页布局调整
- 不做网络流媒体、不做杜比视界（DV 单独立项）
- 不改 P2/P3/P4/P6/P7 既有契约
- 不移除现有软解路径（作为回退保留）
- 不做音频独占/位完美（属 P6 独立议题）

---

## 九、文件清单与命名规范

遵循铁律：单文件单职责、禁止平铺、语义目录。

- 契约头文件：`backend/include/kivo/cinema_engine/video_plane_gpu/<子域>/<职责>.hpp`
- 实现文件：`backend/src/video/video_plane/<子域>/<职责>.cpp`
- 测试：`tests/video_plane/<子域>/<职责>_tests.cpp`
- 每个新增模块必须在 `backend/CMakeLists.txt` 以独立 target 注册，禁止并入大 target

详细家族树见 `docs/design/backend-core/01-family-tree.md`。
