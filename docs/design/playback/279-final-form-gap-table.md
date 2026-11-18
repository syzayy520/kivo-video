# 279 · 最终形态：对齐并超过 SenPlayer —— 差距表与执行清单

> 目标（用户 2026-11-15 05:13 原话）："最终我们的播放器形态需要和 senplayer 对齐，
> 所以必须按照最强的去做，不要怕麻烦，目标就是最好，能超过 senplayer 是我们的目标。"
>
> 本文件是**差距表**（我们 vs SenPlayer 的逐项硬参数对比）与**执行顺序**，
> 不是愿景。每一条都有二进制/源码/真机证据。

## 一、逐项硬参数对比（全部取证，非推测）

| 能力 | SenPlayer | 我们（278 之后） | 差距 |
| --- | --- | --- | --- |
| 内核 | mpv（`libmpv` 字符串命中） | mpv 0.39 + libplacebo v7.391 | **无**（同代） |
| 视频输出通路 | **vo=gpu-next**（`gpu-next` 字符串命中） | **vo=gpu-next（默认）** | **已对齐** |
| libplacebo 渲染器 | 61 个 `pl_*` 符号 | 完整（`pl_dispatch_compute` / `pl_peak_detect` / `pl_render_image` / `pl_shader_*` / `pl_icc_*` / `pl_custom_lut` / `pl_film_grain_params`） | **已对齐**（我们版本更高） |
| compute shader | 有 | 有（`GL_VERSION='OpenGL ES 3.2'`，ES3 补丁生效） | **已对齐** |
| HDR 峰值计算 | 有（`pl_peak_detect`） | **有**（"Disabling HDR peak computation" 已消失） | **已对齐** |
| 色彩可配置项 | tone-mapping / inverse-tone-mapping / target-contrast / gamut-clipping / hdr-reference-white / bt.2446a / spline / st2094-40 / perceptual / Hable / `PL_HDR_PQ` | 同代 libplacebo ⇒ 同样具备 | **已对齐** |
| **gpu-next 下的硬解** | 库里**没有** `aimagereader` ⇒ 只有 `mediacodec`/`mediacodec-copy` ⇒ **帧要拷回 CPU** | **aimagereader 零拷贝** | **★ 我们更强** |
| Android HDR 输出 | 有 `ANativeWindow_setBuffersDataSpace`（dataspace 路线） | 已有 L2（映射），**真 PQ 输出未做** | **待补（P2）** |
| 杜比视界 | `dovi_split` bsf + `dovi-fel-hwdec` + `dolby-vision-profile` | ffmpeg 有 `av_dovi_metadata_alloc`/`av_dovi_alloc`（RPU 元数据）但**无 `dovi_split` bsf**；mpv 侧无 dovi 通路 | **待补（P3）** |
| 字幕 | libass | libass（同一代） | 无 |

## 二、已完成的（278 及之前）

- **默认出画通路 = vo=gpu-next**（与 SenPlayer 同架构），三层回退（系统属性 / intent / 设置项）。
- **ES3 补丁**：给 mpv `egl_helpers.c` 打的补丁（config/context 双改 + 回退 ES2），
  脚本入库 `android/app/buildscripts/mpv/patch-mpv-es3.py`，任何人可复现。
- **aimagereader 零拷贝**硬解（gpu-next 必需；零拷贝 ⇒ 比 SenPlayer 少一次 CPU 回拷）。
- **HDR→SDR 色调映射（L2）**：`playback/MpvHdrToneMap.kt`，读内核 `video-params`，
  `gamma=pq/hlg` 才下发 BT.2390；SDR 片源零影响（102.3 → 102.2）。
- **上屏帧率量具**：`tools/p274_sf_latency.{mjs,sh}`（SurfaceFlinger latency），
  与出画通路无关，两条路同一把尺子。

## 三、待做（按"收益/风险"排序）

### P2 · 真 HDR 输出（面板收到 PQ/BT.2020）

**四层，缺一不可**：

| 层 | 谁做 | 怎么做 | 现状 |
| --- | --- | --- | --- |
| ① Surface buffer = 10bit | 我们（Kotlin） | `SurfaceHolder.setFormat(PixelFormat.RGBA_1010102)` | 未做 |
| ② Surface dataspace = BT2020_PQ | 我们（Kotlin 或 native） | `ANativeWindow_setBuffersDataSpace(…, ADATASPACE_BT2020_PQ)` | 未做 |
| ③ EGL surface colorspace = BT2020_PQ | **mpv（要打补丁）** | `eglCreateWindowSurface` 的 attribs 加 `EGL_GL_COLORSPACE_KHR = EGL_GL_COLORSPACE_BT2020_PQ` | 未做 |
| ④ libplacebo 输出 PQ 码值 | mpv 参数（**不用改**） | `target-trc=pq` + `target-prim=bt.2020` + `target-colorspace-hint=yes` | 未做 |

**已确认可行**：
- 设备 EGL 有 `EGL_EXT_gl_colorspace_bt2020_pq` + `EGL_KHR_gl_colorspace`（真机 dumpsys 取证）。
- 设备屏 `supportedColorModes=[0,7,9]`（含 HDR=9）、`mSupportedHdrTypes=[1,2,3,4]`、峰值 420 nit。
- libplacebo 支持 `pq` transfer（库内 128 处 `pq` 字符串）；`pl_swapchain_frame.color_space` 会回报目标空间。
- mpv `vo_gpu_next` 支持 `target-colorspace-hint`（走 `pl_swapchain_colorspace_hint`）+ `target-trc`。
- NDK 有 `ANativeWindow_setBuffersDataSpace`（API 28+）、`ADATASPACE_BT2020_PQ=163971072`、
  `AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM=0x2b`。

**风险与退路**：
- 10bit surface 在部分设备上 compositor 不支持 ⇒ 必须能自动退回 8bit（不能黑屏）。
- mpv 要重编（第二个补丁，同一套工具链，本机可复现：meson/ninja + NDK 27.0.12077973）。
- 失败时回落到 L2（现有映射路径），画面正常，只是不是真 HDR。

### P3 · 杜比视界（DoVi）

- ffmpeg 侧：需要 `dovi_split` bsf（把 RPU 从码流剥出来）——我们的 ffmpeg **没有**，
  要重编 ffmpeg（FFmpeg-n7.1 源码在 `tools/ffmpeg-src/FFmpeg-n7.1`）。
- mpv 侧：需要把 RPU 侧数据喂给 libplacebo 的 `pl_hdr_metadata`（DoVi 动态元数据）。
- 收益：DoVi 片源（profile 5/8）能正确显示 ⇒ **超过 SenPlayer 需不需要重看它的实现深度**。

### P4 · 其它可选增强（按需）

- `target-colorspace-hint-strict`（SenPlayer 有，我们 libplacebo 同代也有）。
- HDR10+ 动态元数据（libplacebo `pl_hdr_metadata_hdr10` 已在库里，需 mpv 侧喂 metadata）。

## 四、验收判据（每条都要真机证据）

| # | 判据 | 目标 |
| --- | --- | --- |
| P2-1 | SurfaceFlinger 层面 dataspace | `Bt2020Pq`（不是 `Unknown`） |
| P2-2 | 内核日志 | 无 `Disabling HDR peak computation`；`target-trc=pq` 被接受 |
| P2-3 | 10bit 不兼容设备 | 自动退回 8bit，画面正常（不黑屏） |
| P2-4 | SDR 片源 | 零变化（102.3 → 102.2 基线） |
| P2-5 | 上屏帧率 | ≥ 23.5 fps（当前 23.98） |

---

## 五、P2 真 HDR 输出的执行记录（2026-11-15 05:20–06:15）

### 5.1 四层的真实状态

| 层 | 内容 | 状态 | 证据 |
| --- | --- | --- | --- |
| ① | Surface buffer = 10bit | **✅ 通了** | `EGL_RED_SIZE=0xa` / `EGL_NATIVE_VISUAL_ID=0x2b` / SurfaceFlinger `req fmt:43`（0x2b = RGBA_1010102） |
| ② | Surface dataspace = BT2020_PQ | **✅ 通了** | `[hdr] dataspace(PQ)=ok`（`ANativeWindow_setBuffersDataSpace` 返回 0） |
| ③ | EGL surface colorspace = PQ | **❌ 这台设备不闭环** | 见 §5.3 |
| ④ | libplacebo 输出 PQ 码值 | **⏸ 待 ③ 通后启用** | 需要 `target-trc=pq`；现在用 L2 的 `bt.1886`（正确的 SDR 映射） |

⇒ **架构四层已全部打通并落码**，剩下的是**设备/驱动**的限制，不是代码缺口。

### 5.2 关键实测数据（K40 / Android 13 / Qualcomm EGL）

**dataspace 候选全表探测**（intent `--ei kivo.hdrProbe 1`，一次跑完）：

```
UNKNOWN=0              : ok
SRGB=142671872         : ok
BT2020=147193856       : 失败(EINVAL)
BT2020_PQ=163971072    : ok    ← 用这个
BT2020_ITU_PQ=298188800: 失败(EINVAL)
BT2020_HLG=168165376   : 失败(EINVAL)
BT2020_ITU_HLG=302383104: 失败(EINVAL)
P3=1432716183          : 失败(EINVAL)
```

⇒ 只有 **PQ** 这一个 HDR 传递函数被这台设备接受（**HLG 不接受**）。
（曾误以为该用 `ITU_PQ`，实测打脸 ⇒ **取值必须实测，不能按规范猜**。）

**dataspace 的时机依赖**：
- 与 `holder.setFormat(10bit)` **同时**设 ⇒ **EINVAL**（那时 EGL 还没建面，buffer 还是 8bit，
  "8bit buffer + PQ dataspace" 是非法组合）；
- 等到**首帧之后**（EGL 已用 10bit config 建好面）再设 ⇒ **ok**。
⇒ 真 HDR 输出必须在 `MpvStartSequence` 的"首帧就绪"之后设置 dataspace。

### 5.3 第 ③ 层为什么在这台设备上不闭环（三条路都试过）

| 写法 | 结果 |
| --- | --- |
| `eglCreateWindowSurface` + `EGL_GL_COLORSPACE` attrib | **EGL_BAD_ATTRIBUTE (0x3004)** —— 旧入口不接受 colorspace attrib |
| `eglSetSurfaceAttrib(EGL_GL_COLORSPACE_KHR, PQ)` | 返回 FALSE 但错误码是 **EGL_SUCCESS (0x3000)** ⇒ 这台驱动**静默无效**（不报错也不生效） |
| `eglCreatePlatformWindowSurface`（EGL 1.5）+ attrib | 确实能设上，但 **VO 初始化挂死**（内核日志停在 `EGLConfig dump` 之后再无输出）⇒ **不能用** |

⇒ 结论：**代码路径与能力已就位**（补丁入库 `patch-mpv-hdr-surface.py`），
   换一台 EGL 驱动支持 PQ surface 的设备即可生效；本机受**驱动限制**。

### 5.4 为此加的两道保险（都已在真机验证）

1. **能力闸门** `Hdr10BitProbe`（用 `ImageReader` 开一个 10bit buffer queue
   ⇒ 走 gralloc 的真实分配路径）：不支持就**根本不让 mpv 用 10bit config**
   ⇒ 从根上避免"VO 初始化挂死"。
2. **默认关闭**：`HdrOutputSwitch` 默认 `auto`（= 关），系统属性
   `debug.kivo.hdrOutput=on` 才开；且**只在 gpu-next 通路下有效**。

⇒ **默认路径零影响**（真机验证）：8bit config、亮度 mean 91.7（与基线一致）、
   首帧就绪、补丁分支未被触及。

### 5.5 顺带修掉的两个真实缺陷

1. **intent 开关从来没生效过**：`MpvStartSequence` 的 `app` 是 `applicationContext`
   ⇒ `(app as? Activity)` 恒 null ⇒ `kivo.renderMode` / `kivo.gpuNext` /
   `kivo.demuxerMb` / `kivo.maxRangeMb` / `kivo.hdrProbe` **全都读不到**。
   （同一个坑在 `MpvPlaybackSession` 已经犯过一次，这里重犯 ⇒ 改成
   **intent 显式作为构造参数传入**。）
2. **编译失败被我自己掩盖了**：一直用 `grep -E "error:"` 过滤 gradle 输出，
   而 **Kotlin 的错误格式是 `e: file://…`** ⇒ 连续几次"装机"其实装的是**旧包**。
   ⇒ 教训：编译判定必须看 `BUILD SUCCESSFUL`，不能只看 grep 命中。

### 5.6 顺带量到的：HDR 亮度四臂对照（设备截屏）

同一段 HDR10(PQ) 素材，同一台设备：

| 臂 | 渲染器 | L2 | mean | max | p95 |
| --- | --- | --- | --- | --- | --- |
| G2 | gpu-next | 关 | **64.6** | 146 | 126 |
| G1 | gpu-next | 开 | **91.7** | 193 | 173 |
| L1 | legacy | 关 | **74.9** | 167 | 144 |
| L2 | legacy | 开 | **93.7** | 185 | 169 |

⇒ **L2 在 gpu-next 下确实有效**（64.6 → 91.7，提升 42%），且与 legacy 效果同量级。
⇒ **gpu-next 无 L2 时比 legacy 更暗**（64.6 vs 74.9）——这正是"完整 libplacebo
   忠实按 PQ 码值输出、legacy 偷偷做了一部分转换"的表现 ⇒ **gpu-next 更"正确"**，
   L2 才是把它拉回 SDR 观感的那一步。

---

## 六、285 · DoVi 差距的二进制实证复核（2026-11-15 19:4x，**修正本文 §一/§三 P3 的过时判断**）

> 用户要求"必须参考 275，把 SenPlayer 吃透，不要猜"。本节用二进制符号取证纠正本文早前判断。

### 6.1 取证方法（两个坑）
1. **Git Bash 没有 `strings` 命令** —— 用它搜会命中 0，造成"没有 DoVi 支持"的假阴性。
   必须用 `grep -aoiE`。本文 §一"我们 ffmpeg 无 dovi"的结论部分源于此工具假象，需重判。
2. 本文 §三 P3 写的 ffmpeg 源码路径 `tools/ffmpeg-src/FFmpeg-n7.1` **不存在**；
   真实源码在 `third_party/ffmpeg-android/build/` 与 `/c/re-tools/ffmpeg-src`。

### 6.2 符号级对比（git 证据，非推测）
| 符号 | SenPlayer libFFmpegKit.so | 我们 libmpv.so | 判定 |
|---|---|---|---|
| `dovi_rpu` / `dovi_rpu_bsf` | ✅ | ✅ | 都有 |
| `DOVI_METADATA` / `DOVI_RPU_BUFFER` | ✅ | ✅ | 都有 |
| `pl_hdr_metadata_from_dovi_rpu` | ✅ | ✅ 已编进产物 | 都有 |
| `mp_image_params_restore_dovi_mapping` | ✅ | ✅ 已编进产物 | 都有 |
| `dovi_reshape` / `dovi_compose_nlq` / `dovi_mapping` | ✅ | ✅（28 处 dovi 命中） | 都有 |
| `dolby-vision-profile` | ✅ | ✅ | 都有 |
| **`dovi_split` / `dovi_split_bsf`** | ✅ | ❌ 命中 0 | **真差距** |
| **`dovi-fel` / `dovi-fel-hwdec`** | ✅ | ❌ 命中 0 | **真差距** |

### 6.3 修正后的判断
1. **mpv 0.39 原生支持 DoVi profile 5/8**（`mp_image.c` 的
   `pl_map_avdovi_metadata` / `pl_hdr_metadata_from_dovi_rpu` 已编进 so）。
   ⇒ "我们完全没有 DoVi 通路"是**错的**；profile 5/8 基础能力已具备。
2. **ffmpeg dovi_rpu bsf 已编译**（`CONFIG_DOVI_RPU_BSF=yes`）⇒ §三 P3"要重编 ffmpeg"**不必**。
3. 真正差的只有两项：`dovi_split` bsf 与 `dovi-fel-hwdec`，都指向 **FEL（profile 7 增强层）**。
   - FEL 主要见于蓝光原盘 UHD BD；流媒体（Emby/网飞）基本是 profile 5/8（无 FEL）。
   - Android MediaCodec 本身也不提供 FEL 解码 ⇒ 即便有 bsf，端侧收益有限。
   - ⇒ **优先级应从"重编 ffmpeg"下调**：先验证我们 profile 5/8 通路在真机是否真的出正确颜色，
     再决定 FEL 是否值得投入。
4. 下一步不再是"重编 ffmpeg"，而是**找一段 DoVi profile 5/8 样本做真机验证**，
   确认 mpv 原生通路是否已超越/对齐 SenPlayer；FEL 作为独立可选项。

