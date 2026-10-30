# P12-E4 设计：tone mapping —— 先把"能做什么"测清楚，再谈算法

- 版本：V1.0
- 日期：2026-10-30
- 卡片判据：**有明确算法与对比截图，不假装支持**
- 关联：`40-p12e3-status.md` §五（E3 给 E4 的交接）

---

## 一、判据先拆成三句，否则"做了 tone mapping"和"画面变暗了"分不开

| 判据片段 | 它要求什么 | 容易怎么骗自己 |
| --- | --- | --- |
| **有明确算法** | 说得出"输入是什么、输出是什么、依据哪个参数" | 把"调了一下 dataspace"说成算法 |
| **有对比截图** | 同一帧内容，映射前后各一张，能看出差别 | 拿两次不同片源的截图对比 |
| **不假装支持** | 做不到就说做不到，带 reason | 把系统做的映射说成我们做的 |

**这张卡最容易犯的错**：把"系统合成器自己把 HDR 映到 SDR"写成"我们实现了 tone mapping"。
所以本卡的第一件事不是写算法，而是**确认这件事到底发生在哪一层**。

---

## 二、一手观测：三条候选路径，先各打一枪

### 2.1 路径 A：把屏幕变成"SDR 屏"，看系统怎么处理

Android 有 `cmd display set-user-disabled-hdr-types`，理论上能让显示链路把 HDR 类型全部禁用。

**实测（2026-10-30，K40）：这条路在本机造不出来。**

```
$ adb shell cmd display set-user-disabled-hdr-types 1 2 3 4
$ adb shell cmd display get-user-disabled-hdr-types
User disabled HDR types: [1, 2, 3, 4]      ← 刚设完是对的
（数秒后）
$ adb shell dumpsys display | grep userDisabledHdrTypes
userDisabledHdrTypes []                     ← 被清回去了
$ dumpsys SurfaceFlinger | grep dynamic_range
current dynamic_range: HDR                  ← 播放 HDR 内容时仍然是 HDR
```

MIUI 的 display mode director 会把这个值清掉，或者该值只对下一次模式切换生效。
**结论：本机无法用这个开关模拟"SDR 屏"。** 谁想复现这个实验，别以为是自己命令写错了。

> 这条失败同样要留在文档里 —— 与 D5 的"层名少 `#id`"一样，
> **一个"设了但没生效"的开关，比一个不存在的开关更容易误导人。**

### 2.2 路径 B：让解码器自己转 SDR（`color-transfer-request`）

`MediaFormat` 有一个独立于 `color-format` 的键：`color-transfer-request`（API 28+，字面量
`"color-transfer-request"`）。语义是"请把输出转成这个 transfer"，与 E1 试过的
`color-format`（像素布局）不是一回事 —— E1 已证这颗 OMX **不校验** `color-format`，
但**认** `color-standard/transfer/range` 三键（E2 §三：输出报 6/6/2 而不是默认 1/3/2）。
所以这个键**有可能**被采纳，必须单独测，不能从 E1 的结论外推。

判据（设备端可测，不需要读像素）：对 HDR10 源配置
`color-transfer-request = 3 (COLOR_TRANSFER_SDR_VIDEO)`，**start 之后**读输出格式的
`color-transfer`：

- 变成 `3` ⇒ 解码器接受了 SDR 输出请求（像素是否真被映射，要再看截图）；
- 仍是 `6` ⇒ 组件忽略该键（与 `color-format` 同样的行为），这条路作废。

### 2.3 路径 C：自己写 GL shader

需要把解码输出从 Surface 改成 SurfaceTexture/EGLImage，再在片元着色器里做 PQ→SDR 映射。
**这是换架构**，不是加功能：零拷贝直出变成"解码→纹理→合成"，P12-B/C/D 的全部数字
（175 fps / 0 丢帧 / 上屏时刻）都要重新取证。E1 设计 §5.2 已把它列为"单独立卡"。
本卡**不做**，只在结论里写清"不走这条路的代价是什么"。

---

## 三、做法（按 2.2 的测量结果分岔）

### 3.1 先测（本轮必做）

