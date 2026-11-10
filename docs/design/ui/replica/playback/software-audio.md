# 软件音频接线（2026-11-09）

目标：保持硬件视频、AAudio、音频主时钟及 seek 静音栅栏，先补齐 AC3/E-AC3，
不以视频软解替代网络容器解析。用户已授权按音频/视频分开处理的建议推进。

核验纠正：Android 构建没有 `--disable-everything`。本机
`C:/re-tools/ffmpeg-android/build/config_components.h` 的 AC3/EAC3 decoder 均为 1，
dist/lib/libavcodec.a 也含对应 decoder 符号。此前“未启用 EAC3”判断撤回。
旧 audio_plane/decode/ffmpeg 只输出帧元数据，局部 AVFrame 随 receive 返回销毁，
不能满足当前播放器的 PCM 供给契约；不直接复用该运行时。

自然家族树与单一职责：

```
backend/include_private/video/video_plane/
  core/decode/{decode_outcome,sample_view}.hpp    平台中立的已有值类型
  core/audio/audio_decoder.hpp                  PCM 借用块及供数接口
  backends/mediacodec/{decode,source}/           旧类型名保留兼容别名
  backends/ffmpeg/audio/ffmpeg_audio_decoder.hpp libavcodec 音频实现声明
backend/src/video/video_plane/backends/
  ffmpeg/audio/ffmpeg_audio_decoder.cxx          压缩包→交错 PCM；不解封装/出声
  mediacodec/session/audio_decode_feeder.cxx     继续唯一拥有供数线程与环背压
  avsync/session/audio_playback.cxx             依 MIME 选择音频实现；生命周期不变
android/app/src/androidTest/.../playback/
  NetworkAudioConnectionInstrumentation.kt      真实生产宿主与新鲜证据断言
docs/evidence/p240/network-native-playback/software-audio/  本卡证据
```

core 不含平台头、不反向依赖 backend。新 FFmpeg 音频 target 链接 avcodec/avutil，
并满足此 FFmpeg 构建解码器注册表的 swresample 链接依赖；实际 AC3/EAC3 路径不调用重采样；
avsync 选择 AC3/EAC3 软件实现；AAC 保持已验的系统软件解码器，其他 MIME 暂保留原路径。
没有 FFmpeg 或格式不可用时显式失败，不尝试视频回退。
packet 自有 padding，send EAGAIN 留包重试，receive EOS/flush遵循 libavcodec 契约；
planar PCM 交错化保持声道与采样率，不做隐式降混。实际输出不符则拒绝。

验收：两机独立 AAC 对照、AC3/EAC3 stereo、EAC3 5.1 合成片通过生产 HTTP 宿主，
暂停冻结、恢复、前后 seek、退出；新鲜 sync 报告确认 codec、实际 PCM、非零声音样本、
AAudio 输出与 audio_timestamp。另验平板真实来源2第3条；K40真实解析问题单列，
不把合成通过等同真实来源整体完成。无新鲜证据不收口。

追加检查：暂停仅暂停 AAudio 消费，供数线程保留在途压缩包和 PCM，环满自然背压；
恢复不重建线程，避免原 stop/start 丢掉半块 PCM。seek/结束仍显式停止供数。
`AudioPlayback::runtime_error()` 只在 feeder 发布 finished 后读取终态错误，
JNI summary 将音频错误交给现有 UI 轮询，禁止解码失败后无声继续播放。

独立验证归属 `backend/tests/video/audio_plane/ffmpeg_audio_decoder_tests/main.cpp`：
使用 libavformat 仅为测试取压缩包，直接验证生产 decoder 的声道频率/交错顺序、
EAGAIN 留包重试、EOS、flush 重放一致性和加密拒绝；Android CMake 独立可执行目标，
不进入产品音频回调。6ch 测试每声道不同频率，包含低频 LFE。

独立探针发现本机 FFmpeg 的 AC3/EAC3 注册结构没有 flush 回调，
仅调用通用 `avcodec_flush_buffers` 不会清掉 IMDCT 重叠与抖动历史。
seek 时重建这一个音频解码上下文，AAudio 与音频时钟不重开；
同片段 flush 重放的 PCM hash 必须相同。已验布局限定 mono/stereo/5.1(side)，
其他布局显式拒绝，不能只看声道数量就认为映射正确。