### 6.4 证据文件
- SenPlayer so：`build/rea-senplayer/raw/lib/arm64-v8a/libFFmpegKit.so`（52MB）
- 我们 so：`android/app/src/main/jniLibs/arm64-v8a/libmpv.so`（32MB）
- mpv 源码 DoVi 实现：`/c/re-tools/mpv-build/mpv/video/mp_image.c`（1098–1124 行）、
  `video/out/gpu/video.c:992`、`video/out/vo_gpu_next.c:498`

---

## 七、286 · DoVi Profile 8.1 真机验证 + hwdec 重大回归发现（2026-11-15 20:0x）

> 用户指令："我 nas 里面应该有，你找找看"。在群晖 NAS `192.168.5.36` 找到 DoVi 片源，
> 截取样本在 K40(b6e950ee) 与 SenPlayer 做**同源、同机、同时段**对照。

### 7.1 片源确认（ffprobe 8.0.1 硬证据）
片源：`\\NAS\12T\剧集\Squid.Game.S03...DV.HDR.H.265-HHWEB\S03E01....mkv`（6.05GB）
截取前 60s → `docs/evidence/p285-dovi/dovi-p81-60s.mkv`（113,654,699 B），`-c copy` 保留元数据。

| 字段 | 值 | 判定 |
|---|---|---|
| codec / profile | hevc / Main 10 / yuv420p10le | ✅ |
| `dv_profile` | **8** | DoVi Profile 8 |
| `dv_bl_signal_compatibility_id` | **1** | **Profile 8.1**（HDR10 兼容基底） |
| `el_present_flag` / `rpu_present_flag` | 0 / 1 | 无增强层、有 RPU |
| 首帧 side data | Mastering display + CLL + `Dolby Vision RPU Data` + `Dolby Vision Metadata`（vdr_rpu_profile=1, disable_residual_flag=1） | ✅ 完整 RPU |

### 7.2 Kivo 真机结果（kernel log：`docs/evidence/p285-dovi/kernel-rerun.log`）
| 证据 | 内容 | 判定 |
|---|---|---|
| demuxer 识别 | `[mkv] + Dolby Vision - version: 1.0, profile: 8, level: 6, RPU: 1, EL: 0, BL: 1, ID: 1` | ✅ 原生识别 |
| **RPU 逐帧解析** | `hevc: Multiple Dolby Vision RPUs found in one AU. Skipping previous.` **71 次** | ✅ **铁证**：ffmpeg hevcdec.c:3625 逐个 AU 提取 NAL type 62 RPU 并 `ff_dovi_rpu_parse()` |
| 映射通路 | mp_image.c `pl_map_avdovi_metadata` + `pl_hdr_metadata_from_dovi_rpu`（源码确认静默执行，无日志） | ✅ 已编进 so |
| 出画 | gpu-next + libplacebo 7.391 + Adreno 650，截图画面正常、肤色自然、无偏色/黑屏 | ✅ |
| 帧率 | **本地装置无法量**：片段 60s 缓存吃空后假卡顿，与 284 教训一致 ⇒ 不作帧率结论 | ⚠️ not_measured |

> **关键方法论**：DoVi 映射路径**无任何运行时日志**（grep mpv 源码确认），
> 所以 `kernel log 里 dovi 命中 0` 不能证明 RPU 没生效；真正的因果判据是 ffmpeg 的
> "Multiple Dolby Vision RPUs" 逐帧解析警告 —— 这证明 RPU 元数据确实进入管线。
> **结论：mpv 0.39 原生 DoVi Profile 8.1 通路在 Kivo 上真机跑通，无需重编 ffmpeg。**

### 7.3 SenPlayer 同源同时段对照（同机 b6e950ee，同一 113,654,699 B 文件）
- SenPlayer v12（versionCode=12），VIEW intent 仅支持 `content`/`file`，不支持 http/smb 直拉
  ⇒ 用 `file:///sdcard/Download/dovi-p81-60s.mkv` 拉起（先 `pm grant READ_MEDIA_VIDEO`）。
- 结果：`docs/evidence/p285-dovi/senplayer-stable-A.png` —— 画面**明显网格/纱状暗纹叠加、
  整体偏暗、色调偏紫红、肤色不自然**；对比 Kivo `playing.png` 画面正常。
- 起播：SenPlayer 约 50s 才出画（约 0–30s 全黑屏+加载），Kivo 约 2s 首帧就绪。
- ⚠️ 诚实的验证强度：网格暗纹**可能**部分是 screencap 采集伪影，不能 100% 归因为 DoVi 渲染 bug；
  但"偏暗+偏紫红+起播慢"是多帧一致的。此对照只做定性，不出性能数字。

### 7.4 ★★★ 重大回归发现：所有 gpu-next 播放实际在软解
- 现象：本次 DoVi kernel `Set property: hwdec="aimagereader" -> 1` →
  `[vd] Unsupported hwdec: aimagereader` → `Using software decoding`。
- 根因：`mpv_options.cpp:275` 把 gpu-next 的 hwdec 写成 **`"aimagereader"`**（非法值）。
  mpv 合法值是 **`"mediacodec"`**（codec API），加载后再自动选 `aimagereader` interop。
  历史成功日志（p273/p281）均为 `hwdec="mediacodec"` → `Loading hwdec driver 'aimagereader'` → `Using hardware decoding`。
- **影响范围**：查 `p274-startup/armB-gpunext`、`p281/live/remux-dts` 等历史 gpu-next 日志，
  **全部** `Unsupported hwdec: aimagereader` + 软解 ⇒ 这是**全局回归**，不是 DoVi 特有。
- **后果**：此前"帧插值 4K HDR 59.47fps / 1080p 60.44fps 超越 SenPlayer"等所有 gpu-next 性能数据
  其实都是**软解**跑出来的，口径需重新界定（软解都 60fps 说明插值有效，但不能称"零拷贝硬解"）。
  也意味着 §一/§六"aimagereader 零拷贝硬解超越 SenPlayer"这一条**当前版本并未生效**。
- **修复方向**：见 §八（用户裁决"按建议推进"后已执行、编译出包、真机复验通过）。

### 7.5 证据文件
- `docs/evidence/p285-dovi/`：dovi-p81-60s.mkv、kernel.log、kernel-rerun.log、
  playing.png(Kivo)、senplayer-stable-A/B.png(SenPlayer)、origin.log、latency.txt

---

## 八、287 · hwdec 回归修复 + 真机复验（2026-11-15 20:2x，用户裁决"按建议推进"后执行）

### 8.1 根因（在读透源码后定死，非推测）
| 事实 | 依据 |
|---|---|
| `hwdec` property 合法值 = **解码 API 名** | mpv `video/decode/vd_lavc.c` 的 hwdec 表：只有 `mediacodec` / `mediacodec-copy` / `auto` / `no` … |
| `aimagereader` **不是** hwdec 值 | 它是 GPU 侧 interop driver（`video/out/gpu/hwdec.c` 的 `ra_hwdec_drivers[]`），声明 `.imgfmts={IMGFMT_MEDIACODEC}` / `.device_type=AV_HWDEVICE_TYPE_MEDIACODEC` |
| gpu-next 会自动加载它 | `gpu-hwdec-interop=auto`（video.c:409）→ `ra_hwdec_ctx_init(..., load_all_by_default)` 按需加载匹配驱动 |
| 正确日志链 | 历史 p273 成功日志：`Set hwdec=mediacodec` → `Loading hwdec driver 'aimagereader'` → `Using hardware decoding (mediacodec)` |

277 把 interop driver 名误当成 hwdec 值写进 property ⇒ mpv 报 `Unsupported hwdec: aimagereader`
⇒ **静默回落软解**；因软解也能正常出画，一直没被发现 ⇒ 此前所有 gpu-next 数据均为软解口径。

### 8.2 改动（单文件一处，先规划后实现）
- 文件：`android/app/src/main/cpp/mpv/mpv_options.cpp`
- `hwdec_out = (vo == "gpu-next") ? "aimagereader" : "mediacodec";` → **`hwdec_out = "mediacodec";`**
  （两分支统一；libmpv 分支原本就是 mediacodec，不受影响）
- 同步把 263–275 行基于错误前提的注释块替换为源码事实说明（不留被替代的错误叙述）。
- 编译：`cd android && ./gradlew :app:assembleDebug` → **BUILD SUCCESSFUL in 27s**，
  `configureCMakeDebug` / `buildCMakeDebug` 触发 native 重编，`libkivo_mpv.so` 时间戳更新，
  merged_native_libs 已刷新；装包 K40 `lastUpdateTime=20:20:48`。

### 8.3 真机复验（K40 b6e950ee，同一 DoVi 8.1 4K 片段，本地高速供片）
| 判据 | 软解(285) | **硬解(287)** | 结论 |
|---|---|---|---|
| `Set property: hwdec` | `aimagereader` | `mediacodec` | ✅ 合法值 |
| `Unsupported hwdec` | 有 | **0** | ✅ 消失 |
| `Using software decoding` | 有 | **0** | ✅ 消失 |
| `Using hardware decoding (mediacodec)` | 无 | **有** | ✅ **真硬解** |
| `Loading hwdec driver 'aimagereader'` | 无 | **有** | ✅ interop 自动加载 |
| `VO: [gpu-next] 3840x2160 mediacodec` | `yuv420p10`(软解) | **mediacodec**(零拷贝) | ✅ 帧格式印证 |
| `acquireLatestImage failed` / `-30001` / `Waiting for frame` | — | **全部 0** | ✅ **277 顾虑未复现** |
| 上屏帧率（帧插值） | — | **60.57 fps** / max 38.4ms / >200ms 0 | ✅ 硬解下插值仍满帧 |
| 画面 | 正常 | 正常（肤色/绿衣自然，无偏色） | ✅ |

> 帧率为**本地高速供片 3.9s 稳定窗口的冒烟级**证据（装置 60s 片段会耗尽，不能当 240s 长稳结论），
> 但足以证明"改硬解没有让插值退化"。4 个 `[e]` 均为 LCMS2 未编译 + 装置 2MB 截断重连，与硬解无关。

### 8.4 ★ 真实 trade-off：硬解会丢掉 DoVi RPU 动态元数据
| 信号 | 软解(285) | 硬解(287) |
|---|---|---|
| `Multiple Dolby Vision RPUs`（=`ff_dovi_rpu_parse` 逐 AU 解析） | **71 次** | **0 次** |

根因（源码定位）：`AV_FRAME_DATA_DOVI_RPU_BUFFER` 只在 ffmpeg **软解 `hevcdec.c:3048`**
挂到 AVFrame；`mediacodecdec.c` 完全没有 DoVi 处理，硬解输出 `mediacodec` 格式帧不附着 RPU
⇒ mpv `mp_image.c` 的 `pl_hdr_metadata_from_dovi_rpu` / `pl_map_avdovi_metadata` 收不到动态元数据。

**影响界定（不夸大）**：
- 丢的是**逐帧动态元数据**，不是"不能播 DoVi"。Profile 8.1 是 **HDR10 兼容基底**，
  硬解画面仍由 PQ/BT.2020 + HDR10 静态元数据 + L2 tone-map 正确呈现（截图已证颜色正常）。
- K40 本就是 SDR 屏、真 HDR 输出不可用（做 HDR→SDR tone-map），逐帧动态元数据的边际收益更低。
- 与 SenPlayer 的对照仍成立：它同源画面偏暗/偏紫红/带纱纹，Kivo 硬解反而正常（§7.3）。
- 若强行保留 RPU 而退回软解，等于用「4K 软解 + 功耗/发热」换「逐帧动态优化」，
  对绝大多数普通 HDR/SDR 片源是净退步 ⇒ **不采纳**。
- 在硬解路径上从 bitstream 层用 `dovi_rpu` bsf 拆 RPU 再喂 libplacebo，是 mpv 上游对
  Android MediaCodec + DoVi 也尚属空白的重活 ⇒ 列为独立可选项，需单独评估，不在本轮擅自投入。

### 8.5 口径更正（重要，影响既往结论）
- 282 帧插值、277/279 里所有 **gpu-next 性能数据**（59.47 / 60.44fps 等）均为**软解口径**。
- 本节 287 的 60.57fps 是修复后**首个硬解口径**数据。
- "aimagereader 零拷贝硬解超越 SenPlayer"这一条，在 287 之前**从未真正生效**；
  287 起才成立（真 `Using hardware decoding (mediacodec)` + aimagereader interop）。

### 8.6 证据文件
- `docs/evidence/p286-hwdec-fix/`：kernel.log、kernel-fps.log、latency-fps.txt、
  playing-hardware.png（硬解画面）、net-diag（cache 曲线）
- 编译日志：`/tmp/p286-build.log`（BUILD SUCCESSFUL in 27s）

---

## 九、288 · 下午卡顿终判执行清单（287 硬解口径，先固化再跑）

> **为什么单列一节**：287 修掉了全局 hwdec 软解回归 ⇒ 此前所有 gpu-next 的卡顿/帧率
> 结论都是**软解口径**，必须用新包（硬解口径）重跑才算数。而卡顿终判有**时段铁律**
> （275-review：夜间源站快、判不了，必须下午同时段）⇒ 不下午到位不跑、不拿夜间数据凑结论。
> 本节把「脚本缺陷 + 前置校验 + 精确命令」固化，下午一到位即可无返工执行。

### 9.1 ★ 量具缺陷——289 已修复（本节原为"必须先修"，现为竣工记录）
> 原缺陷两处，均已在 289 修掉，下午**无需再手工干预**，直接用 `p274_soak.sh` 一键跑。
- **缺陷 A（旧量具恒 0）**：原 `tools/p274_soak.sh:51` 用 `p273_gaps.mjs`（靠 mpv
  render 回调计数）；vo=gpu-next 无 render 回调 ⇒ 硬解口径下统计恒为 0（278 B 臂已踩）。
  ⇒ 289 把 `p274_soak.sh` 改造为一体化采集器，**彻底移除** `p273_gaps.mjs` 调用，
  全程用 `p274_sf_latency.mjs` 读 `dumpsys SurfaceFlinger --latency`（层名带 `(BLAST)#id`，
  脚本自动查 id 带 30s 重试）。`p273_gaps.mjs` 文件保留（8 个 legacy/实验脚本仍引用）。
- **缺陷 B（旧版 `sleep 300s` 中途不采样）**：原 soak 起播后 `sleep` 死等、末尾才 dump
  一次，覆盖不到长窗口卡顿；§九 旧附录甚至要求"另开终端手工查 id + 手敲 167 次循环"。
  ⇒ 289 改为 wall-clock 循环**全程采样拼接 + 自动收尾取证 + 一次性出全部判据**。
- **缺陷 C（跨 dump 漏采样伪影）**：289 彩排新发现，详见 §十一。旧
  `p274_sf_latency.mjs` 把全局去重帧排序算 gap，两次 dump 之间漏采的帧被算成"大 gap"
  ⇒ 假卡顿。已修：帧间隔/`>200ms` 只取块内连续帧，显式暴露漏采窗口；采样间隔收紧
  到 **1.2s（< 2.1s 环形缓冲容量）**。⇒ 下午若报「漏采窗口 N 段」必须重采，不可写 0。
- 结论口径：帧率/卡顿看 sf_latency 拼接结果（且须「漏采窗口 0 段」才可信）；缓冲事件
  看内核全程 `Enter buffering` / `Render queue underrun` / `dropped` 计数（连续日志，
  不受采样窗口限制）。

### 9.2 前置校验（下午开跑前逐条过）
| 项 | 状态 | 校验动作 |
|---|---|---|
| token | `C:/Users/codex/AppData/Local/Temp/embywork/token.txt`（32B，287 夜探 HTTP 206/1.67s 有效） | 下午开跑前 `curl -r 0-0` 重探一次，防过期 |
| 源站基线 | 必测（源站会老化） | `run-as com.kivo.cinema curl -r 0-8388608` 记基线，性能数字必须带基线 |
| renderMode | 默认 gpu-next；`PlaybackStack.intentRenderMode()` 兼容 `kivo.renderMode` 与 `kivo.gpuNext` | 以 kernel 里 `Using hardware decoding (mediacodec)` + `vo/gpu-next` + `Loading hwdec driver 'aimagereader'` 为准，不盲信 intent 开关 |
| 时段 | **下午同时段**（275-review 铁律） | 夜间臂只能当对照，不能下卡顿结论 |
| 片源 | 485 的 1080p 版本 `aafdd622…`（4.1Mbps，可直传）；勿用 2160p 54.5Mbps 版 | 保持与历史两臂同 item 同 MediaSourceId |
| keepalive 开关 | `debug.kivo.originKeepAlive` 系统属性（`OriginHttpConnection.kt:97` 反射读），**非 intent**，不受 Activity 架构坑影响 | `setprop` 置位后 force-stop 重启生效 |

