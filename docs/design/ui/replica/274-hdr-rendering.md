# 274 · HDR 渲染与成像方案

状态：**设计稿**（实现卡待开，排在设计体系 S1–S6 之后）
上游：[274-design-system.md](274-design-system.md)
相关：[273-emby-stutter-fix.md](../playback/273-emby-stutter-fix.md) §十九（G-HDR 缺口）

---

## 〇、本文档只负责一件事

定义 **HDR 的能力探测、色彩空间链路、性能降级策略**。
它**不**定义最终参数取值（`target-peak` 多少、tonemap 用哪条曲线）——
那些必须真机 A/B 实测确定，见 §六。

---

## 一、现状（全部实测，非推断）

### 1.1 设备侧能力（K40 `b6e950ee`，2026-11-14 实测）

```
mSupportedHdrTypes      = [1, 2, 3, 4]     // Dolby Vision / HDR10 / HDR10+ / HLG
mMaxLuminance           = 420.0
mMaxAverageLuminance    = 210.16
mMinLuminance           = 0.323
supportedColorModes     = [0, 7, 9]        // 含 wide gamut 与 HDR
isHbmEnabled            = true
GLES                    = OpenGL ES 3.2 (Adreno 650)
```

**EGL 扩展**（`dumpsys SurfaceFlinger` 实测，含）：
```
EGL_EXT_gl_colorspace_bt2020_pq          ← HDR10 PQ 必需
EGL_EXT_gl_colorspace_bt2020_linear
EGL_EXT_gl_colorspace_display_p3
EGL_EXT_gl_colorspace_scrgb
EGL_EXT_pixel_format_float
EGL_KHR_gl_colorspace
EGL_EXT_gl_colorspace_display_p3_passthrough
```

⇒ **设备侧完全就绪，缺的只是我们没把链路打开。**

### 1.2 应用侧现状（代码实测）

| 项 | 现状 | 证据 |
| --- | --- | --- |
| mpv HDR 选项 | `mpv_options.cpp` 里 `target-trc`/`target-prim`/`target-peak`/`target-colorspace`/`tone-mapping` **一个都没设** | grep 0 命中 |
| EGL 配置 | 写死 `EGL_RED/GREEN/BLUE/ALPHA_SIZE = 8` + `EGL_RENDERABLE_TYPE = EGL_OPENGL_ES2_BIT` | `mpv_egl_surface.cpp:15-23` |
| EGL 色彩空间 | **完全没设** `EGL_GL_COLORSPACE` | 同上 |
| GL context 版本 | `EGL_CONTEXT_CLIENT_VERSION = 2` | `mpv_egl_surface.cpp:26` |
| Surface 图层 dataspace | `UNKNOWN (0)`，`hdr metadata types=0` | 273 §十九 G-HDR 实测 |
| 深色模式 | 不存在（`themes.xml` 无 `values-night/`） | 实测 |
| 色调映射选项 | mpv 侧无 | grep 0 命中 |

### 1.3 libmpv 能力（`libmpv.so` 二进制扫描实测）

```
libplacebo         520 处引用   ← 内置 libplacebo（HDR 渲染栈在）
target-trc           1 处       ← 选项存在
target-prim          1 处       ← 选项存在
target-peak          1 处       ← 选项存在
target-colorspace   1 处       ← 选项存在
tone-mapping        10 处       ← 选项存在
vo_gpu_next        12 处       ← 可用
```

⇒ **重大结论：HDR 不需要重新编译 mpv。**
libplacebo 已在库里，四个 target 选项全在，**只需下发选项 + 改 EGL 配置**。
这与"要重编 libplacebo + libmpv"的悲观预期完全不同，是本卡成本最低的部分。

---

## 二、色彩空间链路设计

### 2.1 三层色彩空间必须分别正确

