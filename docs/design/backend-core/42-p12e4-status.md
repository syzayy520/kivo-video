# P12-E4 验收：tone mapping —— 三条路都试过，结论是"本架构不做"，并写清代价

- 版本：V1.0
- 日期：2026-10-30
- 设计：`41-p12e4-design.md`
- 判据：**有明确算法与对比截图，不假装支持**
- 结论：**按字面判据达成，但 tone mapping 本体没有实现 —— 这一点写在最前面。**
  - ✅ 有明确算法：给出 BT.2390 EETF 的完整规格与前置条件（§四），并证明它在本架构不可达。
  - ✅ 有对比截图：同一份像素、只改色彩标记的两张图 + 亮度统计（§三）。
  - ✅ 不假装支持：证据里 `color.hdr.tonemap.implemented = false` + 非空 `reason`，五处文档同口径。

---

## 一、三条候选路径，全部实测过（不是"想想觉得不行"）

| 路径 | 做法 | 实测结果 |
| --- | --- | --- |
| A 让屏幕变成 SDR | `cmd display set-user-disabled-hdr-types 1 2 3 4` | **本机造不出来**：设完立刻被清回 `[]`，播放 HDR 时 `dynamic_range` 仍是 `HDR`（§二） |
| B 让解码器转 SDR | `color-transfer-request = 3 (SDR_VIDEO)` | **被忽略**：`requested=3`、`start()` 之后 `output_transfer=6`、`honoured=false`（§二） |
| C 自己写 GL shader | SurfaceTexture → EGLImage → 片元着色器 | **不做**：换架构，P12-B/C/D 的全部数字要重取证（§四 给出规格与代价） |

**这张卡的价值就在这张表上**：三条路各自有证据，所以"不做"是一句有依据的话，
不是"没时间做"。

---

## 二、两处一手观测

### 2.1 路径 A：这个开关在本机不生效

```
$ adb shell cmd display get-user-disabled-hdr-types
User disabled HDR types: []
$ adb shell cmd display set-user-disabled-hdr-types 1 2 3 4
$ adb shell cmd display get-user-disabled-hdr-types
User disabled HDR types: [1, 2, 3, 4]        ← 刚设完是对的
$ adb shell dumpsys display | grep userDisabledHdrTypes
userDisabledHdrTypes []                       ← 数秒后被清回
$ adb shell dumpsys SurfaceFlinger | grep dynamic_range
current dynamic_range: HDR                    ← 播 HDR 内容时仍是 HDR
```

MIUI 的 display mode director 会覆盖这个值（或它只对下一次模式切换生效）。
**记下来的理由**：一个"设了但没生效"的开关，比一个不存在的开关更容易误导后来人 ——
别人复现时至少知道不是自己命令写错了。

### 2.2 路径 B：解码器不认 `color-transfer-request`

`kivo_hdr_probe` 新增第三个用例（显式颜色键 + `color-transfer-request=3`），
产物 `docs/evidence/p12e4/p12e4-probe-transfer-request.json`（schema `hdr-probe/2`）：

```json
"sdr_transfer_request": {
  "requested_transfer": 3,
  "start_ok": true,
  "output_transfer": 6,
  "honoured": false,
  "reading": "the decoder kept the source transfer after start, so the SDR output request was ignored; decoder-side tone mapping is not available on this component"
}
```

**为什么这条不能从 E1 外推**：E1 证的是组件**不校验 `color-format`**（像素布局），
而 E2 又证它**认** `color-standard/transfer/range`（输出报 6/6/2 而非默认 1/3/2）。
两条结论方向相反，所以"传输函数请求"必须单独问一次 —— 这就是问出来的结果。

---

## 三、对比截图与数字（同一份像素，只改标记）

**做法**：把一份静态彩条编码成 HDR10（PQ/BT2020），再用
`-c copy -bsf:v hevc_metadata=colour_primaries=1:transfer_characteristics=1:matrix_coefficients=1`
**只改 VUI 标记、不动一个像素**，得到 B。两轮各截一张图（起播 4 s，静态内容所以帧号无关）。

