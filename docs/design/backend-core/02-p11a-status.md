# P11-A 实施记录

- 日期：2026-10-28
- 阶段：P11-A（硬件设备与 D3D11VA 解码器）
- 结论：**代码层已落地并通过编译；FFmpeg 链接阻塞已解除（零管理员权限）；D3D11VA 硬解已实测可用**

---

## 一、已落地文件

### 契约层（跨模块，`backend/include/kivo/cinema_engine/video_plane_gpu/`）

| 文件 | 职责 |
|---|---|
| `device/gpu_device_contract.hpp` | GPU 设备抽象描述与不透明句柄 |
| `device/gpu_decode_capability.hpp` | 解码能力集合与不可用原因 |
| `surface/gpu_surface_contract.hpp` | 纹理借用视图（句柄/格式/尺寸/子资源） |
| `frame/gpu_frame_contract.hpp` | GPU 帧（无字节缓冲）+ 色彩信息 + 释放令牌 |
| `decode/hardware_decode_backend.hpp` | 硬解后端纯虚接口 |

铁律校验：5 个头文件均**不含** `<d3d11.h>` / `<libav*>`，第三方类型只以 `void*` 不透明句柄穿越。

### 实现层（`backend/src/video/video_plane/decode_gpu/`）

| 文件 | 职责 |
|---|---|
| `device/dxgi_adapter_selector.cpp` | 枚举适配器、跳过软件适配器、取显存最大者 |
| `device/d3d11_video_device.cpp` | 建带 `VIDEO_SUPPORT` 的设备、查 `ID3D11VideoDevice`、开多线程保护 |
| `hwaccel/ffmpeg_get_format_policy.cpp` | `get_format` 纯策略：只认 `AV_PIX_FMT_D3D11`，否则返回 `NONE` 显式失败 |
| `hwaccel/ffmpeg_hwdevice_binding.cpp` | `AVHWDeviceContext` ↔ `ID3D11Device` 绑定 |
| `hwaccel/ffmpeg_d3d11va_decoder.cpp` | 送包/收帧，产出 `GpuFrameContract`；非 D3D11 帧一律拒收 |

### 私有头文件（`backend/include_private/video/video_plane/decode_gpu/`）

`dxgi_adapter_selector.hpp`、`d3d11_video_device.hpp`、`ffmpeg_get_format_policy.hpp`、
`ffmpeg_hwdevice_binding.hpp`、`ffmpeg_d3d11va_decoder.hpp`

### 测试

`backend/tests/video_plane/decode_gpu/ffmpeg_get_format_policy_tests.cpp`（4 个用例）

---

## 二、验证证据

```text
构建目标 kivo_video_plane_gpu_device  → 成功
build/vs-debug-ffmpeg/backend/Debug/kivo_video_plane_gpu_device.lib   (442,930 B)

构建目标 kivo_video_plane_gpu_hwaccel → 成功
build/vs-debug-ffmpeg/backend/Debug/kivo_video_plane_gpu_hwaccel.lib

构建目标 kivo_video_plane_get_format_policy_tests → 成功
build/vs-debug-ffmpeg/backend/Debug/kivo_video_plane_get_format_policy_tests.exe

运行 → 退出码 0（全 4 用例通过）
```

测试覆盖的关键行为：

1. 硬件格式与软件格式混在一起时，必须选中 `AV_PIX_FMT_D3D11`；
2. 只提供软件格式时，必须返回 `AV_PIX_FMT_NONE`（**拒绝静默降级**）；
3. 空指针输入必须失败关闭；
4. 仅含终止符的列表不得被误判为支持硬解。

---

## 三、原阻塞项：FFmpeg 导入库无读取权限 —— **已解除，且不需要管理员**

### 现象

```text
LINK : fatal error LNK1104: 无法打开文件
"C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1\lib\avformat.lib"
```

### 诊断（精确到文件级）