| 层 | 概念 | 现状 | 目标 |
| --- | --- | --- | --- |
| **L1 源色彩空间** | 片源是什么（BT.709 SDR / BT.2020 PQ / HLG / DV） | mpv 自动识别 ✓ | 保持自动识别，**但要能读到** |
| **L2 渲染目标色彩空间** | 内核输出到 GL 的缓冲是什么 | RGBA8888 8-bit | **BT.2020 10-bit（`GL_RGBA10_A2`）** + PQ 编码 |
| **L3 显示层** | 屏幕按什么色彩空间解释 | `dataspace=UNKNOWN` | `BT2020_PQ` dataspace + HDR metadata |

**关键**：Android 的 `Surface` 通过 `setColorSpace()` 告诉系统显示层如何解释像素。
不设它 ⇒ 系统按 SDR 解释 ⇒ HDR 内容会被当成超范围 SDR，画面发灰。

### 2.2 EGL 配置改造（`mpv_egl_surface.cpp`）

**改造点 1：config 属性加 10-bit 与 HDR 能力**

```c
// 现：8-bit + ES2
// 改：申请 10-bit（若设备不支持则回落 8-bit，见 §五降级）
constexpr EGLint kConfigAttribsHdr[] = {
    EGL_RED_SIZE, 10,
    EGL_GREEN_SIZE, 10,
    EGL_BLUE_SIZE, 10,
    EGL_ALPHA_SIZE, 2,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_NONE,
};
```

> **注意**：`EGL_OPENGL_ES2_BIT` 必须**保留**。
> mpv render API 要求 ES 2.0+；ES3 context 需要 `EGL_OPENGL_ES3_BIT`，
> 而 `vo_gpu_next` 在 ES2 context 下同样可用 libplacebo 的 HDR 路径。
> **是否升 ES3 留到实现卡实测决定**（见 §六 A/B 臂）。

**改造点 2：context 版本**

```c
// ES2 → 先不改。升 ES3 的前提是 libplacebo 的 half-float 路径确实需要它，
// 而实测 K40 有 GL_EXT_color_buffer_half_float 与
// GL_EXT_texture_type_2_10_10_10_REV（273 §十九 已记录）—— ES2 下也能走。
```

**改造点 3：surface 色彩空间（这是最关键的一步）**

```c
// 在 eglCreateWindowSurface 之后设置：
// EGL_GL_COLORSPACE = EGL_GL_COLORSPACE_BT2020_PQ
// 需要先 eglBindAPI(EGL_OPENGL_ES_API)
```

**改造点 4：Android Surface 侧**

```java
// PlayerSurfaceView / SurfaceHolder
holder.setFormat(PixelFormat.RGBA_10BIT)   // 10-bit
surface.setColorSpace(android.graphics.ColorSpace.Named.BT2020_PQ)
```

> ⚠️ **`setColorSpace` 与 `PixelFormat` 必须成对设置**，只设其一无效。
> 详见 §六 A/B 臂验证顺序。

### 2.3 mpv 侧选项（`mpv_options.cpp`）

**按"内容自适应"而非"一刀切"**：

| 选项 | SDR 片源时的值 | HDR 片源时的值 |
| --- | --- | --- |
| `target-colorspace` | `auto` | `auto` |
| `target-trc` | `auto` | `auto`（libplacebo 自动映射到 PQ） |
| `target-prim` | `auto` | `bt.2020` |
| `target-peak` | `auto`（≈面板亮度） | `1000` 或按面板 `mMaxLuminance=420` 折算 |
| `tone-mapping` | `auto`（=clip，SDR 无需映射） | `hable` 或 `bt.2390`（**实测比选**） |
| `gpu-api` | `opengl` | `opengl` |

**关键认知**：mpv 0.37+ 的 `vo_gpu_next`（libplacebo）**能自动识别片源色彩空间并做正确映射**。
我们不需要自己判断"这是不是 HDR 片源"——**内核知道**。
所以选项策略是：**给出正确的显示目标，不去干预内容判断**。

