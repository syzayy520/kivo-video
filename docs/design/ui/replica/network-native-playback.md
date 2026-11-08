# 网络播放接入同一核心 · 2026-11-09

目标：Emby URL 与现有 SMB 回环 Range 代理进入现有 PlaybackSession；删除网络专用 Java 视频循环，共用原生硬解、AAudio、音频主时钟、暂停、seek 和 Surface 重挂。不是重写核心。

## 数据源决策与家族树（实现前门闸）

优先验证 Android API26 StorageManager.openProxyFileDescriptor：HTTP Range 随机读包装为可定位 fd，复用已存在的 JNI fd 入口。比新增两套 URL extractor 打开路径改动少；若真机不支持则明确失败，不下载整片、不启用软解。该方案替代 playback-audio-connection.md 中尚未实施的 NDK URL 扩展候选。

- net/HttpRangeFile.kt：HTTP(S) 大小/Range 校验、每个 fd 独立读取流与回调线程、有界流式读取、超时/取消；不认识媒体轨道。只接受206及正确Content-Range，拒绝服务器忽略Range、长度变化、提前EOF，不输出URL/token。
- bridge/PlaybackSession.kt：增加可选描述符提供方和明确长度；默认本地行为不变，仍 native 释放之后才关两个独立 fd。
- playback/NetworkPlaybackSession.kt：替换为现有核心的适配器；单工作队列负责准备、暂停、seek、重挂、停止与证据。UI只读volatile快照。准备不占主线程，取消使迟到结果不可回写，关闭HTTP再停native解除读等待，最后释放描述符/回调线程。
- ui/compose/PlayerComposeActivity.kt：异步网络启动与SMB准备，控制和Surface回调接适配器；Activity拥有网络会话，离场取消并停止，暂不交旧Service双重持有。
- androidTest/.../playback/NetworkAudioConnectionInstrumentation.kt：合成Range服务→生产Compose入口；独立验证Range字节、拒绝错误响应、音频输出、音频时钟、暂停位置、前后seek、面恢复与取消。合成素材不等于真实Emby/SMB验收。
- docs/evidence/p240/network-native-playback/：原始真机输出与有界结论；真实服务器URL/账号/片单不入库。

## 生命周期与闸门

状态 preparing→playing/paused→ended/failed→closed。准备、控制、native释放同一队列；close可从主线程调用且不join，不在native仍读取时关fd。取消先设置原子标志、断开HTTP连接，工作队列最终close核心，再close数据源。挂面只调用既有rebind，null保持原生本地摘面语义，不修改setOutputSurface策略。准备期间失去Surface则取消此次启动，不能对废弃面继续建码。

先验证代理fd跨JNI有效、音频opened/started/output与audio_timestamp，再验暂停/seek/退出、错误与取消、Surface恢复。真实Emby/SMB持续播放、人耳与音画另行验收。任何故障必须用户可见；不把提交时刻当实际音画同步，不把零xrun等同流畅。
