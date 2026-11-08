# 正式本地播放页音频接线 · 2026-11-09

本轮修复 PlayerComposeActivity 本地路径漏调 openAudio。严格探测音轨，有音轨先打开原生音频再配置节拍；探测或音频打开失败明确显示原因并关闭会话。无音轨显示纯视频提示。不改 JNI/AAudio/Surface，不新增音频时钟。

## 验证事实

- 用户在 K40 旧 PlayerActivity 播放已有合成片时明确回复“有”，为该次核心输出的人耳确认；不是修复版 Compose 或网络的听音验收。
- assembleDebug/assembleDebugAndroidTest 成功，固定签名覆盖安装两设备，未卸载、未清数据。测试 runner 为 com.kivo.cinema.playback.LocalAudioConnectionInstrumentation。
- 平板 MRR-W29 生产 Compose Activity 测试三组通过，见 tablet-local-audio.txt 与原始 tablet-local-audio.json：音轨探测三态、audioAttached、位置推进；audio.opened/started=true、输出146976帧、clock_source=audio_timestamp、error为空、视频89帧/丢帧0、xrun=0。
- 测试约2.5秒后主动 stop，completed=false/end_reason=interrupted 为预期。audio.feed.frames_dropped=384，不能写成全链路零丢弃；其是否由停止截断导致尚未独立验证。av_drift 在 surface_at_time 下不可测，不能据此声称音画同步通过。
- 首轮短测出现约513ms提交间隔，最终重跑最大约9.33ms；只说明短测结果不稳定，不满足持续流畅验收。首轮 JSON 被最终重跑覆盖，前述首轮数值仅为当时工具观察，不具备独立复算原件。
- K40 修复版已覆盖安装，font_scale读回1.0。phone-local-audio.txt 与 phone-local-audio-retry.txt 两次测试均在宿主启动等待超时；未进入播放断言，不算手机通过。直接 am start -W 正式Compose页返回Status:ok/冷启动1126ms，但启动成功不能代替出声/画面/音画验收。当前 crash缓冲仅有旧系统init记录，未看到本次应用崩溃。

## 复现

先把下述合成片推到应用外部 files/media/，分别命名local-audio-connection.mp4和local-video-only.mp4，再运行：

    adb -s <device> shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.playback.LocalAudioConnectionInstrumentation
    adb -s <device> exec-out run-as com.kivo.cinema cat files/evidence/local-audio-connection.json

合成片生成：

    ffmpeg -f lavfi -i testsrc2=size=640x360:rate=30 -f lavfi -i sine=frequency=440:sample_rate=48000 -t 12 -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -ac 2 -movflags +faststart local-audio-connection.mp4
    ffmpeg -i local-audio-connection.mp4 -an -c:v copy local-video-only.mp4

SHA256（本轮实际构建/fixture）：

app-debug.apk: a61e9785206ebcc25805c0e56faabe7cbd2c1477b14925998a03b0312420c5a1
app-debug-androidTest.apk: 763a193f3ca77e90195d251c8105d5a3ddb54af615a67a5b0d0d546dcbab3a64
local-audio-connection.mp4: ab5c31ef2b0e77fc814b42fca68b62063d40266f4011340b2ff2b2bdd5e7c0e6
local-video-only.mp4: c389356d3653810fe360371ca58356473f9805f0834a0c3464f25dc2d4a41eb8

## 未验边界

Emby/SMB NetworkPlaybackSession 仍仅视频；需要URL接同一原生音频/视频核心的独立设计与实现，不能把本轮称网络声音修复。暂停、前后seek、后台Surface恢复、持续流畅及手机真实输入适配仍待验。下一步优先网络完整播放，视觉精修后置。