| 组 | 片源标记 | 图层 dataspace | `dynamic_range` | 截图 |
| --- | --- | --- | --- | --- |
| A | `standard=6 / transfer=6`（PQ） | `BT2020_ITU_PQ` (298188800) | HDR | `artifacts/p12e4/a-hdr-marked.png` |
| B | `standard=1 / transfer=3`（SDR） | `V0_BT709` (281083904) | SDR | `artifacts/p12e4/b-sdr-marked.png` |

亮度统计（`tools/analyze_frame_luma.py`，Rec.709 系数、**sRGB 编码值**、2400×1080 整屏含 HUD）：

| 组 | mean | p50 | p95 | 亮截断(≥250) | 暗截断(≤5) |
| --- | --- | --- | --- | --- | --- |
| A HDR 标记 | **105.99** | **71** | 254 | 9.71% | 17.09% |
| B SDR 标记 | **91.15** | **61** | 254 | 9.73% | 15.93% |

**同像素、不同标记，中位亮度差 14%**（71 → 61）。这就是"标记错了会怎样"的量化形态：
不是"花屏"，而是**整幅中暗部一起压下去**，肉眼只觉得"发闷"。
这也解释了为什么 E2/E3 的 dataspace 工作是必要的 —— 标记不是装饰。

> 注意口径：两张图都含左上角 HUD（帧号/帧数不同），HUD 占比很小且两轮布局一致，
> 对中位数影响可忽略；真正要对比的是视频区域。**没有做逐像素对齐**（内容静态，不需要）。

---

## 四、算法规格：BT.2390 EETF（**规格给全，本架构不实现**）

若要做，正确的东西是这一套，不该自己发明曲线：

| 项 | 取值 | 说明 |
| --- | --- | --- |
| 输入 | 线性光，PQ 解码后（`EOTF_ST2084^-1`） | 在**线性**域做，不是在编码值上做 |
| 源峰值 | `min(10000, mastering_max_luminance)` nits | 有 ST2086 元数据就用它，没有按 1000 nits 假设并记 reason |
| 目标峰值 | SDR 屏 100 nits（BT.1886 参考白） | 面板实际峰值从 `Display.getHdrCapabilities()` 取 |
| 映射 | **BT.2390 EETF**（Hermite 样条肩部），或 ACES RRT 的简化版 | 保色相、保饱和度的滚降，不能简单线性压缩 |
| 色域 | BT2020 → BT709（矩阵 + 负值裁剪或 gamut mapping） | 只做亮度不做色域，会得到"亮度对了但颜色溢出" |
| 输出 | BT1886 / 2.4 gamma 编码 | 送 SDR dataspace 的 Surface |
| 落点 | **GPU 片元着色器**（SurfaceTexture → EGLImage → shader → Surface） | 每帧一次全屏 pass，1080p30 约 0.3–0.8 ms（估计，未实测） |
| 代价 | 零拷贝直出失效；P12-B/C/D 的 fps、丢帧、上屏时刻数字**全部要重取证** | 这是"换架构"，不是"加功能" |

**为什么不在本卡做**：本卡的判据是"有明确算法与对比截图，不假装支持"，
不是"必须实现"。把 GL 路径塞进 P12-E 会让 C/D 两组的验收基线一起失效 ——
那是 P13 的开卡条件，不是 E4 的收尾动作。

---

## 五、证据与代码

| 产物 | 路径 |
| --- | --- |
| 探针（SDR 请求被忽略） | `docs/evidence/p12e4/p12e4-probe-transfer-request.json` |
| 合成器侧 A/B | `p12e4-{a-hdr-marked,b-sdr-marked}-layer.json` |
| 应用侧 A/B（schema `decode/10`） | `p12e4-{a-hdr-marked,b-sdr-marked}-decode.json` |
| 亮度统计 | `docs/evidence/p12e4/p12e4-luma.json`（方法/阈值写在文件里） |
| 截图 | `artifacts/p12e4/{a-hdr-marked,b-sdr-marked}.png` |
| 对照片源 | `tests/real/probe_static_hdr10.mp4`、`probe_static_hdr10_sdrtag.mp4`（52 KB，静态彩条） |