| 路径 | 读取 | 说明 |
|---|---|---|
| `include/libavcodec/avcodec.h` | ✅ 可读 | 编译期正常 |
| `bin/*.dll`（全部 8 个） | ✅ 可读 | 运行期正常 |
| `lib/avcodec.lib` `avformat.lib` `avutil.lib` `swresample.lib` `swscale.lib` | ❌ 拒绝访问 | 链接期失败 |
| `lib/avdevice.lib` `avfilter.lib` `postproc.lib` | ✅ 可读 | 未被保护 |
| `lib/*.def` `lib/*.exp` | ✅ 可读 | **关键突破口** |

- `lib/` 目录 ACL 含 `Users:(RX)`、`Authenticated Users:(M)`，**目录本身可写**；
- 被拒的 5 个文件带**显式保护 DACL**，`icacls` 连读自己的 ACL 都被拒；
- `icacls /grant` 自助授权失败：当前账户无 `WRITE_DAC`。

### 解法：影子 SDK（零管理员权限）

既然 **DLL 可读、头文件可读**，就不需要原件：

1. `dumpbin /exports <dll>` 取导出表（不需要任何特权）；
2. 生成 `.def`（`LIBRARY xxx-61.dll` + `EXPORTS` + 符号表）；
3. `lib /def /machine:x64 /out:` 生成导入库。

产物落在 `third_party/ffmpeg-sdk/`，`include` 与 `bin` 用 junction 指向原 SDK
（junction 不需要管理员），`lib` 为本地重新生成：

```text
third_party/ffmpeg-sdk/
├── include -> <原 SDK>\include     (junction)
├── bin     -> <原 SDK>\bin         (junction)
├── lib/    avcodec|avformat|avutil|swscale|swresample .def + .lib  (本地生成)
├── bootstrap.cmd                   (幂等重建脚本)
└── README.md
```

生成的导入库与原版**字节数完全一致**：

| 库 | 字节数 | 导出符号 |
|---|---|---|
| avcodec | 40,496 | 168 |
| avformat | 46,990 | 190 |
| avutil | 134,658 | 599 |
| swscale | 8,308 | 31 |
| swresample | 6,432 | 23 |

`CMakePresets.json` 的 `FFMPEG_ROOT` 已改为 `${sourceDir}/third_party/ffmpeg-sdk`。
换机器或 SDK 升级后，跑一次 `bootstrap.cmd "<sdk-root>"` 即可重建。

### 解除证据

```text
cmake --build ... --target kivo_p6c_decode_backend_tests
  -> kivo_p6c_decode_backend_tests.exe   链接成功（原先 LNK1104 消失）

dumpbin /dependents
  -> avcodec-61.dll, avutil-59.dll     真实导入成功

运行 -> 10 项用例 ALL PASS，退出码 0
```

---

## 四、硬解能力实测（本机 Intel UHD Graphics 630）

### 4.1 D3D11VA 真实可用

| 检查项 | 结果 |
|---|---|
| `ffmpeg -hwaccels` | 含 `d3d11va`（`cuda/vaapi/dxva2/qsv/d3d11va/opencl/vulkan/d3d12va`） |
| `-init_hw_device d3d11va` + `hwupload` | 输出 `d3d11(progressive)` 硬件帧，成功 |
| 1080p HEVC Main10 | `Format d3d11 chosen by get_format()` → **硬解初始化成功，无回退** |
| 1080p H.264 High 8bit | 硬解可用 |
| H.264 **4:4:4** | `Failed setup for format d3d11 ... retrying get_format() without it` → **回退软解** |

H.264 4:4:4 回退属**硬件限制**（Intel 不支持该 profile 硬解），不是 SDK 或代码缺陷。
这条现象恰好验证了 `ffmpeg_get_format_policy` 的「拒绝静默降级」策略是必要的：
不显式判定，就会像 CLI 一样悄悄转软解，上层完全无感。

### 4.2 性能对比（1080p HEVC Main10，300 帧，libx265 生成）

