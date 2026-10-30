# P11-A2 实施记录：真实片源硬解探针

- 日期：2026-10-28
- 阶段：P11-A2（自研解码器跑真实媒体）
- 结论：**PASS — 自研 D3D11VA 解码器在 Intel UHD 630 上跑通 1080p HEVC 10bit，300/300 帧，P010 保真，443 fps**

---

## 一、为什么要做这一步

P11-A 只证明"能编译"。P5G 的 13 项"全 PASS"是 `Fake*` 类的假绿，
所以必须有一个**用自己写的解码器、跑真实文件、出真实数字**的探针。
没有它，后面所有阶段都建在沙子上。

---

## 二、本轮新增文件（13 个，单文件单职责）

| 文件 | 职责 |
|---|---|
| `include_private/.../probe/probe_options.hpp` + `src/.../probe/probe_options.cpp` | 命令行解析 |
| `include_private/.../probe/probe_clock.hpp` | 墙钟 / CPU 周期 / CPU 时间三类计时 |
| `include_private/.../probe/media_source.hpp` + `src/.../probe/media_source.cpp` | 解封装：文件 → 视频包（FFmpeg 类型不外泄） |
| `include_private/.../probe/probe_report.hpp` + `src/.../probe/probe_report.cpp` | 指标结构、百分位、JSON 证据、控制台格式 |
| `include_private/.../probe/decode_probe.hpp` + `src/.../probe/decode_probe.cpp` | 硬解会话主循环 |
| `include_private/.../probe/software_baseline.hpp` + `src/.../probe/software_baseline.cpp` | 软件解码对照（给硬解一个分母） |
| `src/.../probe/main.cpp` | 装配与输出 |

构建目标：`kivo_hw_decode_probe`（链接 `kivo_video_plane_gpu_hwaccel` + FFmpeg 导入库）。

---

## 三、过程中修掉的两个真 bug（都不是编译问题，是设计问题）

### 3.1 EAGAIN 被当成成功 → 静默丢包

原接口把「送包」和「收帧」绑成一个 `decode_packet()`。
`avcodec_send_packet` 返回 `EAGAIN` 时包**并未被消费**，原实现直接丢弃。

现象：

```text
[hevc] Could not find ref with POC 6
[hevc] hardware accelerator failed to decode picture
decoded frames : 4   from 15 packets
```

**修复**：契约层拆成 `submit_packet()` / `receive_frame()` / `submit_end_of_stream()`，
与 D3D11VA、MediaCodec、VideoToolbox 的真实语义一致（三端都需要这个形状）。
`NeedsDrain` 明确表示"包未被消费，请先排空再重投"。

结果：4 帧 → 86 帧，错误消失。

### 3.2 单缓冲复用导致在途包被覆盖

送包后解码器仍持有该缓冲的引用；下一包直接写同一块内存，破坏了尚未解码的数据。

现象：第 97 包 `avcodec_send_packet failed: -1094995529`（`AVERROR_INVALIDDATA`）。

**修复**：改为 24 个 staging 槽位，只复用 `av_buffer_is_writable()` 为真的槽；
全部占用时返回 `NeedsDrain`，形成天然背压，而不是分配或破坏。
稳态下提交路径仍然零分配（只做引用计数增减）。

结果：86 帧 → **300/300 帧**。

### 3.3 每帧哈希表 + `av_frame_alloc/free`（G2 未达标的真因）

首轮达标测量 CPU 比值 **11.31%**，超过 10% 门槛。拆开看：分子里除了 GPU 解码，
还有 `unordered_map` 的每帧节点分配/查找，以及 `av_frame_alloc` + `av_frame_free` 的每帧堆操作。

**修复**：改为 64 槽固定表 + 常驻 `AVFrame` 对象，
释放变成一次 `in_use = false` 写入，帧对象跨整个流复用。

结果：CPU 周期 **1.29e9 → 0.83e9（−36%）**，比值 **11.31% → 7.66%**，G2 转为达标。

---

## 四、实测数据（1080p HEVC Main10，300 帧）

| 指标 | 硬解 D3D11VA | 软解 | 对比 |
|---|---|---|---|
| 解码帧数 | **300 / 300** | 300 / 300 | 一致 |
| 帧率 | **501.52 fps** | 98.92 fps | **5.07x** |
| 墙钟 | 598 ms | 3,033 ms | 降 80% |
| CPU 周期 | **0.83e9** | 10.79e9 | **7.66%** |
| 像素格式 | **P010** | yuv420p10le | 十位均保真 |
| 设备 | Intel(R) UHD Graphics 630 | CPU | — |
| 提交成本 p99 | 0.071 ms | 9.932 ms | CPU 侧开销 |

证据文件：`artifacts/p11/p11a2/hevc10_1080p.json`

### 为什么用 CPU 周期而不是 CPU 时间

本机 `GetProcessTimes` 明显失真：软件解码墙钟 3.1 秒，却只报 62 ms CPU，
且数值被量化成 15.6 ms 的整数倍、不随负载线性增长。
改用 `QueryProcessCycleTime` 后，周期数随帧数线性变化，且**比值与 CPU 频率无关**，
才可以作为证据。CPU 时间字段保留但不再作为验收依据。

---

## 五、闸门评估

| 目标 | 门槛 | 实测 | 结论 |
|---|---|---|---|
| G1 硬解吞吐与保真 | 1080p HEVC10 ≥ 250 fps 且 P010 | **501.5 fps / P010** | ✅ 通过 |
| G2 硬解 CPU 效率 | ≤ 软解 10% 周期 | **7.66%** | ✅ 通过（3.3 修复后） |
| G10 证据产物 | 可复核 JSON | `artifacts/p11/p11a2/hevc10_1080p.json` | ✅ 通过 |
| G3 稳态零分配 | 热路径 0 次堆分配 | 提交路径已零分配；帧表已池化，尚未插桩计数 | ⏸ 待插桩 |
| G4 表面池不泄漏 | leaked = 0 | 纹理池尚未实现 | ⏸ 待 P11-B |
| G5 零拷贝上屏 | CPU 拷贝 = 0 | 呈现路径尚未接入 | ⏸ 待 P11-C |

**G2 未达标的原因分析**（已修复，保留过程记录）：11.31% 的分母只有解码，分子却包含解封装、
包拷贝、`unordered_map` 帧表、`av_frame_alloc/free`。3.3 节的槽位池把后两项消掉后降到 7.66%。
不达标就如实记录，不调整门槛。

---

## 六、下一步

1. **P11-B**：表面池 + 有界帧队列；同时把 `live_frames_` 的 `unordered_map`
   换成固定容量池，直接针对 G2 缺口。
2. **P11-C**：零拷贝上屏，消掉 `sws_scale → RGBA → QImage`（8fps 的真凶）。
3. 三端共用：本轮 `submit/receive` 契约形状可直接映射到 MediaCodec 的
   `queueInputBuffer` / `dequeueOutputBuffer`。

---

## 七、复现命令

```bash
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_hw_decode_probe -j 1

export PATH="/c/kivo video/third_party/ffmpeg-sdk/bin:$PATH"
./build/vs-debug-ffmpeg/backend/Debug/kivo_hw_decode_probe.exe \
  --input tests/real/probe_1080p_hevc10.mp4 \
  --frames 300 --compare-software \
  --evidence artifacts/p11/p11a2/hevc10_1080p.json
```

> 构建坑：必须加 `-j 1`。VS 生成器并行编译时多个 cl.exe 会争抢
> `kivo_hw_decode_probe.dir/Debug/.d.json`，报 `C1083 Permission denied`。
