# K40 真实 MKV 起播 · FFmpeg demux（2026-11-09 16:50）

## 结论

**K40 上真实网络 MKV 已能起播，硬件 HEVC 解码器已打开。**
这是从"45 秒线性扫描返回 0 轨"到"起播"的状态变化，不是推断。

```
CHECK production has_audio=false video_codec=OMX.qcom.video.decoder.hevc
FAIL stage=tracks type=IllegalStateException state=PLAYING
```

读法：

- `state=PLAYING` —— 会话真的进入了播放态；
- `video_codec=OMX.qcom.video.decoder.hevc` —— 平台硬解已打开并配置成功；
- `has_audio=false` —— **当前实现缺口**，见下节，不是实际片源没有音轨；
- `FAIL stage=tracks` —— 测试断言要求"轨道已就绪且含音频"，因为 `has_audio=false` 而失败。
  这是**断言失败，不是播放失败**，不能据此说播放不可用。

## 改了什么

| 层 | 改动 |
| --- | --- |
| 解封装 | 新增 `demux/demux_source` 接口；MKV/WebM（EBML 魔数 `1A45DFA3`）走 libavformat，其余仍走平台 `AMediaExtractor` |
| 门面 | `MediaExtractorSource` 保留类名与公开 API，内部按容器选择实现，调用点零改动 |
| 网络 | 未改。FFmpeg 通过 `AvioBridge` 复用同一个 `HttpRangeFile` 代理 fd，Range 校验、共享缓存、取消语义不变 |
| Kotlin | `probe()` 遇 MKV 不再用平台 extractor 预判（它必然报 0 轨），交给 native 路径 |

读取实测（同一片源，reader=2 顺序读 128 KiB）：

```
read_end offset=1916928 bytes=131072 network_bytes=131072 elapsed_ms=13
read_end offset=2048000 bytes=131072 network_bytes=131072 elapsed_ms=15
read_end offset=2179072 bytes=131072 network_bytes=131072 elapsed_ms=16
```

对比改动前的容器解析路径：每次小读重开 1 MiB 响应、**1500–2700 ms/次**。
现在是 13–20 ms，差距不是调参，是"读什么"变了。

## 当前缺口（不能算完成）

1. **`has_audio=false`** —— `probe()` 对 MKV 直接返回，没有建立音轨事实。
   - 影响：E-AC3 音轨不会打开，播放无声；
   - 不能接受的修法：把 `hasAudio` 写死为 true（无音轨片源会被误判失败），
     或在 `openAudio` 失败时静默继续（违反不静默降级）；
   - 正解：native probe 返回轨道事实，Java 只做展示映射。
2. 首帧实际出现时间未测（本轮只到 `state=PLAYING`，没有截图与呈现时刻）。
3. 暂停/恢复/seek/长稳全部未测。
4. 平板未复测（当前不在线），MKV 路径对平板是新增，需确认不退化。

## 复现

```bash
adb -s b6e950ee logcat -G 4M
adb -s b6e950ee shell am instrument -w -e realSource 1 -e realItem 2 -e realPreflight false \
  com.kivo.cinema.test/com.kivo.cinema.playback.NetworkAudioConnectionInstrumentation
```

带 `realSource`/`realItem` 时测试会主动隐去片源细节（`details withheld for real-source privacy`），
日志中不出现地址、账号或片名。

## 边界

- 本次只证明"能打开容器、能配置硬解、能进入播放态"。
- **不证明**画面已正确上屏、不证明音画同步、不证明可持续播放。
- `has_audio=false` 是有意暴露的缺口，不是可以忽略的日志噪音。