真实源失败后的修订：预灌未达标时，只有已经正常 EOS 且环中仍有 PCM 的短音轨
可以起流；未就绪、解码错误和空输出必须返回明确错误。seek 采用失败即停的顺序，
定位、flush、预灌任一步失败均保持静音，JNI 不恢复视频。feeder 重启清本段 EOS/
error，保留累计 PCM 计数。网络宿主在 native join 前取消 HTTP，失败的音频对象也
交由宿主统一收尾，避免局部析构等待读取。500ms 本地预灌预算暂不放宽，先用现有
NetworkExtractorProbeInstrumentation 增加可选音轨逐包耗时/落点诊断，确认阻塞位置。
测试按实际关键帧落点验证 seek 后推进，另要求 written PCM 和有效消费帧数，
rendered 中的补静音不能当成出声证据。

网络读取修订（逐包证据 `tablet-audio-packets-r1.txt`）：MKV 在一个 1MiB 响应内
反复跳读，旧 Reader 仅允许连续 cursor，导致每次跳读重建约 1–2s 的 HTTP 请求，
首音频读取耗时 38.6s。保持 `net/HttpRangeFile.kt` 为可寻址网络文件的唯一职责，
每个 Reader 增加一个 1MiB 上限的响应缓存，不新增解封装器或跨线程共享缓存。
已读区间直接复制；同一响应的向前跳读继续读流、保留跳过的字节；越界才重建连接。
缓存按需填充，不在打开时下载完整响应；仍校验 Range/总长/编码并填满请求，取消
仍先 disconnect 再 join。两个 extractor 最多新增 2MiB 常驻内存。
`RangeProbeRegression.kt` 在原分包/截短/越界回归外，加入随机前后读的字节一致性与
HTTP 请求次数断言，防止“正确但仍重复建连”。先实测读取改善，再定网络预灌预算。

缓存后逐包实测首包 2350ms，其后同响应读取 0–1ms。网络宿主显式传入 8000ms
PCM 预灌预算（与单次 HTTP 读取超时一致），本地调用默认仍为 500ms；达到水位
立即起流，绝不固定等满预算。预算、首次/最近等待及最大等待写入音频事实。
配置沿 bridge/PlaybackSession → JNI → AudioPlayback::open 传递并保存在该对象，
seek 复用相同预算；失败仍先由宿主取消 I/O 再 join，不在 start 失败分支里阻塞。

seek 完成契约：JNI 在视频线程已停的安全点保存实际落点，独立 getter 只读该值，
不从运行中的 pacer 获取统计。PlaybackSession 保留现有 Int 返回码，成功后读回
落点。NetworkPlaybackSession 为每次请求发序号，并发布不可变完成结果（序号、
目标、落点、错误）；UI 单线程队列不变。集成测试必须等本次序号完成，再按实际
关键帧落点验证推进，不能因旧位置碰巧落入目标区间而提前通过。暂停/恢复音频
失败必须保持静音并回传错误，禁止 JNI 吞掉 false 后继续视频。

r3 操作失败定位归属：mediacodec/session 的送包与收帧 worker 各自发布静态操作名
（原子指针，不分配、不记录片源），DecodeSession 在停止超时错误中抄取两者。
先分清 extractor、codec 与排程等待，再修取消契约；不扩大 500ms 预算。
Surface 生命周期仍归 bridge/PlaybackSession 与 playback/NetworkPlaybackSession；
禁止把句柄无锁跨线程调用，也禁止通过空 Surface 切换平台输出模式。

网络停机边界修订（r4）：真实源后退失败的操作为
`extractor.read_next_sample`。不扩大解码线程的 500ms 停机门限。
增加 `core/decode/async_sample_reader.{hpp,cxx}`，只负责单包异步读取和独占定位：
read/seek 通过函数注入，不含 Android 头。reader 线程独占 extractor；解码线程
非阻塞 poll，尚无数据返回 TryAgain。只有上一包已提交、再次 poll 时才能继续读，
借用包在此之前保持有效。一个自有包上限 64MiB（与 extractor 一致），无无界队列；
不预取第二包，因此最多额外一包内存。普通暂停/seek 不取消 HTTP 或污染 extractor。
seek 等正在进行的 HTTP 读结束后在同一 reader 线程定位，该等待属于网络定位，
不混进解码 stop。最终析构需宿主先 cancel HTTP 再 join，禁止 detach 线程。

