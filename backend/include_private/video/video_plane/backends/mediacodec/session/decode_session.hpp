#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include <android/native_window.h>

#include "video/video_plane/backends/mediacodec/decode/mediacodec_video_decoder.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_failure_slot.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pace_sampler.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pipeline_counters.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pipeline_gate.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_receive_worker.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stage_cost.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stats.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_submit_worker.hpp"
#include "video/video_plane/backends/mediacodec/session/surface_rebuild_tracker.hpp"
#include "video/video_plane/backends/mediacodec/session/present_schedule.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"
#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/metrics/cpu_time_facts.hpp"
#include "video/video_plane/core/pace/frame_pacer.hpp"

namespace kivo::video::mediacodec {

struct SessionOutcome {
    enum class Code {
        kOk = 0,
        kAlreadyOpen = 1,
        kOpenFailed = 2,
        kFormatRejected = 3,
        kConfigureFailed = 4,
        kStartFailed = 5,
        kNotOpen = 6,
        kPacingRejected = 7,
        kPipelineRejected = 8,
        kSeekRejected = 9,   ///< 前置条件不满足（没 open / 没 start / 正在 seek）
        kSeekFailed = 10,    ///< 定位或 flush 失败；原因在 `message` 里
        /// 轨道声明了 sample 级加密（P12-F3B）。**在 `configure()` 之前就拒绝** ——
        /// 加密轨配解码器只会得到垃圾帧，那比直接拒绝更糟（看起来像"播了"）。
        kEncryptedUnsupported = 11,
    };

    Code code = Code::kOk;
    std::string message;

    [[nodiscard]] bool ok() const { return code == Code::kOk; }
};

/// 流水线的运行模式。
///
/// **默认是 `kMerged`。** P12-C6 的真机 A/B（同二进制、同片源、交替 4 轮）显示
/// 拆线程的吞吐增益落在噪声内：1080p8 −0.9%、HEVC1080p10 +2.0%、HEVC4K10 +0.5%，
/// 不满足卡片写死的「增益必须随像素走」，故按 `19-p12c6-design.md` §七 回退。
/// 拆线程本身没坏处，但它把线程数从 1 提到 2 而换不来吞吐，不该当默认。
///
/// `kSplit` 保留为可选实验路径，理由两个：
/// 1. A/B 取证必须在**同一个二进制**上交替跑（见 `19-p12c6-design.md` §6.1），
///    编译期开关做不到"交替"；
/// 2. 这次的判据只否定了"送收往返是瓶颈"这一假设，没有否定"两段可并行"本身 ——
///    换到调度更宽松的平台（或 C7 改了呈现路径）之后值得复测，届时不必重写。
///
/// 判据推导与瓶颈归因见 `20-p12c6-status.md`。
enum class PipelineMode : std::int32_t {
    kSplit = 0,   ///< 送包与收帧各一个线程（实验路径，非默认）
    kMerged = 1,  ///< 两段在同一线程里交替泵（等价 C5 结构），默认
};

[[nodiscard]] const char* pipeline_mode_name(PipelineMode mode) noexcept;

/// 节拍配置。
struct PacingSetup {
    /// 媒体位置来源。**必须非空**：纯视频用 `SystemClockSource`，
    /// 有音轨时用 AAudio 时钟。为空即等于"没有节拍"，会被拒绝而不是静默忽略。
    core::audio::AudioClockSource* clock = nullptr;

    core::pace::PacePolicy policy{};
};

/// 流水线配置。
struct PipelineSetup {
    PipelineMode mode = PipelineMode::kMerged;

    /// 是否把解出的帧交给 Surface。
    ///
    /// **诊断开关，不是产品开关**：置 false 时一律 `release_frame(index, false)`，
    /// 即"只解码、不上屏"。它存在的唯一理由是回答一个问题：
    /// 1080p 停在 ~175fps 到底是解码器的吞吐上限，还是呈现链路（BufferQueue / 合成器）
    /// 在限速。两者必须分开量，否则会把"显示器限速"误判成"解码慢"，
    /// 进而把优化力气花错地方（见 `20-p12c6-status.md`）。
    bool present_frames = true;

    /// 呈现排程方式（P12-C7）。只在 `present_frames == true` 且配了节拍时才有意义 ——
    /// 没有节拍就没有 `due_at`，两者退化成同一件事（收到就交）。
    ///
    /// 默认 `kSurfaceAtTime`，依据见 `23-p12c7-status.md`；
    /// 传 `kCpuPaced` 可退回 C5 结构。
    PresentSchedule present_schedule = PresentSchedule::kSurfaceAtTime;
};

/// 解码会话：把解封装、解码、Surface 输出串成一条可运行的流水线。
///
/// 线程模型（P12-C6 起）：
/// - `open()`/`attach_*()` 在调用线程完成平台初始化；
/// - `start()` 起工作线程：split 模式两个（送包 / 收帧），merged 模式一个（交替泵）；
/// - `stop()` 请求停止并 join，**join 顺序固定 submit 在前**（见设计文档 §3.3）。
///
/// 会话本身不含任何循环体：它只负责编排生命周期、收集统计。
///
/// 明确不做（留给后续阶段）：A/V 偏差统计（C8）、seek 后重同步（P12-F）。
class DecodeSession {
public:
    DecodeSession() = default;
    ~DecodeSession();

