# P12-E1 设计：`COLOR_FormatYUVP010` 协商结果

- 版本：V1.0
- 日期：2026-10-29
- 卡片判据：**查询 `COLOR_FormatYUVP010` 协商结果；协商不到即报 reason，绝不静默受 NV12**
- 关联：`07-p12-plan.md`（G9）、P12-A 的设备能力证据

---

## 一、这张卡真正要回答什么

卡片写的是"查询协商结果"，但**"协商"这个词藏了一个前提**：得先有"请求"。

而当前代码根本没有请求过任何颜色格式 —— `MediaCodecVideoDecoder::configure()` 把
源格式原样交给 `AMediaCodec_configure(codec, format, window, ...)`，`color-format`
这个键连写都没写。Surface 模式下它由平台强制为 `COLOR_FormatSurface`。

所以真正要回答的是三件事，缺一件这张卡就是空的：

1. **这个片源的位深是多少**（10bit 片源和 8bit 片源必须被区分开）；
2. **我们实际拿到的是什么格式**（不是"我们想要什么"，是"平台给了什么"）；
3. **当两者不匹配时，谁负责说清楚**（不许静默降级成 NV12 还当作正常）。

---

## 二、三个必须先承认的硬事实

写在最前面，因为它们决定了这张卡能做到什么程度：

1. **Surface 输出模式下，`color-format` 恒为 `COLOR_FormatSurface`（0x7F000789）。**
   它只说明"输出到 Surface"，**不携带位深**。想知道位深，要么走 ByteBuffer/ImageReader
   路径，要么就得承认"这条路看不到"。

   > **实测更正（2026-10-29）**：K40 上不是 0x7F000789，而是厂商私有的
   > **0x7FA30C06**（高通私有区）。连"这是 Surface"都看不出来。
   > 结论方向不变（仍然不携带位深），但数值写错了会让读的人以为是常量映射漏了。
   > 见 `36-p12e1-status.md` §1.1。
2. **`COLOR_FormatYUVP010`（0x36）只在非 Surface 输出时才是可请求值。**
   配置了窗口还去设它，`configure` 会失败 —— 那不是"不支持 10bit"，
   是"Surface 模式下这个键没有意义"。
3. **NDK 不导出这些常量。** `COLOR_FormatYUVP010` 只存在于 Java 的 `MediaCodecInfo`。
   所以要在 C++ 里用，必须自己写常量并注明来源（和 `csd-0` 那条一样，见
   `mediacodec_format_guard.cxx` 的注释）。

> 也就是说：**"在播放器里查 10bit 协商结果"这件事，在 Surface 架构下天生看不到位深。**
> 卡片判据里"协商不到即报 reason"正好覆盖这种情况 —— 报 reason 就是交付物，
> 不是失败。

---

## 三、做法：两条独立的证据线

### 3.1 播放器侧：记录"实际拿到的"

在 `MediaCodecVideoDecoder::start()` 之后调 `AMediaCodec_getOutputFormat()`，
读出平台真正给的格式，连同源侧信息一起进证据（schema 升 `/7`）：

| 字段 | 来源 | 说明 |
| --- | --- | --- |
| `color.source_profile` | 轨道格式 `profile` | HEVC Main10 = 2，Main = 1 |
| `color.source_is_10bit` | 由 profile 推出 | 只对 HEVC/AVC 有意义，其余写 `null` |
| `color.negotiated_format` | 输出格式 `color-format` | 原样记录，不做映射 |
| `color.negotiated_name` | 纯函数映射 | `surface` / `yuv420p` / `nv12` / `yuvp010` / `unknown(0x…)` |
| `color.negotiated_bit_depth` | 纯函数映射 | 10 / 8 / 0（未知） |
| `color.color_standard` / `color_transfer` / `color_range` | 输出格式 | 有就记，没有写 −1 |
| `color.verdict` | 纯函数 | `ten_bit` / `eight_bit` / `indeterminate` |
| `color.reason` | 纯函数 | `verdict == indeterminate` 时必填 |

**判定规则**（写成纯函数，桌面可测）：

```
若源不是 10bit            → verdict = ten_bit/eight_bit 按输出判定，reason 空
若源是 10bit 且输出 10bit  → ten_bit
若源是 10bit 且输出 8bit   → eight_bit，reason = "source is 10-bit but decoder negotiated 8-bit"
若源是 10bit 且输出 surface → indeterminate，
                             reason = "surface output does not carry bit depth; ..."
```

`indeterminate` **不是失败**，是"这条路径看不到"。它必须带 reason，
否则读的人会把它当成"没问题"。

### 3.2 探针侧：直接问 codec 要 P010

播放器看不到位深，但"这个解码器到底支不支持 P010"是可以直接问的 ——
新建一个设备端可执行文件 `kivo_color_probe`，对同一个片源做三次配置尝试：