### 9.3 精确命令（下午执行，两臂 + 同源对照）
```bash
# 环境
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"; export MSYS_NO_PATHCONV=1
ADB="C:/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
SRC_ID="aafdd622d6e1e15b27dfef460ffa95c8"; SIZE=4316184874
URL="https://emby.bangumi.ca/emby/Videos/485/stream?static=true&MediaSourceId=$SRC_ID"

# 两臂（各 300s，硬解口径，gpu-next 默认）—— 289 起**一键**，脚本自动完成
#   起播 → 自动查 (BLAST)#id → 全程 1.2s sf 采样 → 收尾抓 kernel/net/diag+前台校验+截图
#   → 输出①上屏帧率/卡顿 ②硬解口径确认 ③内核全程事件 ④首帧 ⑤出画模式 ⑥供给。
#   ★ 采样期间绝不可并发其它 adb 命令（会争抢致间隔抖动，见 §十一）。
bash tools/p274_soak.sh fix-nokeepalive-hw 300 0 1   # A 臂：每块新连接（当前默认）
bash tools/p274_soak.sh fix-keepalive-hw   300 1 1   # B 臂：复用连接（回退臂）
# 脚本默认公网 485 1080p；如需本地稳定源彩排，加环境变量覆盖：
#   KIVO_URL=http://127.0.0.1:8132/v KIVO_SIZE=<真实字节> KIVO_TOKEN= bash tools/p274_soak.sh <标签> <秒数> 0 1

# 每臂结束后脚本已自动解析并打印 sf_latency 结果（含「漏采窗口」诊断）。
# 只需确认输出里 [覆盖] 为「漏采窗口 0 段」；若为 N 段 ⇒ 该臂卡顿计数不可信，
# 缩小采样间隔或排除并发干扰后**重跑本臂**（不许拿漏采数据写「0 卡顿」）。

# 同源同时段 SenPlayer 对照（275 §验收：同网络/同温度窗口）
#   同机同片源同分钟，量 sf_latency + 截图；判据见 275 指标口径表。
```

### 9.4 判据（写死，避免事后改口径）
- **A 臂不劣于 B 臂** + 两臂 `Enter buffering` 次数对比 + `>200ms` 卡顿次数，
  用「单次请求字节数 + 耗时」做因果判据（帧率波动 2.2 倍不可靠，方法论 6）。
- 必须带**同源同时段 SenPlayer 对照**（缺了会把"源站慢"错信，方法论 3）。
- 所有数字标注：时段、源站基线、验证时长（≥240s）、口径（硬解）。
- 未测项写 `not_measured + reason`，**不许写 0 伪装健康**（275）。

---

## 十、288 · 硬解口径 ≥240s 长稳（当晚可做，已做）

> 夜间公网卡顿终判做不了（§九），但「新包硬解路径本身长稳」不受时段影响，
> 用 **NAS 局域网稳定源**当晚补掉。这是 287 之后**首个硬解口径 300s 长稳**。
> 边界：此结论只证明「硬解路径 + 帧插值在稳定供给下 300s 不退化」，
> **不是公网卡顿终判**（那个必须下午，§九）。

### 10.1 装置关键修法（根除 284/285/286 反复踩的假卡顿）
`p274_local_origin.mjs` 用 `--file` 指向 **NAS 完整大文件** + `--file-start=0` +
`--size=真实大小` ⇒ `mainEnd=size-1` 覆盖全部 offset ⇒ 任何请求都 `inMain`、
**永不 FALLBACK**、declared size 精确、零上游回落。全程 `FALLBACK 0 / UPERR 0 /
1198 块 / 稳定 7.8MB/s`（`--mbps 8`，不构成瓶颈）。node 能读 UNC `//NAS/...`。
首/中/尾三段 206（含片尾 cues 区）验证通过。端口 8131（8130 被旧装置残留占用）。

片源：`\\NAS\12T\电影\M3gan.2.0...2160p...HDR10+.H.265.mkv`，22.5GB /
3832×1600 / HEVC Main10 / yuv420p10le / bt2020-pq / 23.976fps / 7223s。
纯 HDR10+（非 DoVi，无 RPU 干扰）。MediaFormat 带 `hdr-static-info` +
`hdr10-plus-info` ⇒ **硬解也能带出 HDR10+ 动态元数据**，与 §8.4 的「DoVi RPU 被
MediaCodec 吞掉」不是同一条通路，正好补上对照面。

### 10.2 两臂结果（每臂 150 次 × 1.8s sf_latency 采样拼接去重）
| 判据 | 臂1（中途含 1 次 `screencap`） | **臂2（纯净，全程零截屏）** |
|---|---|---|
| 采样时长 | 301s | **299s**（>240s 铁律） |
| 上屏帧率 | 60.04 fps | **60.13 fps** |
| 帧间隔 p50 / p99 | 16.5 / 25.8 ms | 16.6 / 29.0 ms |
| **>200ms 卡顿** | 1 次（1640.7ms） | **0 次** |
| max gap | 1640.7ms | **150.7ms** |
| `Enter buffering` | 0 | **0** |
| `dropped` / `software decoding` / `Unsupported hwdec` | 0 | **0** |
| `Using hardware decoding (mediacodec)` | 1 | **1** |
| `acquireLatestImage failed: -30001` | 1（342.971s，窗口外） | **0** |
| `Render queue underrun` | 15 | 77 |
| 画面（起/中/尾截图） | 正常 | 正常 |

### 10.3 ★ 两条异常的正确归因（现象可见≠原因正确）
1. **臂1 的 1640.7ms gap = 量具自干扰，不是播放器卡顿**。它精确落在采样起点后
   151.1s，正是第 75 次循环里那次 `screencap -p`（75×约2s≈150s）；4K 截屏抢占
   SurfaceFlinger 合成。kernel 该时段只有正常的 `Output MediaFormat changed`
   （MediaCodec 每帧 verbose），**零 buffering/drop/stall**。剔除这一单点后
   max=117.1ms；**臂2 全程零截屏 ⇒ 直接 >200ms 0 次、max 150.7ms**，坐实归因。
   ⇒ 长采样中**绝不能插入截屏**，截图必须在采样循环之外。
2. **`-30001`（342.971s）= 偶发单次、可自恢复、窗口外**。它是
   「Waiting for frame timed out」一次警告，设备保持 Awake + Display Suspend Blocker，
   之后播放继续到 364s 全为正常 MediaFormat 日志，全程 `Enter buffering=0`。
   臂2（rerun 300s+）**完全不复现**。⇒ 诚实登记为「长跑偶发一次」，但**不构成
   硬解失效**，277 的致命 `-30001` 未成系统性问题。不把它写成 0，也不夸大成 bug。

### 10.4 诚实边界
- `Render queue underrun` 两臂 15/77 次（4K 帧插值既有代价，见 282），
  **未转化为任何上屏卡顿**（>200ms=0、max<151ms）——不隐藏、也不误判为卡顿。
- 起播 2 个 `Stream ends prematurely`（2.4s/2.7s）= demux 探测期一次性重连，
  装置日志此后连续 4MiB 块、无 FALLBACK、`Enter buffering=0`。非装置缺陷、非卡顿。
- cache 曲线尾段 `rate=0.0M/s` 是预读泵在缓存够用时按需暂停（dur 恒 10–18s、
  `underrun=false`），不是缓存吃空（区别于 284 的装置片源耗尽）。
- 全程 60fps 是 **23.976→60 帧插值**在真硬解路径下达成，与软解口径的 282 数据
  不是同一套，但结论一致且现在是硬解口径坐实。

### 10.5 证据文件
- `docs/evidence/p287-hwdec-soak/`：`latency.txt`(臂1) / `latency-clean.txt`(臂2)、
  `kernel-full.log` / `kernel-rerun-full.log` / `net-full.log` / `origin.log`、
  `mid-play.png` / `final-play.png` / `clean-end.png`（画面全程正常）。

---

## 十一、289 · 终判工具竣工 + 上屏量具「漏采样伪影」缺陷修复

> 夜间公网终判仍受时段铁律锁死（§九），本节做的是**当晚能实打实推进的准备工作**：
> 把下午终判从「人工清单 + 手工拼接循环」升级为「一键可靠脚本」，并在彩排中
> 挖出并修掉了一个会**让卡顿数据大量误报**的量具系统性缺陷。代码只动 2 个工具
> 脚本，不碰产品代码。

### 11.1 文件家族树 / 改动范围（铁律 2、3）
| 文件 | 动作 | 说明 |
|---|---|---|
| `tools/p274_soak.sh` | **一体化改造（单文件）** | 起播→自动查层(30s 重试)→全程 sf 采样→收尾取证→出 6 类判据；**彻底移除** `p273_gaps.mjs` 调用；片源支持 `KIVO_URL/SIZE/TOKEN` 覆盖供本地彩排 |
| `tools/p274_sf_latency.mjs` | **口径修正** | 帧间隔/`>200ms` 只取块内连续帧(按后帧去重)、跨块接缝不计 gap、显式诊断漏采窗口 |
| `tools/p273_gaps.mjs` | 不动 | 8 个 legacy/实验脚本仍引用 |
| `tools/p274_sf_latency.sh` | 不动 | 短测量具，职责独立 |

### 11.2 ★ 缺陷 C：跨 dump 漏采样伪影（彩排新发现，会作废卡顿结论）
- **现象**：45s 彩排 A 臂报 `>200ms 2 次 / max 398ms`、B 臂报 `6 次 / max 891ms`，
  但两臂 `Enter buffering=0 / dropped=0`、画面正常——数字与事实矛盾。
- **归因**（现象可见≠原因正确）：latency 是 **128 帧环形缓冲**（60fps ≈ 2.1s）。
  若相邻两次 dump 起点间隔 > 2.1s，中间一段帧从未被任何一次 dump 采到。旧量具把
  「全局去重后排序」的相邻帧算 gap ⇒ 把这段**没采到数据**误算成一个"大 gap"＝假卡顿。
  逐块验证：A 臂 2 次、B 臂 6 次里 5 次都判为「a 是块尾 + b 是块头」＝跨 dump 边界。
- **根因**：旧 `p274_soak.sh` 标称 `sleep 1.8s`，但 **`adb` 往返开销 ≈ 0.5–1.2s**，
  实测块间隔 max 达 2.48s / 2.97s（`>2.1s` 各 6 / 13 次）。B 臂更差是因为彩排时
  并发跑了归因命令争抢 adb。⇒ 采样间隔必须留足余量，**且采样期间严禁并发 adb**。
- **修法**（两处）：
  1. `p274_sf_latency.mjs`：按空行切分采样块，gap 只在**同一块内**连续帧之间算，
     以「后帧 presentTime」为 key 去重（重叠块里同一真实间隔只算 1 次）；跨块接缝
     绝不生成 gap。帧率/时长仍取全局去重并集（覆盖全窗口）。
  2. 新增**覆盖诊断**：相邻块「前块尾→后块头」为正 ⇒ 记为漏采窗口，输出段数/合计/
     最长；有漏采窗口时明确警告「这些区间无采样数据，不可据此写 0 卡顿」。
  3. `p274_soak.sh` 采样间隔 `1.8s → 1.2s`，并写入「采样期间不并发 adb」注释。

### 11.3 回归验证（结论不变 + 伪影归零）
用新量具重算历史与彩排 4 份 latency：

| 数据 | 旧报 `>200ms` | **新量具** | 漏采窗口诊断 | 结论 |
|---|---|---|---|---|
| p287 臂1(300s) | 1 次(1640ms) | **0 次** / max 117.1 | 1 段/1.6s（＝那次截图） | 60.04fps 不变；截图自干扰被正确重分类为漏采，非卡顿 |
| p287 臂2(纯净) | 0 次 | **0 次** / max 150.7 | 6 段/0.4s(最长111ms) | 60.13fps 不变 |
| 289 彩排 A | 2 次 | **0 次** / max 73.6 | 6 段/1.2s | 伪影全剔除，58.75fps |
| 289 彩排 B | 6 次 | **1 次** / max 287.8 | 12 段/3.0s | 剩下那 1 次＝起播 aimagereader 首帧超时的真实事件(@0.3s) |

⇒ **p287 的核心结论（硬解 60fps 满帧、>200ms 0 次）完全不受影响**，量具修正只是
让口径更严谨；并顺带把 287 手臂1 那次"1.6s 卡顿"精确重分类为「截图导致的漏采窗口」。

### 11.4 ★ 真机验证：收紧 1.2s + 零并发 adb → 首个「全窗口可信」数据
`bash tools/p274_soak.sh p289-verify-12 40 0 1`（NAS 稳定源，全程零截图、零并发 adb）：

| 判据 | 彩排 A(1.8s) | 彩排 B(1.8s+并发) | **验证臂(1.2s/零并发)** |
|---|---|---|---|
| 块起点间隔 max | 2.48s | 2.97s | **1.49s** ✅ <2.1s |
| 漏采窗口 | 6 段/1.2s | 12 段/3.0s | **0 段** ✅ |
| 上屏帧率 / >200ms | 58.75 / 0 | 55.85 / 1 | **59.84 fps / 0** ✅ |
| 硬解口径确认 | ✅ | ✅ | ✅ 全链(mediacodec+aimagereader+VO gpu-next) |
| buffering / dropped / -30001 | 0 | 1(起播) | **全 0** ✅ |
| 画面 | 正常 | 正常 | 正常（人物/肤色/灯光无偏色） |

⇒ 验证臂是**首个「全窗口有采样数据、卡顿计数可信」**的干净数据，证明修法有效。
彩排是**验证脚本/量具本身**，本地稳定源、40–45s 短窗口，**不作任何公网卡顿结论**。

### 11.5 方法论增补
- **「>200ms 0 次」必须先证明窗口被完整覆盖**：漏采窗口存在时，0 次只是"没采到"，
  不是"没有"。量具必须显式报漏采，符合 275「未测写 not_measured、不许写 0 伪装健康」。
- **标称 sleep ≠ 实际采样间隔**：adb 往返会叠加，1.8s 标称实测能到 2.97s。
  环形缓冲 2.1s 的尺子下，标称必须留 ≥0.9s 余量，且采样期间禁止并发 adb。
- **截图自干扰(287) 与漏采样伪影(289) 是两类假卡顿**：前者是"这一刻确实没上屏"
  （截屏抢占），后者是"这一刻没被采样"。新量具对两者分别归到漏采窗口，均不当卡顿。

### 11.6 证据文件
- 改造后工具：`tools/p274_soak.sh`、`tools/p274_sf_latency.mjs`
- 彩排/验证证据：`docs/evidence/p274-startup/p289-rehearsal-{a,b}/`、
  `docs/evidence/p274-startup/p289-verify-12/`（各含 latency.txt / kernel.log /
  net.log / diag.log / playing.png）；装置日志 `docs/evidence/p289-soak-tool/origin.log`。

---

## 十二、290 夜间公网两臂 + 291 供数停摆修复（2026-11-15 夜间，用户裁决「必须把整条链路打通 / 参照 275 对齐并分析能否更好」）

> 本节遵循 275 §8：超出设备/协议/片源覆盖矩阵的能力写「未验证」，不宣称「完美支持」；
> 没有采样的字段写 `not_measured + reason`，不用 0 伪装健康。

### 12.1 290 夜间公网两臂（硬解口径，各 300s，标注夜间、不冒充下午终判）

| 属性 | A 臂 `originKeepAlive=0`（当前默认） | B 臂 `=1`（回退臂） |
|---|---|---|
| 主链路吞吐 | **566 KB/s** | **253 KB/s** |
| 上屏帧率 / >200ms | **60.02 fps / 1 次** | **26.58 fps / 15 次** |
| 最长冻结 | 1.98 s | **35.47 s** |
| buffering / `-30001` | 1 / 0 | 15 / 6 |
| 主 chunk 断层 >20s | 1 次（28.7 s） | 3 次（22.6 / 31.7 / **124.1** s） |
| 表头 `hdrMs` p50 | 1670 ms（每块新连接） | 954 ms（复用，但吞吐塌了） |
| 漏采窗口 | **0 段** ✅ | **0 段** ✅ |

⇒ 274 的「长连接吞吐衰减」在硬解口径下坐实；`=1` 回退臂应废弃，不得再作为默认。

**❗ 属性污染（真实事故）**：脚本 B 臂把 `debug.kivo.originKeepAlive=1` 留在设备上未复位，
污染了用户随后的真实播放会话（用户反馈「卡得一动不动」）。已复位 `0` + force-stop +
清日志重起播；`tools/p274_soak.sh` 增加 `trap cleanup_props EXIT INT TERM`，覆盖正常退出、
Ctrl-C、中途失败三条路径。**系统属性是进程级全局状态，force-stop / 重装都不会清。**

**SenPlayer 同源同时段公网对照 = `not_measured + reason`**：`dumpsys package` 显示其
intent-filter 只注册 `content` / `file` 的 VIEW，不吃 http/https 直链；`run-as` 查不到
已配置 Emby 媒体源。严格同 URL 对照架构上不可得，**不硬凑**（275 §6.1）。