扩 `probe/hdr_probe_main.cxx`：加第三个用例
`explicit_color_write_plus_sdr_request`（显式颜色键 + `color-transfer-request=3`），
读 `start()` 之后的 `color-transfer`。产物 schema 升 `kivo.p12e.hdr-probe/2`。

### 3.2 分岔 A：解码器接受（输出 transfer == 3）

算法成立，定义为：

```
若 显示链路支持 HDR（Java 侧 Display.getHdrCapabilities().getSupportedHdrTypes() 非空）
   → 原样输出 PQ + 窗口 dataspace = ADATASPACE_BT2020_PQ（现状）
否则（SDR 屏 / 用户禁用 HDR）
   → configure 时加 color-transfer-request = SDR_VIDEO
     窗口 dataspace 设为 BT709/SDR（不再标 PQ）
     **并在证据里写明 `tonemap = "decoder_request_sdr"`**
```

判据：同一帧内容两张截图（HDR 路径 vs SDR 路径），亮度中位数与截断比例在可解释范围内；
证据里有 `tonemap` 字段说明这一轮走的是哪条。

### 3.3 分岔 B：解码器忽略（输出仍是 6）

**明确宣布本架构不做 tone mapping**，并给出三样东西，缺一不可：

1. **算法结论**：SDR 屏上正确的做法是"解码→GL shader 映射"，本架构（Surface 直出）做不到；
   不写一个假的"我们映射了"。
2. **对比截图**：HDR 内容在 SDR 标记下与 HDR 标记下的差异（同一帧、同一片源），
   量化亮度中位数/截断比例 —— 证明"不映射会怎样"。
3. **reason 非空**：`tonemap = "unsupported"` + `tonemap_reason` 写明"解码器不接受
   transfer 请求，且未引入 GL 路径"，符合 G9（不许静默降级）。

---

## 四、判据与边界

| 项 | 判据 |
| --- | --- |
| 有明确算法 | 证据里有 `tonemap` 字段，取值来自实测分岔；分岔 B 时必须带 `tonemap_reason` |
| 有对比截图 | `artifacts/p12e4/` 下同一帧的两张 PNG，且附亮度统计（中位数 / 截断比例） |
| 不假装支持 | 任何"系统做的映射"都不写成我们的；分岔 B 时明确写"未实现" |
| 不破坏既有 | E1–E3 的证据链不回退：HDR 轮仍 `dataspace.applied=true` 且图层 `BT2020_ITU_PQ` |

**明确不做**：GL/EGL 路径、HLG、DV、真实电影级 tone mapping 曲线（ACES/Rec.2100 全套）、
按场景自适应（HDR10+ 动态元数据）。

---

## 五、家族树

| 新增/改动 | 位置 | 理由 |
| --- | --- | --- |
| 探针第三用例 | `probe/hdr_probe_main.cxx` | 与 A/B 同族，只是多加一种请求 |
| `tonemap` 事实字段 | `decode/hdr_delivery_facts.hpp` | 纯数据，与 `HdrDeliveryFacts` 同族 |
| 请求下发 | `decode/mediaformat_color_io.{hpp,cxx}` | 已经独占 `AMediaFormat` 的读写 |
| 证据渲染 | `session/decode_stats_renderer.cxx` | `color.hdr.tonemap`，schema 升 `/10` |
| 亮度统计（**新**） | `tools/analyze_frame_luma.py` | 用仓库内 ffmpeg 解 PNG → 原始 RGB，算中位数/截断；不引 numpy/PIL（本机没有） |
| 截图 | `artifacts/p12e4/` | 与 `artifacts/p12e2/` 同级 |

---

## 六、风险

| 风险 | 处置 |
| --- | --- |
| "设了没生效"的开关被当成"不支持" | 每个开关都验**生效**（回读/`dumpsys`），不生效就写"本机无法构造该场景" |
| 截图对比不是同一帧 | 用固定片源 + 固定起播后延时；截图前确认 HUD 帧号接近（HUD 上有 frames） |
| 把系统映射说成自己的 | 证据字段用 `tonemap=decoder_request_sdr` / `unsupported`，不用"已支持"这种词 |
| 亮度统计口径不一致 | 统一在 **sRGB PNG 的 Y 通道**上算，写清是"截屏后的编码值"，不是线性光 |