| 尝试 | 配置 | 期望看到什么 |
| --- | --- | --- |
| A | 不设 `color-format`、不给窗口（ByteBuffer 模式） | 平台默认给什么（多半 NV12/I420） |
| B | `color-format = COLOR_FormatYUVP010`、不给窗口 | 支持则 configure 成功且输出格式是 P010；不支持则给出 `media_status_t` |
| C | `color-format = COLOR_FormatSurface`、不给窗口 | 预期失败 —— 用来证明"Surface 模式必须有窗口"，不是"不支持 10bit" |

只有 A 和 B 的对比能回答"这个 codec 能不能吐 10bit"。C 是**反证**，
防止把"少给了窗口"误读成"不支持"。

> **实测更正（2026-10-29）：这一整段的假设是错的。**
> C 在 K40 上**没有失败** —— configure 成功、start 也成功、报的还是 NV12。
> 也就是说这颗 OMX 组件**在 configure 阶段根本不校验 `color-format`**，
> 于是"A/B 都 configure 成功"什么都不能证明，C 也构不成反证。
> 另外 `getOutputFormat` 必须在 **`start()` 之后**读：configure 后读到的是默认值 21。
> 探针已按这两点重写（加了 `start_ok` 与 `format_after_start`），
> 结论见 `36-p12e1-status.md` §1.2。

探针不改播放器、不进证据链，只产出一份探测 JSON。

---

## 四、归属（家族树）

| 新增/改动 | 位置 | 理由 |
| --- | --- | --- |
| `color_format_names.hpp/.cxx`（**新**） | `backends/mediacodec/decode/` | 纯映射：整数 → 名字/位深。零平台依赖，桌面可测。放在 decode/ 是因为它描述的是解码器输出 |
| `video_color_facts.hpp`（**新**，纯数据 + 判定） | `backends/mediacodec/decode/` | 源与输出的颜色事实 + 判定纯函数。**不碰 AMediaFormat**，只吃 int |
| `color_probe_main.cxx`（**新**） | `probe/color/` | 设备端可执行探针，与 `probe/audio/` 同族 |
| `mediacodec_video_decoder.cxx` | 读 `getOutputFormat()` 并填充 facts | 只有它持有 `AMediaCodec*` |
| `decode_stats.hpp` + 渲染器 | 新增 `color` 段 | 证据出口 |

**不新开顶层族**：`probe/color/` 是 `probe/` 下的第二个子域（第一个是 `probe/audio/`），
符合"子域文件数 ≥ 3 才开子目录"吗？不满足 —— 只有 1 个文件。
所以 `color_probe_main.cxx` 直接放 `probe/` 下，不建 `probe/color/`。
（`probe/audio/` 有 6 个文件，是历史；不为了一致性再造一个空壳。）

---

## 五、判据与它的边界

### 5.1 算过

| 项 | 判据 |
| --- | --- |
| 查询到协商结果 | 证据里有 `color.negotiated_format` 原始值 + 可读名，8bit/10bit 两个片源都跑 |
| 协商不到时报 reason | 10bit 源在 Surface 路径上 `verdict = indeterminate` 且 `reason` 非空；探针给出 codec 侧能力 |
| 绝不静默 | 任何 `verdict != ten_bit` 的 10bit 源，日志与证据都必须出现原因；不存在"看起来成功"的空字段 |
| 可复算 | 判定是纯函数，桌面单测覆盖；证据里的 verdict 可由 `source_is_10bit` + `negotiated_format` 重算 |

### 5.2 明确不做

- **不做 HDR 元数据下发**（E2）、不做 BT2020 标记（E3）、不做 tone mapping（E4）。
- **不改 Surface 输出架构**（改成 ImageReader/GL 才能拿到真实位深）。那是 E3/E4 的前提，
  单独立卡；E1 只负责把"看不到"这件事说清楚。
- **不宣称支持 10bit 播放**。哪怕探针显示 codec 能吐 P010，只要播放路径是 Surface，
  "画面是 10bit 的"就没有证据。

---

## 六、风险

| 风险 | 处置 |
| --- | --- |
| 把 `indeterminate` 当成"没问题" | 证据里 verdict 与 reason 必须成对出现；reason 为空时渲染器直接报错，不写文件 |
| 探针的 A/B 对比被误读成"播放器能输出 P010" | 探针产物里写死一句 `"this_probe_does_not_prove_the_player_path"`，并在状态文档里重复一遍 |
| `getOutputFormat` 在 `start()` 前调用返回空 | 放在 start 之后读，且失败不致命：记 `-1` + reason，不中断播放 |
| 常量写错（NDK 不导出） | 常量集中在 `color_format_names.cxx` 一处，注释标出来源，单测钉住数值 |