### 12.2 ★ 127.8s「卡得一动不动」真因（分片取证，推翻单因归因）

片源：`hevc 1920x804 23.976fps`，`duration 8406.4 s` / `4,316,184,874 B`
⇒ **平均码率 501 KB/s**。

| 会话 | 吞吐 | buffering | >20s 断层 |
|---|---|---|---|
| A 臂 300s | 566 KB/s（>501，余量 13%） | 1 次 | 1 次 |
| B 臂 300s | 253 KB/s（缺 50%） | 15 次 | 3 次 |
| **p290-live 973s** | **417 KB/s（缺 17%）** | **14 次** | **5 次：29.7 / 28.6 / 127.8 / 31.1 / 48.3 s** |

127.8 s 断层窗口（+236.2s ~ +364.0s）内的**全部**活动：

| 证据 | 值 | 含义 |
|---|---|---|
| 主 chunk `[play] origin chunk` | **仅 1 条**（`from=1186534978 hdrMs=10101`） | 一块 4 MiB 在慢慢读 |
| `origin warmup hdrMs` | **52 次** | 低优先级 ping 仍在持续 |
| `open fail` / `read fail` / `eof unexpected` | **全 0** | 没有任何异常路径被触发 |
| `[cache]` 64 条 | `fw=3.9M/dur=13.2s` → `fw=0.0M/dur=0.0s/underrun=true` | 内核前向数据耗尽 |
| `[cache] total` | 49.5M → 57.3M（+7.8M / 128s ≈ **61 KB/s**） | 字节还在吐，只是极慢 |

**根因**：`HttpOriginClient.OriginStream.readInto()` 只受 `soTimeout = 30s` 约束，
而那是**单次 `read()` 拿不到数据**的超时。源站以 ~60 KB/s 吐字节时每 <30s 就有一次
正数返回 ⇒ `OriginConnectionLease` 永不 abort、永不重开、**永不留下一行日志**。
275 §5.2「限制每请求超时」已有的是*无数据超时*，缺的是 **整块进度/吞吐下限**。
B 臂 124.1 s 冻结是同一机制（复用连接用久后吞吐塌了）。

> 方法论增补：**「零 fail 日志」不等于「链路健康」**。静默慢读是取证黑洞，
> 每个「没有进展但也没报错」的分支都要有整块进度上限。

### 12.3 物理缺口边界（不能靠缓存宣称修复）

当前公网吞吐 **417 KB/s < 片源 501 KB/s**。按 275 §6.2 高码率文件条款：
*若源站能力不足，明确显示供数瓶颈，不能通过增大缓存宣称修复*。
⇒ 291 能消灭的是**分钟级冻结**（连接层缺陷），**消灭不了**吞吐不足导致的周期性短
buffering。这条是源站侧/降码率（283 HLS 直通）范畴，不是播放器层可「调校到完美」的部分。

### 12.4 291 实现（按 275 §3.2 家族树：优先沿用现有文件，独立变化原因才拆新文件）

| 文件 | 职责（单一） | 开关 |
|---|---|---|
| **新** `net/OriginChunkWatchdog.kt` | 单块吞吐判定：连续 6s 平均 <80 KB/s 判退化，12s 冷却 | `debug.kivo.stallWatchdog=off` |
| 改 `net/OriginConnectionLease.kt` | 每块挂看门狗；退化即 `abort()` + 从当前 writePosition 重开 + 留日志 | 同上 |
| 改 `net/OriginPrefetchBuffer.kt` | `resetTo()` 同时清 `failure`，消除永久 latch（§5.2 有界恢复） | — |
| **新** `playback/OriginSupplyPressure.kt` | `cache-duration` / `underrun` 滞回判定供数压力（8s ON / 16s 连续 3 次 OFF） | `debug.kivo.supplyPressure=off` |
| 改 `MpvCacheDiag` / `OriginWarmupPool` / `PlayerUrlPrepare` / `PlayerSmbStart` | 喂数据 + 压力期停 ping、首帧预热延后；45s 兜底、无采样 8s 解除 | 同上 |
| 改 `tools/p274_soak.sh` | `KIVO_STALL` / `KIVO_SUPPLY` 环境变量 + 属性复位 trap | — |
| 改 `tools/p274_local_origin.mjs` | `--degenerate-every / --degenerate-mbps / --degenerate-after-sec` 确定性注入退化连接 | — |
| **新** `tools/p291_degenerate_ab.sh` | 4 臂可控 A/B（每臂重启装置，条件完全相同） | — |

**两个独立开关 ⇒ 一次装包跑 4 臂归因**；关闭时行为一字不变（对照臂）。
看门狗自救走 `dropStream(countFailure=false)`：**不计入连续失败**，否则 5 次后
`markFailure` 会把主链路永久 latch 住。

### 12.5 与 SenPlayer 对齐 / 能否更好（按 275 骨架逐条）

| 275 契约 | SenPlayer 可追溯事实 | Kivo 290 前 | Kivo 291 后 |
|---|---|---|---|
| §5.2 每请求超时 | 静态链无运行时证据 [S12][S13] | ❌ 只有无数据超时，慢读无上限 → 127.8s 冻结 | ✅ 整块吞吐下限 + 换连接自救 |
| §5.2 读取优先级 | 单句柄 + 单线程 executor（静态） | ❌ 低优先级预取无条件跑，慢源站抢连接 | ✅ runway 滞回闸门，压力期让路 |
| §5.2 有界恢复 | `not_measured` | ❌ `failure` 永久 latch | ✅ 跳读即清 `failure` |
| §6.1 `buffer_runway_ms` | `not_measured`（无公开指标） | ⚠️ 已采但未用于决策 | ✅ 采 + 驱动供数调度 |
| §6.2 高码率文件 | 同源公网对照 `not_measured + reason` | ⚠️ 未标注瓶颈 | ✅ 明确 417<501 KB/s 物理缺口 |
| §8 退出条件 | — | ❌ 多项未闭合 | ⚠️ 仍多项未闭合（见 12.7） |

**可声称「更好」的局部**：SenPlayer 静态证据里没有「按 demux runway 动态调度低优先级
预取」，也没有「单连接吞吐退化自救」；291 这两项是按 275 契约补齐、有真机证据的设计。
**不能声称的**：整体超越 SenPlayer —— 严格同 URL 公网对照不可得（12.1），且真 HDR
PQ surface、DoVi FEL / `dovi_split`、HLS/SMB 自动降级、冷启动/首屏等仍未闭合。

### 12.6 291 真机验证（K40 `b6e950ee`，2026-11-15）

#### 12.6.1 首轮作废记录（实验设计缺陷，留作方法论证据）

首轮 4 臂**不可用于结论**：App 的 `watch.json` 按 URL 存续播位，同一 URL 重跑会从
上一臂进度开始（kernel 实见 `loadfile options="start=93.844 / 188.438 / 231.273"`），
后两臂直接跑出本地片段范围 ⇒ 装置 FALLBACK 到上游只回 **35 字节**，
158 / 192 次无效重试。

**修法**：每臂独立端口 + `?run=<epoch>&arm=<label>` 让 URL 全新（装置不解析 path，
供片字节完全相同）；退化注入提前到 `every=3 / after=12s`；判据补「末尾悬停」
—— 卡在最后一块时没有下一块，相邻间隔算不出断层，会假报「最大断层 8.1s」。

> 方法论增补：**A/B 前必须证明各臂起点/条件相同**。续播位是隐形变量；
> 「FALLBACK = 0」应作为数据有效性的**前置判据**，而不是事后看结果才发现。

#### 12.6.2 可控 4 臂（每臂 100s，确定性注入 0.04 MB/s 退化连接）

| 臂 | stallWatchdog | supplyPressure | 退化注入 | 换连接 | 主 chunk | 末尾悬停 | warmup ping | FALLBACK |
|---|---|---|---|---|---|---|---|---|
| base | off | off | 1 | 0 | 9 | **43.0 s** | 54 | **0** |
| stall | **on** | off | 3 | **2** | **15** | **0.7 s** | 54 | **0** |
| supply | off | **on** | 1 | 0 | 9 | **43.0 s** | **0** | **0** |
| both | **on** | **on** | 3 | **2** | **15** | **0.6 s** | **0** | **0** |

- **看门狗**：对照臂注入 1 次退化后卡在那一块直到窗口结束（末尾悬停 **43.0 s**；
  不再请求新块 ⇒ 装置只注入 1 次，这本身就是「卡住」的证据）；看门狗臂注入 3 次、
  打出 `origin 连接退化` **2 次**并换连接重开，主 chunk **9 → 15**、
  末尾悬停 **43.0 → 0.6–0.7 s**。⇒ p290-live 的 127.8 s 静默慢读被有界恢复取代。
- **压力闸门**：off 臂 54 次 warmup ping，on 臂 **0 次** ⇒ 压力期低优先级预取完全
  让路（275 §5.2 优先级契约落地）。
- **两个开关互不干扰**：`stall`/`both` 表现一致、`base`/`supply` 表现一致 ⇒ 可分别归因。
- 全臂 `FALLBACK=0` ⇒ 数据有效。`buffering=0` 是因为退化前 cache 已攒够 runway；
  本判据看的是**主链路是否恢复供数**，不是「有没有 buffering」。

#### 12.6.3 边界（诚实标注）

- 这是**短窗口确定性机制验证**（本地片段仅 48 MiB），**不是整体流畅度结论**；
  按方法论「播放结论必须 ≥240s」，整体结论仍须公网长稳（12.6.4）。
- 退化连接是**人为注入**，不代表公网源站一定以 0.04 MB/s 吐字节；它验证的是
  「存在这种连接时，播放器会不会自救」这一确定性机制。
- 看门狗下限 80 KB/s / 6s 窗口 / 12s 冷却只在本次注入速率下验证；
  其它退化速率下的误伤/漏判边界 `not_measured`。

#### 12.6.4 公网 300s 两臂长稳（夜间 22:33 / 22:39，硬解口径 gpu-next）

| 判据 | base（都 off） | both（都 on） |
|---|---|---|
| 主链路吞吐 | 589 KB/s | 608 KB/s |
| 上屏帧率 | 60.32 fps | 58.24 fps |
| `>200ms` / max gap | 1 次 / 558 ms ⚠️ | 2 次 / **7795 ms** |
| 漏采窗口 | **1 段 / 0.2 s**（最长 167 ms）⚠️ | **0 段** ✅ |
| buffering | 1 次（0.62 s） | 2 次（**7.767 s** / 2.412 s） |
| `-30001` / dropped | 0 / 0 | 0 / 0 |
| `origin 连接退化` | 0（开关关闭） | **1 次（真实公网）** |
| warmup ping | 134 | **63** |
| 硬解全链 | ✅ | ✅ |

**★ 时间线归因（7.8 s 冻结与看门狗无关）**：both 臂两次 buffering 在 mpv 时钟
`[23.6→31.4 s]` 与 `[58.8→61.2 s]`，都是**起播期供数爬坡**；换连接发生在**首帧后
106.5 s**，晚得多。换连接前一块 `hdrMs=8179`，换连接后 **+1.3 s 即开出新块**
（`hdrMs=1318`），之后稳定每 ~6 s 一块；cache runway 从 `4.4 s → 10.2 s → 19.8 s`，
**未新增 buffering**。闸门日志同时证实滞回与兜底都工作：
`放行 2 项延后预热（超时兜底 2 项）` → `供数压力 OFF：runway=19.8s 连续 3 次健康`。

**诚实边界**：
- 两臂相隔 6 分钟、源站吞吐 589 / 608 KB/s（差 3%），仍非严格同源同时段；
  base 臂有 1 段漏采窗口（167 ms < 200 ms 判据，影响有限，但按 §11.5 仍标注不可信）。
- **不能据此宣称 both 比 base 好**：both 臂起播 7.8 s 冻结更差（源站瞬时波动 +
  起播爬坡），这是 275 §6.2 冷起播未闭合项，291 未修。
- 291 解决的是**分钟级静默慢读冻结**（可控 4 臂 43.0 s → 0.6 s；公网换连接 1.3 s
  恢复），**不是**起播期供数爬坡，也不是吞吐物理缺口。

### 12.7 诚实边界 / 仍未闭合

- ❌ **吞吐物理缺口会随源站时段重现**：p290-live 曾 417 < 501 KB/s；本轮 12.6.4
  两臂 589 / 608 KB/s 高于需求。**不能靠缓存宣称修复**（275 §6.2），291 只消灭分钟级冻结。
- ❌ **起播期供数爬坡未闭合**：both 臂 7.767 s 起播冻结即属此类（275 §6.2 冷起播），
  291 的两个开关都不覆盖它，是下一张卡的优先级最高项。
- ❌ 严格 SenPlayer 同 URL 公网同时段对照 `not_measured + reason`（其 scheme 限制）。
- ❌ 真 HDR PQ surface（K40/Qualcomm EGL 不闭环）、DoVi FEL / `dovi_split`（需重编 ffmpeg）、
  SMB 直读 / HLS 自动降级、字幕音轨闭环 —— 承接 §九，均未闭合。
- ⚠️ 12.6.2 可控 4 臂是**短窗口确定性机制判据**（本地片段仅 48 MiB），不作整体流畅度
  结论；12.6.4 公网 300s 两臂已完成，但相隔 6 分钟且 base 臂有 1 段漏采窗口 ⇒
  **两臂不能直接分高下**，只能证明「真实公网退化连接被有界恢复 1 次」。
- ⚠️ 看门狗阈值（80 KB/s / 6s / 12s 冷却）只在 0.04 MB/s 注入与本次公网 76 KB/s
  一处样本下验证；边界附近是否误伤（源站整体慢 vs 单连接退化）`not_measured`。
  公网那次 `76KB/s` 紧邻阈值，属边界触发，需补样本判定是否该收紧到 40 KB/s。
- ⚠️ **不能宣布「整体对齐或超越 SenPlayer」**：严格同源对照不可得，且上述多项未闭合。
  可声称的只是 §12.5 表中「按 275 契约补齐的两项局部能力」。

### 12.8 证据索引

- 夜间两臂：`docs/evidence/p274-startup/p290-night-{nokp,kp}/`
- 真因会话：`docs/evidence/p274-startup/p290-live/{net.log,kernel.log}`
- 4 臂退化对照：`docs/evidence/p274-startup/p291-degenerate/{base,stall,supply,both}/`
- 工具：`tools/p274_soak.sh`、`tools/p291_degenerate_ab.sh`、`tools/p274_local_origin.mjs`

## 十三、292 起播期供数爬坡专卡（方案 C 封顶 → 真因改写）

> 证据：`docs/evidence/p274-startup/p292-startup/{off,w4,w6,w8,w10}/`；
> 装置 `p274_local_origin.mjs --mbps 0.6 --chunk-hdr-ms 3500`；
> 驱动 `tools/p292_startup_ab.sh`（70s/臂，独立端口 + `run/arm` query 禁续播）。
> **本地确定性短窗口判据，不是公网整体流畅度结论。**

### 13.1 先修接线 bug：方案 C 首轮其实从未生效

首轮 A/B 出现 `rb(ok)=3`（选项写入成功）却与对照逐项持平；
`[rebuffer] 恢复默认（超时兜底(12s)）` 落在 t=+4.767s、与「MPV 首帧就绪」同一毫秒
—— armed 到「超时」只有 4.2 s，不是物理超时。真因：
`MpvCacheDiag.attach()` 开头必调 `stop()`，292 加在 `stop()` 里的
`RebufferPolicy.reset()` 把 `applyPlaybackPolicy → beforeLoad()` 刚设的
`armedAtMs` 清成 `0` ⇒ `note()` 中 `now - 0 > 12s` 恒真 ⇒ 第一次采样即写回默认
⇒ **方案 C 退化成方案 B**（kernel：`cache-pause-initial="no"` 又在首帧 4.191 s 写入）。
修复：删该 reset 调用与 `RebufferPolicy.reset()` 死代码、留防回归注释；
判据新增「恢复=达标 / 超时兜底」列，超时兜底臂直接判无效。

### 13.2 方案 C 机制生效但收益不足

w4 kernel 铁证：`4.176 首帧` → `4.178 Enter buffering` →
`5.049 End buffering (waited 0.872s)` 攒够 4.0 s runway →
`6.179 cache-pause-initial="no"`（首帧后 **2.0 s** 才写回；方案 B 是首帧同毫秒 4.220）。

| 臂 | 恢复 | 首帧 | bf 次 | bf 总 | 最长 | 首次 underrun |
|---|---|---|---|---|---|---|
| off（291 基线） | - | 4.9s | 2 | 4.2s | 2.3s | +13.9s |
| w4 | 达标 | 4.8s | 2 | 3.8s | 2.9s | +18.0s |
| w6 | 达标 | 4.8s | 2 | 3.9s | 2.3s | +20.0s |
| w8 | 达标 | 4.8s | 2 | 3.8s | 2.9s | +20.0s |
| w10 | 超时兜底（无效） | 4.8s | 1 | **5.4s** | **5.4s** | none |