    DecodeSession(const DecodeSession&) = delete;
    DecodeSession& operator=(const DecodeSession&) = delete;
    DecodeSession(DecodeSession&&) = delete;
    DecodeSession& operator=(DecodeSession&&) = delete;

    /// 打开媒体并配置解码器。window 必须在会话整个生命周期内有效。
    ///
    /// 走 fd 而非路径的原因见 MediaExtractorSource::open_fd。
    /// @param label 仅用于诊断，不参与 IO
    [[nodiscard]] SessionOutcome open(int fd, std::int64_t offset, std::int64_t length,
                                     const std::string& label, ANativeWindow* window);

    /// 启动后台流水线。立即返回。
    [[nodiscard]] SessionOutcome start();

    /// 开启按节拍呈现（P12-C5）。**必须在 `start()` 之前调用**。
    ///
    /// 不开节拍时是全速解码：收到就交、不解不睡，用于吞吐取证与基线对照。
    [[nodiscard]] SessionOutcome attach_pacing(const PacingSetup& setup);

    /// 选择流水线模式（P12-C6）。**必须在 `start()` 之前调用**。
    ///
    /// 不调用时用 `kMerged`。`kSplit` 是实验路径，不是"另一种产品行为"。
    [[nodiscard]] SessionOutcome attach_pipeline(const PipelineSetup& setup);

    /// 定位到指定时刻并重新起跑（P12-F1，G8 首帧 < 150 ms）。
    ///
    /// **必须从控制线程调用**（JNI 线程），不能从工作线程调用 —— 它会 join 工作线程，
    /// 工作线程里调等于自己等自己。
    ///
    /// 做四件事，顺序不可换：
    /// 1. `stop()`：只停线程，**不动 codec**（`AMediaCodec_start` 不能重复调用）；
    /// 2. `extractor.seek_to(target)`：`PREVIOUS_SYNC`，返回**实际落点**；
    /// 3. `decoder.flush()`：清在途输入/输出。必须在起线程之前，否则新送的包会被 flush 吃掉；
    /// 4. `begin_run()`：清计数、重锚节拍、重建 worker、起线程 —— 与 `start()` 后半段同一份代码。
    ///
    /// 语义提醒：**帧计数会归零**（新位置就是新时间轴）。所以一轮里做过 seek 的证据，
    /// `counters` 描述的是**最后一段**；`seek.requests` 说明发生过几次。
    /// @param defer_start true = 只做"停线程 → 定位 → flush"，**不起跑**；
    ///        调用方随后必须调 `resume_after_seek()`。
    ///
    /// 为什么要这个参数（P12-F2A）：音频必须跟着**视频的实际落点**跳，否则两边会差一个关键帧间隔。
    /// 实测后向 seek 到 500 ms：视频按 `PREVIOUS_SYNC` 落到 0，音频落到 490 ms ⇒ gap 490 ms，
    /// 按自己的判据就是"脱节"。所以顺序必须变成"视频先定位拿到落点 → 音频按该落点定位 → 一起起跑"。
    [[nodiscard]] SessionOutcome seek_to(std::int64_t target_us, bool defer_start = false);

    /// `seek_to(..., defer_start=true)` 之后的起跑（与 `start()` 共用 `begin_run()`）。
    [[nodiscard]] SessionOutcome resume_after_seek();

    /// 把音频侧的 seek 结果并进同一条证据（P12-F2）。
    ///
    /// **为什么由会话来记**：证据只有一份（`decode/11`），音频侧的事实必须以字段形式
    /// 进同一个文件；否则音画两侧的落点分别躺在两处，没人会把它们相减。
    ///
    /// 调用时机：音频 seek 之后、视频 seek 之后。视频侧的 `seek_to()` 会重置 `requests` 等字段，
    /// 所以这个方法必须在它之后再调。
    void record_audio_seek(bool present, std::int64_t landing_us);

    /// 换输出窗口（P12-D1）。**允许在解码运行中调用。**
    ///
    /// @param window 新的 `ANativeWindow*`；传 `nullptr` 表示"面没了，先切到 ByteBuffer 模式"。
    ///
    /// 为什么不做成 stop/start：`start()` 里有 `counters_.reset()` 与 `pacer_->reset()`，
    /// 走一趟就是"帧计数归零 + 时间轴重锚"，前者让帧数闭合不变量失效，
    /// 后者让"3 帧内恢复"退化成"从头追帧"。换面只该换面。
    ///
    /// 时序约定（`26-p12d1-design.md` §3.2）：`surfaceDestroyed` 先 `rebind_window(nullptr)`，
    /// `surfaceCreated` 再 `rebind_window(new)`。**不许中间空着不管** ——
    /// 那样收帧线程会往一个已经 disconnect 的 BufferQueue 上交付。
    [[nodiscard]] SessionOutcome rebind_window(ANativeWindow* window);