> **`vo` 选项名必须确认**：当前代码设的是 `vo`（具体值见 `mpv_options.cpp:75`）。
> `vo_gpu_next` 与 `vo_gpu`（旧 RA）在 libplacebo HDR 上**行为不同**——
> 旧 `vo_gpu` 没有完整 HDR 路径。**这是本卡第一个要验证的点**（§六 A/B 臂 A）。

---

## 三、能力探测与分层策略

### 3.1 探测链（Kotlin 侧，一次性 + 运行时复核）

```
DisplayManager.getDisplay(Display.DEFAULT_DISPLAY)
  → display.isHdr
  → display.hdrCapabilities.supportedHdrTypes
  → display.hdrCapabilities.maxLuminance
  → SurfaceView.holder.surface
      → surface.isExtendedRangeColorSpace? (API 33+)
```

**降级判定**（任一不满足即降级）：

| 条件 | 不满足时 |
| --- | --- |
| 设备 `isHdr` | 完全不进 HDR 路径 |
| `supportedHdrTypes` 含 `HDR_TYPE_HDR10`(2) | 只支持 DV 时需 libplacebo 解 DV tone-map 兜底 |
| EGL 扩展含 `EGL_EXT_gl_colorspace_bt2020_pq` | 只能走 SDR |
| Surface 接受 `RGBA_10BIT` | 只能走 SDR |

### 3.2 四档能力等级

| 档 | 条件 | 行为 |
| --- | --- | --- |
| **L3 完整 HDR** | 设备支持 + EGL 扩展在 + 10-bit surface 可用 | PQ 链路，色调映射交给 libplacebo |
| **L2 HDR→SDR tone-map** | 设备支持但 surface/EGL 不支持 10-bit | 内核仍做 HDR→SDR 映射（**画面比现在好**：亮部不再死白），输出 8-bit sRGB |
| **L1 基础** | 设备不支持 HDR | 现状不变（`auto` 交给 libplacebo） |
| **L0 强制关闭** | 用户手动关闭 | 显式 `target-trc=bt.1886` 强制 sRGB |

> **L2 是最有价值的一档**：即使 EGL 走不通，让 libplacebo 做 tone-mapping
> 也能显著改善 HDR 片源在 SDR 屏幕上的观感（不会一片死白）。
> **这条路径只需下发 mpv 选项、不改任何 EGL 代码** ⇒ **最先实现，风险最低**。

---

## 四、性能降级策略

### 4.1 HDR 的性能代价（必须实测，不能假设）

| 项 | 代价来源 | 降级手段 |
| --- | --- | --- |
| 10-bit 输出 | 带宽 ×1.25 | 回落 8-bit（L2） |
| libplacebo tone-map | GPU 算力 | 降 `tone-mapping` 复杂度（`hable`→`clip`） |
| 峰值亮度 | 面板功耗↑ | `target-peak` 降到面板实测 `mMaxLuminance=420` |

### 4.2 分级降级链（运行时可切）

```
L3 完整 HDR  (10-bit + PQ + tone-map)
   ↓ 掉帧 / 功耗高 / 用户关闭
L2 HDR→SDR   (8-bit + libplacebo tone-map)   ← 画质损失小，性能几乎无损
   ↓ 内核不支持 tone-map
L1 基础 SDR  (现状：8-bit 硬切，亮部死白)
```

### 4.3 运行时自适应（借鉴 273 既有机制）

273 已经建立了成熟的"按实测调参"流程，本卡沿用同一套做法：

| 触发 | 动作 |
| --- | --- |
| 连续 3 s 帧率 < 20 fps（当前基线 23.96） | 自动从 L3 降到 L2 |
| 温度过高（`dumpsys thermalservice`） | 降级 + 提示 |
| 用户手动开关 | 锁定用户选择，不自动改 |

**降级必须用户可见**（遵守铁律 P5 不静默降级）：播放页状态区显示"已降级为 SDR 色调映射"。

### 4.4 与 273 性能线的冲突点（必须先解决）

