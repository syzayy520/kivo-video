# 177 · P15 计划：把 G1/G2/G3/G5/G8 从"有机制"变成"有数字"

- 状态：**计划，待批准**（这一份不写实现）
- 背景：`03-goals.md` 的 G1–G10 是 P11 为 **Windows 参考实现**定的。Android 侧现在
  **只有 G6 / G7 / G10 有实测数字**，G3 / G5 / G9 机制在但缺数字，G1 / G2 / G4 / G8 缺失或不可照搬。

## 零、先说结论

| 目标 | 能不能在现有机制上补 | 代价 |
|---|---|---|
| **G8 Seek** | ✅ 入口已有（`kivo.seek.ms` / `kivo.seek.at.ms`），只缺出版与判据 | 一格出版 + 脚本 + 判据 |
| **G5 零拷贝** | ✅ 结构上已成立，缺一个"确实是 0"的计数 | 一个计数器 + 判据 |
| **G3 零分配** | ✅ `alloc_probe` 已有 `arm/mark_steady/calls_steady`，缺出版 | 一格出版 + 判据 |
| **G1 / G2** | ⚠️ **不可照搬**：Android 的 `decode_fps` 被节拍器封顶，测不出硬解上限 | 要先重新定义 + 新增 decode-only 路径 |
| **G4 表面池** | ❌ 不适用（系统 Surface 池，没有自研池） | 建议在目标书写"不适用 + 理由" |

结论：**4 条能补，1 条要重定义，1 条不适用**。下面逐条给设计。

## 一、P15-A · G8 Seek（最便宜，先做）

**要的数字**：seek 后首帧显示 < **150 ms**；seek 后 **30 帧内无错序 / 花屏**。

**现在有什么**
- 入口：`PlayerActivity` 已支持 `--ei kivo.seek.ms <目标>` + `--ei kivo.seek.at.ms <何时触发>`；
- 首帧延迟：`video.first_frame_latency_us` 已出版（但那是**会话首帧**，不是 seek 后首帧）；
- 帧序：`timeline.media_pts_span_ns` + `pace.pts_span_ns` 能看出 pts 是否单调推进。

**缺什么**
1. 报告里没有"这一次 seek 发生在哪、seek 后首帧多久"的格子 —— 现在只有会话级 `first_frame_latency_us`；
2. 没有判据；
3. 没有脚本。

**探针设计**
- 出版 `video.seeks[]`（环 4）：每次 seek 一条 `{requested_ms, at_media_ns, first_frame_latency_us, pts_after_ns}`；
- 判据 **AS1**：每条 seek 的 `first_frame_latency_us < 150000`；
- 判据 **AS2**：`pts_after_ns` 必须落在 `requested_ms` 的 ±2 帧内（seek 到哪就从哪开始）；
- 判据 **AS3**：seek 之后 30 帧的 pts 必须严格递增（用 `timeline` 的窗口段或 `pts_span` 判定）。

**脚本**：一次运行里在 5 s / 20 s / 40 s 各 seek 一次，跑两份片源（1080p30 / 4K30）。

## 二、P15-B · G5 零拷贝

**要的数字**：呈现路径 CPU 侧像素拷贝次数 = **0**。

**现在有什么**：MediaCodec 输出到 `Surface`（`present_schedule = surface_at_time`），
结构上**没有** `sws_scale` / `memcpy` 的路径 —— 但这是"我说没有"，不是"证据说 0"。

**缺什么**：一个运行时计数。

**探针设计**
- 在"从解码器拿到帧 → 交给 Surface"这条路上加 `cpu_copy_bytes` 累加器（正常路径上它**没有地方可加**，
  所以恒为 0；一旦有人加了回读，这个数立刻非 0）；
- 出版 `video.cpu_copy_bytes`；
- 判据 **AC5**：必须为 0。

**诚实的风险**：这个计数器只能守住"我们知道的那几条路"。真正要防的是**将来**有人加一条
`Image → Bitmap` 的回读 —— 那需要连 `pipeline_mode` 一起看（出现新的 mode 就该重审）。

