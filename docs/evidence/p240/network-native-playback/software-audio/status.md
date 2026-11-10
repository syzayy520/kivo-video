# 网络音视频阶段交接（2026-11-09）

用户已将播放问题交给另一位助手跟进，本助手转做播放页 UI。**手机真实网络影片未通过，整卡仍为 DOING。** 保留所有工作区实现与设备数据，禁止卸载或清数据。

## 已验证的范围

- 平板真实来源 2、第 3 项 MKV（7,369,631,541 字节）：HEVC 硬解、FFmpeg EAC3 音频、AAudio 主时钟；暂停恢复、前后 seek 后 20 秒稳态通过。`tablet-real-r16c.txt`、`tablet-real-r16c-sync.json` 及同名前缀两个 verify 文件。外部速率 0.9974525，视频迟到丢帧 0、xrun 0；会话累计仍有 2,592 音频丢弃帧、120,704 tail 静音帧，不能声称全流程零补静音。
- 手机 EAC3 合成网络 MP4：操作、自然 EOS 与两套校验通过。`k40-operations-eac3-r16*`、`k40-eos-eac3-r16*`。**这些不能代替手机真实 MKV 验收。**
- r16 已覆盖安装手机 `b6e950ee` 和平板 `CFE6R21B12000179`，签名未变。所有本轮生产与测试改动仍在工作区，未提交；不要 reset 或只取几个文件遗漏依赖。

## 手机阻塞与用户现场

用户随意点选影片后停在“正在准备网络播放…”，时间为 0。截图：`k40-user-playback-r17.png`。现场应用 PID 曾为 11212；该 PID 会变化。当前条目身份未确认，不应直接认定与以下探针同一片。

已知真实 MKV 同源探针对照：`../k40-extractor-trace-r1.txt`、`../tablet-extractor-trace-r1.txt`。K40 系统 MediaExtractor 沿 cluster 扫描中部，45 秒诊断取消后返回 0 轨；平板约 7.7 秒识别三轨。头 64KiB 与直接 HTTP 逐字节相同。解码器尚未打开，不能归因为视频硬解性能。索引、平台解析器或代理 fd 的根因仍未确认。

`NetworkPlaybackSession.start()` 先 Java MediaExtractor 探测，然后 native video/audio 各自打开 extractor。生产准备**没有总体截止时间**，单请求 8 秒超时不能限制持续扫描。补错误提示或超时不能算解决播放。

建议先诊断手机实际失败条目，再设计解析修复；若替换 demux，Java probe 也要一起处理。FFmpeg Android SDK 已有 avformat，但 custom AVIO、csd/NAL、seek、取消和加密语义均需设计，不能转到旧软件视频链路静默兜底。native 仍由唯一 worker 串行拥有；取消网络 → native stop/destroy → 关闭 fd，Surface 生命周期不得提前结束。

## 复现

工作目录 `C:/kivo video`，adb 为 `C:/Android/sdk/platform-tools/adb.exe`。

平板真实同源操作与稳态：

```powershell
adb -s CFE6R21B12000179 shell am instrument -w -e realSource 1 -e realItem 2 -e realPreflight false -e audioCodec ffmpeg:eac3 com.kivo.cinema.test/com.kivo.cinema.playback.NetworkAudioConnectionInstrumentation
```

将 serial 改为 `b6e950ee` 可复测手机，但应先保留用户现场。应用私有报告 `files/evidence/network-native-sync.json` 会被新运行覆盖，必须核对生成时间并另名拉取，不能沿用旧报告。校验使用 `tools/verify_p12c_sync.py`、`tools/verify_p13d_drift.py`。来源账号保存在设备中，不将 URL/token 写入日志或仓库。

未验：手机真实 MKV 起播、SMB、AAC/多声道矩阵、真实源生命周期和更长稳态。UI 工作只改 Compose 展示层，不接管这些播放职责。
