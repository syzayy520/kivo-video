# v3.1d · SMB 拖动预览帧：出帧正确性 + 真实延迟（2026-11-12，K40 b6e950ee）

片源：`视频/kivo-test/shelter.mkv`（12GB 无 Cues MKV，SMB）。内核：mpv 第二实例（vo=libmpv 离屏 FBO）。

## 一、本轮修掉的三个真机缺陷

1. **精确落位前向解码（4.6s/帧主因）**
   native `seek(exact=false)` 发 `absolute`，而 mpv `hr-seek` 默认=absolute ⇒ 仍 hr-seek
   （日志 `hr-seek, skipping to 90`），从关键帧前向解码到目标。
   修：预览实例 `hr-seek=no` ⇒ 真关键帧 seek（日志 `execute seek ... flags 0`）。
2. **音频/字幕占满 demux 队列阻塞视频**
   日志 `audio/1: 9434 packets, 8389424 bytes` + `Too many packets` ⇒ `video=eof`。
   修：预览实例 `aid=no`、`sid=no` ⇒ 队列全留视频（日志 `audio=eof`）。
3. **off-by-one 级联：浮窗画面≠手指位置（最隐蔽）**
   仅靠 capture 序号去重，连续快 seek 时 mpv 合并/滞后（42 queued / 36 executed），
   每次 grab 读到**上一次** seek 的帧 ⇒ 证据曾假报 `warm_p50=24ms`。
   修：capture 每帧记录渲染时 `time-pos`（`frame_pts`），grab 只接受
   `time-pos` 已离开旧位置的帧 ⇒ 斩断级联。修后 `land≈t`（见 grabs）。

另：真黑场（片头黑底）整幅同色，`hasContent` 恒 false；留存同色帧 + 2s 兜底接受，
不再 6s 超时（`readCaptureFrame` 按序号去重，黑场帧只出现一次，必须留存而非再读）。

## 二、修后真实数字（60 点拖动扫描，250ms 间距，latest-wins）

- `requests=31 / frames_acquired=32`（29 个被超越的请求正确丢弃）
- `warm_p50=648ms / p95=664ms / max=877ms`；`cold_first=1574ms`
- grabs 逐条 `land≈t`（关键帧粒度 ≤0.6s）⇒ **画面=手指位置**
- 0 decode_failures；缓存命中通道瞬时（per-second LRU）

**口径**：~0.5s 是 SMB 无索引冷 seek 的物理下限（demux 字节插值+读簇头，多次 SMB 往返）。
连续快拖时浮窗以 ~2fps 更新并在松手后 ~0.5s 内对齐；回拖已缓存秒瞬时。
"瞬时跟手"需预生成缩略图条（sprite/BIF），属后续卡，**不在本轮静默假装达成**。

## 三、证据

- `v31-thumb-v31d-scan.json`：扫描分布 + grabs（land≈t）
- `mpv-thumb-v31d.log`：`hr-seek=no / aid=no / sid=no`、`flags 0`、`audio=eof`
- `preview-v31d-t300-loaded.png`：浮窗=t300 真实正立帧（主画面在 00:04，二者不同 ⇒ 跟手）
- `preview-v31d-loading.png`：冷 seek 期间诚实"预览帧准备中…"（无假帧）
- `thumb-v31d-frame40.png`：离屏读回真实正立帧（方向正确）

## 四、改动文件

- `thumbnail/MpvThumbnailReader.kt`：hr-seek/aid/sid、pts 门控、黑场兜底
- `cpp/mpv/mpv_gl_capture.{hpp,cpp}`：frame_pts 记录与读取
- `cpp/mpv/mpv_session.{hpp,cpp}`、`jni_mpv_bridge.cpp`、`mpv/MpvNative.kt`：captureFramePts 通道
- `ui/compose/PlayerProbeHooks.kt`、`PlayerComposeActivity.kt`：`kivo.thumbScan N` 网络扫描探针