## 三、P15-C · G3 零分配

**要的数字**：稳态循环内 `operator new` 调用数 = **0**。

**现在有什么**：`core/metrics/alloc_probe`（`arm` / `mark_steady` / `calls_steady`），
有单测（`p14a_metrics_tests`），**没有真机数字**。

**缺什么**：`mark_steady()` 是否已经在首帧渲染后调用、以及出版。

**探针设计**
- 确认 `mark_steady()` 的调用点在首帧呈现之后（不在会话 start）；
- 出版 `video.alloc_steady_calls` / `alloc_steady_bytes`；
- 判据 **AG3-1**：`alloc_steady_calls == 0`；
- **如果它不是 0**：不要立刻判红 —— 先把数打出来，看是几百还是几万。几百说明有一条可查的路径，
  几万说明热路径还在分配。**先量后判**，这是这一条与其它几条不同的地方。

## 四、P15-D · G1 / G2（要重新定义，不能照搬）

**原口径**：1080p HEVC10 纯解码 ≥ 250 fps、协商到 P010；硬解 CPU 时间 ≤ 软解的 10%。

**为什么不能照搬**
- Android 的 `decode_fps` 是**整条管线**的帧率，被节拍器封顶在片源帧率（30 / 60）。
  4K30 片源上它永远读 30，与"硬解能跑多快"无关；
- `speed_vs_realtime` 同理（它是"播放倍速"，不是"解码倍速"）；
- P010 在 Android 由 `MediaCodec` 协商，报告里已有 `codec` 块（含 format），但**没有"是否 10bit"的判定**。

**建议的 Android 版（G1′ / G2′）**
- **G1′**：4K60 HEVC 片源连续解码 **300 帧**，`decode_only` 模式下 `decode_fps ≥ 2 × 片源帧率`
  （即硬解有余量），且 `codec.format` 逐字匹配 `COLOR_FormatYUV420P10LE` / `P010`；
- **G2′**：同一段片源用 FFmpeg 软解跑一遍，`cpu_ns_per_frame(硬) / cpu_ns_per_frame(软) ≤ 0.10`。

**代价**：要新增一条 `decode_only` 路径（不限速、不呈现、不节拍），并出版每帧 CPU 时间。
**这是五条里唯一要动产品代码路径的一条**，所以要单独一张卡、单独一轮对照。

## 五、P15-E · G4（建议写"不适用"）

Android 上没有自研表面池 —— 输出面由 `MediaCodec` + `Surface` 管理。
G4 的"池 `in_use` 归还为 0、`allocated` 收敛"在这里没有对应物。

**建议**：在 `03-goals.md` 的 G4 后面加一句"Android 不适用，理由：…"，
而不是让它永远挂在"未验"里。**空着的验收项比错误的验收项更容易被忘掉。**

## 六、顺序与理由

```
P15-A（G8 seek）     ← 入口已有，只缺出版与脚本；一份证据就能同时回答"能不能用"
P15-B（G5 零拷贝）   ← 一个计数器，改动最小
P15-C（G3 零分配）   ← 先量后判，可能牵出一条热路径
P15-E（G4 不适用）   ← 文档改动，顺手
P15-D（G1′/G2′）     ← 唯一要改产品路径的一条，放最后
```

前三条都是"出版 + 判据 + 一轮真机"，彼此不依赖；做完之后 G 表的缺口从 6 条降到 2 条
（G1′/G2′ 与 G4 的"不适用"说明）。

## 七、不做的事

1. **不为凑 G 表去改阈值**（250 fps / 10% 是 P11 的 Windows 数字，Android 用 G1′/G2′ 的写法）
2. **不在这一批里碰播放行为**（除了 P15-D 的新增 `decode_only` 分支，且它默认关闭）
3. **不追溯**：G 表更新只对 `/42` 之后的证据生效
