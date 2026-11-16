# 310 · HLS ABR 设计方案（基于 309 实测的 program 模型约束）

状态：**DRAFT_FOR_REVIEW** —— 规划稿，不含实现
依据：`docs/evidence/p309-hls-abr/status.md`（实测）、275 §5.3 / §6.2（口径）
时间：2026-11-16 05:20–06:10

---

## 一、先定事实：两种 HLS 结构，能力完全不同

这是 309 最重要的发现，也是本方案的地基。

| master 结构 | 典型来源 | FFmpeg 解析成 | mpv 暴露 | 运行时切档 |
| --- | --- | --- | --- | --- |
| **A：无 `#EXT-X-MEDIA`** | 简单转码工具、我做的测试流 | 1 个 program + N 个 video stream | **N 个 track**（`--vid=1..N`） | ✅ **可以**，实测 **293 ms**、无黑帧 |
| **B：有 `#EXT-X-MEDIA`** | **Apple / Netflix / Emby fMP4 全是这种** | **N 个 program**（每档一个，带 `variant_bitrate`） | **1 个 track** | ❌ **不能** |

实测证据（Apple 官方流）：

```
# master 里有 24 个 EXT-X-STREAM-INF + 5 个 EXT-X-MEDIA
ffprobe -show_programs → program_id=0..23（每 program 1 video + 1 audio，带 variant_bitrate）

我们 app 里：
hls: Opening '.../v5/prog_index.m3u8'   ← 8 个 variant 全被打开
hls: Opening '.../v9/prog_index.m3u8'
...
● Video --vid=1 (h264 960x540 60 fps)   ← 但 mpv 只列出 program 0
```

**归因**：mpv **没有 `--program` 选项**（`options.c` 无此定义），一次只能播一个 program，
且无法在运行时换。⇒ 结构 B 的流，**mpv 路径上的运行时 ABR 不可行**。

> 与我们代码无关：代理的 `resolve()` 正确（`dir + path`），8 个 variant 全部成功拉取；
> `ffprobe` 直连也能看到全部 stream。**限制来自 mpv/FFmpeg 的 program 模型。**

---

## 二、那 ABR 还能怎么做：三条路线

### 路线 1：`--hls-bitrate` 启动选一档（最小改动，不算 ABR）

- 做什么：起播前按探测带宽设 `hls-bitrate=<bps>`，mpv 选最接近且不超过的那档；
- 能力边界：**一次性决策**，播放中不换档；卡了也不会降；
- 275 对照：不满足「每段采样 / EWMA / 迟滞 / 驻留」任何一条；
- 结论：**只能作为兜底**，不能宣称 ABR。

### 路线 2：代理层裁剪 master + 重开（推荐）

**核心思路：把结构 B 变成结构 A，让 mpv 只看到一档。**

```
mpv ──GET master.m3u8──> 我们的代理
                          │ 1. 拉真实 master
                          │ 2. 按 ABR 决策选出一档
                          │ 3. 返回只有该档的 master（其余剔除）
                          ▼
                        mpv 只看到 1 个 variant → 无 program 冲突
```

切档时：
- 代理改变"裁剪目标" ⇒ 需要 mpv **重新加载**才能生效（VOD 的 master 不会 reload）；
- 重载代价 = 重新缓冲。**这是 VOD ABR 的本质代价**，不是实现缺陷；
- 可用「先预取新档首个分片，再触发重载」把黑屏压到最小。

**为什么这条能超越 SenPlayer**：它的 mpv 同样受 program 限制，
大概率只能"选一档不能切"或"切了要黑屏重来"。我们把选档做成**有依据的、可观测的、可回退的**，
并且把重载窗口压到最小 —— 这是工程优势，不是玄学调参。

### 路线 3：不用 HLS 引擎，自己做 segment 调度

自己解析 manifest、自己拉分片、自己喂解码器。
- 上限最高（真无缝切档）；
- 成本最大（要重写 source 层、时间线、加密、字幕组）；
- 与 mpv 的 demux 直接冲突，等于放弃 mpv 的 HLS 能力。

**结论：不做。** 275 的原则是"先验证引擎能力再设计"，引擎既然能用，就不该绕过它重造。

