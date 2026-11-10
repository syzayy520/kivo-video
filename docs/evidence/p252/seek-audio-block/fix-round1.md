# 修复第一轮：音频 seek 不再阻塞（落点已正确，位置还没跳）

日期：2026-11-10 深夜

## 一、改了什么

`FfmpegDemuxSource::seek_to`（backend/src/video/video_plane/backends/ffmpeg/demux/ffmpeg_demux_source.cxx）：

1. **音轨一律用文件级索引定位**：`av_seek_frame(ic, -1, ts, BACKWARD)`，
   让 FFmpeg 用默认流（视频轨）的 cues，而不是按音轨自己的索引。
   依据：MKV 的 cues 是文件级的 —— 实测视频轨 653 条索引，音轨的 `index_entries` 没被填上；
   按音轨定位时 FFmpeg 退化成在整个 12 GB 文件里二分。
2. **定位后"读到本轨的包为止"的循环加上界**（512 包）：扫不到就换文件级索引再试一次，
   还不行就**快速失败**，不再扫十几分钟。
3. 本轨没有索引条目时（`nb_index_entries == 0`）同样退回文件级索引。

## 二、效果（真机，Shelter 12.19 GB，跳 600 s）

```
修复前：ams=-1（音频 seek >20 s 不返回）→ 整个 native seek 永不返回
        → Kotlin 结果永不写 → 界面停在起播位置

修复后：ams=2      音频 seek 2 ms
        astep=8    8 个步骤全部完成
        ams_ms=0,1,0,0,0,0,1,0
        vms=237    视频 seek 237 ms
        landing=594469000  落点 594.469 s（与 ffprobe 一致）
```

**阻塞解除，落点正确。**

## 三、还没解决的：位置没有跳过去

```
SEEK target=600000 ms landing=594469 ms actual=2341 ms
```

落点是对的（594.469 s），但播放位置仍在 2.3 s —— **管线恢复后仍在旧位置播**。
下一步要查的是：seek 之后是谁把视频的读取位置带回了开头
（候选：音频侧 `prefill()` 只用了 1 ms，说明环里可能还是旧样本；
音频钟被 rebase 到 594 s，但样本还在旧位置 → 以音频钟为准的位置就还是 2.3 s）。

## 四、回归

- `local-seek.mkv`（60 s，H.264）seek **4/4 PASS**（落点 27306 ms）
- 诊断设施全部保留在 summary 通道里（demux 事实 / 会话计数 / JNI 计数 /
  快照落点 / 句柄落点 / 视频与音频分段耗时 / 音频各步耗时 / native 错误）