结论：**mpv 选项层路线已封顶**。起始 runway 只把首次穿仓平移（w4 +4.1 s，
w6/w8 再只多 2 s，边际递减），bf 次数恒为 2；wait≥10 触发 12 s 超时兜底，
把连环短冻结合并成一次 5.4 s 长冻结 —— 正是方案 A 已证伪的负优化翻版。
三次方案（A 拉高 `cache-pause-wait`、B `onFirstFrame` 立即恢复、C runway 达标恢复）
全部证伪或收益不足。

### 13.3 真因改写：不是「首帧零缓冲」，是周期性供给归零

w4 `[cache]` 时间线显示每块 4 MiB 前 `hdrMs≈3620ms` 表头空窗，
期间 `rate=0.0M/s`、供给为 0，一次空窗吃掉约 3.6 s runway：

| 相对首帧 | 事件 | runway |
|---|---|---|
| +1.95s | `恢复默认（runway达标 5.5s）` | 5.5 → 6.2s 峰值 |
| +9.6s | `chunk#2 hdrMs=3621` | 1.8s 并继续掉 |
| +18.0s | `dur=0.0s underrun=true` | 穿仓 |
| +19.8s | `chunk#3 hdrMs=3622` | 0.0s |
| +40s 后 | 缓存攒到 24.8M | 15–19s，再无 underrun |

⇒ 起播连环 buffering 的结构性来源是 `OriginPrefetchPump` **串行补块**：
读满一块 → 归还连接 → 开下一块付表头 → 这段窗口环缓冲不增长。
起始基线 5.5 s 只够扛一个空窗，连第二个就穿仓；等缓存自然攒到 15 s+ 才稳。
这**推翻 292 初判的一半**（「首帧零缓冲」不是主因），也正是 275 §5.2
「短时前瞻」的缺口。物理吞吐不缺（614 > 327 KB/s），是调度缺口。

方法论增补：`cache-pause-initial` 的代价**不能用首帧时间衡量** ——
所有臂首帧都 4.8 s 不变，因为 `MpvFirstFrameWait` 量的是首帧上屏，
而 initial buffering 在首帧**之后**才进入；代价要量「首帧后恢复播放延迟 / bf 总时长」。

### 13.4 下一步：方案 D（Source 层有界并行短前瞻，未实现）

当前块剩 ~2 MiB（≈3.4 s @614KB/s）时后台预开下一块 Range，把 TTFB 等待与
当前块尾段传输重叠；读满后把 pending 流**提升**为 current，跳过 `open()` 表头。
**不是叠加带宽，是重叠等待**（预开在等响应头时装置不吐 body）⇒ 规避 275 §5.2
「并发预读须证明源站允许叠加带宽」的顾虑。

家族树（先规划再实现，单文件单职责）：新增 `net/OriginChunkPrefetch.kt`
（异步预开 / 暂存 / 生成号校验 / 作废 / 开关 ≤300 行）；改
`net/OriginConnectionLease.kt`（挂接 `open/finishChunk/closeCurrent/close/dropStream`）、
`net/OriginPrefetchPump.kt`（循环调 `maybePrefetch()`）、
`tools/p292_startup_ab.sh`（`KIVO_PREFETCH` 开关 + cleanup 复位）。
红线：跳读 / 让路 / 关闭必须 abort pending；总连接数 ≤2；pending 只等响应头
不读 body（socket 背压自限驻留字节）；`debug.kivo.chunkPrefetch=off` 对照臂走旧路径；
每次预开 / 提升 / 作废留 `[prefetch]` 日志，不许静默。

⚠️ 装置 `--mbps` 是**每连接限速**，本地 A/B 会偏乐观 ⇒ 需补 `--global-mbps`
（总带宽固定）保守对照；公网友源站是否允许重叠等待仍需另测。

### 13.5 状态与诚实边界

- 方案 C 代码暂留作 A/B 开关，但已改 **opt-in**（`debug.kivo.rebuffer=on` 才启用，
  缺省 / `off` 完全不动作）⇒ 真实会话走 291 基线（收益仅「首次穿仓推后 4 s」，
  bf 次数不减、最长略增，不构成修复）；两个 soak 脚本的 cleanup 与默认值同步复位 `off`。
- `originKeepAlive=0` / `stallWatchdog=on` / `supplyPressure=on` 维持 291 默认。
- 本节全部是**本地装置短窗口确定性判据**，不是公网整体流畅度结论；
  §12.7 的边界不变：**不能宣布整体对齐或超越 SenPlayer**。

### 13.6 证据索引

- 292 A/B：`docs/evidence/p274-startup/p292-startup/{off,w4,w6,w8,w10}/`
  （各含 `net.log` / `kernel.log` / `diag.log` / `playing.png`）
- 装置日志：`docs/evidence/p274-startup/p292-startup/origin-*.log`
  （`CHUNK-HDR#` 注入记录、`FALLBACK=0` 有效性前置判据）
- 工具：`tools/p292_startup_ab.sh`、`tools/p274_local_origin.mjs`

## 十四、293 方案 D：Source 层有界并行短前瞻（消除表头空窗）

> 承接 §13.3 真因：串行补块每块 `hdrMs≈3620ms` 表头空窗、`rate=0` 供给归零。
> 证据：`docs/evidence/p274-startup/p292-startup/{ctl,pf}/`、`docs/evidence/p274-startup/p293-seek{,2}/`。
> 装置 `p274_local_origin.mjs --chunk-hdr-ms 3500 [--global-mbps N]`；
> 驱动 `tools/p292_startup_ab.sh`（70 s/臂，独立端口 + `run/arm` 禁续播）。

### 14.1 做法（★ 重叠等待，不是叠加带宽）

当前块剩 2 MiB 时后台预开下一块 Range，**只等响应头、不读 body**；
当前块读满后把 pending 流**提升**为当前块，跳过 `open()` 的表头等待。
预开在等响应头时源站还没开始吐字节 ⇒ 两条流时间错开；被 TCP 背压卡住的
预开响应不推进全局带宽桶 ⇒ 不抢主链路配额。这正面回应 275 §5.2
「并发预读须证明源站允许叠加带宽」——本方案**不需要叠加**。

### 14.2 家族树（单文件单职责）

| 文件 | 职责 |
|---|---|
| `net/OriginChunkPrefetch.kt`（新增 ~200 行） | 异步预开 / 暂存 / `epoch` 校验作废 / opt-in 开关 |
| `net/OriginConnectionLease.kt` | `open()` 先试复用、`closeCurrent/close/dropStream` 作废、`maybePrefetch()`（含越界判定） |
| `net/OriginPrefetchPump.kt` | ④ 分支调 `maybePrefetch()` |
| `tools/p292_startup_ab.sh` | 5 段规格 `label:rebuffer:wait:initial:prefetch` + `pf=` 判据 |
| `tools/p274_local_origin.mjs` | `--global-mbps` 全局带宽桶（保守对照） |

红线：跳读 / 让路 / 丢流 / 关闭必须 abort pending；总连接 ≤2；起点精确匹配才复用；
每次动作留 `[prefetch]` 日志；`debug.kivo.chunkPrefetch` **opt-in（默认 off）**。

### 14.3 三场景 A/B（本地装置确定性，`CHUNK_HDR_MS=3500`）

| 场景 | 臂 | 首帧 | bf 次 | bf 总 | 最长 | 首次 underrun | 末 runway |
|---|---|---|---|---|---|---|---|
| 每连接 0.6 MB/s（乐观） | ctl | 4.9s | 2 | 4.2s | 2.3s | +14.0s | 12.4s |
| | pf | 4.8s | **0** | **0s** | **0s** | **none** | 14.0s |
| 总带宽 0.6 MB/s（保守） | ctl | 4.7s | 2 | 5.2s | 2.9s | +19.9s | 16.8s |
| | pf | 5.2s | **0** | **0s** | **0s** | **none** | 17.6s |
| 总带宽 0.35 MB/s（余量 9%） | ctl | 5.1s | 7 | 24.6s | 6.9s | +19.5s | 1.6s |
| | pf | 5.0s | **4** | **12.3s** | 5.3s | +11.5s | **9.3s** |
| **240 s 长稳**（扩容片段 128 MiB） | ctl | 4.8s | 3 | 6.1s | 2.6s | +11.8s | 17.3s |
| | pf | 4.9s | **0** | **0s** | **0s** | **none** | 15.9s |

- 240 s 长稳 `FALLBACK=0`（片段充足）：pf 臂**全程零 buffering**；`underrun=false` 全程、
  runway 在 **11.3–19.8 s** 锯齿波动、最低 11.3 s 从未穿仓；`total` 封顶 **72 M**
  （= mpv 64 MiB 前向 + 8 MiB 回看，**无累积超买**）。
- 长稳期间仅 1 次 `[play] prefetch 作废 pending（丢流）` + 2 次 `[thumb] 换流作废`，
  均无后续 buffering ⇒ 作废 + 重开路径正确；**无 `prefetch 失败` / `等待超时`**。

- 前两场景 bf **2→0**（起播连环 buffering 完全消除）。
- 保守对照**独立复跑两次**：首帧 4.7/5.2 s 与 4.8/4.8 s ⇒ 首帧代价**在噪声内**
  （不是稳定的 +0.5 s）；两次 pf 臂均 bf=0、首次 underrun none、`FALLBACK=0`。
- pf 臂 kernel **全程零 `Enter buffering`**；`[cache] underrun=false`、runway 单调爬升。
- 极限场景（物理余量仅 9%）pf 仍显著改善（bf −43%、总时长 −50%、末 runway ×5.8），
  但**不能完全消除** —— 符合 §6.2 物理缺口边界，不宣称"修复"。

### 14.4 seek 正确性（数据错乱的致命风险）

`kivo.landSeekSecs "100,150"` 走与用户拖动松手**完全相同**的 `seekTo` 路径：
- **首轮**：`prefetch 作废 @1935985234（期间换流/关闭）` ⇒ pending 在换流时正确作废；
  同时暴露**越界 bug** —— 最后一块 `endExclusive == 片源总长` 时仍预开，请求打到片尾之外
  ⇒ 装置 `FALLBACK=1`。
- **修复**：`maybePrefetch()` 补 `total > 0 && next >= total ⇒ return`（`open()` 早有同样判定）。
- **复验**：`FALLBACK=0`、越界请求消失、seek 后 `启动 @33091867 → 复用预开块 @33091867` 正常。

### 14.5 边界（未闭合，不许宣称完成）

- ✅ **长稳已闭合**：用 curl 从源拉 128 MiB 片段（`main-128.bin`，覆盖 20–148 MiB），
  240 s 两臂 `FALLBACK=0`、pf 全程零 buffering、runway 最低 11.3 s、`total` 封顶 72 M。
  （原 48 MiB 片段只够 12 块，240 s 会 `FALLBACK=199` ⇒ 那次数据作废；
  **教训：片段长度必须 ≥ 时长 × 码率**，且脚本 `OUTROOT`/`MAIN` 必须可覆盖，
  否则多场景互相覆盖、证据不再可复算。）
- ⚠️ **公网未验**：公网表头 1.5–8 s 随机、源站可能限并发 ⇒ `not_measured + reason`。
- ⚠️ 默认保持 **opt-in off**（`debug.kivo.chunkPrefetch=off`）—— 证据尚不足以上默认。
- ⚠️ `thumb` 源同样会预开（多一条连接）；与 `OriginSupplyPressure` 让路闸的关系未单独验证。
- ⚠️ 整体仍**不能宣布对齐或超越 SenPlayer**：§12.7 / §13.5 的边界一字不改。

### 14.6 证据索引

- 保守对照（独立留档，两次一致）：`docs/evidence/p274-startup/p293-conservative/{ctl,pf}/`
- **240 s 长稳**（扩容片段）：`docs/evidence/p274-startup/p293-longrun/{ctl,pf}/`
- 最终版冒烟：`docs/evidence/p274-startup/p293-final/pf/`
- 极限场景（总带宽 0.35 MB/s）：`docs/evidence/p274-startup/p292-startup/{ctl,pf}/`
- seek 首轮（含越界暴露）：`docs/evidence/p274-startup/p293-seek/`
- seek 复验（修复后 `FALLBACK=0`）：`docs/evidence/p274-startup/p293-seek2/`
- 装置 / 驱动：`tools/p274_local_origin.mjs`、`tools/p292_startup_ab.sh`

## 十六、298 HDR10+（SMPTE ST 2094-40）实测

真实片源：NAS `M3gan.2.0.2025...HDR10+.H.265-CHDWEB.mkv`
（21 GB / **3832x1600 宽银幕** / 10bit / HDR10+ / DDP 5.1 Atmos）。

| 路径 | 解码器 | CL | 上屏 fps | p99 | >200ms |
|---|---|---|---|---|---|
| 硬解 | `hevc_mediacodec` | unknown | **60.51** | 29.0 ms | 0 |
| 软解 | `hevc`（ffmpeg） | **uhd** | 50.41 | 83.9 ms | 0 |

- **★ 元数据三层**：码流里完整（Mastering **4000 nits** / Content light **884,689** /
  **HDR10+ maxscl [12167, 9551, 10732]、targeted_max_luminance 500、9 个分布百分位**）；
  硬解只把 64 B `hdr10-plus-info` 交给 MediaCodec 的 Java 层，ffmpeg **不转 side data**
  ⇒ mpv 看不到；软解能拿到（`CL=uhd`）。
- **我们主动用内容分析**：`hdr-compute-peak=yes` + `tone-mapping=bt.2390`，不依赖逐帧元数据
  —— mpv 推荐方案（对元数据标错的片源更鲁棒），**不是缺陷**。
  ⚠️ **未验证**：内容分析 vs `tone-mapping=st2094-40` 的观感差异（需参考显示器）。
- **边界**：真 HDR 输出仍不闭环（K40 gralloc 无 10bit，与编码无关）；只测片头段，
  **未测暗部/高光滚降/快速运动**；软解 p99 83.9 ms 有周期性抖动（硬解 29.0 ms）。
- 新增诊断开关 `debug.kivo.hwdec`（缺省不写，`=no` 强制软解）。
- 证据：`docs/evidence/p298-hdr10plus/`（硬解）、`p298-hdr10plus-soft/`（软解）。

## 十五、293-A AV1 支持（dav1d 1.5.0 重编 ffmpeg）

- **修复前：不支持**。`libavcodec.so` 无 dav1d；ffmpeg 原生 `av1dec` 在没有 dav1d/libaom 时
  只是硬件加速外壳、不能真正软解 ⇒ K40（SM8250 无 AV1 硬解）硬解失败后回退，
  反复 `av1: Get current frame error`、**黑屏 `00:00/00:00`**。
  （PC 对照：dav1d 解同一文件 `exit=0`；强制 `-c:v av1` 复现一模一样错误。）
- **修复后：支持**。`libdav1d (av1) - dav1d AV1 decoder by VideoLAN` 注册、
  `Selected decoder: libdav1d`、`libdav1d 1.5.0`、`VO: [gpu-next] 1920x1080 yuv420p` 出画。
  1080p AV1 实测 **上屏 60.53 fps**（源 30 fps + 帧插值）、帧间隔 p50 16.5 ms、
  `>200ms` **0 次**、漏采窗口 0 段。
- **手法**：dav1d 1.5.0 用 meson + NDK30/API26 交叉编译（`libdav1d.so` 849136B / AArch64）；
  ffmpeg n7.1 在**原 configure 参数原样**基础上加 `--enable-libdav1d --enable-decoder=libdav1d`；
  只替换 `jniLibs/arm64-v8a/libavcodec.so` 并新增 `libdav1d.so`
  （`libmpv.so` 是**动态**依赖 libavcodec 且 SONAME 无版本号 ⇒ **无需重编 mpv**）。
- **回归**：HEVC 片源 `Using hardware decoding (mediacodec)` + `aimagereader` + `gpu-next`
  + bf 2 次 / 首帧 4.8 s / `FALLBACK=0` —— 与换 ffmpeg 前逐项一致。
- **4K AV1（同一份 dav1d）**：SVT-AV1 3840x2160/30fps/CRF45 测试片，`Selected decoder: libdav1d`，
  **上屏 60.39 fps**（417 帧/6.889 s，p50 16.4 ms、p99 27.9、`>200ms` **0 次**、漏采 0 段）；
  时间码 4 s 内推进 **183 帧 / 5.9 s** ⇒ **解码 ≥30 fps（实时）**且有余量。
  机理：dav1d 瓶颈是像素级运算（loop filter / CDEF / 预测），由**分辨率主导**，
  码率只影响熵解码小部分 ⇒ 该判定对"分辨率能力"有效。
