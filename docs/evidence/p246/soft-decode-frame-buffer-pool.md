# 软解卡成 PPT 的根因：每帧重建 8.3 MB 帧缓冲（24x）

日期：2026-11-10
设备：MatePad MRR-W29（kona / SD865 家族，Android 12）
片源：`[恐怖废墟].The.Ruins.2008...VC-1.TrueHD.5.1-CMCT.mkv`（SMB，1080p VC-1 + TrueHD 5.1）

## 用户报的现象

「软解卡成PPT了」。上一轮交付的结论是「VC-1 单线程解码 88 ms/帧，只能 0.31x，
无法修复」——**这个结论是错的**。88 ms 里 69 ms 与 VC-1 无关。

## 真机端到端数据（App 内，不是会话级 instrument）

走 `PlayerComposeActivity` 真实播放，播放页每 2 秒打一条 `soft rate`：

| 阶段 | fps | 每帧 | ratio |
|---|---|---|---|
| 修复前 | 7.5 | 133.1 ms | 0.25 ~ 0.35 |
| 帧缓冲池化后 | **23.6** | **42.4 ms** | **0.95 ~ 1.01** |

## 每帧预算（`KivoSoftDecode: frame_budget`）

```
修复前 per=133.1ms | send=8.3 recv=0.0 sws=1.5 resize=69.3 read=19.0 render=4.8
修复后 per= 42.4ms | send=16.2 recv=0.0 sws=2.9 resize= 0.1 read=15.7 render=3.2
```

（修复后 send 从 8.3 涨到 16.2，是因为此时跑的是**满画质**版本——实验期间的
`skip_loop_filter = AVDISCARD_ALL` 已撤掉，VC-1 环路滤波恢复。跳过滤波只省 3 ms，
不值得拿画质换。）

## 根因

`FfmpegAdapter::receive_frame_rgba` 每帧执行：

```cpp
result.frame_data.resize(1920 * 1080 * 4);   // 8,294,400 字节
```

而 `result` 是每次 `decode_video_rgba()` 新建的对象，`frame_data` 每帧都从 size 0 开始，
于是**每帧**走一次 8.3 MB 的分配 + `vector::resize` 的零初始化（memset）。
8 MB 级 malloc 在 Android 上走 mmap，2000+ 页首次写入触发缺页中断 → **69 ms/帧**，
比真正的解码（16.2 ms）贵 4 倍。

`sws_scale` 只 2.9 ms 说明 YUV→RGBA 本身很快（NEON），慢的不是转换，是**为转换准备场地**。

## 修法（帧缓冲池）

- `FfmpegAdapter` 增加线程内缓冲池 `thread_local std::vector<uint8_t> g_frame_buffer_pool`
- `receive_frame_rgba`：尺寸命中则 `swap` 取现成块（O(1)，零分配零清零；
  `sws_scale` 会写满整块，残留内容不可见），否则才 `resize`
- 新增 `FfmpegAdapter::recycle_frame_buffer(std::vector<uint8_t>&&)`
- 会话在 `sink_->render()` **之后**归还：
  `recycle_frame_buffer(std::move(result.frame.frame_data))`
  （渲染是 `ANativeWindow_lock → 逐行 memcpy → unlock`，同步消费，归还安全）

## 同时做的两件事

1. **音频改用独立的 `SmbByteSource` / `RangeFd` / fd**。`SmbByteSource` 只有一把锁和
   一个 1 MiB 预读窗口，音频（64 KiB 粒度）与视频（按帧）偏移相差几 MB，共用会让
   窗口互相顶掉、每次读退化成一整次 1 MiB 往返。实测这一项**没有**改变 ratio
   （0.31 → 0.31），说明它不是主因，但结构上必须分开。
2. **播放页加端到端速率日志** `soft rate media=… wall=… ratio=…`。会话内部的
   `decode_ms/wall_us` 量的是解码线程自己，量不到用户观感。

## 回归

`SoftDecodeSmbInstrumentation` **15/15 PASS**（视频 8 + 音频 5 + 速率 2），
证据里 `frames_decoded=490`、`frames_rendered=490`、`render_failures=0`、
`send_ms_per_call=18.5`、`sws_ms_per_frame=2.6`。

## 仍未处理

- pacing 仍是 `pts_sleep_simple`，2 秒窗口 ratio 在 0.82 ~ 1.44 之间摆动，
  会有轻微忽快忽慢；需要更平滑的 pacer 或读包预取线程。
- `read=15~20 ms/帧` 仍是第二大开销，可考虑解码线程与 IO 线程分离。
