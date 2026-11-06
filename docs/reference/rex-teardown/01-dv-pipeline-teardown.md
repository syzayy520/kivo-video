# Rex 1.0.2 (iOS) 拆解报告 —— 阶段二核心发现：Dolby Vision 映射的完整实现路线

> 状态：**进行中**（阶段一/二已完成 DV 链路；UI/交互拆解待续）
> 分析者：Agent（2026-11-06）
> 对象：`F:\Files\实用工具\苹果相关\ipaDown20261030\App\syzayy91@gmail.com\Rex_1.0.2.ipa`（244 MB）
> 证据等级标注：【已验证】= 来自包内文件/符号直接读取；【推断】= 有依据的推导；【未知】= 需补充材料

## 〇、一句话结论

**Rex 的杜比视界映射不是自研黑盒，而是"开源组件拼装 + 自研 Metal shader"**：
FFmpeg 内置的 DV RPU 解析器 + libdovi（MIT）解析元数据，KSPlayer（GPL-3.0）的
`DOVIDecoderConfigurationRecord` 承载配置，最后由自研 `DoviMatrix` Metal 着色器
在 GPU 上完成 reshaper + 色彩空间映射。**这套组件在 Android 上全部有对应物。**

## 一、分析条件与工具核验【已验证】

| 项 | 结果 |
| --- | --- |
| rea 版本 | `rea-agents 6.8.0`（本地 clone，`node scripts/rea.mjs` 可运行） |
| rea 深度反编译 | **Windows 上不可用**：Ghidra 需配置且 Win 版只支持 PE（Mach-O 被明确拒绝 `target_format_unsupported`）；Hopper 不支持 win32；IDA/Kuna 未配置 |
| rea Apple 命令 | `inspect-macho` / `inspect-plist` 等全部依赖 macOS 原生工具（`native-macos provider requires macOS`） |
| 实际使用路径 | 直接解压 IPA + NDK `llvm-nm`/`llvm-objdump` + Python 解析 metallib/plist + 上游开源源码比对 |
| 加密状态 | 主二进制 Rex：无加密 load command（已砸壳，可分析）；libdovi/libdav1d/libass 等未加密；**Libavcodec/format/filter/util、libplacebo、AMSMB2 为 FairPlay cryptid=1**（导出表不可读，行为从上游开源版本推断） |

## 二、组件清单【已验证，来源：`Payload/Rex.app/Frameworks/`】

| 组件 | 作用 | 协议 | 加密 |
| --- | --- | --- | --- |
| `Libavcodec/format/filter/util/swresample/swscale` | FFmpeg 全家桶：解复用 + 软解 + 滤镜 | LGPL-2.1+ | 是 |
| `libdovi` | **Dolby Vision RPU 解析**（dovi_tool 的库形态） | MIT | **否** |
| `libplacebo` | GPU 渲染 / tone mapping（mpv 同款引擎） | LGPL-2.1+ | 是 |
| `libdav1d` | AV1 软解 | BSD | 否 |
| `libass` + freetype/fribidi/harfbuzz/fontconfig | ASS/SSA 字幕完整渲染栈 | ISC 等 | 否 |
| `lcms2` | ICC 色彩管理 | MIT | 否 |
| `AMSMB2` | **SMB 网络共享**（Rex 的核心卖点之一） | GPL-3.0 | 是 |
| `libsrt` | SRT 流媒体协议 | MPL-2.0 | 否 |
| `libzvbi` | Teletext 字幕 | GPL-2.0+ | 是 |
| `onnxruntime` | ML 推理（用途【未知】） | MIT | 否 |
| `DoviMatrix.bundle/default.metallib` | **自研 Metal shader（44 KB）** | — | — |

主二进制 90 MB，7275 个符号。UI 依赖：SwiftUI + JXPhotoBrowser（相册浏览）等。

## 三、内核血统【已验证】

主二进制含 Swift 符号 `_$s8KSPlayer13SubtitleModelC...`（KSPlayer 模块名 +
mangled class）。上游 `kingslay/KSPlayer`（1690★，GPL-3.0，描述原文
"support HLG、HDR10、HDR10+、Dolby Vision、Dolby Atmos"）的 xcframework 中
`arm64-apple-ios-simulator.swiftinterface` 导出与 Rex 主二进制引用完全对应：

```
DOVIDecoderConfigurationRecord ×7   isDovi ×4   HDRType ×4   DynamicRange ×18
```

**结论：Rex = KSPlayer 播放内核（GPL-3.0）+ 自研 UI/网盘服务 + 自研 DoviMatrix shader。**
【推断，高置信】Rex 需遵守 GPL-3.0 提供对应源码；App Store 分发与 GPL 的相容性是它的合规风险，也是我们文档层面的差异化点。

KSPlayer 上游结构注意：仓库 `Sources/` 只有 `KSPlayer.xcframework`（预编译二进制）、
`KSPlayerUI`、`MPVPlayer` 三部分——**核心源码不开源，公开的是 Swift API 接口（swiftinterface）与二进制**。