| 指标 | 软解 | D3D11VA | 变化 |
|---|---|---|---|
| CPU 时间 | 0.469 s | **0.016 s** | **降 96.6%** |
| 墙钟 | 1.122 s | 0.605 s | 降 46% |
| 峰值内存 | 208 MB | 511 MB | 升 2.4 倍（硬件表面池） |

### 4.3 重要修正：解码**不是** 8fps 的瓶颈

软解 1080p HEVC 10bit 已达 **280 fps（9.34x 实时）**。而 `p10` 交接文档记录的
实测只有 **8.6 fps**。二者相差 30 倍，说明：

> 瓶颈不在解码吞吐，而在**解码之后的呈现路径**
> —— `sws_scale → RGBA → QImage → Qt 贴图` 这段每帧两次 CPU 拷贝。

因此 P11 的推进顺序需调整：**零拷贝上屏（P11-C/D）的优先级高于解码器优化**。
硬解的真正价值不是提升帧率上限，而是**释放 97% 的 CPU**——这部分预算必须
留给 4K、10bit 转换、字幕合成与色调映射。

---

## 五、下一步

1. 硬解探针可执行（`backend/tests/video_plane/decode_gpu/` 下的 real 组）：
   输出设备名、协商纹理格式（NV12/P010）、纯解码帧率，对照 ≥ 100 fps。
2. P11-B：GPU 帧队列 + 表面池（吸收 `gpu_hw` 的 `cross_adapter_ban` / `gpu_budget`）。
3. **P11-C 提前**：flip 交换链零拷贝上屏，先消掉 `sws_scale → RGBA → QImage`。
4. P11-D：呈现节拍器，闭环 8fps 问题。

---

## 六、实施中发现的重要事实（修正前期判断）

### 5.1 P5 视频平面**有**测试，但测的是假对象

`backend/tests/video_plane/` 下存在 p5a~p5s 共 9 个测试程序，均已注册并成功构建。
其中 `kivo_video_plane_d3d11va_p010_tests.exe` 实测：

```text
[G4] Nv12Srv PASS      [G10] CrossAdapterBan PASS
[G5] P010IntegerGate PASS   [G11] SharedTextureGate PASS
...
=== P5G Summary: all 13 exit criteria PASS ===   退出码 0
```

但对应实现在 `include/kivo/video_plane/gpu_hw/`（13 个头文件、619 行），
核心类型名是 **`FakeD3D11vaHwcontext`** —— 全部为模拟实现，不触碰 FFmpeg 与 D3D11。

**结论：这 13 条 PASS 不能作为"硬解已就绪"的证据。** 这是典型的假绿，
会让`证据充分`的表述与真实能力脱节。

### 5.2 存在两套包含根，属结构隐患

| 根目录 | 内容 | 使用者 |
|---|---|---|
| `include/kivo/...` | P5 视频平面契约、`gpu_hw` 假契约族 | p5a~p5s 测试 |
| `backend/include/kivo/cinema_engine/...` | P0~P10 引擎契约 | 主引擎 |

两者都在构建搜索路径上，存在同名头文件歧义风险。P11-A 的新契约放在 `backend/include/`，
未与根 `include/` 混用；建议 P11-G 收口时统一，但**本次不动**，避免破坏既有 9 个测试。

### 5.3 既有假契约族中值得吸收的设计

`gpu_hw` 族虽为假实现，但其契约切分有参考价值，P11-B/C 应予采纳而非重造：

| 既有概念 | 价值 | 对应本计划阶段 |
|---|---|---|
| `cross_adapter_ban` | 禁止跨适配器共享纹理，避免隐式回读 | P11-B |
| `shared_texture_gate` | 共享纹理准入闸 | P11-B |
| `p010_integer_gate` / `p010_unorm_gate` | 10bit 纹理采样语义闸 | P11-E |
| `gpu_budget_runtime` | 显存预算 | P11-B |
| `context_lock_bridge` | 设备上下文锁桥 | P11-C |
| `tier_a_evidence` | 证据分层 | P11-G |