家族树追加：core/decode/async_sample_reader 为读取所有权；mediacodec/session
继续编排，submit worker 只适配 poll/保留 pending；PipelineSetup.async_input 仅网络
宿主显式开启，经 bridge/JNI 传递，本地默认关闭。core 测试归
`backend/tests/video/decode/async_sample_reader_tests/`，验证阻塞读取不阻塞 poll、
借用包生命周期、seek 与 read 互斥、EOS/失败/上限与退出；同源码注册桌面和 Android。

暂停保留 submit worker（sequencer 和 pending）；seek/结束才清理。输出帧排程等待
的可取消及跨暂停保留尚未实现，不能宣称这部分已无损；下一步须保留到恢复再呈现，
seek/结束则显式归还输出槽。暂停失败不置成功态，seek 停机/定位/flush 任一步失败不得重启。
JNI 开始定位前启用音频静音栅栏，失败保持静音。

r6 稳态失败及逐包证据定位：`tablet-real-r6-diagnostic.txt` 20s 只推进14.263s；
`tablet-video-packets-r1.txt` 每跨1MiB重连耗时约1.0–1.2s，首包及MKV簇读取可达3.55s。
压缩包PTS乱序是HEVC重排，不能据此判成时间轴损坏。修订HTTP读取契约：响应请求
从当前位置持续到已知文件末尾，内存仍为每Reader 1MiB环形窗口，按需从流消费。
连续跨窗不重连；窗内回读直接复制；被淘汰的回读或超过一窗的向前跳读才重开。
服务器缩短合法响应时仍在边界续接；响应不等于内存下载量，不做整片预下载。
归属仍为net/HttpRangeFile，不改解封装或音频时钟。Range回归需验证跨窗、环绕、
回读和淘汰后重开时的精确字节及HTTP请求数；真实同源复测后才能说改善。

本轮验收：桌面并发测试；两机真实/合成来源暂停、恢复、前后 seek 与退出的新鲜
证据。真实源网络定位耗时独立保留，不能用合成成功代替；退出 -10000、重复 PTS、
xrun 继续作为未收口问题，须实测查因。

## r8 输出所有权与退出顺序

r7 真实平板已接上 HEVC 硬解和 ffmpeg:eac3；20s 仅推进16.752s，
退出 releaseOutputBuffer=-10000，received440/released439。不能以旧15s推进断言
宣称流畅。K40相同源仍在系统容器解析阶段超时；视频逐包吞吐探针不等于生产稳态。

自然家族树保持已有职责，不新增顶层模块：

- core/pace/FramePacer：可取消 deadline 等待；跨暂停平移待呈现决策、上一帧
  due/present 和本帧 reading_mono，媒体时间及决策次数不变。无 pending 沿用 resume_at。
- mediacodec/session/DecodeReceiveWorker：唯一拥有已 dequeue 的 pending 帧和一次性
  PaceDecision；取消等待保留输出槽，恢复不再 receive/arrive；seek/stop 在 join 后
  render=false 显式归还。release 失败不重试，交由 codec 最终销毁，不掩盖失败。
- DecodeSession：暂停保留两个 worker，恢复更新墙钟起点及 pending 时间；最终停止即使
  已暂停也清理 pending。500ms 停机门限不变。
- decode_pipeline_counters / decode_stats / report：停止归还单列 frames_discarded_on_stop，
  不冒充迟到或无窗口丢帧；旧证据缺字段按0。顺带补齐 no_surface 的 reset。
- playback/NetworkPlaybackSession：重复 stop 的完成监听都可靠送达，仍由串行线程
  唯一操作 native；I/O cancel → native stop/证据/close → fd关闭 → 完成监听。
- ui/compose/PlayerComposeActivity：正常 finish 先等待异步网络收尾，再 super.finish，
  保留原 Surface 到 native 停止；系统强制销毁仍由 onDestroy 兜底。既有 Activity 已超
  400行，本轮只加生命周期编排，不再加入网络/解码实现或新页面职责。

验证先 fresh build core 测试：长等待取消、同帧单次决策、暂停时间与证据坐标一致；
再两机生产宿主合成 EOS/操作与真实平板退出，检查新鲜帧账、音频和错误。持续流畅
另以20s采样曲线/冻结间隔验收，不沿用15s/20s的宽松门。整卡保持 DOING。

r8 统计与专测补充：恢复时把连续播放统计切段（复用 PaceStats 的
note_anchor_reset），不清累计决策、呈现、丢帧、直方图；暂停空白不能进入
clock_lead 或 media_rate 的首末跨度。pending 决策仍只平移墙钟，不重复 arrive。
停止归还增加 frames_discarded_on_stop_undelivered 精确子集：仅已判定
Present/Wait、尚未交付的画面计入；未决策、EOS、已判 Drop/CatchUp 不计入。
总输出槽账使用 stop 总数，节拍未交付账使用此子集，禁止混用。

