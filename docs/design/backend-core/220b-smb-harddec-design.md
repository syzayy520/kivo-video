# 220b · 硬解过代理 —— 设计稿

> 状态：**设计完成，实现开工**。上一卡（220 v1）实测软解 720p SMB 只有 0.37×
> （解码占 wall 68%），正解 = 让网络源走 MediaCodec 硬解。
> 参照：220 v1 的 LocalRangeProxy 已把远端源变成 http://127.0.0.1 —— 本卡让
> MediaExtractor 直接吃这个 URL，走框架硬解管线（与本地文件完全同构）。

---

## 一、一句话目标

**SMB/远端片源经本地代理走 MediaCodec 硬解 + SurfaceView 零拷贝——720p30 实时（≥1.0×），
本地路径行为不变（灰度共存）。**

## 二、设计决策

### D1 管线：MediaExtractor.setDataSource(http://127.0.0.1:port/kivo)

- MediaExtractor 原生支持 http(s) URL + Range（框架自带 http 数据源）；
- LocalRangeProxy 已实现 Range → 框架侧无感；
- 与本地文件路径唯一差异：数据源从 fd 变 URL——**解封装/解码/渲染全部复用
  213/214 的硬解管线一行不动**。

### D2 会话形态：Kotlin 侧全 Kotlin 管线（不走 C++ 会话）

- 220 v1 的 C++ URL 分支（FfmpegAdapter::open_url）保留（软解兜底）；
- 220b 新增 Kotlin 硬解会话：MediaExtractor + MediaCodec + SurfaceView——
  与 213 播放页的本地硬解管线同构，数据源换 URL；
- **新建 `playback/NetworkPlaybackSession.kt`**（单文件单职责）：封装
  extractor(URL) → 选轨 → MediaCodec configure(surface) → dequeue/渲染循环
  → positionUs/durationUs/seekTo/release；
- 节拍：MediaCodec 直接渲染到 surface（releaseOutputBuffer(render=true)），
  vsync 对齐——不需要 pts sleep（框架管线自带节拍）。

### D3 路由：SMB 分支按分辨率/编解码自动选软硬

- 720p 及以下 + h264 → 硬解（220b 新管线）优先；
- 软解失败（codec 不支持/rm/wmv）→ 回退 220 v1 软解 URL 通道；
- reason 全程出版（G9）。

### D4 seek：MediaExtractor.seekTo + MediaCodec flush

- extractors.seekTo(pos, SEEK_PREVIOUS_SYNC) → codec.flush → 重新起步；
- positionUs = extractor.getSampleTime()。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `playback/NetworkPlaybackSession.kt` | **新** | URL 硬解会话：extractor(URL)→codec→surface 循环 + position/seek/release |
| `ui/compose/PlayerComposeActivity.kt` | 旧（小改） | SMB 分支加分辨率路由（硬解优先→软解回退） |

复用（不动）：LocalRangeProxy、Smb2Native.Source、SmbActivity、PlayerControls。

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 720p30 SMB 硬解实时 | 节拍曲线 ≥0.95×（pace_curve 仪表） |
| AC2 | 画面正确 | SurfaceView 渲染 SMB 片源内容（截图） |
| AC3 | seek | 拖动后位置正确落位 |
| AC4 | 灰度共存 | 本地文件路径回归不变（本地播放照常） |
| AC5 | 起播 | SMB 720p30 起播 ≤2s（含代理） |

## 五、明确不做

rm/wmv 等框架不支持的容器（走 220 v1 软解兜底）· 音频管线（audio track 后续）·
多音轨/字幕轨选择（后续卡）。