**已知冲突**：`273 §十九 G-显存` 记录 `EGL mtrack 88.5 MB` + `Graphics 101 MB` ≈ **194 MB** 显存占用，
是 273-B 待收的缺口。

**HDR 会推高显存**：10-bit 帧缓冲比 8-bit 多 25%（4K 3840×1920 时
`RGBA10_A2` = 15.4 MB/帧 vs `RGBA8888` = 29.5 MB/帧——但注意硬件可能仍按 64-bit 分配）。

⇒ **顺序约束**：
1. **273-B 显存收口必须先于 HDR 实现**，否则 HDR 是在未收敛的显存基线上叠加；
2. HDR 实现后**显存基线要重新量**，§一 的"194 MB"作废。

---

## 五、与 UI 层的契约

> **2026-11-14 用户裁决（两次，合并）**：
> ① **不允许写 UI，UI 已经设计好了**；
> ② **设置开关里面的功能可以加，但不得改变现有 UI 风格**。
>
> ⇒ **§5.1（设置页开关）本轮实施**，但**严格复用现有行原语**（`SwitchRow` / `RadioRow` / `SectionCard`），
> **不新增任何视觉元素、不改任何颜色/字号/间距** ⇒ 视觉零变化。
> ⇒ **§5.2（播放器状态区 / 视频信息面板）本轮不实施**——那是播放页视觉，落在"不允许写 UI"范围内。
> ⇒ **§5.3（深色模式交互）作为将来参考保留**。
> ⇒ **降级状态本轮怎么保证"不静默"**：写进 `mpv-kernel.log`（`[hdr] degraded L3→L2 reason=...`）
> 并通过 `mpv` 属性暴露档位 ⇒ **真机可取证**，将来 UI 侧接入即可。

### 5.1 设置页开关（本轮实施 · 视觉零变化）

**放置位置**：`设置 → 外观与交互`（`SettingsAppearanceSub.kt` 的"外观"卡内，
紧跟「外观模式」那一行之后）。

**关键约束：复用现有行原语，一行新样式都不加。**

| 项 | 复用 | 说明 |
| --- | --- | --- |
| 行容器 | `SwitchRow`（`SettingsRows.kt:167`） | 已有 `role = Role.Switch`、行高 `DesignTokens.rowHeight`、开关配色全在 |
| 三档选择 | `RadioRow`（`SettingsRows.kt:199`） | 已有 `role = Role.RadioButton` + 右蓝 ✓ |
| 卡片容器 | `SectionCard` | 不新增卡片结构 |
| 图标 | `RowIcon` + `SettingsVisuals.blue` | 不引入新图标资源 |

**为什么不放播放器设置页**：播放设置子页（262）管的是**片源与内核行为**，
HDR 是**显示能力**，与「外观与交互」同族。

**行的形态**（二选一，实施时按设置页现有惯例定）：

| 方案 | 形态 | 用哪个原语 |
| --- | --- | --- |
| **A（推荐）** | 三个 `RadioRow`：「自动」「始终开启」「始终关闭」 | `RadioRow` |
| B | 一个 `SwitchRow`：「HDR 色调映射」开=强制开，关=自动 | `SwitchRow` |

> **推荐 A 的理由**：三档（自动/开/关）能表达"设备不支持时不要硬开"这件事；
> 二元开关没有"自动"态，用户会以为开了就一定是 HDR。
> 若选 A，则需 3 个 `RadioRow` 而非 1 个 —— **仍然是现有原语，不是新样式**。

**状态与写回**：

| 项 | 规范 |
| --- | --- |
| 存储 | 复用 `SettingsStore` 的既有偏好机制（`home.autoplay` 同族） |
| 写回失败 | **必须带 `onFailure`** —— 272 批次一已在 `SettingsDataSub` 修过这个坑，照做 |
| 设备不支持 | 行**不置灰**（灰行要显示真值，不能显示假值）；在 `subtitle` 里如实写「本机屏幕不支持 HDR」 |
| 副标题 | 写当前**实际生效档位**（L1/L2/L3），不写"已开启"这种模糊话 |