    /// 请求停止并等待线程退出。可重复调用。
    void stop();

    /// 取统计快照。线程安全，可在任意线程调用。
    [[nodiscard]] DecodeStats snapshot() const;

    /// 取节拍器的**完整**摘要（含直方图桶计数），用于 C8 的独立复算导出。
    ///
    /// ⚠️ **只能在 `stop()` 之后调用。** 节拍器不加锁、按约定只归收帧线程所有；
    /// 跑起来时读它就是数据竞争。`stop()` 已经 join 了全部工作线程，那时读才安全。
    ///
    /// 为什么不做成 `snapshot()` 的一部分：`PaceSummary` 带六个 1024 桶直方图（约 48KB），
    /// 而 `snapshot()` 每秒会被取样好几次 —— 把直方图塞进快照等于每帧复制 48KB。
    /// 分成一个"只调一次"的接口，两边都不用妥协。
    [[nodiscard]] core::pace::PaceSummary pace_summary_after_stop() const;

private:
    /// `start()` 与 `seek_to()` 共用的"起跑"段：清计数、重锚节拍、重建 worker、起线程。
    ///
    /// 抽出来的理由只有一个：**两条路径必须逐字相同**。手写两遍的下场是
    /// seek 之后少了 `pacer_->reset()`，首帧被判迟到，而"首帧时延"这个数字看起来还正常。
    [[nodiscard]] SessionOutcome begin_run();

    /// merged 模式：两段在同一线程里交替泵。判据见设计文档 §7。
    void run_merged();

    /// 按当前模式起线程。
    void launch_workers();

    MediaExtractorSource source_;
    MediaCodecVideoDecoder decoder_;

    std::unique_ptr<core::pace::FramePacer> pacer_;

    /// 跨线程只经过这一个闸门：停不停、输入有没有空位、送包段收工没有。
    DecodePipelineGate gate_;
    DecodePipelineCounters counters_;
    DecodeFailureSlot failure_;
    DecodePaceSampler pace_sampler_;
    SurfaceRebuildTracker rebuild_tracker_;

    /// 面已摘掉（P12-D1）。置位期间收帧段一律 `render=false` ——
    /// 只把缓冲还回去，不碰任何 Surface。
    ///
    /// 为什么不调 `AMediaCodec_setOutputSurface(codec, nullptr)`：
    /// **在这颗 decoder 上切不回来。** K40 实测：切到 ByteBuffer 模式之后
    /// 再切回 Surface 直接返回 `AMEDIA_ERROR_UNKNOWN`（status=-10000），
    /// 会话只能作废。所以"摘面"必须是本地行为，平台侧保持 window→window 直换 ——
    /// 那正是 ExoPlayer 换面走的路。
    std::atomic<bool> surface_detached_{false};

    /// 四段平台调用的耗时事实。分开记才能回答"收益从哪来"（设计文档 §6.3）。
    StageCostMeter submit_cost_;
    StageCostMeter eos_cost_;
    StageCostMeter receive_cost_;
    StageCostMeter release_cost_;

    std::unique_ptr<DecodeSubmitWorker> submit_worker_;
    std::unique_ptr<DecodeReceiveWorker> receive_worker_;

    std::thread submit_thread_;
    std::thread receive_thread_;
    std::thread merged_thread_;

    /// mime / 尺寸 / 时长 / codec 名，open 之后不再变化。
    DecodeStats context_{};

    /// seek 事实（P12-F1）。只在控制线程写，`snapshot()` 在任意线程读 —— 所以这里
    /// 不做原子：读写都发生在"工作线程已 join"的窗口里（`stop()` 之后、`begin_run()` 之前）。
    core::seek::SeekFacts seek_{};

    /// 冷启动首帧时延（P13-B）。**第一次 `seek_to()` 之前抓一次，之后不再改**。
    ///
    /// 为什么需要单独存：`counters_.first_frame_latency_us` 在 `begin_run()` 里被重置，
    /// seek 后它就变成"seek 之后的首帧"。不另存一份，冷启动的数字就永久丢了 ——
    /// 而 G8a 判的正是那个数字。
    std::int64_t cold_start_first_frame_us_ = -1;

    PipelineMode mode_ = PipelineMode::kMerged;
    bool present_frames_ = true;
    PresentSchedule present_schedule_ = PresentSchedule::kSurfaceAtTime;

    std::chrono::steady_clock::time_point run_started_at_{};

    /// CPU 时间起点（P14-A）。**只在第一次 `begin_run()` 时取**：
    /// seek 会再进一次 `begin_run()`，如果每次都重置，G2 的账就只剩"最后一次 seek 之后"，
    /// 而那是整场里最没有代表性的一段。
    core::metrics::CpuTimeSnapshot cpu_begin_{};

    bool opened_ = false;
    bool started_ = false;
};

}  // namespace kivo::video::mediacodec