专测归属 backend/tests/video/pace/pause_resume_tests/main.cpp，独立 CTest
目标 kivo_pause_resume_pace_tests：注入时钟/等待，验证取消优先、等待分块、
同帧跨多次暂停只决策一次、坐标平移与无暂停对照一致、两条恢复路径的连续段
不含暂停时间且累计计数保留。已有千行测试文件不继续扩张。

r8 证据升版：decode/20、sync/44 从版本上强制 stop 总数及未交付子集同时在场，
均为非负整数，子集不得大于总数。旧版缺格按0且打印兼容说明，不伪造旧证据。
输出槽账加入 stop 总数；节拍账只加入未交付子集，名义跨度不包含 stop。
家族树：tools/verification/frame_accounting.py 负责两份校验器共用的停止归还
字段契约；tools/verification/tests/test_frame_accounting.py 负责旧证据兼容及
篡改计数/删字段/错误类型的反例。既有大校验器只保留原有编排和判据展示。
网络收尾仍由单一串行队列拥有 native；取消异常必须记录并继续排队收尾，
不得因异常永久漏发完成通知。监听异常逐个隔离，重复 stop 仍全部送达。

## r9 平板时钟误判与恢复账

r8两机EOS功能测试通过，但完整校验明确否掉平板：540帧零丢帧却媒体速率0.823，
clock_lead=-3501ms，271帧外推、175帧FreeRun，尾部补静音3.49s。不能称流畅。
原始候选窗口呈现约50ms一阶的设备更新，而96帧burst在48kHz仅2ms；
回调burst不是硬件时间戳发布周期，拿2个burst（4ms）判陈旧没有成立依据。

职责与家族树：
- backends/aaudio/clock/AaudioClockSource：将时间戳最大外推年龄明确设为
  max(100ms, 2*burst)。100ms容纳已观察50ms更新的两个周期，是有界预测预算，
  不是宣称硬件精度为100ms；超过预算仍标stale，绝不无限外推。报告实际年龄最大值、
  预算与stale读数，供真机复算。低层FramePositionExtrapolator保留调用方显式预算。
- core/pace/ClockPositionTracker：用“原始位置+仅由跳变/漂移修正产生的偏移”恢复
  新鲜位置，禁止在已外推的位置上再加原始delta（重复计算），也禁止FreeRun结束后
  只补最后一个delta（丢掉整段）。跳变比较用上一次新鲜读数对应的完整墙钟跨度，
  避免把长失效后的正常补报当跳变。pause时前移墙钟原点及新鲜比较基线，排除停播
  时段，保持lead口径一致。临时倒退仍钳制且不改偏移，seek/reset清偏移。
- backend/tests/video/pace/timestamp_cadence_tests/main.cpp：组合生产AAudio时钟与
  tracker，复现50ms设备发布/高频查询、短失效恢复、长失效恢复和真冻结的有界预测。
  新独立CTest目标，不继续扩张已有千行测试。
- NetworkAudioConnectionInstrumentation：稳定窗口每秒采样，检查全窗速率与
  连续2秒窗口推进，EOS报告也要求media_rate及clock_lead过闸。暂停与seek不进窗口。

先保留r8失败证据并令新专测打红，再修实现；桌面相关回归通过后两机EOS/操作，
平板真实源复验。r9本身不替代整张软件音频/网络卡的AAC/多声道/SMB等剩余验收。

## r10 seek 前后同范围帧账

r9两机EOS完整校验通过；操作轮次在两次seek后发现pace累计165次决策，
未交付3帧，而video只保留最近一段的1帧停止归还。不能拿不同范围相减，
也不能清掉历史节拍统计或把输出槽计数单独改成累计。

家族树沿用core/pace、mediacodec/session、avsync/report与tools/verification：
- FramePacer累计discarded_on_stop、discarded_no_surface，与决策计数同寿命；
  reset只切时间轴。worker成功release后记录已决策Present/Wait的归还，
  EOS、未决策、Drop/CatchUp、release失败均不能解释未交付差额。
- PaceSummary经decode_pace_report传递到两种报告，decode/21、sync/45强制
  accounting_scope=session及两个累计归属字段。video/counters仍是最近一段。
