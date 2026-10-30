# P12-E2 设计：HDR 静态元数据下发

- 版本：V1.0
- 日期：2026-10-29
- 卡片判据：**HDR 静态元数据下发（`AMediaFormat` → `setHdrMetadata` 路径）；K40 上 HDR10 片源可识别为 HDR**
- 关联：`36-p12e1-status.md` §六（E1 给 E2 的交接）

---

## 一、先把"下发"这个词拆开

卡片写的是"`AMediaFormat` → `setHdrMetadata` 路径"。在 Android 上这条链路实际有三段，
**每一段的事都不是"调一个 API"**：

| 段 | 载体 | 谁负责 |
| --- | --- | --- |
| ① 静态元数据来自哪 | HDR10 的 mastering display / MaxCLL 存在 **HEVC SEI** 里；容器里对应 `mdcv` / `clli` 盒 | 片源 |
| ② 怎么送到解码器 | `configure` 时喂进去的 `AMediaFormat`（`color-standard` / `color-transfer` / `color-range` / `hdr-static-info`） | 我们 |
| ③ 怎么让**显示链路**把它当 HDR | Surface 的 **dataspace**（`ANativeWindow_setBuffersDataSpace`） | 我们 |

**K40 上的第一手观测（本轮先做的）**：

```
hdr10_1080p.mp4（自造，BT2020/PQ + SEI 静态元数据）
  容器：profile=4096（HEVCProfileMain10HDR10）
        color-standard=6（BT2020_CONSTANT_LUMINANCE）
        color-transfer=6（ST2084 / PQ）
        color-range=2（limited）
  容器盒：有 colr、**没有** mdcv / clli
```

也就是说：**色彩三键在容器里齐全，静态元数据只在码流 SEI 里。**

> **实测更正（2026-10-30，真机 + 字节级核对）**：这段的两句都要修。
>
> 1. **色彩三键不是来自"容器盒"。** 自造片源里根本没有 `colr` 盒：
>    `python -c "d=open(f,'rb').read(); print(d.count(b'colr'), d.count(b'mdcv'), d.count(b'clli'))"`
>    → `0 0 0`。而设备端读到的轨道格式仍然是 `6/6/2`。
>    来源只能是 **HEVC SPS 的 VUI**（ffmpeg 用 `-color_primaries/-color_trc/-colorspace`
>    写进码流），`MediaExtractor` 解析 SPS 后把它暴露成 `color-standard/transfer/range`。
>    结论方向不变（我们确实读到了三键），但"容器里齐全"这句话会把后来人引到
>    解析 MP4 盒子的错误方向上。
> 2. **静态元数据确实只在 SEI 里，而且设备端读不到。**
>    同一个文件 ffprobe 能看到 `Mastering display metadata` +
>    `Content light level metadata` 两条 frame side data；而 `hdr-static-info`
>    在轨道格式里**不存在**（两份探针产物里 `hdr_static_info_bytes` 都是 `null`）。
>    也就是说：这一轮**没有真正的"静态元数据下发"**，只有"色彩三键下发"。
>    这是 E2 核心判据上的一处**未达成项**，已写进 `38-p12e2-status.md` §五 诚实清单，
>    不掩盖。要发 SEI 里的 mastering 信息，得自己解析 HEVC SEI 并构造
>    `hdr-static-info` blob —— 那是另一张卡的量级（设计 §五 已列为"不做"）。

这直接决定了 E2 的做法：

- 三键本来就在轨道格式里，我们 `configure` 时原样传下去 —— **但那是"碰巧"，不是"下发"**。
  要变成下发，就得显式读出来、显式写进去，并在容器缺失时补上默认。
- `hdr-static-info` 这个键在**没有 mdcv/clli 盒**的片源上大概率不存在。
  不存在时**必须写 `null` 并说明"SEI 里有、容器里没有"**，
  不能因为读不到就当"没有 HDR 元数据"。
- 真正决定"屏幕上是不是 HDR"的是 **dataspace**，不是那三个键。
  只写 color 三键、不设 dataspace，合成器仍按 SDR 处理。

---

## 二、做法

### 2.1 源侧：判定"这片源是什么"

在 E1 已有的 `VideoColorFacts` 上补两个字段：