- **4K 10bit AV1**：`Decoder format: 3840x2160 yuv420p10 bt.2020-ncl/…`、
  `VO: [gpu-next] 3840x2160 yuv420p10` ⇒ 10bit 通路打通；**上屏 59.16 fps**
  （p50 16.2 / p99 50.3 / max 60.4 ms、`>200ms` 0 次），时间码 **198 帧 / 6.6 s**
  ⇒ **解码 ≥30 fps 实时**。比 8bit 吃力（p99 27.9→50.3 ms）。
  > ⚠️ **【301 已作废上面这句的"实时"结论】这是 SVT-AV1 合成图案（色条/斜线/棋盘格）的数字。
  > 2026-11-16 用真实 4K AV1 10bit 剧集实测只有 **10.87 fps**（差 5.4 倍）。
  > 原因：AV1 的 CDEF / loop filter 开销与**画面边缘密度强相关**，色条几乎没有边缘。
  > ⇒ **合成片只能验通路，不能验性能。** 引用"4K AV1 能播"必须注明片源类型。详见 §十九。
- **4K 10bit HDR AV1（PQ / BT.2020）完整闭环**：mpv 识别
  `yuv420p10 bt.2020-ncl/bt.2020/pq` 并启用 `tone-mapping=bt.2390` + `hdr-compute-peak`；
  我们的 L2 tone-map 自动下发 `target-trc=bt.1886 / target-prim=bt.709 / target-peak=100`；
  真 HDR 输出按 K40 能力（gralloc 无 10bit）**正确降级**到 8bit + SRGB（有日志，非静默）。
  上屏 **58.99 fps**（p50 18.4 / p99 45.9 ms、`>200ms` 0 次）⇒ **AV1 的 HDR 内容可正常播放
  （tone-map 到 SDR）**，与 HEVC HDR 行为一致。
  （手法：SVT-AV1 的 HDR 参数须走 `-svtav1-params "color-primaries=9:transfer-characteristics=16:
  matrix-coefficients=9"`，无 `--` 前缀；`-c copy` 改不了码流内的 color config。）
- **边界**：K40 **无 AV1 硬解**，本项是纯软解；4K 用**合成图案**，真实高码率/复杂运动
  4K AV1 未测；**真 HDR PQ 输出未闭环**（K40 gralloc 无 10bit，属硬件限制，与 AV1 无关）；
  AV1 的 DoVi 通路未测。
- 证据：`docs/evidence/p293-av1/`（修复前）、`p293-av1-dav1d/`（1080p + 帧率）、
  `p293-av1-4k/`（4K + 时间码）、`p293-regress/`（回归）。
  回滚点：`ffmpeg-shared-dist.bak-p293`、`jniLibs-av1-bak`。

## 十七、299 真实 AV1 电影实测（补「真实片源」缺口）

> 293-A 用的是**自造 SVT-AV1 合成片**，真实电影级 AV1 一直是遗留缺口。
> 2026-11-16 02:25 用户下载了真实 AV1 电影，本轮当场实测。

**片源（ffprobe，非文件名推断）**：`[奥德赛].The.Odyssey.2026...1080p.AV1.OPUS.5.1-CMCT.mkv`
3,691,322,886 B / 10,355.7 s（2h52m）/ 2.85 Mbps / **AV1 Main 1920×1080 `yuv420p10le` 23.976 fps**
/ **OPUS 5.1 48 kHz** / 4 条 PGS / MKV cues 在片尾。

| 项 | 实测 | 判读 |
| --- | --- | --- |
| 解码 | `av1_mediacodec: Unsupported or unknown profile` → **libdav1d 软解** | K40 无 AV1 硬解，有日志、非静默降级 |
| 位深 | `Decoder format … yuv420p10`、`VO: [gpu-next] 1920x1080 yuv420p10` | **10bit 全程保持** |
| 上屏 | **60.44 fps**，p50 16.7 / p99 24.8 / max 49.8 ms，`>200ms` **0 次** | 软解满帧 |
| 音频 | `AO: [audiotrack] 48000Hz 5.1 6ch float`，无 underrun | **5.1 直通未降混** |
| 远跳 9000 s | seek→首帧 **708 ms**，落点 **9000.033 s（误差 33 ms）** | 3.44 GiB / 片尾 cues 正常 |
| 供数 | `underrun=false`，runway 10–18 s | 健康 |

**边界（未闭合）**：① **1080p 不是 4K**，真实 4K AV1 仍未测；
② 只测起播 + 9000 s 两点，**无 30 分钟长稳**；
③ 日志 `can't find selected font provider` / `fontselect: failed … Noto Sans CJK JP`
⇒ **ASS/文本字幕缺字体**；PGS 位图不依赖字体，但本轮**未开字幕，PGS 上屏仍未验证**
（此为 p299 当轮边界；字幕三类已在 §二十一 / D303 全部闭环，缺字体根因 `osd-fonts-dir` 已修）；
④ 供片恒定 4 Mbps（真实 1.4 倍），低带宽下未测。

证据：`docs/evidence/p299-av1-real-movie/status.md`。

## 十八、300 Surface 重建 100 次 + 连续 seek 20 次（275 §6.2 两项验收 + 一个真 bug）

> 275 §6.2 要求「100 次前后台或视图重建无 native crash/泄漏」与
> 「连续 20 次 seek 无旧请求覆盖新目标」。两项此前**零证据**，本轮补齐。
> 用本地 26.6 MB / 180 s 片源（不用公网，避免网络波动污染结论）。

### 18.1 Surface 重建 100 次 —— PASS，但挖出一个 global ref 泄漏

| 判据 | 结果 |
| --- | --- |
| pid 不变 | ✅ 12783（修复前）/ 24086（修复后），100 次全程未变 |
| 真实重绑次数 | ✅ diag `attach-surface gpu-next (wid set)` **1→101**（每轮都触发，不是"看着没崩"） |
| Native Heap | 17344→16224 / 18008→15676 KB（下降，无暴涨） |
| crash / 被杀 | 无 |

**发现的 bug**：`jni_mpv_bridge.cpp:142` 每次 `NewGlobalRef(surface)`，而
`mpv_session.cpp:48` 直接 `surface_global_ref = wid` **覆盖旧值**；
全仓 `DeleteGlobalRef` 只有会话销毁一处 ⇒ **每次 rebind 泄漏 1 个 global ref**。
压测的 `1→101` 与泄漏次数一一对应。表满会 `global reference table overflow` abort。

**修复**：先 `mpv_set_option("wid")` 成功 → 再 `DeleteGlobalRef(旧)` → 最后存新值。
**CheckJNI 验证**（`debug.checkjni=1` 跑 20 次）：`JNI ERROR` / `global reference` /
`use after` / `Fatal signal` **0 条** ⇒ 删除时机安全，未引入野指针。

⚠️ **诚实边界**：泄漏在 100 次量级**测不出内存后果**（每次约 1 KB，淹没在波动里）。
本条 PASS 靠**代码路径 + 计数**证明，不靠内存数字；要观测后果需跑到表上限（数千次），未做。

### 18.2 连续 20 次乱序 seek —— PASS，但"取消过期 seek"仍未实现

| 指标 | 第 1 次 | 第 2 次 |
| --- | --- | --- |
| 落点误差 | 0–458 ms | 0–542 ms |
| 单次耗时 | 100–216 ms | 100–175 ms |
| 无有效落点 | 0 次 | 0 次 |

**暂停态 seek**：`setPaused(true)` 生效 → seek(60 s) → **仍 PAUSED**、落点 **60000 ms** 精确命中
（275 §4.2 第 4 条）。装置：`MpvSeekBurstInstrumentation`（新增 androidTest，走真实会话路径）。

⚠️ **口径**：`seekTo` 是**同步等落点**（上限 1.5 s），20 次实为**串行**（输出 `serial=true`）。
本轮证明的是「乱序连发都正确且不卡死」，**不**证明「取消/合并过期 seek」——
该机制**仍不存在**，快速拖动会在 UI 侧排队。这是未闭合 gap，不能被本条 PASS 掩盖。

### 18.3 仍未覆盖的 Surface 重建路径

旋转、分屏、PiP 进出、SMB/公网源下的 rebind 均未测（本轮只覆盖 HOME→回前台 + 本地源 + K40）。

证据：`docs/evidence/p300-surface-rebind/status.md`、`seek-burst.txt`；
装置：`tools/p300_surface_rebind_soak.sh`、`MpvSeekBurstInstrumentation.kt`。

## 十九、301 真实 4K AV1 HDR：10.87 fps —— K40 CPU 上限（并作废合成片结论）

> 片源：`Bridgerton.S01E01.2020.2160p.NF.WEB-DL.DDP5.1.Atmos.HDR.AV1-HHWEB.mkv`
> **AV1 Main / 3840×2160 / yuv420p10le / smpte2084(PQ) + bt2020 / EAC3 Atmos 5.1 / 58m34s / 11.9 Mbps**

### 19.1 链路全通（这部分是 PASS）

`Selected decoder: libdav1d` → `Decoder format: 3840x2160 yuv420p10 bt.2020-ncl/bt.2020/pq CL=uhd`
→ `VO: [gpu-next] 3840x2160 yuv420p10` → `tone-mapping="bt.2390"` → `eac3` 解码；
真 HDR 输出仍因 K40 gralloc 无 10bit 退回 SDR + SRGB（有日志，非静默）。

### 19.2 帧率 10.87 fps，且已排除其它原因

| 指标 | 值 |
| --- | --- |
| 上屏 | **10.87 fps**（p50 19.8 / p99 **1001.4** / max 1761.8 ms，`>200ms` **60 次**） |
| 供数 | `underrun=false`、`fw` 9→38.5 MB、runway 10–19 s ⇒ **排除** |
| 插值 | `Detected fps ratio 0.0040 below threshold, disabling interpolation` ⇒ 内核 **3.6 s 就自己关了**，排除 |
| 丢帧 | `Discontinuous source PTS jump 191→201→210→218…` 每次跳 7–10 s ⇒ **丢帧追音频** |
| CPU | `766% user` / idle 55%；`top -H` **9 个线程 55–87%** = dav1d 的 9 个解码线程 ⇒ **解码打满** |

### 19.3 归因（三重对照，不是推测）

1. 同机 **4K HDR HEVC 硬解 = 59.47 fps**（282）⇒ 4K 渲染/显示/tone-map 能跑；
2. 同机 **1080p AV1 10bit 软解 = 60.44 fps**（299）⇒ AV1 通路正确；
3. **4K AV1 10bit 软解 = 10.87 fps 且 CPU 打满** ⇒ 瓶颈是 **dav1d 的 CPU 算力**。

### 19.4 结论与边界

| 场景 | 结果 |
| --- | --- |
| 1080p AV1 10bit 真实电影 | ✅ 60.44 fps |
| **4K AV1 10bit 真实剧集** | ❌ **10.87 fps，不可看** |
| 4K HEVC HDR 真实内容 | ✅ 59.47 fps（硬解） |

K40（SM8250）**无 AV1 硬解**，软解 4K 10bit 超出 CPU 能力 ⇒ **设备硬件上限，非项目缺陷**。

**未闭合**：① **SenPlayer 同源实测未做**（理论上同受 CPU 限制，但无证据，记 `not_measured`，
不可断言"它也卡"）；② `hdr-compute-peak` 关闭对照的采样跨度 88 s > 采样 30 s，
口径已污染，**不能**据此说"关掉更差"；③ 4K AV1 **8bit** 未测。

### 19.5 方法论（第二次付代价换来）

- **合成片只能验通路，不能验性能**（本轮推翻 §十五 的 59.16 fps）。
- **做性能 A/B 前先查调试属性残留**：本轮 `debug.kivo.hdrPeak=on` 是历史取证残留，
  首轮 10.87 fps 里混了逐帧峰值计算。清掉后仍 10 fps 量级（主因是解码），
  但**残留足以让归因跑偏**。

证据：`docs/evidence/p301-4k-av1-hdr/status.md`。

## 二十、302 4K AV1 选源闸门（把 301 的结论变成防线）

> 301 证明「4K AV1 无硬解 = 10.87 fps」，而 4K AV1 的码率（11.9 Mbps）**低于**选源阈值 12 Mbps
> ⇒ 原 `EmbySourcePicker` 会把它选成"可播版本里画质最好的"。**这是个会导致用户点开就卡的逻辑缺口。**

### 改动（3 文件）

| 文件 | 改动 |
| --- | --- |
| `playback/Av1HardwareProbe.kt`（新） | `hasHardwareAv1()` 查 MediaCodecList 硬解（软解不算）；`isLikelyUnplayable()` 只拦"4K+ AV1 且无硬解" |
| `emby/EmbySourcePicker.kt` | `Item` 带 `codec`/`height`（从各 source 自己的 `MediaStreams` 读）；`pick()` 先过编码闸门再走原码率规则；reason 写明跳过/全部不可播 |
| `ui/compose/PlayerStartPolicy.kt` | **复用既有规格 Toast**，追加「本机无 AV1 硬解·可能无法流畅」（零新控件、零布局变化） |

### 验证：15/15 PASS

| # | 用例 | 结果 |
| --- | --- | --- |
| 1–6 | 原有 6 条全部仍 PASS（**无回归**） | ✅ |
| 7 | 4K AV1 11.9M + 1080p HEVC 8M，无硬解 ⇒ 选 **1080p HEVC** | ✅ |
| 8 | 同输入，有硬解 ⇒ 选 **4K AV1**（不误伤） | ✅ |
| 9 | 全 4K AV1 无硬解 ⇒ 选最小 + 原因写明 | ✅ |
| 10 | 1080p AV1 **不拦**（实测 60.44 fps） | ✅ |

**真机探测值**：`av1Hardware=false`（K40）⇒ 闸门在真机上确实生效，不是只在注入参数时成立。

### 边界

1. **Toast 未在真实 4K AV1 实播中目视确认**（NAS 可达但 K40 未保存该 SMB 源/凭据）——
   判定链已由单测+代码路径确认，"用户看到那句话"未取证。
2. Emby 侧无真实 4K AV1 条目，用例 7–10 是构造 JSON（字段取自真实响应结构）。
3. 只覆盖 AV1；4K VP9 等软解跑不动的组合**不猜**（无实测证据）。
4. 选源只影响 Emby；本地/SMB 直选文件无多版本可选，只能靠 Toast 提示。

证据：`docs/evidence/p302-av1-source-guard/status.md`。

## 二十一、303 字幕闭环：PGS 已通，SRT/ASS 修好（漏设 `osd-fonts-dir`）

> 275 §5.4 一直记「PGS 未闭环」，301 又冒出 `can't find selected font provider`。本轮一次查清。

### 21.1 PGS 位图 —— 闭环

| 步骤 | 结果 |
| --- | --- |
| 轨道 | `● Subs --sid=1 '中上英下' (hdmv_pgs_subtitle)`，共 4 条 |
| 真实区间 | ffprobe 扫包 = **33.450 s 起**（不是 253 那部片的 13.6 s） |
| seek 34 s 截图 | **「一支舰队… / a fleet…」清晰上屏** |

⇒ PGS 在 gpu-next 路径下能画。之前"OSD 落在看不见的位置"的假设**不成立**。

**教训**：必须先 ffprobe 确认"这一刻本来有没有字幕"。本轮第一次拍 16 s（片头 Logo）
差点误判成"PGS 坏了"。

### 21.2 SRT/ASS 文本 —— 根因是漏了一个选项

现象：`can't find selected font provider` + `fontselect: failed to find any fallback` ⇒ 画面全空。

**误导项**：`can't find selected font provider` 是无害的。
libass 没编 `CONFIG_FONTCONFIG`（config.h 实读，coretext/directwrite 也无），
`font_constructors[]` 只剩 `{NONE, NULL, NULL}` ⇒ 循环不执行 ⇒ 打警告。
但 `ass_embedded_fonts_add_provider()` 会**无条件**扫描 `fonts_dir` 并打 `Loading font file`，
**与 fontconfig 无关**。

**真根因**：**`osd-fonts-dir` 没设**。
- `sub-fonts-dir` 设了，且 `options/sub-fonts-dir` 读回来**正确**（"看配置"看不出问题）；
- 但 `mp_ass_init()` 读的是 **`osd_style_opts.fonts_dir`**（字幕由 OSD 层统一渲染）；
- ⇒ libass 拿到空目录 ⇒ **一条 `Loading font file` 都没有**。

注释里已写「`osd-font` 与 `sub-font` 都要设」，**漏了同样要设的 `osd-fonts-dir`**。

**修复（一行）**：`mpv_sub_options.cpp` 加 `{"osd-fonts-dir", dir, true}`。

### 21.3 修复后证据

```
Loading font file '.../subfonts/NotoSansCJK-Regular.ttc'
Loading font file '.../subfonts/NotoColorEmoji.ttf'
fontselect: (Noto Sans CJK JP, 400, 0) -> NotoSansCJKjp-Regular, 0, NotoSansCJKjp-Regular
```

