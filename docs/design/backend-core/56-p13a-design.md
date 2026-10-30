# P13-A · 高分辨率 / 高帧率覆盖（设计）

- 版本：V1.0
- 日期：2026-10-30
- 上游：`54-p12f4-status.md` §六（能力边界：4K/HEVC/60fps **未测**）、§七（交接：先补覆盖面）
- 前提：K40（SM8250 / 骁龙 865）是唯一设备；不引入第二台设备也能推进

---

## 一、先纠一个已有的错误认知

`p12a` 能力矩阵里，所有硬解 codec 都报 `max_width = max_height = 8192`：

```
OMX.qcom.video.decoder.hevc | hevc-main10-hdr10 | max 8192x8192
```

**这个数字不能用来判断"4K60 能不能实时解"。** 它来自
`VideoCapabilities.supportedWidths.upper` —— 是这块芯片的**绝对尺寸上限**，
与帧率、与像素吞吐都无关。拿它当"支持 4K"的证据是错的，拿它当"支持 4K60"更是错两次。

正确的 API 是另外三个：

| API | 回答什么 |
| --- | --- |
| `videoCapabilities.isSizeSupported(w, h)` | 这个尺寸能不能解 |
| `videoCapabilities.getSupportedFrameRatesFor(w, h).upper` | **在这个尺寸下**的最大帧率 |
| `videoCapabilities.achievableFrameRatesFor(w, h)` | 考虑像素吞吐后的**可达**帧率 |

后两个是**分辨率相关**的 —— 同一个 codec 在 1080p 能跑 240fps、在 4K 可能只到 60fps。
所以必须先按分辨率查，才能定片源规格。

---

## 二、这一步的判据（只做能力探测，不做播放）

**目标：产出一张"分辨率 × 帧率"能力表，用它来决定造什么片源。**

| 分辨率 | 帧率候选 |
| --- | --- |
| 1920×1080 | 30 / 60 / 120 |
| 2560×1440 | 30 / 60 |
| 3840×2160 | 30 / 60 |

对每个硬解 codec（`video/hevc` 的 main / main10、`video/avc` 的 high）查上面三个 API。

**判据**：表里必须能回答"4K60 是否可达"。达不到就老实降到 4K30 或 1440p60，
不硬凑 —— 造一个解不动的片源，测出来的"丢帧"是片源选的错，不是播放器的错。

---

## 三、为什么单独开一个入口，不改能力矩阵

现有能力矩阵的证据（`kivo.p12a.capabilities/1`）**已经收口，9 字段固定**。
往里加帧率字段要改 parser、改 renderer、改 schema 版本，还会让 P12-A 的证据失效。

而这两件事的语义也不同：

| | 能力矩阵（P12-A） | 帧率探测（P13-A） |
| --- | --- | --- |
| 回答 | "这台机器有哪些 codec" | "这些 codec 在特定尺寸下能跑多快" |
| 粒度 | 每 codec 一条 | 每 codec × 每分辨率一条 |
| 变化频率 | 换设备才变 | 换片源规格就要重查 |

**结论：新开一个独立探针**，不动 P12-A。

落位（遵守单文件单职责）：

| 文件 | 唯一职责 |
| --- | --- |
| `bridge/MediaFramerateInventory.kt` | 调 `VideoCapabilities` 的三个 API，摊平成 TSV |
| `ui/FramerateDumpActivity.kt` | 无 UI 入口：采集 → 写文件 → `finish()` |
| `device/mediacodec_framerate_*.cxx` | 解析 + 渲染 JSON + 写盘（与 P12-A 同构，复用既有分层） |

> 也可以省掉 C++ 层，直接在 Kotlin 里写 JSON。但项目一致的规矩是
> **Kotlin 只读平台 API 并摊平，判定与序列化在 C++** —— 这条不破例。

---

## 四、拿到能力表之后（本卡的后续步骤，先写清楚）

1. **定片源规格**：按能力表选 2–3 个规格，用仓库 ffmpeg 造（H.264 + HEVC，含高码率）。
2. **跑播放取证**：复用现有 decode 证据链路（`kivo.media` + `kivo.evidence`），
   判据沿用 G8/D 组那套：帧数闭合、零丢帧、`av_drift`、`seek` 可用。
3. **高码率单列**：50 Mbps 以上的码率对 I/O 和缓冲是另一类压力，单独一组。
4. **已知边界**：K40 屏幕 2400×1080，4K 片源必然被 Surface 缩放 ——
   这测的是**解码 + 缩放上屏**，不是 4K 原生输出。这一点不再重复强调，但要写进证据的 `scope`。

---

## 五、风险

1. **模拟器路线已封死**（`55-device-matrix-design.md` §四）：x86_64 要管理员权限的驱动，
   ARM 镜像被 emulator 37.x 移除。所以本卡只能在 K40 上做。
2. **能力表是"宣称值"，不是"实测值"。** `getSupportedFrameRatesFor` 返回的是厂商声明，
   真跑不动的情况存在。所以第 2 步必须真播，不能拿能力表当结论。
3. **4K 片源体积大**：60 Mbps × 10 s ≈ 75 MB，仓库里放一两个就够，不堆。
