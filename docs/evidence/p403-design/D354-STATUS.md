# 354 · 播放输入通道策略化（本地文件直连 mpv）— 状态

> 设计：`docs/design/playback/354-input-channel-strategy.md`
> 提交：（见 git log `354:`）
> 设备：K40 `b6e950ee`（A13）

---

## 一、做了什么

本地文件（`kivo.media` 入口）不再经过 loopback HTTP 代理，直接 `loadfile <绝对路径>`。
SMB / WebDAV / HTTP / 预览内核**保持走代理**（它们需要协议转换，这是代理存在的理由）。

| # | 文件 | 动作 | 行数 |
|---|---|---|---|
| 1 | `playback/PlaybackInputMode.kt` | 新建 · 通道决策唯一入口（`direct` / `proxy` / `auto`） | 103 |
| 2 | `playback/DirectFileMedia.kt` | 新建 · 路径规范化 + 预检 | 84 |
| 3 | `playback/MpvInputPreparation.kt` | 新建 · 决策→预检→资源创建，收敛到一处 | 88 |
| 4 | `net/LazyFileByteSource.kt` | 新建 · 回退代理时**按需**打开（直连路径零 fd） | 33 |
| 5 | `playback/MpvStartSequence.kt` | 改 · 输入准备外移，`Prepared` 的 proxy/id 可空 | 356（存量） |
| 6 | `playback/MpvPlaybackSession.kt` | 改 · `input` 可空 + `directPath`；自建源自己关 | 409（存量） |
| 7 | `ui/compose/PlayerLocalStart.kt` | 改 · 不再 `new FileByteSource` | 93 |
| 8 | `ui/compose/PlayerSmbStart.kt` | 改 · `source` 可空 + `localPath` | 294 |
| 9 | `cpp/mpv/mpv_sub_options.cpp` | 改 · `sub-auto` `fuzzy` → `exact` | — |
| 10 | `ui/compose/PlayerSiblingSubtitle.kt` | **删除** · 被内核原生 `sub-auto` 取代 | −45 |

门禁：`ui_gate` **PASS（无回归）**；`assembleDebug` **BUILD SUCCESSFUL**。

---

## 二、验证结果

### V2 三种容器（`d354-local-matrix.txt`）

| 容器 | 决策 | 内核识别 | loadfile 实际参数 |
|---|---|---|---|
| `.mp4` | `mode=direct` | `mov,mp4,m4a,3gp,3g2,mj2` | `/storage/emulated/0/Movies/s.mp4` |
| `.mkv` | `mode=direct` | `Matroska` | `/storage/emulated/0/Movies/s.mkv` |
| `.ts` | `mode=direct` | `mpegts` | `/storage/emulated/0/Movies/s.ts` |
| **中文+空格+`%`** | `mode=direct` | `mov,mp4,...` | `/storage/emulated/0/Movies/测试 视频 100%.mp4` |

最后一行是本批最重要的边界验证：`loadfile` 是独立 argv、走文件名字符分支，
`%` / 空格 / 中文都不需要转义（见 `DirectFileMedia` 文件头）。

### V4 同名字幕（`d354-sub-same.png`）

内核日志：

```
[find_files] Loading external files in /storage/emulated/0/Movies/
● Subs  --sid=1  's.srt' (subrip) [external]
```

同目录同时存在 `other.srt`（不同名）⇒ **没有被挂上**（`sub-auto=exact` 生效）。
画面底部显示「354 同名字幕自动挂载 OK」。

⇒ 此前为绕过代理写的 `PlayerSiblingSubtitle`（只认 4 种扩展、无编码探测、无字体附件）
已删除，功能由内核接管且更强。

### V5 seek（`d354-seek-log.txt`）

```
recordSeek req=109889ms land=109889ms t=164ms ok
recordSeek req=107591ms land=107634ms t=100ms ok
recordSeek req=103336ms land=103336ms t=107ms ok
```

落点误差 **0 / 43 / 0 ms**（判据 ≤200 ms），耗时 100–164 ms，seek 后正常出画。

