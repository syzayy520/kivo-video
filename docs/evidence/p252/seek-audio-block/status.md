# 远距离 seek 失效的真正根因：音频 seek 阻塞（已定位，待修）

日期：2026-11-10 深夜
设备：K40 `b6e950ee`；片源 `Shelter`（12.19 GB，2×TrueHD + PGS）；目标 seek 600 s

## 一、最终结论（全部实测）

```
vms=235    视频 seek：235 ms，落点 594.469 s —— 正确
ams=-1     音频 seek：>20 s 未返回 —— 卡住
seekms=-1  整个 native seek：永不返回
```

**因果链：**

1. 视频侧完全正常：`av_seek_frame` 落到 594.469 s（与 ffprobe 同一文件的结果一致），
   avio 位置从 0 跳到 1.275 GB；
2. 随后 native 调用去 seek 音频（TrueHD 走 FFmpeg 软音频会话）→ **这一步阻塞**；
3. 于是 `nativeSessionSeek` 永不返回 → Kotlin `command{}` 里的 `native.seekTo()` 一直挂着 →
   `lastSeekResult` 永远不写；
4. 界面拿不到落点 → 位置停在起播处 → 用户看到"拖了进度条，画面从头开始"，
   **而且没有任何错误提示**。

## 二、之前几轮为什么"看起来"是别的问题

| 现象 | 真实原因 |
| --- | --- |
`landing=0` | 落点其实是 **-1**（没写），测试打印 `landing / 1000` 毫秒 → 整数除法显示成 0 |
`error` 为空 | 测试的 20 秒轮询**没等到结果**（不是失败），错误串自然没被填 |
native 侧 `last=-1` | 赋值在函数末尾，而末尾**根本没走到**（卡在音频 seek） |
snapshot 两秒后有值 | demux 与会话确实知道 594.469 s，只是没人把它交给上层 |

## 三、音频 seek 的内部结构（`AudioPlayback::seek_to`）

```
prepare_seek → feeder.stop → source.seek_to → decoder.flush → ring.reset
→ feeder.start → prefill() → clock.rebase
```

阻塞点在这条链的任意一步；当前证据只能定位到"音频 seek 这一整段"。
下一步要分的是 `source.seek_to`（FFmpeg 对**音频轨**的 seek —— 视频轨有 653 条索引，
音频轨未必有）还是 `prefill()`（从 SMB 灌到起播水位，TrueHD 解码慢就慢）。

## 四、顺带确认的其它事实

- `Shelter` 的**视频轨**索引有 653 条、长度 12 186 004 520 正确、demux 后端是 ffmpeg；
- 同一文件 ffprobe 跳到 600 s 只用 0.294 s、落点 594.469 s（与我们的 demux 落点一致）；
- 硬解关预读时 4K 与 Shelter 都是 0 丢帧、1.00x；
- **这台设备的硬解 HEVC 会进坏状态**（重跑几轮后平台改选 `c2.android.hevc.decoder` 软解，
  于是"位置不推进"）。这不是代码问题，但会污染测量 —— 每轮测量前先确认 `codec=OMX.qcom.*`。

## 五、下一步（两件事，顺序有讲究）

1. **先让 seek 不再被音频拖死**：音频 seek 加**时间预算**（例如 3 s）并放到后台线程；
   超时就先按视频落点起跑，音频定位完成后再对表。
   目标：拖进度条**立刻**有反应（判据：native seek 在 1 s 内返回）。
2. **再修音频 seek 本身**：先分清是 `source.seek_to`（音频轨没索引）还是 `prefill()`（灌水慢）。
   若是前者，参照视频轨的做法用同一份索引定位；若是后者，缩小起播水位或改为渐进。

判据（不变）：任意位置 `|actual − target| ≤ 5 s`，且**不出现"画面从头开始"**。