---

## 三、推荐方案：路线 1 + 路线 2 的组合

### 3.1 三层结构（家族树）

```
playback/hls/
├── HlsMasterProbe.kt        —— 解析 master：有几个 variant、结构 A 还是 B、每档 bandwidth/resolution/codec
├── HlsVariantPolicy.kt      —— 纯决策：给定「可用档位 + 实测吞吐 + runway + 设备能力」→ 选哪一档 + 为什么
├── HlsMasterRewriter.kt     —— 把 master 裁成只剩目标档（文本级处理，不碰分片）
└── HlsAbrController.kt      —— 编排：采样吞吐 → 调 Policy → 需要换档时通知代理改裁剪 + 触发重载
```

依赖方向：`HlsAbrController → (Policy, Rewriter, Probe)`；
`Policy` 是**纯函数**（无 IO、无 Android），可直接 JVM 单测。

### 3.2 与现有代码的接缝

| 现有件 | 角色 |
| --- | --- |
| `PassthroughSource` / `LocalRangeServer` | HLS 子请求转发点 → **Rewriter 挂在这里**（master 请求时改写响应体） |
| `OriginLatencyStats` | 吞吐来源（需**按分片**切分，现在按请求） |
| `RebufferPolicy` | runway 逼近下限的判定（降档触发） |
| `JitterTracker` | 抖动噪声 → 抑制误判 |
| `Av1HardwareProbe` / `resolutionTier` | codec × 分辨率闸门（302 已有先例） |
| `PlaybackMetrics` | 切档原因/前后档位/估计带宽的可观测落点 |
| `MpvPlaybackSession.seekToAsync` | **重载实现可复用它**（303 的票据机制同样适用） |

### 3.3 决策算法（严格对齐 275 §5.3）

| 275 要求 | 落地 |
| --- | --- |
| 每段采样有效吞吐 | 每个 segment 完成时记 `bytes / elapsed`，进 `OriginLatencyStats` 的分片维度 |
| EWMA 抑波 | `θ ← α·sample + (1-α)·θ`，α 取 0.25（单段权重不超过 1/4） |
| 峰值带宽 < 安全吞吐预算 | `budget = θ × 0.7`（留 30% 抖动余量），候选档 `BANDWIDTH ≤ budget` |
| 满足 decoder profile / 分辨率 / runway | 三者并联闸门：`Av1HardwareProbe` + `resolutionTier` + `runway ≥ 2×segment` |
| 升档需连续多窗口余量 | 连续 **3** 个采样窗口满足 `budget ≥ 目标档 × 1.3` 才升 |
| 降档快速 | `runway < 1.5×segment` 或 `θ < 当前档 × 0.8` ⇒ **立即降**（单窗口即可） |
| 最短驻留时间 | **10 s**（避免来回抖） |
| 升降档不同阈值 | 升 ×1.3、降 ×0.8（迟滞带 1.3/0.8） |
| 切档原因可观测 | `PlaybackMetrics` 记 `{from, to, reason, θ, runway, at}`，`NetDiag` 同步一行 |

### 3.4 起始档怎么定（解决 309 暴露的"默认盲选 max"）

现状：mpv 默认 `hls-bitrate=max` ⇒ 一上来就拉最高档 ⇒ 慢源必然卡（这与 274 的 4K 选源错误同源）。

新逻辑：起播前用**已有实测**做首档决策：
1. `OriginLatencyStats` 的首包延迟 + 表头耗时（≈3 s 内可得）；
2. 或退一步：用 `MAX_PLAYABLE_BPS` 同源阈值（`EmbySourcePicker` 已在用 12 Mbps）；
3. 再叠设备闸门（AV1/4K 不可播则直接排除该档）。

⇒ **首档 = 满足预算的最高档**，而不是"最高的那档"。

---

## 四、验证口径（275 §6.2 逐条，先写判据再实现）

