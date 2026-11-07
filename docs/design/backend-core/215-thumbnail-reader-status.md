# 215 · 进度条缩略图 —— 状态稿（收口）

> 状态：**收口 2026-11-07，M1 最后一张卡，6/6 全绿**。
> 判据原文：1080p 片源拖动缩略图首现 ≤500 ms；主链路丢帧率不回退；LRU 淘汰。
> 证据：`docs/evidence/p215/`（3 JSON + 1 截图）· 校验器 `tools/verify_p215_thumbnail.py`（PASS + 8/8 变异打红）。

## 一、验收判据表

| # | 判据 | 结果 | 数字 |
| --- | --- | --- | --- |
| AC1 | 首现 ≤500ms（冷启动含解码器 init） | **PASS** | 冷启动 **71 ms**（预算的 14%）；三轮 73/75/71 可复算一致 |
| AC1 | warm 拖动流延迟 | **PASS** | p50 41 / p95 48 / max 50 ms |
| AC2 | 主链路零干扰 | **PASS** | 60 点扫描（59 张 1080p 解码）期间 `frames_dropped_late 0→0`、`pace.dropped 0→0`；收到=放出=渲染（64→562，delta 498 全部上屏） |
| AC3 | LRU 淘汰 | **PASS** | 61 请求（60 个不同秒级位置 + 1 复用探针）→ 淘汰 9 次、cache_size_final = 50 = 上限 |

- 片源：`p13af_1080p30_60s_av.mp4`（h264 1080p30 60s，设计判据的 1080p）· 设备：K40 `b6e950ee`
- 视觉证据：`p215-drag-k40.png` —— 浮窗（白描边圆角卡）正中 thumb 上方，内容 = 30s 处真实解码帧，时间显示拖点 0:00:30 / 0:01:00

## 二、实现要点（对照设计稿 D1–D4）

- D1 独立实例：独立 `MediaExtractor`+`MediaCodec`，与主会话零共享 ✓
- D2 LRU：秒级键、容量 50、`LruCache` 淘汰计数 ✓（缓存位图缩到 320px 宽 ≈230KB/张，50 张 ≈11MB）
- D3 懒初始化：首次 `seekTo` 才开解码器（AC1 冷启动数字即含 init）✓；5s 无请求自动释放 ✓
- D4 HandlerThread 串行 ✓；回调节发回主线程更新 Compose 状态 ✓
- 控制条接线：拖动中只更新本地值 + 请求缩略图（`onScrub`），**松手才提交 seek**——修掉旧实现逐 tick 打主链路的干扰源
- 取证 harness：`kivo.evidence=p215` → 2s 后 60 点自动扫描（250ms 间距）+ 基线/收尾 `writeStats` 对照 + cache-hit 探针 + 30s 预览演示窗口

## 三、崩溃根因（真机实证，留档）

第一轮取证进程死亡，dropbox 栈：

```
android.media.MediaCodec$CodecException: Error 0xffffffde
    at MediaCodec.releaseOutputBuffer
    at ThumbnailReader.decodeFrameAt          ← seek 后不 flush 直接灌新位置采样
```

**根因**：换位置 seek 后不 `flush()`，codec 的输出缓冲被作废，release 旧索引即炸。
**修复**：`decodeFrameAtInner` 每次 `ex.seekTo` 后先 `mc.flush()`；并整体 try/catch
（CodecException/IllegalState → 记 `decode_failures`，不再杀进程）。

## 四、顺带修掉的坑（都在 215 家族树文件内）

1. **Image 未 close** → codec 输出缓冲饿死（60 点扫描必踩）；现在 `getOutputImage` 后 `img.close()`
2. **全分辨率解码缓存爆炸** → 2×2 半采样 + 缩 320px（8MB/张 → 230KB/张）
3. **bitmap recycle 崩渲染** → LRU 淘汰不再手动 recycle（UI 可能仍持有）
4. **KEY_DURATION 缺键崩溃** → `containsKey` 守卫；MediaCodec create/configure 全部兜异常
5. **EOS 后 position 外推无上界**（真机 205s/60s 片）→ positionPoll clamp 到时长
6. **控制条时长恒 0:00:00**（213 遗留）→ 复用 harness 的 `probeDurationUs` 一次性探测
7. 拖动中 3s 自动隐藏会打断长拖 → `onScrub` 刷新 lastTouchMs

## 五、诚实清单

- **decode_failures = 1**：60 点扫描的末点（59.5s，片长 60s）落在 EOS 边界。判据允许 ≤1，点不阻塞。
- **时长来源**：Kotlin 侧一次性 extractor 探测（`probeDurationUs`），非 native 会话字段——与容器一致，但严格说与 `source.duration_us` 是两处来源。M2 若接更多容器格式，桥上补正式导出。
- **平板取证未做**：平板 USB 闪断（CFE6R21B12000179 中途掉线），K40 为 M1 证据机；平板补跑属可选增强。
- **取景误差**：浮窗居中偏移用 `BoxWithConstraints` 按轨宽计算，视觉验证通过；未做逐像素对照 Rex。
- **previewMs 参数**：取证 harness 的程序化预览走与拖动**同一条**浮窗 UI（非旁路），顺带成为未来"外部请求预览"的入口。

## 六、M1 出口状态

M1 六卡全收口（210-215）。出口验收待办：全部门禁跑一遍 + 新旧 `PlayerActivity` 并存灰度确认。
