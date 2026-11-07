# 215 · 进度条缩略图 —— 设计稿

> 状态：**设计完成，实现进行中**。总计划 M1 最后一张卡。
> 卡片原文：独立 `ThumbnailReader`（低分辨率 MediaCodec 实例批量出帧 + LRU 缓存），
> 与主链路完全解耦；验收：1080p 片源拖动时缩略图首现 ≤500 ms；主链路丢帧率不回退。
> 参照：SenPlayer `PreviewReader$Still`（03 拆解 §三）。

---

## 一、一句话目标

**进度条拖动时在拖动位置上方显示视频画面缩略图（≤500 ms 首现），
主播放会话零干扰（独立解码实例），M1 收官。**

## 二、设计决策

### D1 独立解码实例：与主链路完全解耦

独立 `MediaExtractor` + `MediaCodec` 打开同一文件（低分辨率 surface 或 ImageReader
输出）。seek 独立——**主会话不知道缩略图的存在**，主链路零干扰。
2160p 4K 片源解码为 160×90 缩略图（MediaCodec 的 `KEY_MAX_WIDTH/HEIGHT` 或
ImageReader 的 width/height 参数限制输出尺寸）。

### D2 LRU 缓存：LinkedHashMap（accessOrder=true）

- 缓存键：**秒级位置**（`positionUs / 1_000_000`）——同一秒内的 seek 复用同一缩略图；
- 容量 50 张（每张 160×90×4 = 57KB，50 张 = 2.9MB 内存）；
- `LinkedHashMap(accessOrder=true, capacity)` + `removeEldestEntry` 超容量淘汰。

### D3 懒初始化：首次拖动才开解码器

缩略图解码器在**首次拖动**时启动（用户可能从不拖进度条）；
5 秒无拖动请求 → 释放解码器（省资源）。

### D4 后台线程：HandlerThread

缩略图解码在 `HandlerThread("thumbnail")` 上串行执行——
MediaCodec 非线程安全，串行队列天然保证。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `thumbnail/ThumbnailReader.kt` | **新** | 独立 MediaExtractor+MediaCodec 实例，seek→decode→Bitmap，LRU 缓存 |
| `ui/compose/PlayerControls.kt` | 旧（小改） | 拖动时显示缩略图浮窗 |

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 首现 ≤500ms | 拖动开始 → 缩略图位图非空 ≤500ms |
| AC2 | 主链路零干扰 | 主会话丢帧率不回退（前后各 60s 对照） |
| AC3 | LRU 淘汰 | >50 次不同位置 seek → 缓存不无限增长 |