| 275 要求 | 判据 | 装置 |
| --- | --- | --- |
| manifest 完整链路 | 结构 B 的流也能起播（裁剪后单档） | Apple 官方流 |
| segment 完整链路 | 分片全部成功，`Failed to open segment` = 0 | 同左 + 日志计数 |
| 鉴权 | 401/403 场景给出明确原因 | 需构造鉴权流（**未做**） |
| seek | 裁剪后的流 seek 正常 | **当前 URL 路径无 seek 钩子，需先补** |
| 网络恢复 | 断网 → 恢复 → 时钟前进 ≤ 3 s | 复用 308 的 `NetworkRecoveryWatch` |
| **切换无明显黑帧** | 重载窗口内出帧缺口 < 500 ms（YAVG 不落 16） | 录屏 + `signalstats`（309 已建好） |
| **无 ABR 振荡** | 10 分钟内升/降档 ≤ 3 次 | `PlaybackMetrics` 切档序列 |
| 切档可观测 | 每次切档有四元组 `{from,to,reason,θ}` | metrics JSON |

**注意**：因为 VOD 切档要重载，**"无黑帧"这一格很可能达不达标**。
若实测重载黑屏 > 500 ms，则该结论必须如实记「VOD 切档有可见中断」，
并把它作为**与 SenPlayer 对比的实测项**（它大概率同样黑，甚至更久）。

---

## 五、超越 SenPlayer 的具体着力点

275 §1.4 明确：SenPlayer 的 ABR「**没有证据指出吞吐估计、buffer rule、切换阈值、升降档迟滞或切换时机**」——即**未知，不是已知更好**。

我们能建立优势的三处（都可实测）：

1. **首档不盲选**：从"永远 max"改成"按实测带宽 + 设备能力选"。慢源下这是**首屏与起播卡顿**的直接差异。
2. **决策可观测**：每次切档有四元组落盘。SenPlayer 是黑盒 —— 出了问题我们能复算，它不能。
3. **重载窗口压到最小**：先预取新档首分片再切，把黑屏压到接近 0。

**不承诺**：做不到"真无缝切档"（那要走路线 3，代价过大）。这一点必须写在结论里，不能吹。

---

## 六、实施计划（建议分三卡，不要一次做完）

| 卡 | 内容 | 依赖 | 验收 |
| --- | --- | --- | --- |
| **310-A** | `HlsMasterProbe` + `HlsMasterRewriter` + 代理接缝 | 无 | Apple 流能起播（结构 B → 单档）；`ffprobe` 对照档位正确 |
| **310-B** | `HlsVariantPolicy`（纯函数）+ 单测 | A | 时间表式单测：给定吞吐序列，档位序列确定 |
| **310-C** | `HlsAbrController` + 重载 + metrics | A+B | 275 §6.2 八条判据全跑一遍（含黑帧与振荡） |

**每卡都必须先出判据再实现**（AGENTS.md 铁律：未完成规划不得进入实现）。

---

## 七、这一稿**没有**解决的事

1. **鉴权 HLS 未测**（401/403、Cookie、DRM）—— 这会影响 Rewriter 能否直接改响应体；
2. **URL 路径没有 seek 钩子** ⇒ 275 要求的 "seek 完整链路" 现在测不了，需先补钩子；
3. **重载黑屏时长未知** —— 这决定"无黑帧"那格能不能过；
4. **DASH 完全未碰** —— 本方案只覆盖 HLS；
5. **多音轨/字幕组**（`#EXT-X-MEDIA` 的 AUDIO/SUBTITLES 分支）在裁剪 master 时如何保留，未定；
6. **加密（`#EXT-X-KEY`）** —— 裁剪不能破坏 key 关联规则，未设计。

> 这六条是**方案的前置未知**，不是可以跳过的细节。尤其 5、6 若不处理，裁剪 master 会直接把流弄坏。

---

## 八、请裁决

1. 走**路线 1+2 组合**（裁剪 master + 重开），还是只做**路线 1**（启动选档，不做运行时 ABR）？
2. 若走组合，先做 **310-A**（能起播结构 B 的流）——这是所有后续的前提，且它本身就有独立价值
   （现状：**Apple/Emby 风格的 fMP4 多码率流，我们只能播到第一档，且是 960x540 这种低档**）。
3. 第七节六条前置未知，哪几条必须先补测再动手？