### 5.2 播放器 UI 的三处契约（**本轮不实施**，留接口）

| 位置 | 契约 |
| --- | --- |
| 状态区（`PlayerStatusLayer`） | 降级发生时显示"已降级为 SDR 色调映射"，**不许静默** |
| 视频信息面板 | 显示当前实际档位（L3/L2/L1） |
| 详情页 | HDR 徽章（DV/HDR10/HLG 角标）——**需新增元数据解析**，属独立工作量 |

> **本轮怎么保证"不静默"**：降级事件写进 `mpv-kernel.log`（`[hdr] degraded L3→L2 reason=...`）
> 并通过 `mpv` 属性暴露档位 ⇒ **真机可取证**，UI 侧将来接入即可。

### 5.3 深色模式的交互（延后但要记）

**问题**：HDR 在 OLED 上如果亮度拉满，亮部非常刺眼。
**决策**：深色模式（尚未实现）落地时，**HDR 默认档位从 L3 降为 L2**（SDR 输出 + tone-map），
让用户在深色下看到的是"正确映射的 SDR"，而非"刺眼的全亮度 HDR"。
用户可在设置里显式选"始终开启"。

---

## 六、实现卡要跑的 A/B 臂（参数留到那时定）

**每臂必须有：真机截图 + `dumpsys SurfaceFlinger` 的图层 dataspace/hdr metadata + 帧率 + 显存**。

| 臂 | 变更 | 要回答的问题 |
| --- | --- | --- |
| **A** | `vo` 从 `vo_gpu` 换 `vo_gpu_next` | 旧 RA 路径是否真的没有 HDR？（**第一优先，成本最低**） |
| **B** | A + `target-prim=bt.2020` + `target-peak=420` | 不改 EGL，仅靠选项能否改善亮部？ |
| **C** | B + `tone-mapping` 对比 `hable` vs `bt.2390` vs `auto` | 哪条曲线在 K40 上观感最好？ |
| **D** | C + EGL `GL_RGBA10_A2` + `EGL_GL_COLORSPACE_BT2020_PQ` + `Surface.setColorSpace(BT2020_PQ)` | 10-bit PQ 链路是否成立？Surface dataspace 是否变为 BT2020_PQ？ |
| **E** | D + ES3 context | ES3 是否必需？ |
| **F** | E + 显式 `target-trc=pq` | 自动识别是否已足够？ |

**判据**：
- 色彩正确性：`dumpsys SurfaceFlinger` 显示图层 `dataspace=BT2020_PQ` 且 `hdr metadata types` 非 0；
- 亮度正确性：HDR 内容在截图中**亮部不出现死白块**（用 `tools/p273_png_avg.mjs` 复算近白像素占比）；
- 性能：帧率 ≥ 23.9 fps（当前基线 23.96）、`Enter buffering` = 0；
- 显存：`dumpsys meminfo` 的 `EGL mtrack` + `Graphics` 增量 ≤ 20 MB（相对 L1 基线）。

---

## 七、家族树增量

> **本轮范围：只做 C++ 侧 + 最小 Kotlin 能力探测。**
> `HdrPolicy.kt` 的 UI 消费（设置页开关、状态区提示）**本轮不做**，只留接口。

```
android/app/src/main/cpp/mpv/
├─ mpv_egl_surface.cpp        改：config 属性 / context 版本 / EGL 色彩空间 / surface 色彩空间
├─ mpv_egl_surface.hpp        改：init 支持"要 HDR / 不要 HDR"两种模式
├─ mpv_options.cpp            改：新增 HDR 档位相关的选项下发（按档位分组）
├─ mpv_hdr_profile.hpp        【新增】只做一件事：HDR 档位定义（枚举 + 每档的 mpv 选项表 + EGL 属性表）
└─ mpv_session.cpp            改：按档位选择 profile；运行时降级的落点

android/app/src/main/java/com/kivo/cinema/
├─ mpv/MpvDisplayCapability.kt  【新增】只做一件事：探测设备/EGL/Surface 三层 HDR 能力 → 输出档位
└─ mpv/HdrPolicy.kt             【新增】只做一件事：档位决策（设备能力 × 性能降级 × 用户偏好）

ui/design/settings/
├─ SettingsAppearanceSub.kt    改：新增「HDR 色调映射」三行（自动/始终开启/始终关闭）
└─ settings/SettingsStore.kt   改：新增 HDR 偏好项（与 home.autoplay 同族）
```