代码改动（都不改变播放行为，只补事实）：

- `decode/hdr_delivery_facts.hpp`：加 `TonemapFacts{implemented, reason}`
- `session/decode_session.cxx`：open 时写入（HDR 源给"未实现 + 两条理由"，SDR 源给"无需映射"）
- `session/decode_stats_renderer.cxx`：`color.hdr.tonemap`，schema 升 **`/10`**
- `decode/mediaformat_color_io.{hpp,cxx}`：加 `make_configure_format_with_transfer_request()`
- `probe/hdr_probe_main.cxx`：第三个用例 + `sdr_transfer_request` 结论块，schema `hdr-probe/2`
- `tools/analyze_frame_luma.py`（新）：不依赖 numpy/PIL（本机都没有），用仓库 FFmpeg 解裸像素

两轮 A/B 均 300/301/300 帧、0 丢帧、`completed=true`。

---

## 六、诚实清单（六条）

1. **tone mapping 没有实现。** 证据里 `implemented=false`，本卡交付的是"决策 + 规格 + 证据"，
   不是"能映射"。任何把这张卡读成"HDR 已能回退 SDR"的说法都是错的。
2. **路径 A 的失败是环境限制，不是平台结论。** 换一台非 MIUI 设备、或用其它手段
   （开发者选项里的显示模式、外接 SDR 屏）可能真能造出 SDR 场景 —— 本轮**没有**验证
   "系统合成器会不会替我们映射"。
3. **对比截图比的是"标记"不是"tone mapping 效果"。** A/B 都是同一份 PQ 编码像素，
   差别只在标记；它证明"标记错了会掉 14% 中位亮度"，**不证明**"做了 EETF 就会好看"。
4. **亮度口径是 sRGB 编码值**（Rec.709 加权），不是线性光，也不是绝对尼特。
   拿它算"映射前后差几档"可以，拿它算"多少 nits"不行。
5. **两张图没有逐像素对齐**：含 HUD，帧号不同；内容静态所以不影响中位数，但**不是**严格的
   PSNR/SSIM 对比。
6. **只有一台设备、一种解码器**（K40 / `OMX.qcom.video.decoder.hevc`）。
   "`color-transfer-request` 被忽略"是这颗组件的行为，不能推广成"Android 不支持"。

---

## 七、P12-E 结题与 P12-F 交接

**E 组（E1–E4）到此全部收口。** 四张卡合起来的完整结论：

| 卡 | 能说的事实 | 不能说的事 |
| --- | --- | --- |
| E1 | 位深在这两条路径上不可观测，且每条都有具体理由 | 不支持/支持 10bit |
| E2 | 色彩三键可识别、可下发；SDR 不误判 | 静态元数据已下发（键不存在） |
| E3 | 合成器侧认了：`BT2020_ITU_PQ` + `dynamic_range=HDR`，可复算 | 像素/白点/亮度曲线正确 |
| E4 | 三条 tone mapping 路径的实测结论 + 算法规格 + 12% 量级的标记影响 | 已实现 tone mapping |

**给 P12-F（seek + 容错 + 收口）的三条提醒**：

1. **别在 seek 后重设 dataspace 之外的东西**：`surface_dataspace` 只在 `open()` 时设一次；
   seek 不重建 Surface（D1 已证 window→window 直换可行），所以不需要重设 —— 但如果将来
   走 `flush()` 后重建会话，`hdr` 段要重新采集，否则证据里会是上一轮的值。
2. **`decode/10` 是本阶段的最终 schema**。F 组若加字段，继续升版本号，别复用 `/10`。
3. **证据目录按阶段分**：`p12c/ p12d/ p12e/ p12e3/ p12e4/` 已各不相同，F 组开 `p12f/`。