- frame_accounting.py统一检查同范围闭合与累计归属不小于最近一段；旧版本
  保留旧判据并显式兼容，不篡改历史JSON。两个完整校验器及真机测试共守新契约。
- pause_resume_tests追加两次reset及最终stop的累计归还回归；Python测试覆盖
  多seek、删字段、错误类型、累计小于当前段及伪造归属。帧数门限不放宽。

两机重跑EOS与操作，完整sync与drift复算后再跑平板真实源；整卡仍DOING。

## r11 seek 后保留会话诊断

r10手机操作的完整校验失败：drift_rebases=1仍在，trace.sampled却被seek清成0。
修正次数、最大值、最近8笔及不连续账统一为会话寿命；时间戳相对首次会话读数，
不能在seek后归零。reset仍清锚点、原始位置偏移与速率估计，启动盲区按新段起点。
不连续账保留累计诊断，但记录每段开始时的offset基线，step_repay=false只扣本段
增量，避免把旧段台阶带入新段。家族树不变：ClockPositionTracker负责寿命，
DiscontinuityLedger继续只记账，pause_resume_tests验证两次seek前后校正、环回绕、
时间顺序与探针对照。先以回归复现失败，再修实现，不放宽校验门。

## r12 生产并发读取定位

r10真实平板在20s稳态开始后约2s触发冻结门，合成片不能解释真实等待。
先增加有界观测，不改变缓存大小、预填或时钟门限。家族树：
- net/NetworkReadTrace.kt：仅debug启用，记录首次32笔及最近512笔固定格式事件，
  单调时间和累计事件数；不接收URL、媒体名、账号或异常message。
- net/HttpRangeFile：各descriptor分配数字reader ID，read记录本次网络消费字节，
  connect/ready/read事件携带相同ID；0是长度探测，后续依次为轨道探测/视频/音频。
- playback/NetworkPlaybackSession：持有debug记录器，读取源释放后仍可取快照；
  不改变单线程native所有权。release构建不启用追踪。
- androidTest/playback/NetworkAudioConnectionInstrumentation：收尾后输出快照，
  成功失败均保存生产并发路径，定位长连接、淘汰回读和供给间歇。

先实测同一真实源，再按数字决定修复；不以放宽稳态门或扩大500ms停机预算过闸。

## r13 同一媒体会话共享稀疏字节

r12真实平板录到三次重复头部约475KiB、尾部约35KiB，以及seek时尾索引挤掉
连续媒体流后1–2.5s重新建连。位置曲线是视频pacer更新，不等于直接音频时钟，
不能仅据冻结曲线断言音频停顿。先消除已证实的重复I/O，再保持原稳态门实测。

家族树与职责：
- net/HttpByteCache.kt：每个HttpRangeFile独占一个共享稀疏缓存，64KiB页，最多
  128页（8MiB数据+最多1MiB有效位）。按字节记录有效位，空洞绝不能当零返回。
  已读头512KiB/尾256KiB优先保留，其余LRU；页数达到上限必须淘汰，禁止整片缓存。
  只缓存实际收到的字节；无地址/凭据/磁盘持久化；同步只包内存操作，不包网络。
- net/HttpRangeFile：各fd仍独占原1MiB环与HTTP连接。先读共享命中段；命中不能
  改变该fd的流游标、关闭连接或重置环。所有收到的网络字节同步写共享缓存，
  未命中保持原范围校验、短读填满、取消及错误行为。close释放共享页。
- androidTest/playback/RangeProbeRegression：同媒体两fd的头/尾重复读应只有
  探测+头+尾共3次HTTP，按确定字节模式逐字节校验；旧实现先明确打红。
  原单fd环回绕测试改为共享缓存命中契约，不再要求已经缓存的头部重开连接。
- androidTest/playback/HttpByteCacheRegression：空洞、跨页、覆盖、LRU淘汰、
  头尾保留、有界容量和多线程不同区间写入读取，直接验证生产缓存。

8MiB是有界局部复用预算，容纳r12约3.1MiB初始区间和约0.8MiB前跳区间及余量；
不宣称任意seek无等待。超出缓存仍正常请求，真实网络不足仍应失败，不能放宽门。

## r14 分离真实稳态与跳转诊断

r13共享缓存回归两机通过，真实平板在两次seek后仍未过20s稳态。
日志里的SocketException发生在测试取消I/O之后，不据此判定原始网络错误。
先在相同生产宿主增加`steadyOnly=true`诊断：真实源正常起播后连续20s，
不暂停、不seek、不等长片EOS；原操作轮次和门限保留。构建环境以本次Java
参数恢复，不修改项目或全局系统设置。