**本轮不涉及**（留接口，将来接 UI 时再建）：
`ui/compose/player/PlayerStatusLayer.kt` 的降级提示、播放信息面板的档位显示、详情页的 HDR 徽章。

**设置页的"视觉零变化"承诺**（用户硬约束）：

| 项 | 保证方式 |
| --- | --- |
| 行样式 | **只用现有 `RadioRow` / `SwitchRow`**，不新增行原语 |
| 卡片 | **只用现有 `SectionCard`**，不新增卡片结构 |
| 颜色 | 全部走 `SettingsVisuals` / `DesignTokens`，**不写任何字面色值** |
| 字号 | 全部走 `RowLabels` 内的既有字阶，**不新增字号** |
| 图标 | 用 `RowIcon` + 既有 `ImageVector`，**不引入新资源** |
| 验收 | 设置页改动前后**逐像素截图一致**（除新增行占据的空位） |

**拆分理由**（铁律"变化原因不同则拆"）：

| 文件 | 变化原因 |
| --- | --- |
| `mpv_hdr_profile.hpp` | "**档位 → 选项/属性的映射表**"是一份数据，mpv 侧与 Java 侧共用语义但表达不同（C++ 表 vs Kotlin 枚举），故 C++ 侧独立成头文件 |
| `MpvDisplayCapability.kt` | "**探测**"与"**决策**"是两件事：探测是只读事实采集，决策含用户偏好与降级状态 |
| `HdrPolicy.kt` | "**决策**"单独一个文件，因为它要被设置页、播放器状态区、播放信息面板**三方**消费（本轮只有内核侧消费，但接口先立好） |
| 不并入 `mpv_options.cpp` | 该文件已 13 个选项，HDR 档位是**一组有条件的选项**，与基础组不同源 |

---

## 八、诚实边界

| 项 | 状态 |
| --- | --- |
| 设备 HDR 能力 | ✅ 实测确认（K40 支持 HDR10/DV/HLG，EGL 扩展齐备） |
| libmpv HDR 能力 | ✅ 二进制实测确认（libplacebo 内置，四选项齐备） |
| **具体参数取值** | ❌ **全部待实测**（§六 A/B 臂） |
| **10-bit PQ 链路是否成立** | ❌ 未验证（K40 的 `RGBA_10BIT` surface 支持情况未查） |
| Dolby Vision | ⚠️ mpv 对 DV 的支持依赖 libplacebo 版本，**本卡不保证** |
| HDR 性能代价 | ❌ 未实测 |
| 与 273-B 显存收口的冲突 | ⚠️ 顺序约束已记录，未解决 |
| 深色模式交互 | ⚠️ 策略已定，深色模式本身未实现 |

---

## 九、验收判据（实现卡收口时用）

- [ ] `dumpsys SurfaceFlinger` 显示播放器图层 `dataspace` 为 `BT2020_PQ`（或明确记录为何不是）
- [ ] HDR 片源截图中近白像素占比 < 5%（用 `tools/p273_png_avg.mjs` 复算）
- [ ] 帧率 ≥ 23.9 fps，`Enter buffering` = 0
- [ ] 降级链路可复现：强制触发降级时，状态区**显示**降级原因
- [ ] 关闭 HDR 后行为与当前基线**逐像素一致**（可回退）
- [ ] 所有新增/改动 `.kt` ≤ 300 行，`.cpp/.hpp` ≤ 400 行
