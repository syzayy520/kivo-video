# 网络原生音视频接线 · 当前证据

日期：2026-11-09

## 系统解析读取对照（最新，定位范围）

[K40读取记录](k40-extractor-trace-r1.txt)与[平板读取记录](tablet-extractor-trace-r1.txt)
对照同一真实条目：平板跳到文件尾部并在进程t=7763ms识别3轨；K40逐段读取中部，
在45秒诊断预算到期取消后返回0轨。详见[失败记录](real-server-failures.md)。
这是解封装前后读取行为差异的证据，尚未确定索引/系统解析根因；不是视频解码性能结论。
只读观察器默认关闭、异常隔离，不输出地址/凭据/片名。未修改生产解码策略。
`assembleDebug assembleDebugAndroidTest`通过；观察器关闭时避免诊断字符串构造的最终版
已在两机`install -r`成功，均返回MainActivity。未将重新安装等同真实源音视频通过。

用户建议默认软解。两机合成证据实际音频codec均为`c2.android.aac.decoder`，
已经是系统软件音频解码；真实E-AC3缺失需要接入相应实现，现有Android FFmpeg构建尚未启用该codec。
音频兼容与视频默认策略必须分别验收，不能靠切换一个总开关算播放闭环。

## 真实源读取修复（最新，整体验收仍 DOING）

已修复两个可复现缺陷：单字节 Range 探测被 CDN 误报为 1 字节文件；网络分包短读直接成为文件短读。
设计、边界及家族树见 [real-server-plan.md](real-server-plan.md)，失败与反例见 [real-server-failures.md](real-server-failures.md)。

最终读取实现已覆盖安装两台设备；分包延迟、服务器截短单次响应、小文件尾读、越界响应拒绝、独立描述符随机读与生产宿主操作回归通过：

| 设备 | 操作输出 | 同轮核心证据 | 视频渲染/迟到丢帧 | seek total | 音频 xrun / 补静音帧 |
| --- | --- | --- | --- | --- | --- |
| K40 | [合成回归](k40-range-final-synthetic.txt) | [JSON](k40-range-final-sync.json) | 118 / 0 | 20.281ms | 1 / 4800 |
| 平板 | [合成回归](tablet-range-final-synthetic.txt) | [JSON](tablet-range-final-sync.json) | 123 / 0 | 33.071ms | 0 / 7040 |

这些是含暂停与 seek 的短流程累计量，不是纯稳态采样；补静音均记在 `silence_tail_frames`，不能仅凭该字段断定成因。视频此轮零丢帧，仍不能声称音频零欠载、整体流畅或长期稳定。诊断新增输出轨道 MIME，并检查最终证据文件生成时间，拒绝沿用旧文件。

证据新鲜度断言在K40另轮通过：[输出](k40-range-freshness-synthetic.txt)、[同轮JSON](k40-range-freshness-sync.json)。该轮118渲染/0迟到丢帧、seek 34.626ms、1次xrun/3648补静音帧；与上表分开保存。

真实服务器结果：

- 来源1：两机登录超时，见 `k40-real-source1-r3.txt`、`tablet-real-source1-r2.txt`。
- 来源2第1片：平板完整解析5轨，生产宿主因系统缺少 `audio/eac3` 解码器明确失败，见 [输出](tablet-real-source2-r1.txt)。
- 来源2第2片：平板只识别1轨，生产宿主进入 PLAYING，但不满足测试的音轨闸门；不能记音视频通过，见 [输出](tablet-real-source2-item2.txt)。尚未确认原文件是否真的没有音轨。
- 来源2第3片：平板完整解析 `video/hevc,audio/eac3,audio/eac3`，音频解码仍失败，见 [输出](tablet-real-source2-item3.txt)。
- K40 对第2/3片，头64KiB与直接HTTP逐字节相同，但完整系统解析触发45秒诊断取消，见 `k40-real-source2-item2.txt`、`k40-real-source2-item3.txt`。45秒仅是测试诊断截止时间，不是生产准备超时保障。

下一步按顺序：K40系统解析的读取请求/耗时取证与有界取消 → 音频兼容设计 → 真实来源音视频、稳态与生命周期矩阵。仓库已有 `backend/src/video/audio_plane/decode/ffmpeg/`，需先审计Android构建接入和PCM契约；不能据目录存在就说 E-AC3 已支持，也不得改成静默视频软解。SMB真实共享尚未验证。

## 前序合成接线证据

平板 `CFE6R21B12000179` 与 Redmi K40 `b6e950ee` 都通过了同一条合成 Range 测试：

- [tablet-r1.txt](tablet-r1.txt)
- [k40-r1.txt](k40-r1.txt)
- [tablet-sync.json](tablet-sync.json)
- [k40-sync.json](k40-sync.json)

每轮均通过独立描述符随机读、生产 Compose 宿主启动、硬件视频轨、音轨接入、暂停冻结/恢复推进、前后跳转和退出释放；核心证据包含 `audio.opened=true`、`audio.started=true`、正的 `frames_rendered` 与 `clock_source=audio_timestamp`。

这证明的是网络适配器与现有原生会话在两台设备上的合成数据闭环。它还不等于真实 Emby/SMB 长时间播放、弱网取消、Surface 重建、人耳音画同步或完整流畅性验收；这些仍按后续卡片分别取证。暂停状态下 seek 当前提示先恢复播放，避免原生核心拒绝 paused seek。
