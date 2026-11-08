# 正式播放页音频接线 · 2026-11-09

用户优先级为手机适配→流畅影音播放→视觉精修。K40用户在本轮测试片听音后明确回复“有”，证明已存在的核心与该次手机输出可出声；这不是网络音频验收。

## 当前缺口与边界

PlayerComposeActivity本地分支未调用openAudio；NetworkPlaybackSession仅视频且独立用墙钟，暂停无接线、seek跨线程并发、主线程准备、Surface恢复及异常释放待修。现有C++音频核心已有AAudio、音频主时钟和seek能力，不能另造一套Java音频时钟来掩盖未接入。

## 本次实现门闸与家族树

bridge/MediaTrackProbe新增严格Result音轨探测，区分无音轨和探测故障；保留旧探针调用语义以免扩大变更。ui/compose/PlayerComposeActivity本地打开顺序为openVideo→严格音轨探测→有轨时openAudio→configurePacing→configurePipeline→start。探测/有轨打开失败显示原因并终止，不静默静音；确实无音轨则画面可播且明确提示。失败页保留返回能力，不直接finish隐藏错误。不改JNI、AAudio或Surface行为。

androidTest/.../playback/LocalAudioConnectionInstrumentation专验正式Compose Activity实际会话，隔离已知测试片：音频已接、媒体位置推进、停止后合并证据确为audio_timestamp且有音频输出。使用真实设备，结束清宿主与服务，不访问正式服务器。证据归docs/evidence/p240/playback-audio。手机若离线可在平板检验实现，但不能称手机新版已验。

## 网络下一卡（尚未实现）

优先让URL/SMB代理进入同一C++会话的数据源入口：mediacodec/source增加URL输入契约，audio与video各自独立extractor、同一AudioPlayback主时钟；JNI桥新增显式URL打开，Kotlin会话负责异步准备/取消和Surface生命周期。先完成独立家族树及错误/取消所有权设计再动核心，不直接向旧NetworkPlaybackSession添加另一套音频循环。真实Emby和SMB分别验证起播、音画、暂停、前后seek、后台恢复及持续播放；只出画或测试音频出声不算闭环。