```cpp
std::int32_t hdr_static_info_bytes = -1;  // 容器里 hdr-static-info 的字节数；-1=没有
```

新增纯函数 `classify_hdr(facts)`：

| 条件 | 结论 |
| --- | --- |
| `transfer == 6 (ST2084)` 且 `standard ∈ {5,6}`（BT2020 / BT2020-CL） | `kHdr10` |
| `transfer == 7 (HLG)` 且 `standard ∈ {5,6}` | `kHlg` |
| 其他 | `kSdr`（含"根本没有色彩键"的情况） |

> **实测更正（2026-10-30，实现前用 javap 反查 android.jar）**：上表的
> `standard ∈ {5,6}` 是错的，**只能认 6**。
> `javap -constants android.media.MediaFormat` 的实际取值是：
> `COLOR_STANDARD_BT709=1`、`BT601_PAL=2`、**`BT601_NTSC=4`**、`BT2020=6`；
> `MediaFormat` 里**没有** `BT2020_CONSTANT_LUMINANCE`（那是 `DataSpace` 的
> `STANDARD_*`，不是 `color-standard` 键的取值）。
> 认 7 等于认一个在这条键上不存在的值 —— 与 E1 那次"把 0x7FA30C06 当常量漏映射"
> 是同一类错误。常量表落在 `decode/color_key_names.hpp`，由 `kivo_p12e2_hdr_tests` 钉住。
>
> 顺带确认另一处：`COLOR_TRANSFER_SDR_VIDEO = 3`。E1 在无标记片源上看到的
> `standard/transfer/range = 1/3/2` 就是 BT709 / SDR_VIDEO / LIMITED，
> 不是"SMPTE_240M"——这一点也写进了常量表注释，防止后人"顺手修正"回去。

> 判据为什么不用 profile：`profile == 4096` 确实是 HDR10 的强信号，
> 但它是**编码侧**的声明，而这里要判的是**显示侧**该按什么处理。
> 两者不一致时（例如有人把 SDR 编码成 Main10HDR10）以色彩键为准，
> profile 只在证据里记录、供人对照。

### 2.2 解码器侧：显式下发

`configure()` 里，喂给 `AMediaCodec_configure` 的格式不再直接用轨道格式，
而是**复制一份再写**：

```
copy = 轨道格式的必需项（mime / width / height / csd-0）
if (源有 color-standard)  set color-standard
if (源有 color-transfer)  set color-transfer
if (源有 color-range)     set color-range
```

为什么"复制再写"而不是就直接传：直接传等于"平台有什么就用什么"，
缺键时解码器自己猜默认值（E1 已经演示过它猜成 BT709/SDR）。
显式写一遍之后，"我们下发了什么"才有据可查。

⚠️ 这里**不伪造**：源没有的键不补。补默认值会让一份 SDR 片源被标成别的，
那是造假。缺失就缺着，证据里 `null`。

### 2.3 显示侧：Surface dataspace

这条是 E2 真正的"让显示链路认账"的一步：

```cpp
ANativeWindow_setBuffersDataSpace(window, ADATASPACE_BT2020_PQ);  // 163971072
```

**不能直接调用**：该符号 API 28 才有，而工程 `minSdk = 26`。
用 `dlsym(RTLD_DEFAULT, "ANativeWindow_setBuffersDataSpace")` 在运行时取，
取不到就记 `skipped_reason = "ANativeWindow_setBuffersDataSpace requires API 28"`。

时机：**`configure` 之前**。dataspace 影响之后 dequeue 的 buffer 的标记，
configure 之后再设，首批缓冲已经按旧 dataspace 出去了。

新文件 `present/surface_dataspace.{hpp,cxx}`，只做一件事：
「按 HDR 类型选 dataspace 并设置，返回结果」。选择规则：

| HDR 类型 | dataspace |
| --- | --- |
| `kHdr10` | `ADATASPACE_BT2020_PQ`（163971072） |
| `kHlg` | `ADATASPACE_BT2020_HLG`（168165376） |
| `kSdr` | **不动**（返回 `applied=false` + reason「SDR 源不需要改 dataspace」） |

### 2.4 证据

`decode/8` 在 `color` 段里加 `hdr` 子对象：