家族树：androidTest/playback/NetworkSteadyProgress负责每秒位置采样、全窗
0.98–1.02及滚动2秒0.90–1.10门和数值型失败证据；自有Failure仅携带
数字/固定标签，可安全输出，不放行真实源异常message。原Instrumentation
仅选择流程、调用和报告，生产网络/缓存/时钟配置本轮诊断保持原值。
先留下正常连续播放失败的可复算曲线，再决定修改何种I/O边界。

r14实测：不seek约5秒即滚动速率0.876失败；音频32帧补静音、xrun0，
不能归因为音频缺供。视频自己的流停在约0.97MiB，持续命中音频已下载字节
到6.08MiB后遇空洞，因为离自己的旧游标超过1MiB又重建连接。视频冷起时
还额外等待约2.5s。共享字节却不共享连续响应，导致复用越多游标越落后。

修复设计与自然家族树（r15）：
- net/HttpRangeFile继续负责HTTP校验、取消、绝对位置文件描述符；将响应环
  改为同媒体会话共用，最多2条，每条1MiB，取代每fd一条。所有fd偏移独立，
  共享传输的读取以会话I/O锁串行，内存命中无需等待网络锁；miss取得锁后
  重查缓存，选择可回读或向前距离<=1MiB的最近响应，否则淘汰最久未用响应。
  音频已推进的响应可直接填下一个洞，视频不再从落后几MiB的自有连接重开。
  尾索引可占第二条，禁止每次小读扩大请求或无界保存连接。网络仍按需读取。
- net/HttpRangeStream负责一条HTTP响应的1MiB环和游标，由File注入打开/关闭
  连接、校验、存储实际字节及取消检查；不持有fd/Android类型。File拥有连接
  注册表，cancel不取I/O锁即可disconnect；close在取消和native退出后释放响应。
- androidTest/playback/SharedRangeRegression增加交错双fd连续读超过1MiB、
  保留尾索引、共享洞/淘汰后的逐字节和请求数断言，旧实现必须明确红。
  RangeProbeRegression的本地HTTP服务支持两个未读完的响应并发，避免用
  单线程假服务把保留响应误判成生产死锁，退出关闭所有测试socket并join。

缓存仍8MiB、响应环总计2MiB；无无界预读，不改PCM/时钟或流畅门。
先旧实现回归打红，再新实现绿，随后同源无操作/有操作分别复测及退出帧账。

r15 并发与取消补充验收：共享 I/O 锁不能让缓存命中等待网络，也不能阻挡取消。
家族树新增 androidTest/playback/RangeConcurrencyRegression.kt，仅负责真实代理 fd
的并发协调与断言；RangeProbeRegression 仍唯一拥有本地 HTTP 服务，增加响应体
发送前的测试栅栏，不新增服务框架或生产注入点。三组可复算场景：两 fd 同洞
并发仅下载一次；一个 fd 的远端洞被阻塞时另一 fd 的缓存命中仍在500ms内返回；
两 fd 分别卡在网络和共享锁时，cancel 在500ms内返回、两次读取都明确失败，
随后关闭源与服务线程无泄漏。响应体栅栏有界且 finally 放行，测试失败不能遗留
阻塞线程。真实网络的超时轮次保留，不把失败后重试通过当成网络可靠性已解决。

## r16 已读完尾索引不应淘汰有效媒体响应

r15无操作真实20s通过完整sync/drift校验；含暂停及前后seek轮次在回到开头
约2秒后仍失败。trace显示尾索引已读到文件末尾，却仍占两响应名额之一；向前
seek创建第三条时按LRU淘汰原媒体响应，回退后在3.62MiB缓存洞重新建连。
这里不是缺少更大缓存，而是已耗尽响应占用了可继续读取的名额。

家族树保持net/HttpRangeStream与HttpRangeFile：Stream提供只读exhausted状态，
File在一次read成功后移除并关闭已读到响应末尾的流；实际字节仍在共享缓存，
有限响应未读到末尾不能提前关闭。两响应/8MiB缓存上限保持，失败仍显式报告。
SharedRangeRegression新增“头部→读完尾索引→远处前跳→头部下一洞”逐字节
与请求数断言；旧实现应为5次，新实现仅probe+媒体+尾+前跳共4次。先保留红证据，
再修实现；重跑两机网络/取消回归及平板真实含操作稳态，不放宽原始门限。
