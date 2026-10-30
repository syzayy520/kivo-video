# P12-E1 验收：`COLOR_FormatYUVP010` —— 两条路径都拿不到位深，但都说得出为什么

- 版本：V1.0
- 日期：2026-10-29
- 设计：`35-p12e1-design.md`
- 判据：**查询 `COLOR_FormatYUVP010` 协商结果；协商不到即报 reason，绝不静默受 NV12**
- 结论：**达成，但结论是否定的** —— 在这台设备上，播放路径与探针路径**都拿不到可判定的位深**。
  按卡片口径，"报得出 reason"就是交付物；本次给出两个各自独立的、可复算的理由。

---

## 一、两个否定结论

### 1.1 播放路径（Surface）：厂商私有格式，不表达位深

| 片源 | 编码/Profile | 声明的输出格式 | 位深 |
| --- | --- | --- | --- |
| `h264_1080p8.mp4` | AVC / High(8) | `qcom_private` = **2141391878**（0x7FA30C06） | 不可判定 |
| `hevc_1080p10.mp4` | HEVC / Main10(2) | `qcom_private` = **2141391878**（0x7FA30C06） | 不可判定 |

**8 bit 与 10 bit 片源给出完全相同的值。** 这个值落在高通的私有格式区
（`0x7FA30C00`–`0x7FA30CFF`），不是 `COLOR_FormatSurface`（0x7F000789），
也不携带任何位深信息。

所以证据里写：

```
"verdict": "indeterminate",
"reason": "source is 10-bit (profile 2) but the negotiated output is
           qcom_private(2141391878) [vendor-private: does not express bit depth]
           — bit depth is not expressed by this value, so 10-bit delivery is
           unverified on this path"
```

### 1.2 探针路径（ByteBuffer）：组件根本不采纳 `color-format`

新写的 `kivo_color_probe` 对同一片源做三次 configure，**每次都用新解码器实例**，
并在 `start()` 之后（端口协商完成）再读一次输出格式：

| 请求 | configure | start | start 前格式 | start 后格式 |
| --- | --- | --- | --- | --- |
| 不设 `color-format` | ok | ok | 21 (NV12) | **21 (NV12)** |
| `color-format = P010` | ok | ok | 21 (NV12) | **21 (NV12)** |
| `color-format = Surface`（**不给窗口**） | ok | ok | 21 (NV12) | **21 (NV12)** |

第三行是关键：**Surface 输出不可能在没窗口时成立**，但它照样 configure 成功、start 成功。
这说明这颗 OMX 组件**在 configure 阶段完全不校验 `color-format`**，
于是"请求 P010 后 configure 成功"**什么都不能证明**。

两个片源、三次尝试，六组结果全部是 `21 (NV12)`。探针据此写出：

```
"verdict": {
  "same_format_for_all_requests": true,
  "format_after_start": 21,
  "reading": "every request produced the same output format after start; this component
              does not honour color-format requests, so neither P010 support nor its
              absence can be concluded from this probe"
}
```

> **这是设计稿被实测推翻的第二处。** `35-p12e1-design.md` §3.2 原本假设
> "configure 成功 ⇒ 接受 P010"，并且把"Surface 无窗口应失败"当成反证。
> 两者都不成立 —— 详见 §四。

---

## 二、顺带量到的一件事：解码器看不见 HDR 标记

三份证据里 `color-standard / color-transfer / color-range` 全是 **1 / 3 / 2**
（BT709 / SDR / limited），**8bit 与 10bit 片源一模一样**。

用 ffprobe 核对片源本身：

```
hevc_1080p10.mp4: profile=Main 10, pix_fmt=yuv420p10le,
                  color_space=unknown, color_transfer=unknown, color_primaries=unknown
```

也就是说：这个 10bit 片源是**没有色彩标记的 10bit SDR**，容器里根本没有 BT2020/PQ 信息，
解码器报 1/3/2 只是它的默认值。

**两件事因此清楚了，都对 E2/E3 有用：**

1. 想验证 HDR，必须先有一个**带 BT2020/PQ 标记的真 HDR10 片源**；
   拿这个 10bit SDR 片源去测 HDR 只会测到"默认值"。
2. 解码器不会凭空知道片源是 HDR —— 标记要么来自容器（`color-standard` 等键），
   要么由我们通过 `AMediaFormat` 下发给它（E2 要做的正是这件事）。

---

## 三、代码与证据

| 改动 | 位置 | 说明 |
| --- | --- | --- |
| 格式名/位深映射（**新**） | `decode/color_format_names.{hpp,cxx}` | NDK 不导出这些常量，数值手写并单测钉住 |
| 颜色事实与判定（**新**） | `decode/video_color_facts.{hpp,cxx}` | 纯函数：`assess_color()` 给出 verdict + reason |
| 读实际输出格式 | `decode/mediacodec_video_decoder.cxx` | `configure` 抄源 profile；`start` 后读 `getOutputFormat` |
| 证据段 `color` | `session/decode_stats_renderer.cxx` | schema 升 `kivo.p12c.decode/7`；未知一律写 `null` |
| 探针（**新**） | `probe/color_probe_main.cxx` | 设备端可执行 `kivo_color_probe` |