画面级：SMB + 外挂 `probe.srt` ⇒ `sub-add added=true` / `sub-select id=6 ok=true`
⇒ 截图底部显示 **`KIVO-MPV-SUBTITLE-PROBE-AAAA`**。

### 21.4 界面外的两个坑（都是操作者造成的）

1. `kivo.subFile` / `kivo.subtitleId` **只在 SMB 路径生效**（`PlayerSmbStart:193`）；
   `kivo.playurl` 的 URL 路径**完全不执行**这段 probe。
2. `kivo.subFile` 是**相对 `filesDir` 的文件名**，传绝对路径会被拼成不存在的路径。

### 21.5 ASS 特效 —— 也闭环了

`kivo-fx-test.ass`（`\an8` / `\pos`+`\c` / `\frz`+`\fscx/y` / `\k` 四类一次覆盖）挂到 SMB 链路，
`sub-add added=true` / `sub-select id=6 ok=true`，画面逐项对上：

| 特效 | 画面结果 |
| --- | --- |
| `\an8` 顶部 | 「ASS 特效测试 · 顶部固定」在顶部 ✅ |
| `\pos(960,760)`+`\c` 黄 | 「黄色 · 定位 pos」位置对、**黄色** ✅ |
| `\frz20\fscx120\fscy120` | 「旋转缩放 frz」**明显倾斜 + 放大** ✅ |
| `\k60` 卡拉 OK | 正常渲染 ✅ |

`Arial` 本机不存在 ⇒ libass 自动 fallback 到 `Noto Sans CJK JP`，字形正常（fallback 也工作）。

⇒ **PGS / SRT / ASS 三类字幕全部闭环**，275 §5.4 的字幕缺口到此收口。

### 21.6 边界

文本字幕只在 **SMB 路径**做画面级验证（URL/本地路径的 probe 钩子不执行，但渲染与路径无关）；
滚动弹幕与复杂 `\t` 动画只有旁证未逐项断言；中文 SRT 未测（字体已匹配 CJK）；
字幕位置/样式设置（`sub-pos`/`sub-align-*`）未验。

证据：`docs/evidence/p303-subtitle-close/status.md`。

## 二十二、304 异步 seek + 合并过期请求（275 §4.2 最后一格）

> 300 把这条标成 `serial=true`。追下去发现**比排队更严重**：`performSeek()` 在
> **UI 线程**调同步 `seekTo()`（等落点上限 1.5 s，实测 100–216 ms/次）
> ⇒ 拖动松手那一刻主线程被占住 6–13 帧；连拖 5 次累加 0.5–1 s，画面还逐个跳。

### 改法（2 文件）

| 文件 | 改动 |
| --- | --- |
| `playback/MpvPlaybackSession.kt` | 新增 `seekToAsync()`：投递会话自己的单线程 worker，**票据比大小，非最新直接返回**（连 seek 都不发）；落点回来再比一次，过期结果不许覆盖 `positionUs`；新增 `seekExecuted` 诊断。同步 `seekTo()` 保留给探针/instrumentation |
| `ui/compose/PlayerActionRouter.kt` | `performSeek()` 的 **mpv 分支**改调异步；其余三条链路未动（无实测数据，不凭猜改） |

### 验证：3/3 PASS（`MpvSeekAsyncInstrumentation`）

| 用例 | 实测 |
| --- | --- |
| 20 次乱序提交 | **1 ms**（同步版 2 s+） |
| 实际执行 | **1 次**（提交 20 次） |
| 落点 | 165333 ms（目标 165000）drift **333 ms** |
| 连发 170s→20s | 落点 **20000 ms（drift 0）**，170s 被跳过 |
| **主线程**提交 20 次 | **1 ms** |

### 行为变更（必须记录）

1. **`kivo.seekStress` 语义变了**：原先专压"覆盖竞争"，现在中间目标被合并
   ⇒ 覆盖竞争不再存在 ⇒ **历史数据与今后数据不可直接比较**。
2. `kivo.landSeekSecs` 探针也走异步，但投递只花微秒级，对秒级测量影响 <5 ms。
3. 异步 seek 期间 `poll()` 会推迟（共用 worker），落定后位置被落点覆盖。

### 边界

**未做手势级体感验证**（"主线程 1 ms"是硬指标，体感改善是推论）；
只改 mpv 链路；只测 K40 + 本地片源（公网源落点更慢，合并收益应更大，未实测）。

证据：`docs/evidence/p304-async-seek/status.md`。

## 二十三、305 补 `network_recovery_ms` + `thermal_context`（275 §6.1 指标收口）

### 23.1 `thermal_context`

`playback/ThermalSnapshot.kt`（新）：PowerManager 热状态（API 29+）+ 扫
`/sys/class/thermal/` 下所有 `thermal_zoneN/temp` 取最高温。

真机两次会话：`{"status":"NONE","code":0,"max_temp_c":67.4}` / `61.6`。

### 23.2 `network_recovery_ms`

275 的定义是「**网络恢复 → 媒体时钟再次前进**」。状态机三道闸门缺一不可：
**位置停 ≥3s** + **系统报离线** + **时钟重新增长**。
（只判"离线"会把暂停记成断网；只判"网络回来"会在等表头时提前记完。）

| 文件 | 作用 |
| --- | --- |
| `playback/NetworkRecoveryWatch.kt`（新） | 纯状态机，不碰 Android API，可单测 |
| `playback/MpvCacheDiag.kt` | 每 2 s 喂一次（位置 / `pause` / `isOnline`）；attach 加 context |
| `playback/PlaybackMetrics.kt` | `finish()` 输出该字段，未发生则 `not_measured` |

**状态机单测 4/4 PASS**：正常播放不计 / **暂停不误判** / 断网恢复精确记 2000ms /
**位置停但在线不计**（后两条最容易误报）。

**真机端到端（第一次成功）**：
`[recovery] 网络恢复 → 媒体时钟前进 2002ms（离线 40058ms）`
—— 与 P253 手工测的 2.0–2.2 s 一致。

### 23.3 第二次端到端失败，但暴露一个真问题

同样流程第二次跑，没有记到恢复。日志：

```
PROXY read fail @528303793 attempt=360: SMB 连接已断开，重连失败
[ffmpeg] http: Will reconnect at 528303793 in 0 second(s), error=Connection timed out.
```

**SMB 层重试 360 次仍未恢复**，时钟确实没前进 ⇒ 指标**正确地**没记
（网络回来 ≠ 播放回来，这正是 275 要的口径）。

⚠️ **待查**：`svc wifi disable/enable`（接口消失再出现）场景下 `SmbByteSource` 重连失败。
P253 的 62 s 能 2.2 s 接上，本轮 70 s 却失败 —— 是时长边界还是"接口消失"这一差异，需单独复现。
**未修，记入待办。**

### 23.4 边界

网络恢复分辨率 ±2 s（采样周期）；只记最近一次；thermal 是**时间点快照**不是曲线；
断网测试用 `svc wifi`（接口消失），与"路由器掉线"（接口在/链路断）不是同一种失败模式。

证据：`docs/evidence/p305-metrics/status.md`。

## 二十四、306 SMB 重连排查：策略无缺陷，端到端被环境挡住

> ## ⚠️ 24.2 的结论已作废（308 复验推翻，2026-11-16 04:30）
>
> 原文「小米路由 SMB 共享不可达」**是错的**。路由 SMB 一直正常（起播 `[cache]` rate 8.2 M/s）；
> 之前失败是「`svc wifi enable` 后立刻起播、网络栈未就绪」的**操作时序问题**。
> 复验：**断网 70 s → 2021 ms 恢复**、**断网 40 s → 2002 ms 恢复**，`[smb] 重连失败` **0 次**。
> ⇒ **SMB 重连工作正常**，24.1 的代码审查结论成立，24.2 的环境判断收回。详见 §二十六。

> 305 第二次断网出现 `attempt=360 重连失败`，需要判定是策略缺陷还是环境。

### 24.1 策略层：没有缺陷

| 检查点 | 结论 |
| --- | --- |
| 退避 | 400→800→1600→3200→4000 封顶；**离线时不翻倍** |
| 恢复即重试 | `canAttempt()` 离线转在线时**清零退避** ⇒ 不白等 4 s |
| 会话重建 | `SmbReader.open()` **每次 `nativeCreate()` 新建 ctx** ⇒ 无全局残留 |
| 死句柄 | 重连前 close + `dead=true` |

**JVM 单测 6/6 通过**（含 `link recovery allows an immediate attempt`）。

### 24.2 端到端未复现：路由 SMB 不可达

复现时起播即失败，UI 停在「播放失败 · 无法连接服务器」，net-diag 只有 3 行 `[hdr]`；
设备网络正常（`wlan0 192.168.5.152/24`、`wifi_on=1`）
⇒ **小米路由 `192.168.5.1` 的共享当前不可达**，与 `attempt=360` **可能同源**。

⇒ 当前无法区分「路由 SMB 掉了」与「重连真有缺陷」，**不下结论**。

### 24.3 补上的真实缺陷：失败日志丢了 `detail`

`SmbPreadChannel` 原只记 `failed.message`，而 `SmbFailure` 带着
`stage` + `detail`（`nt=0x… raw=…`）—— 那是区分「认证错 / 共享不存在 / 网络不可达」的唯一信息。
丢掉后三种故障在日志里长得一样，本次就被这点卡住。

**改法**：`NetDiag.log("[smb] 重连失败 stage=… detail=…")`，**用户文案与行为不变**。

### 24.4 待办

① 等路由 SMB 恢复后复现，按 NT 状态码判定（`0xC00000CC`=共享不在 / `0xC000006D`=认证 /
`0xC0000121`=网络超时，**只有后者才可能是重连问题**）；
② 路由短期不回就用 PC 的 SMB 共享做同样实验；
③ 62 s（P253 成功）与 70 s（305 失败）之间是否有边界，做 60/70/90 s 三档。

### 24.5 这一卡的实际产出

✅ 排除重连策略缺陷（审查 + 6 单测）｜✅ 补 `detail` 日志 │
❌ **没修重连代码**（无证据表明它坏）｜❌ **端到端未复现**（环境阻塞，记为阻塞不是通过）

证据：`docs/evidence/p306-smb-reconnect/status.md`。

## 二十五、307 WatchStore 原子写（观看进度不再可能被写坏）

> 275 §5.4 要求「恢复点按固定间隔、暂停、seek、后台离开和播放结束落盘，写入节流且**原子替换**」。
> 原来「节流」有，「原子替换」没有。

### 25.1 风险链

`WatchStore.persist()` 用 `file.writeText()` 直接覆写 `watch.json`（**每秒级高频写**）：
写到一半掉电/被杀 ⇒ 半个 JSON ⇒ 下次 `JSONObject()` 抛异常 ⇒
**被 `runCatching` 整个吞掉（连日志都没有）** ⇒ 续播进度与统计**静默归零**。

### 25.2 修法

| 改动 | 说明 |
| --- | --- |
| `AtomicFile` | 与 `SourceStore`/`FilmMarksStore` 同写法：`startWrite → write → finishWrite`（失败 `failWrite` 回滚）。保证任何时刻磁盘上都有一份完整文件 |
| 损坏留证 | 解析失败时把坏文件改名为 `watch.json.corrupt-<ts>` + `NetDiag.log`，然后从空开始 |
| 构造加 `fileName` | 默认 `watch.json` 不变；测试用 `watch.test.json` **不碰用户数据** |

**为什么不照搬 `SourceStore` 的"永不覆盖"**：源列表低频写、配置最重要；
`watch.json` 高频写，"永不覆盖"会让**新进度也存不下**。折中为"备份 + 从空开始"。

### 25.3 验证 4/4 PASS

正常写读 ✅ / 半截 JSON 不崩且新进度可读 ✅ / 坏文件留 1 份备份 ✅ /
连续写 50 次每次立刻解析都成功 ✅。

### 25.4 边界（不许夸大）

用例 3 **不能证明"旧实现会坏"**（`writeText` 单次小写，50 次正常时序下不易撞上半个文件），
它只证明**没有回归**。原子性的真实价值在**崩溃/掉电窗口**，无法在正常运行中复现。
未测 `ENOSPC`（写满）与真实掉电注入。语义/阈值（95% 视为看完）**一行未动**。

证据：`docs/evidence/p307-watchstore-atomic/status.md`。

## 二十六、308 SMB 重连复验通过（推翻 §二十四 的环境判断）

### 26.1 更正

306 判定「路由 SMB 不可达」是**错的**。实测：`ping 192.168.5.1` 2/2 通、直接起播
`[cache] rate=8.2M/s dur=19.6s` 正常。之前失败的真因是
**`svc wifi enable` 之后立刻起播，网络栈未就绪**（操作时序，非服务端/代码问题）。

### 26.2 有效复验（前提：先确认起播成功再断网）

| 轮次 | 断网 | 恢复耗时 | `[smb] 重连失败` |
| --- | --- | --- | --- |
| A | 70 s | **2021 ms**（离线 44083 ms） | **0 次** |
| B | 40 s | **2002 ms**（离线 6008 ms） | **0 次** |

两次 ~2.0 s，与 P253 手工测的 2.0–2.2 s 一致；失败分支一次都没走到（首次重连即成功）。

⇒ **SMB 重连无缺陷**，§二十四 24.1 的代码审查结论与实测一致。

### 26.3 指标同时闭环

B 轮关掉 `auto_pip` 后拿到真实落盘值：

```json
"network_recovery_ms": 2002,
"thermal_context": {"status":"NONE","code":0,"max_temp_c":62.8},
"writes": 2
```

—— 这是该字段**第一次以落盘形式**被记录（305 两次都被 PiP 挡在 `finish` 之外）。

### 26.4 两个必须记住的坑

1. **`svc wifi enable` 后不要立刻起播**：图标亮 ≠ 网络就绪，先 ping 或等 5–10 s。
2. **`auto_pip` 默认 true** ⇒ HOME 进画中画 ⇒ `onStop` 不触发 ⇒ `PlaybackMetrics.finish` 不执行
   ⇒ metrics 停在起播时刻。验证"结束后才落盘"的指标必须先关它。

### 26.5 仍未覆盖

只测 `svc wifi`（接口消失）；**路由器掉线**（接口在/链路断）是另一种失败模式，未测；
断网时长边界未测；断网期间手动 seek 的交互未测。

证据：`docs/evidence/p308-smb-reconnect-retest/status.md`。

## 二十七、309 HLS / ABR 能力评估（按 275 §5.3「先验证引擎，再实施 ABR」）

> 275 §5.3 原文：「**先验证所选引擎实际支持相应 HLS/DASH 变体，再实施 ABR；
> 不能因为 APK 中有 FFmpeg 命名库就宣称支持。**」本卡就是执行这句话。

### 27.1 HLS **直通可用**（4 项实测通过）

测试流：本地 3 档 HLS（360p/720p/1080p，各 3 个 4 s 分片）+ master。

| 项 | 实测 |
| --- | --- |
| manifest 解析 | `Found 'hls' at score=100`，三个子清单全被打开 ✅ |
| segment 拉取 | `HLS request for '.../v0_000.ts', playlist 0`，经我们的 `LocalRangeServer` ✅ |
| 多档识别 | 3 档 = **3 个独立 video track**（`--vid=1/2/3`）✅ |
| 默认选档 | 启动即 `--vid=3`（1080p）= mpv 文档「Default: max」✅ |
| 手动切档 | `Set property: vid="1"` → 104 ms 解码器就绪 → **293 ms** `VO reconfig` ✅ |
| 切换无黑帧 | 录屏逐帧 YAVG：切档窗口稳定 32.7，**无一帧掉到 16**；仅 ~100 ms（≈3 帧）缺口 ✅ |

### 27.2 ★ 结构差异（309 补测，决定性）

| | 结构 A（无 `#EXT-X-MEDIA`） | **结构 B（有，Apple/Netflix/Emby fMP4 全是）** |
| --- | --- | --- |
| FFmpeg | 1 program + N video stream | **N 个 program**（`ffprobe -show_programs` = 24 个） |
| mpv | N 个 track | **1 个 track**（只 program 0） |
| 运行时切档 | ✅ 293 ms 无黑帧 | ❌ **不能**（mpv 无 `--program` 选项） |

**不是我们的 bug**：代理 `resolve()` 正确、8 个 variant 全被打开、`ffprobe` 直连可见全部 stream；
限制来自 **mpv/FFmpeg 的 program 模型**。

⇒ **真实 HLS 在 mpv 路径上做不了运行时 ABR**。设计必须绕开"切 track"，见 310。

### 27.3 ABR **完全不具备**（不能宣称）

三条硬证据：

1. **mpv 官方文档否定**（`options.rst:935`）：
   > it's **not suitable for this purpose** for various technical reasons. (**It's slow, which can't be really fixed.**)