## 四、DV 映射的完整数据流【已验证 + 推断】

```
HEVC/DV 文件
  │  FFmpeg demux（读出 dvcC/dvh1/dvhe box + RPU NAL）
  ▼
DOVIDecoderConfigurationRecord     ← KSPlayer swiftinterface 明确导出
  （dv_profile / dv_level / rpu_present / el_present / bl_present /
    dv_bl_signal_compatibility_id / dv_md_compression）
  │
  ▼
RPU 逐帧解析：
  ff_dovi_rpu_parse   （FFmpeg libavcodec/dovi_rpu.c 内部函数，Rex 主二进制 U 符号【已验证】）
  ff_dovi_get_metadata（RPU → DOVIMetadata：reshaper 曲线）
  av_dovi_find_level  （由 RPU level 选映射曲线）
  ff_dovi_ctx_unref
  │
  ▼
Metal GPU 映射（DoviMatrix default.metallib，44 KB【已验证】）
  shader 函数名（air 符号直读）：
    reshape_data(comp)                    ← DV reshaper：pivots/poly/MMR/NLQ 曲线重映射
    dovi_metadata                         ← DOVIMetadata 的 Metal struct 镜像
      （dm_data/dm_coeffs/mmr/pivot/slo/hi/min_order/max_order/has_poly/has_mmr/
        nlq_offset/nlq_deadzone_slope/nlq_deadzone_threshold/disable_residual/nlq_active）
    yuvToRGBMatrix (+ colorOffset + leftShift)  ← YUV→RGB（按 DV 元数据的矩阵系数）
    toneCurve                             ← 亮度映射曲线
    hlgToPQ                               ← HLG→PQ EOTF 归一
    displayICtCpTexture / displayICtCpBiPlanarTexture   ← ICtCp（DV 中间色彩空间）显示路径
    displayHDRVividYUVTexture / displayHDRVividBiPlanarTexture
    applyVividToneMapping(hdr_vivid_uniforms)           ← HDR Vivid（CUVA 广电标准）同场实现
      hdr_vivid_uniforms = { source_transfer, target_maximum_pq, saturation_c0/c1/exponent,
                             mastering_maximum_pq }
  │
  ▼
CAMetalLayer 显示（HDR 类型按 DynamicRange 切换）
```

### metallib 对比【已验证】

| | Rex DoviMatrix（44 KB） | KSPlayer 上游（51 KB） |
| --- | --- | --- |
| 共享符号 | 26 个（采样/坐标/编译器属性） | 同左 |
| 独有 | **HDR Vivid 三函数、ICtCp×2、reshape_data、toneCurve、hlgToPQ、yuvToRGBMatrix(leftShift)** | yadif 去隔行、ASS 字幕渲染、NV12/YUV 直通显示 |

**结论**：DoviMatrix 不是 KSPlayer metallib 的裁剪版，而是同一渲染层的**映射特化分支**——
去掉了隔行扫描与字幕渲染，加入 DV reshaper、ICtCp 显示路径与 HDR Vivid。

### 未验证项【未知】

- 具体的 tone mapping 曲线公式（在 metallib AIR 字节码内，需 Metal 反编译或运行时观察）；
- libplacebo 在链路中的实际调用点（framework 加密；可能用于 SW 路径或备用 tone mapping）；
- `onnxruntime` 的用途；
- UI 层全部页面/交互（阶段三范围）。

## 五、对 kivo video（Android）的直接启示【设计建议，非参考产品事实】

Android 上这套路线**每个组件都有对应物**：

| iOS（Rex） | Android 等效 |
| --- | --- |
| FFmpeg `ff_dovi_rpu_parse` | 同一个 FFmpeg（LGPL，可交叉编译）；或 libdovi 直接解析 |
| libdovi（MIT） | 原样可用（NDK 交叉编译） |
| Metal shader | **libplacebo 的 Vulkan 后端**（现成 DV→HDR10/SDR 映射，mpv 同款），无需自写 shader |
| CAMetalLayer | SurfaceView + `AHardwareBuffer`/Vulkan swapchain |

关键工程差异（风险）：
1. MediaCodec 的 Surface 输出模式拿不到像素做 GPU 后处理 → 需要 `ImageReader`/ByteBuffer 模式（有性能代价）或接受 libplacebo 全接管解码（FFmpeg 软解）；
2. libplacebo Vulkan 集成（swapchain、HDR 元数据传递）是主要工作量；
3. K40 无 DV 解码器不再是障碍：**软解 + libplacebo 路线不需要 DV 硬解**——这正是 Rex 路线的核心价值。

## 六、覆盖清单

- [x] 包结构 / 加密状态 / 组件清单（完整）
- [x] DV 映射数据流（组件级完整，曲线公式待 Metal 反编译）
- [x] 内核血统（KSPlayer 确认）
- [ ] UI 页面/交互/动效（待阶段三：storyboard 需 macOS 工具，改从 Assets/本地化字符串切入）
- [ ] 服务端机制（【未知】：需抓包或运行观察）
- [ ] 性能实测（【未知】：需真机运行）