单测 `kivo_p12e1_color_tests`：**10 例 / 52 断言**，覆盖常量数值、映射、
以及"10bit 源 + surface/NV12/P010"三种判定分支与两种未知情况。

证据（`docs/evidence/p12e/`）：

| 文件 | 内容 |
| --- | --- |
| `p12e1-8bit-r1.json` | AVC High，`decode/7`，`verdict=indeterminate`，reason 说明不影响判定 |
| `p12e1-10bit-r1.json`、`-r2.json` | HEVC Main10，两轮结果一致 |
| `p12e1-probe-8bit.json`、`p12e1-probe-10bit.json` | 三次 configure 的完整结果 + verdict |

---

## 四、设计里被实测推翻的两处（已回写设计稿）

| 位置 | 原假设 | 实际 |
| --- | --- | --- |
| §3.1 | Surface 输出时 `color-format` = `COLOR_FormatSurface`(0x7F000789) | 实际是厂商私有 `0x7FA30C06`，连"这是 Surface"都看不出来 |
| §3.2 | "请求 P010 后 configure 成功 ⇒ 接受 P010"；Surface 无窗口应失败（可作反证） | 两者都错：组件不校验 `color-format`，Surface 无窗口也成功 |

第三处（不算推翻，是补充）：**必须在 `start()` 之后读**。第一版只读 configure 后的值，
读到的 21 是默认值，不是协商结果。

---

## 五、诚实清单

1. **这张卡没有证明"支持 10bit"，也没有证明"不支持"。** 它证明的是：
   在这台设备的这两条路径上，**位深不可观测**，并且给出了各自的具体理由。
2. **只有一台设备（K40 / Android 13 / 高通）**。"组件不采纳 `color-format`"是高通 OMX 的行为，
   换厂商/换平台可能完全不同 —— 所以探针的结论写成了 `same_format_for_all_requests` 这种
   **就事论事的形式**，而不是"该组件不支持 P010"这种一般性断言。
3. **片源是 10bit SDR，不是 HDR。** 全部 `color-standard/transfer` 结论只对"无标记片源"成立；
   HDR 判别必须等 E2/E3 用真 HDR10 片源再测。
4. **探针的 ByteBuffer 路径与播放路径不是同一件事。** 它只能说明 codec 在 ByteBuffer 模式下
   报什么；播放走 Surface，两条路的格式协商互不代表。产物里 `scope` 字段写死了这句话。
5. **`getOutputFormat` 的读取时机**只有一个（`start()` 之后、第一帧之前）。有设备要等
   首个输出缓冲之后才定稿；本机实测 start 后即稳定，但没在别的设备上验证。
6. **没有取 `getOutputImage()` 的 `Image.format` 做二次确认。** 那是 ByteBuffer 模式下的
   另一条独立证据，能进一步确认 P010 与否 —— 但播放路径不用它，本卡没做。
7. **`color-standard/transfer/range` 的数值没有映射成可读名**（1/3/2），只有原始值。
   映射表属于 E3，且需要真 HDR 片源才能验证对错，现在写容易写成"看起来对"的东西。

---

## 六、对 E2 / E3 的交接

- **必须准备一个带 BT2020 + PQ 的真 HDR10 片源**（ffprobe 能看到 `color_transfer=smpte2084`、
  `color_primaries=bt2020`）。现在的 `hevc_1080p10.mp4` 是 10bit SDR、无标记，测不了 HDR。
- **E2 的下发点在 `configure()` 之前**：把 `color-standard` / `color-transfer` /
  `color-range`（以及 HDR 静态元数据）写进喂给 `AMediaCodec_configure` 的格式里。
  现在的代码原样传轨道格式，一个颜色键都没加。
- **E3 的观测点在 `start()` 之后**：`color` 段已经能读这三个键了，E3 只需要在
  "有真 HDR 片源"的前提下看它们有没有变成 BT2020/PQ。
- **判据别写成"支持 HDR"**：本卡已经演示了"看起来有值 ≠ 值有意义"（1/3/2 是默认值）。
  E3 的判据应该是"片源标 BT2020/PQ 时，输出格式里的对应键也跟着变"。

---

## 七、产物

- 设计：`35-p12e1-design.md`（§3.1/§3.2 已按实测加更正）
- 新增：`decode/color_format_names.{hpp,cxx}`、`decode/video_color_facts.{hpp,cxx}`、
  `probe/color_probe_main.cxx`（可执行 `kivo_color_probe`）
- 改：`mediacodec_video_decoder.cxx`、`decode_stats.hpp`、`decode_session.cxx`、
  `decode_stats_renderer.cxx`（schema `/7`）、Android CMake
- 单测：`kivo_p12e1_color_tests` 10 例 / 52 断言
- 证据：`docs/evidence/p12e/` 5 份