2. **`--hls-bitrate` 只是启动时静态选择**（`options.rst:5371`，`OPT_CHOICE no/min/max`），
   语义是「selected by default」，**没有吞吐输入**；
3. **无吞吐估计 / EWMA / 迟滞 / 最短驻留** —— 275 要的那套决策层一条都没实现，mpv 也不提供配置项。

⇒ 275 §1.4 的「不得宣称支持 ABR」**仍然有效**。

### 27.4 自研 ABR：技术可行，但两个前提未验

**可行点**：最危险的"切换动作"已实测 293 ms、无黑帧 ⇒ 决策层搭起来就能用。
已有基础件：`OriginLatencyStats`（吞吐）、`RebufferPolicy`（runway）、`Av1HardwareProbe`（codec 闸门）、
`NetDiag`（可观测通道）。缺的是：**按分片采样 + EWMA + 升降档不同阈值 + 最短驻留**。

**未验前提**：
1. **公网下切档耗时** —— 293 ms 是本地 HTTP，不能外推；
2. **音频跟随切档**（3 档各带一条 aac，切 vid 时爆音/断音未测）。

### 27.5 现状应记为

> HLS **直通可用**（manifest/segment/多档/手动切档已实测）；
> **ABR 未实现**；**DASH 完全未测**（MPD/模板/时间线一条都没验）。

### 27.6 未测清单（不许当通过）

seek（URL 路径无钩子，HLS 走不了 SMB 路径）｜鉴权（401/403、Cookie、DRM）｜
HLS 分片级网络恢复｜DASH 全部｜公网 HLS｜`#EXT-X-MEDIA` 多音轨/字幕组｜音频跟随切档。

证据：`docs/evidence/p309-hls-abr/`（含 `sw.mp4` 录屏与 `yavg.txt` 逐帧亮度）。

## 二十八、310 HLS master 选档（960x540 → 1920x1080）

> 309 的结论是「真实 HLS 只能播第一个 program」。本卡把「**选档**」这件事真正实现了 ——
> 它不是 ABR，而是**起播时选对一档**（SenPlayer 也是一个固定档，它只是请求时自己指定了码率）。

### 28.1 做法：代理层裁剪 master

HLS 请求本来就经代理（Java 层做 TLS），所以代理能看到 master：
**解析 24 档 → 按策略选一档 → 只返回那一档**。
内核拿到只有 1 个 variant 的 master ⇒ 不存在 program 选择问题。

新增独立 `hls` 包（不依赖任何业务包）：`HlsVariant` / `HlsMaster`（纯文本解析裁剪）/
`HlsVariantPolicy`（纯决策）/ `HlsForwardPolicy`（参数）。
分片继续流式转发，只有 `.m3u8` 读全量。

### 28.2 验证

| 场景 | 结果 |
| --- | --- |
| JVM 单测（含真实 Apple master 6291 B） | **15/15** |
| Apple 多档流（真机） | `master 6291 → 1735`、选 **1920x1080 8.2 Mbps**、子清单 10+ → **2 个** |
| 单档流（真机回归） | `200 → 200`（不裁剪），行为不变 |
| 画面 | `sp-apple-1080p.png` 正常 |

**像素量 4 倍**（960×540 → 1920×1080）。

### 28.3 踩坑：`maxHeight` 用屏幕短边会挡掉 1080p

K40 `displayMetrics` 短边只有 **1036**（扣状态栏/导航栏），1080p 档要 1080 —— 差 **44 px**
就被拒，第一次真机选了 720p。修法：`短边 × 1.2`（理由是超采样降采样更清晰；
4K 2160 仍远超 1243 被挡）。

### 28.4 边界

**不是 ABR**（只选一次，不换）；只对 HLS 生效（MP4/MKV/SMB 不走这段）；
单档流不变；带宽预算 12 Mbps 固定、无起播前探测（慢源仍可能选中高档然后卡，那是 ABR 要解决的）；
未测 Emby 多档 master 形态、鉴权 HLS、`#EXT-X-KEY` 加密流的裁剪安全性。

证据：`docs/evidence/p310-hls-variant-pick/status.md`。
## 二十九、314 275 §6.1 指标收口（最后两项补上）

> 275 §6.1 的 11 个指标，之前有两项标「⚠️ 部分」。实测后发现：**一项是属性名写错、一项是字段文案没跟上**。

### 29.1 `vo-drop-frame-count` 这个属性**不存在**

查 mpv `player/command.c`：
```
{"decoder-frame-drop-count", mp_property_frame_drop_dec}   // 解码丢帧
{"frame-drop-count",         mp_property_frame_drop_vo }    // VO 丢帧
```
我们写的是 `vo-drop-frame-count`（不存在）⇒ `frames_dropped_vo` **恒为 not_measured**；
而 `frame-drop-count` 被当成了「total」。
⇒ 改为 `frames_dropped_vo = frame-drop-count` + 新增 `frames_dropped_decoder = decoder-frame-drop-count`。
**实测从 not_measured 变成 5 / 0。**

### 29.2 `seek_total_ms` 早就在采，只是字段写死「尚未埋点」

`recordSeek()` 280 就接了（p50 实测 401 ms），但 `metrics.seek_total_ms` 字段没跟上传导链
⇒ **报未测但其实有数据**，比缺数据更容易误导。
⇒ 改为取 seeks 的 p50。**实测 not_measured → 126 ms。**

### 29.3 11 项最终状态

9 项直接可采｜`read_latency_ms`/`render_gap_ms` 由别的组件采（不在 JSON）｜
**`first_decode_ms`/`first_present_ms` 明确不可采**（mpv 未暴露解码完成事件、首帧回调本身已是"已上屏"）
—— 这是内核限制，不是我们的债。

### 29.4 ★ 275 §6.2 验收：连续 20 次 seek —— **PASS**

`kivo.seekStress 20`（1.2 s 间隔、目标按质数步长循环、专门制造覆盖竞争）：

```
count 20 / failed 0 / p50 105ms / p95 156 / max 166 / error p50·max 0·0
```

**20 次全成功、零失败、落点零偏差** ⇒ 「无旧请求覆盖新目标」通过。

（曾误判"落点钩子只跑第一个"——实际是 `at = delayMs + index*delayMs`，
4 个落点要 32 秒，我只等了 16 秒。**把"没等到"当成"没执行"**，又一次"现象可见≠原因正确"。）

### 29.5 仍未解决

探针日志 `smbTrace` 没进 `net-diag.log`（grep 命中 0），本轮靠 metrics `count` 作铁证；
`open_ms` 用 `source_ready` 近似。

证据：`docs/evidence/p314-metrics-gap/status.md`。
## 三十、315 DASH 能力评估（不支持，原因已定位）

> 275 §1.4 里「DASH 完全没碰」——本卡把它定性了。

**实测**：`.mpd` 被内核当成 **matroska/EBML** 去解（`EBML header parsing failed`）⇒ **不支持**。

**原因**：APK 内 `libavformat.so` 只有 **2.5 MB**（完整版 10–20 MB），且**没有 libxml2**
⇒ FFmpeg 的 `dashdec` 未编入（它解析 MPD 依赖 libxml2）。
**不是代码问题，是构建配置**：补 `--enable-libxml2` 重编即可。

**建议暂不做**：我们已有可用的自适应路径（HLS 309+310）；实际片源以 Emby 转码流(HLS)、
SMB 原盘(MKV/MP4) 为主；重编 FFmpeg 动的是**同一个 .so 里的所有 demuxer**（含 HLS），
收益（多一个容器）与风险（HLS 回归）不成比例。**有明确 DASH 片源需求时再立卡，且必须带 HLS 回归。**

证据：`docs/evidence/p315-dash-assessment/status.md`。

## 三十一、317 Emby 4K 黑屏有声（vo 初始化失败 → 静默失败已消除）

**现象**：播 Emby 第二个源的 4K 原盘（HEVC 3840×2160 / 27.2GB / 直连）⇒ **有声音、无画面**。

**根因链**（四步都有日志）：源站 **330 KB/s**（12.7 秒给 4MB）→ 打开耗时 **61 秒**
（99.3s→160.2s，端口 40739→38171 说明中途重开过）→ 建 vo 时
`Failed to create ANativeWindow`（Surface 的底层 window 在 61 秒里已失效，
因为 `wid` 只是 jobject 引用，mpv 真正取 ANativeWindow 是在建 vo 那一刻）
→ `Video: no video` + `audio=playing`。

**★ 比黑屏更严重的是静默**：`MpvFirstFrameWait` 把 `vid=no` 一律当"纯音频"放行，
UI 显示"正在播放"，用户无限干等。

**修复（A+C）**：用 **`video-codec` 非空却 `vid=no`** 区分"真纯音频"与"vo 失败"，
后者**抛异常**让 UI 报错（文案带可采取动作）；等待循环每 5 秒留一条慢源日志。

**验证**：正常视频/纯音频均不误报；**真实 vo 失败场景未实测**（需 4K+慢源，靠下次真机）。

**未做**：vo 自动降级（`gpu-next→gpu`）——会让 HDR 与帧插值全失效，需单独评估。

证据：`docs/evidence/p317-vo-surface-lost/status.md`。

## 三十二、317 两条真 bug（OSD 双进度条 + 横滑 seek 缺失）

### 32.1 拖动后多出一条更粗的进度条

**现象**：拖动进度条松手后，画面上方多出一条更粗、跟着播放走的进度条。

**排查过程（走了三次弯路，值得记）**：
① 先猜 `AnimatedVisibility` 双实例 → 加实例 ID 打点，**日志证明只有 1 个实例**（908 条同一 ID）；
② 再猜缩略图浮窗被裁剪 → **临时禁用浮窗，白条仍在**；
③ 最后查 mpv 源码：**`osd-on-seek` 默认值是 `bar`**（`options/options.c` 的
`OPT_CHOICE(osd_on_seek, {"no",0},{"bar",1},...)`）⇒ **seek 后内核自己在视频画面上画一条**，
在渲染层、不是 Compose ⇒ 查遍 UI 代码当然找不到。

**修法**：`mpv_options.cpp` 加 `options.push_back({"osd-on-seek", "no", false})`。
**验证**：用户确认白条消失。

### 32.2 ★ 真正的教训：Gradle C++ 增量编译在假成功

这一轮最大的时间浪费不是猜错方向，而是**反复拿旧包做实验**：

- `assembleDebug` 连续多次 `BUILD SUCCESSFUL in 2–7s`，`.so` 时间戳始终不变；
- `--rerun-tasks` 也假成功；
- **只有删掉 `app/.cxx` + `merged_native_libs` + `stripped_native_libs` 后全量重编（49 秒）才真正编进去**。

⇒ **凡改 C++，必须验证 `.so` 真的更新**（体积/时间戳二者之一变化）；只看 `BUILD SUCCESSFUL` 不够。

### 32.3 全屏横滑 seek 缺失

旧注释明说"横向 seek 由上层进度条处理"，**画面上横滑没有任何处理** ⇒ 用户横滑被竖向检测器抢走，
表现为"左右滑调亮度/音量"。已补 `detectHorizontalDragGestures` + HUD（详见 318）。

证据：`docs/evidence/p317-vo-surface-lost/`（OSD）、`docs/evidence/p318-gesture-audit/`（手势）。

---

## 三十三、317 下午公网两臂终判（2026-11-16 16:44–18:01，用户裁决「回到主线，现在正是下午」）

> 承接 §九 / §12.7 挂了多天的「卡不卡终判」。这次在**下午窗口**跑（夜间源站快、判不了），
> 硬解口径 gpu-next、片源 485 1080p（同 MediaSourceId）、每臂 300s 纯净采样。
> 三臂全部 `FALLBACK=0`、前台播放器校验通过、采样全程零截图不并发 adb。

### 33.1 前置与源站基线（方法论 2）
- 时段：A1 起 16:44 / B 起 17:50 / A2 复跑 17:56，均在下午；源站会老化，数字必带基线。
- 源站基线：本机 curl `-r 0-8388608` = **3.15 / 3.69 MB/s**；K40 设备侧 run-as curl = **2.84 MB/s**
  （206，8,388,609B / 2.96s）。需求 501 KB/s（485 4.1Mbps）⇒ 带宽有 5–7 倍余量。
- **禁续播修正**：`p274_soak.sh` 本轮补 `&kivo.run=<label>-<ts>` 唯一 query（watch.json 按完整
  playUrl 存续播位，见 `PlayerSessionBridge.watchKey()`）。Emby 忽略未知参数（curl 实测 206）
  ⇒ 每臂从 0 起播，A/B 起点相同（落实 11-15 方法论增补 2，堵住续播位隐形变量）。

### 33.2 三臂数据（权威判据）
| 判据 | A1 `ko=0` 首跑 | B `ko=1` | A2 `ko=0` 复跑 |
|---|---|---|---|
| 上屏帧率 | 18.30 fps ❌ | 58.86 fps ⚠️ | **60.16 fps** ✅ |
| 漏采窗口 | 0 段 ✅ | 9 段/7.2s ❌ | 1 段/32ms（可忽略） |
| `>200ms` / max | 2 次 / 237.8ms | 1 次 / 260.0ms | 1 次 / 750.5ms |
| **Enter buffering** | **0** | **0** | **0** |
| **Video frame dropped** | **0** | **0** | **0** |
| acquireLatestImage 失败 | **2485** | 0 | **0** |
| Render queue underrun | 1 | 25 | 28 |
| 公网连接退化/换连接 | 4 | 7 | 6 |
| origin chunk / FALLBACK | 59 / 0 | 63 / 0 | 62 / 0 |
| 首帧耗时 | 7.545s | 3.588s | 5.177s |

### 33.3 ★ A1 首跑 18.30fps = 随机 GPU 故障，不是 keepalive 策略问题
- A1 在 mpv 时钟 18.989s 出现 `Waiting for frame timed out` → `acquireLatestImage failed: -30001`
  → `Mapping hardware decoded surface failed` → **`Could not render image for frame mixing.. disabling!`**，
  之后 aimagereader 硬解 surface 映射持续失败直到 303.9s（2485 次），libplacebo 帧插值被永久禁用
  ⇒ 上屏退化到源 23.976fps 附近（实测 18.30）。`frame mixing disabling` 在**全部历史证据中仅此一次**。
- **同参数复跑 A2（ko=0）完全不复现**：60.16fps、acquireLatestImage 失败 0、Enter buffering 0、dropped 0。
  ⇒ 判定为**一次性 GPU/libplacebo 初始化竞态**（随机），**与 ko=0/1 无因果**（keepalive 管 HTTP 连接复用，
  acquireLatestImage 是 MediaCodec surface/GPU 层，无链路）。默认 ko=0 配置无需改。
- 因果判据（方法论 6，看供给不看帧率）：A1 虽 4 次公网退化，但 `FALLBACK=0`、chunk 持续在供、
  rate 爬回 1.9–2.0M/s、缓存 dur 始终 ≥10s、underrun=false ⇒ 供给健康，18.30fps 纯属渲染侧假象。

### 33.4 结论（诚实边界）
- **两臂都无卡顿**：A2 / B 三臂 `Enter buffering=0 / dropped=0`，300s 全程连续渲染。
  keepalive=0（默认）与 =1（回退臂）在**不卡顿**这一终判上等效；首帧 ko=1 略快（3.6s vs 5.2–7.5s），
  与历史同向，但都 <8s，不构成改成 keepalive=1 的理由（274 已证 =1 长连接吞吐衰减，应废弃）。
- **帧率口径**：A2 60.16fps（漏采可忽略）可信；B 58.86fps 因 9 段漏采（块间隔 max 4.02s > 2.1s
  环形缓冲，疑似采样期 dumpsys 抖动）**不作卡顿结论**，但内核全程事件可信且与 A2 一致。
- **SenPlayer 同源同时段对照 = `not_measured + reason`**（沿用 §12.1/§12.7 裁决，非本次遗漏）：
  真机复核 `dumpsys package com.wuziqi.SenPlayer`（v12/targetSdk36）VIEW intent-filter 仍只有
  `content`/`file`，**无 http/https** ⇒ 吃不了公网 Emby 直链；数据目录非 debuggable 不可读，
  手工配 Emby 源既可能误触更新（11-15 事故）又走其自有客户端策略、非严格同 URL。
  ⇒ 不硬凑，按 275 §6.1 写未测。
- **仍不能宣布整体对齐/超越 SenPlayer**：严格同 URL 对照不可得、第二 SoC 未验证、真 HDR PQ surface /
  DoVi FEL 未闭合——§12.7 边界一字不改。本轮只闭合了「下午公网卡顿终判」这一项。

证据：`docs/evidence/p274-startup/fix-nokeepalive-hw-afternoon{,-r2}/`、
`docs/evidence/p274-startup/fix-keepalive-hw-afternoon/`（各含 latency.txt / kernel.log / net.log / diag.log / playing.png）。