```
"hdr": {
  "kind": "hdr10" | "hlg" | "sdr",
  "reason": null | "...",
  "static_info_bytes": null | 1234,
  "dataspace": { "applied": true, "value": 163971072, "reason": null }
}
```

`kind` 与 `reason` 成对：`sdr` 时 reason 说明"依据哪两个键判成 SDR"（或"没有色彩键"）。

---

## 三、判据怎么算过

| 项 | 判据 |
| --- | --- |
| 能识别 | HDR10 片源证据里 `hdr.kind == "hdr10"`，且 `color_standard/transfer` 为 6/6 |
| 能下发 | `hdr.dataspace.applied == true` 且 `value == 163971072`；解码器 configure 收到的格式里确实带这三个键（由"复制再写"保证） |
| 不误判 | SDR 片源（`h264_1080p8` / `hevc_1080p10`）必须 `kind == "sdr"` 且 `dataspace.applied == false` |
| 不伪造 | 缺键时证据写 `null` 并带 reason；**不许**用默认值把 SDR 标成 HDR |

**"可识别为 HDR"在这张卡里的定义就是这三条**：色彩键读到了、dataspace 设上了、SDR 不误报。
至于"屏幕上的画面是不是真的看起来像 HDR"，需要人眼或取色，**不在本卡**。

---

## 四、归属（家族树）

| 新增/改动 | 位置 | 理由 |
| --- | --- | --- |
| `present/surface_dataspace.{hpp,cxx}`（**新**） | 现有 `present/` 族 | 它操作的是 `ANativeWindow`，与 `surface_window_binder` 同族 |
| `decode/video_color_facts.cpp` | 加 `classify_hdr()` | 纯判定，与 `assess_color` 同一文件同一性质 |
| `decode/mediacodec_video_decoder.cxx` | configure 改为"复制再写" | 只有它持有 `AMediaCodec*` |
| `session/decode_session.cxx` | open 时调 dataspace | 会话是"窗口 + 解码器"的编排者 |
| `session/decode_stats_renderer.cxx` | 加 `color.hdr` | 证据出口 |

不新开族：`present/` 已有 `surface_window_binder` 与 `gles_oes_presenter`，
dataspace 是同一批"对 Surface 做的事"。

---

## 五、明确不做

- **不做 tone mapping**（E4）。SDR 屏上放 HDR 内容会过曝，那是 E4 的事；
  E2 只负责"让链路知道这是 HDR"。
- **不解析 SEI 里的静态元数据**。SEI 由解码器自己读，我们读不到也不需要读；
  真要读就得自己写 HEVC SEI parser，那是另一个量级。
- **不改容器**。给片源补 `mdcv`/`clli` 盒是"造片源"，不是播放器的事。
- **不宣称"支持 HDR 播放"**。本卡只证明"识别 + 下发 + 不误判"，
  画面是否正确要等 E4。

---

## 六、风险

| 风险 | 处置 |
| --- | --- |
| `dlsym` 取不到符号 | 记 `skipped_reason`，不抛错、不影响播放 |
| dataspace 设了但合成器不认 | 证据里只报"设了"，不报"生效了"；后者要 SurfaceFlinger 侧证据，留给 E3 |
| 自造 HDR10 片源是 SDR 上变换的 | 状态文档里写清"内容由 SDR 上变换，颜色不准；本轮只验元数据链路" |
| 复制格式时漏抄 csd-0 | 复制逻辑单独一个函数 + 单测（喂一个只带 csd-0 的假格式，断言抄到了） |

> **实测更正（2026-10-30，实现时发现）**：这一行做不到，原因是**没有"假格式"可造** ——
> `AMediaFormat` 只在 Android 上存在，桌面测试进程里根本构造不出来。
> 于是改成两条：
> 1. 纯策略（哪些键该写、源没标记就不写）抽进 `SourceColorTags::any()` 与
>    `classify_hdr()`，桌面单测覆盖；
> 2. 真正的"抄没抄到"由设备端 `kivo_hdr_probe` 的 A/B 两次 configure 证明：
>    若副本漏了 csd-0，B 组会 configure 失败，产物里一眼可见。
> 这比"喂一个假格式"更硬 —— 它验的是平台真的接受了这份副本。