### V7 开关四态

| 开关值 | 决策日志 | loadfile 参数 |
|---|---|---|
| `debug.kivo.inputMode=proxy` | `mode=proxy（开关强制 proxy）` | `http://127.0.0.1:40099/1?...` |
| `=direct` | `mode=direct（开关强制 direct）` | `/storage/.../s.mkv` |
| `=nonsense` | `mode=direct（开关值不可识别（nonsense）⇒ 按默认）` | `/storage/.../s.mkv` |
| `=auto` / 不设 | `mode=direct（本地文件默认直连）` | `/storage/.../s.mkv` |

### V8 SMB 不回归（`d354-smb.png`）

SMB 播 38.4 GB REMUX：`mode=proxy（无本地路径（SMB/HTTP 源））`，正常出画。

### V1 首帧时间 —— **结论是"没有变快"**

同文件（421 MB mp4）、同构建、`direct`/`proxy` 交替，共 13 次 direct / 8 次 proxy：

| 分段 | direct | proxy | 差 |
|---|---:|---:|---|
| `kernel_created` | 38–58 ms | 37–39 ms | 预检成本，最终收敛到 **+3 ms** |
| `loadfile→首帧` | 378–414 ms | 416–423 ms | **direct 快 20–43 ms** |
| `startup_total` 中位 | 453–471 ms | 461–464 ms | **差异在噪声内** |

**如实结论：首帧总时间基本持平。** 直连确实把 `loadfile→首帧` 这一段稳定缩短了
20–43 ms（少一次 socket 往返 + 一次全量拷贝），但预检与调度抖动把它吃掉了。

⇒ **本批的价值不是性能，是功能与可靠性**：
同名字幕恢复、格式识别不再受响应头支配（D12I 那类缺陷对本地彻底消失）、
少一层可能出错的组件。

---

## 三、过程中我自己造的三处错误（都已修）

1. **开关静默失效**：`LOOPBACK` 常量写成 `"loopback"`，而文档与 `setprop` 用的是 `proxy`
   ⇒ `when` 不匹配、落到默认，**强制回退根本没生效**。真机 `setprop debug.kivo.inputMode proxy`
   后日志仍是 `mode=direct` 才发现。修法：常量值改为 `proxy`，并保留 `loopback` 别名，
   未知值显式报「不可识别」而不是静默按默认。
2. **预检成本吃掉收益**：首版读 64 KiB 做实读验证，`kernel_created` 从 38 ms 涨到 52–83 ms。
   降到 4 KiB 仍多 20 ms。最终**去掉实读**，只保留一次 `length()`——
   "权限位可读但读不出"的场景 mpv 会给明确错误（不是黑屏），不值 20 ms 首帧。
3. **验证脚本误判**：矩阵脚本用 `[a-z0-9,]+` 匹配格式名，漏掉了大写的 `Matroska`，
   一度报 `.mkv` "未识别"。**是脚本 bug，不是产品 bug** —— 查内核日志原文才发现。

---

## 四、未覆盖（如实标注）

| 项 | 状态 |
|---|---|
| Emby 入口回归 | 未单独复测。它走 `kivo.playurl`，`hasLocalPath=false` ⇒ 与 SMB 同一条 `mode=proxy` 分支（已由 SMB 实证该分支不变） |
| 投屏 | 未复测。已核实 `CastController.castSource` **自己 new `FileByteSource`** + 独立 `LocalRangeProxy`（绑 `0.0.0.0`），不依赖播放代理 |
| 私密区 | 未截图验证。页面 `FLAG_SECURE`，screencap 无输出；它同样走 `kivo.media` ⇒ 与普通本地同一路径 |
| 直连失败回退 | 只验证了开关强制回退（V7），**没有构造"文件存在但 mpv 打不开"的真实场景** |
| 首帧统计强度 | 每组 3–5 次，样本量不足以给出显著结论（所以只写"基本持平"） |
| `demuxer-cache-time` 语义 | 本地无网络缓存，该值含义与网络源不同，345 契约未加注 |
