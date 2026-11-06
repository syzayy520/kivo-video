#include "video/video_plane/backends/mediacodec/session/decode_session.hpp"

#include "video/video_plane/core/pace/pace_now.hpp"

#if defined(__ANDROID__) || defined(__linux__)
#include <pthread.h>
#endif

#include "video/video_plane/backends/mediacodec/decode/dv_config_facts.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediaformat_color_io.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediaformat_dv_io.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediacodec_format_guard.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/backends/mediacodec/present/surface_dataspace.hpp"

namespace kivo::video::mediacodec {
namespace {

using SteadyClock = std::chrono::steady_clock;

/// merged 模式单轮最多连送几包再回头收帧。
///
/// 这个数与 P12-B 的 `kMaxSubmitBatch` 保持一致，是**对照公平性**问题：
/// 合并模式的职责是回答"不拆线程有多快"，若它少了批量灌包这一层，
/// 拆线程的收益就会被虚报。拆线程后这个批量不再需要（两段本来就并行），
/// 所以它只出现在 merged 路径里。
constexpr int kMergedSubmitBatch = 8;

}  // namespace

const char* pipeline_mode_name(PipelineMode mode) noexcept {
    switch (mode) {
        case PipelineMode::kSplit:
            return "split";
        case PipelineMode::kMerged:
            return "merged";
    }
    return "unknown";
}

const char* present_schedule_name(PresentSchedule schedule) noexcept {
    switch (schedule) {
        case PresentSchedule::kCpuPaced:
            return "cpu_paced";
        case PresentSchedule::kSurfaceAtTime:
            return "surface_at_time";
    }
    return "unknown";
}

DecodeSession::~DecodeSession() {
    stop();
}

SessionOutcome DecodeSession::open(int fd, std::int64_t offset, std::int64_t length,
                                   const std::string& label, ANativeWindow* window) {
    if (opened_) {
        return {SessionOutcome::Code::kAlreadyOpen, "session already opened"};
    }

    const ExtractOutcome extracted = source_.open_fd(fd, offset, length, label);
    if (!extracted.ok()) {
        return {SessionOutcome::Code::kOpenFailed, "extractor: " + extracted.message};
    }

    const DecoderFormatOutcome format_check = validate_decoder_format(source_.track_format());
    if (!format_check.ok()) {
        return {SessionOutcome::Code::kFormatRejected, "format: " + format_check.message};
    }

    // P12-F3B：加密轨在这里挡掉，**早于 `configure()`**。
    //
    // 为什么不在 `submit()` 里挡（那里也有一道）：K40 实测，
    // `AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED` 对真 CENC 片源**恒为 0**，
    // 而轨道格式里 `crypto-mode` 一直在。只看标志位的结果是"配好解码器、解出 25/90 帧
    // 垃圾像素、`completed = true`" —— 比拒绝更糟，因为它看起来像播放成功。
    const EncryptionAssessment encryption = assess_encryption(source_.track_encryption());
    if (encryption.encrypted) {
        return {SessionOutcome::Code::kEncryptedUnsupported, encryption.reason};
    }

    // 210：DV 识别，**早于颜色分类**。
    //
    // 为什么在前：dv5 片源轨道上连颜色键都没有，`classify_hdr` 会判 sdr 放过去，
    // 最后死在模糊的"没有解码器"上（探针实测：createDecoderByType 对
    // video/dolby-vision 返回 null）—— 那正是这张卡要消灭的失败形状。
    // 判定与理由全部来自 `assess_dv`（纯逻辑，桌面单测钉住）；这里只接线。
    const DvTrackKeys dv_keys = read_dv_track_keys(source_.track_format());
    std::vector<std::uint8_t> csd0_bytes;
    const bool has_csd0 = read_csd0_bytes(source_.track_format(), csd0_bytes);
    const BlCodec bl_codec =
        has_csd0 ? sniff_bl_codec(csd0_bytes.data(), csd0_bytes.size()) : BlCodec::kUnknown;
    const DvAssessment dv = assess_dv(source_.track_info().mime, dv_keys, bl_codec);
    if (dv.verdict == DvVerdict::kUnsupported) {
        return {SessionOutcome::Code::kDolbyVisionUnsupported, dv.reason};
    }
    context_.dv.verdict = dv.verdict;
    context_.dv.dv_profile = dv.dv_profile;
    context_.dv.dv_level = dv.dv_level;
    context_.dv.bl_compatibility_id = dv_keys.bl_compatibility_id;
    context_.dv.reason = dv.reason;

    // P12-E2：先判 HDR 类型，再设 dataspace，最后才 configure。
    //
    // 顺序是硬要求：dataspace 决定之后从 BufferQueue dequeue 出来的缓冲被标成什么。
    // configure 之后再设，首批缓冲已经按旧 dataspace 出去了 —— 那不是"晚了一点"，
    // 是这一轮的标记整体错了。见 `37-p12e2-design.md` §2.3。
    const SourceColorTags source_tags = read_source_color_tags(source_.track_format());
    VideoColorFacts source_facts;  // 只为分类：这里不碰解码器，也不需要输出格式
    source_facts.mime = source_.track_info().mime;
    source_facts.source_color_standard = source_tags.standard;
    source_facts.source_color_transfer = source_tags.transfer;
    source_facts.source_color_range = source_tags.range;
    HdrAssessment hdr;
    if (dv.verdict == DvVerdict::kPassthroughHdr10) {
        // DV 8.x：kind 由 DV 事实决定，不让色彩键的 hdr10 结论盖过去 ——
        // 证据里 kind=dolby_vision 才能说清"这是 DV 片源按 HDR10 透传"，
        // 与真 HDR10 片源是两回事（dataspace 动作相同，来源不同）。
        hdr.kind = HdrKind::kDolbyVision;
        hdr.reason = dv.reason;
    } else {
        hdr = classify_hdr(source_facts);
    }

    context_.hdr.kind = hdr.kind;
    context_.hdr.kind_reason = hdr.reason;
    context_.hdr.static_info_bytes = source_tags.static_info_bytes;
    context_.hdr.dataspace = apply_surface_dataspace(window, hdr.kind);
    // P12-E3：设完立刻回读一次。"调用返回 0"与"窗口上真是这个值"是两件事 ——
    // E2 已经演示过"函数指针其实是 null 却报告自己成功了"这类自欺。
    // 回读仍然**不**等于合成器认账：那要看 SurfaceFlinger 的图层 dump。
    context_.hdr.dataspace_readback = read_surface_dataspace(window);

    // P12-E4：把"没有做 tone mapping"写成事实，而不是留给读者去猜。
    // 依据两条实测：① Surface 直出，进程拿不到像素；② 解码器拒绝 color-transfer-request
    // （探针 honoured=false，见 42-p12e4-status.md）。SDR 源本来就不需要映射，分开写。
    context_.hdr.tonemap.implemented = false;
    context_.hdr.tonemap.reason =
        hdr.kind == HdrKind::kSdr
            ? "source is SDR; there is nothing to tone-map"
            : "not implemented: this pipeline writes decoded frames straight to the Surface, so "
              "there is no pixel access; the decoder also ignored color-transfer-request (probe "
              "kivo_hdr_probe, honoured=false), so no HDR-to-SDR mapping happens here";

    const VideoTrackInfo& info = source_.track_info();
    // 210：8.x 透传的 configure 用**重建格式 + 覆盖 mime**：
    // - mime 不重写就是"识别了却播不了"（这颗设备没有 dolby-vision 解码器，探针实测）；
    // - 重建格式（make_configure_format 家族）天然不抄 csd-2 与 DV 专有键
    //   （profile=256 这类 DV 位标志对 HEVC 解码器是毒键：它不是合法 HEVC profile）。
    const bool dv_passthrough = dv.verdict == DvVerdict::kPassthroughHdr10;
    DecodeOutcome configured;
    if (dv_passthrough) {
        AMediaFormat* rebuilt =
            make_configure_format_with_mime(source_.track_format(), source_tags,
                                            dv.bl_mime.c_str());
        if (rebuilt == nullptr) {
            return {SessionOutcome::Code::kConfigureFailed,
                    "decoder: rebuilding the DV passthrough configure format failed"};
        }
        context_.dv.bl_mime = dv.bl_mime;
        context_.dv.configure_format_rebuilt = true;
        configured = decoder_.configure(dv.bl_mime, rebuilt, window);
        // configure 不接管所有权（P12-E2 记过这条 API 29 教训）：用完即删。
        AMediaFormat_delete(rebuilt);
    } else {
        configured = decoder_.configure(info.mime, source_.track_format(), window);
    }
    if (!configured.ok()) {
        return {SessionOutcome::Code::kConfigureFailed, "decoder: " + configured.message};
    }

    opened_ = true;

    context_.mime = info.mime;
    context_.width = info.width;
    context_.height = info.height;
    context_.media_duration_us = info.duration_us;
    context_.codec_name = decoder_.codec_name();

    return {};
}

SessionOutcome DecodeSession::attach_pacing(const PacingSetup& setup) {
    if (started_) {
        return {SessionOutcome::Code::kPacingRejected, "attach_pacing after start"};
    }
    if (setup.clock == nullptr) {
        // 没有时钟就没有节拍。允许空指针等于允许"悄悄不节拍"，
        // 证据里会显示成 enabled=false，看起来像"这轮没开"，实际是配错了。
        return {SessionOutcome::Code::kPacingRejected, "pacing requires a clock source"};
    }
    core::pace::FramePacer::Setup pacer_setup;
    pacer_setup.clock = setup.clock;
    pacer_setup.policy = setup.policy;
    // 节拍器与两个 worker 共用同一个停止标志：多一个停止量就多一处要同步的状态。
    pacer_setup.stop_flag = gate_.stop_flag();
    auto pacer = std::make_unique<core::pace::FramePacer>(pacer_setup);
    if (!pacer->usable()) {
        return {SessionOutcome::Code::kPacingRejected, "pacing policy is not self-consistent"};
    }
    pacer_ = std::move(pacer);
    // attach_pipeline 可能已先跑过：按它记下的排程方式决定"谁来等"。
    pacer_->set_sleep_before_present(present_schedule_ == PresentSchedule::kCpuPaced);

    // 先发布一次"零态"快照：没开跑之前读 `snapshot()` 也该看到 enabled=true 与真实时钟名，
    // 而不是一片空白 —— 空白会被读成"没开节拍"。
    // P16-P1B12：频差观测与摘要同批发布（同一线程读 pacer，不跨线程）。
    pace_sampler_.publish(pacer_->summary(), pacer_->last_rate_ppm(),
                          pacer_->last_phase_error_ns());
    return {};
}

SessionOutcome DecodeSession::attach_pipeline(const PipelineSetup& setup) {
    if (started_) {
        return {SessionOutcome::Code::kPipelineRejected, "attach_pipeline after start"};
    }
    mode_ = setup.mode;
    present_frames_ = setup.present_frames;
    present_schedule_ = setup.present_schedule;
    // pacer 可能已经建好了（attach_pacing 在前），把它对齐到当前的排程方式。
    if (pacer_ != nullptr) {
        pacer_->set_sleep_before_present(present_schedule_ == PresentSchedule::kCpuPaced);
    }
    return {};
}

SessionOutcome DecodeSession::rebind_window(ANativeWindow* window) {
    if (!opened_) {
        return {SessionOutcome::Code::kNotOpen, "rebind_window called before open"};
    }

    if (window == nullptr) {
        // 摘面：**只置本地标志，不碰平台**。
        //
        // 为什么不像最初设计那样切到 ByteBuffer 模式：K40 实测那颗 AVC decoder
        // 切过去就切不回来（`setOutputSurface` 返回 AMEDIA_ERROR_UNKNOWN / -10000），
        // 整个会话作废。这个失败是精确的、可复现的，见 `27-p12d1-status.md`。
        //
        // 置位之后收帧段一律 render=false：缓冲照还、Surface 不碰。
        // 旧窗口引用继续留着 —— codec 还指着它，这时候 release 就是悬空指针。
        surface_detached_.store(true, std::memory_order_release);
        rebuild_tracker_.note_detach(core::pace::monotonic_now_ns());
        return {};
    }

    // 上表面：window → window 直换。这是平台支持的转场（ExoPlayer 换面同一条路），
    // 也是唯一在 K40 上验证通过的那条。
    const DecodeOutcome swapped = decoder_.set_output_surface(window);
    if (!swapped.ok()) {
        // 失败时保持摘面状态：继续 render=false 至少还能丢帧跑完，
        // 假装换成功了会让收帧段往一个不存在的面上交付。
        return {SessionOutcome::Code::kConfigureFailed, "surface: " + swapped.message};
    }
    surface_detached_.store(false, std::memory_order_release);
    rebuild_tracker_.note_rebind(core::pace::monotonic_now_ns());
    return {};
}

SessionOutcome DecodeSession::start() {
    if (!opened_) {
        return {SessionOutcome::Code::kNotOpen, "start called before open"};
    }
    if (started_) {
        return {SessionOutcome::Code::kOk, "already started"};
    }

    const DecodeOutcome started = decoder_.start();
    if (!started.ok()) {
        return {SessionOutcome::Code::kStartFailed, "decoder: " + started.message};
    }

    // codec 名在 start 之后取：部分平台要到组件真正启动才确定具体实现。
    // 放在这里而不是工作线程里，是为了让 context_ 只在工作线程启动前被写一次，
    // 避免与 snapshot() 形成数据竞争。
    const std::string started_name = decoder_.codec_name();
    if (!started_name.empty()) {
        context_.codec_name = started_name;
    }

    return begin_run();
}

SessionOutcome DecodeSession::seek_to(std::int64_t target_us, bool defer_start) {
    if (!opened_) {
        return {SessionOutcome::Code::kNotOpen, "seek called before open"};
    }
    if (!started_) {
        return {SessionOutcome::Code::kSeekRejected, "seek called before start"};
    }
    if (target_us < 0) {
        return {SessionOutcome::Code::kSeekRejected, "seek target is negative"};
    }

    const auto seek_began = SteadyClock::now();

    // 1. 只停线程，**不动 codec**。`start()` 会再调一次 `AMediaCodec_start`，那是非法操作；
    //    seek 必须复用已经 started 的 codec，靠 `flush()` 回到"已启动、无在途数据"。
    stop();

    // 1.5 抓一次冷启动首帧时延（P13-B）。**必须在 `begin_run()` 之前** ——
    // 它会把 counters 重置，之后这个数字就没了。只抓第一次。
    if (cold_start_first_frame_us_ < 0) {
        const std::int64_t cold = counters_.first_frame_latency_us.load(std::memory_order_relaxed);
        if (cold >= 0) {
            cold_start_first_frame_us_ = cold;
        }
    }

    // 2. 定位。返回**实际落点**（PREVIOUS_SYNC ⇒ 应 ≤ target）。
    const std::int64_t landing = source_.seek_to(target_us);
    seek_.requests += 1;
    seek_.target_us = target_us;
    seek_.landing_us = landing;
    seek_.first_frame_us = -1;  // 由 snapshot() 从 counters 补

    // 3. flush 在起线程之前：反过来新送的包会被 flush 吃掉，表现为"seek 后前几帧没了"。
    const DecodeOutcome flushed = landing < 0 ? DecodeOutcome{}
                                              : decoder_.flush();
    const auto seek_ready = SteadyClock::now();
    seek_.prepare_us =
        std::chrono::duration_cast<std::chrono::microseconds>(seek_ready - seek_began).count();

    // 4. 起跑（或交给调用方起跑）。
    //    `defer_start` 用于"视频先定位、音频按视频落点定位、再一起起跑"这条编排（F2A）。
    const SessionOutcome resumed = defer_start ? SessionOutcome{} : begin_run();

    if (landing < 0) {
        return {SessionOutcome::Code::kSeekFailed,
                "extractor seek failed for target " + std::to_string(target_us) + " us"};
    }
    if (!flushed.ok()) {
        return {SessionOutcome::Code::kSeekFailed, "flush after seek failed: " + flushed.message};
    }
    return resumed;
}

void DecodeSession::pause() {
    if (!started_ || paused_) {
        return;
    }
    // P16-P1B32：从"按下的那一刻"起算，`stop()` 里 join 线程的几毫秒也算 pause
    // —— 那段时间画面确实没在推进。
    pause_started_ns_ = core::pace::monotonic_now_ns();
    ++pause_count_;
    // `stop()` 只停线程、disarm 分配计数、置 `started_=false`；codec / extractor / surface
    // 都保留 —— 这正是 `seek_to` 敢在 `stop()` 之后直接 `begin_run()` 的原因。
    stop();
    paused_ = true;
}

SessionOutcome DecodeSession::resume() {
    if (!paused_) {
        return {SessionOutcome::Code::kSeekRejected, "resume called without pause"};
    }
    if (!opened_) {
        return {SessionOutcome::Code::kNotOpen, "resume called before open"};
    }
    // P16-P1B32：先把本次 pause 的时长入账，再 `begin_run()` ——
    // 它要用 `paused_total_ns_` 算 `started_at`。
    if (pause_started_ns_ > 0) {
        paused_total_ns_ += core::pace::monotonic_now_ns() - pause_started_ns_;
        pause_started_ns_ = 0;
    }
    // 先把 due 锚点推到 now：pause 超过 500 ms 时音频钟会 stale -> FreeRun，
    // 那里 `due_at = last_due + fd`，旧锚点会让第一帧迟到几百毫秒。
    if (pacer_) {
        pacer_->resume_at(core::pace::monotonic_now_ns());
    }
    const SessionOutcome outcome = begin_run(/*preserve_stats=*/true);
    if (outcome.ok()) {
        paused_ = false;
    }
    return outcome;
}

void DecodeSession::record_audio_seek(bool present, std::int64_t landing_us) {
    seek_.audio_present = present;
    seek_.audio_landing_us = landing_us;
}

SessionOutcome DecodeSession::resume_after_seek() {
    if (!opened_) {
        return {SessionOutcome::Code::kNotOpen, "resume_after_seek called before open"};
    }
    if (started_) {
        return {SessionOutcome::Code::kSeekRejected, "the pipeline is already running"};
    }
    return begin_run();
}

SessionOutcome DecodeSession::begin_run(bool preserve_stats) {
    // P16-P1B29：pause-resume 走 `preserve_stats=true` —— 不清计数、不重锚节拍。
    // `start()` 与 `resume_after_seek()` 仍走默认 `false`，行为逐位不变。
    if (!preserve_stats) {
        counters_.reset();
        if (pacer_) {
            // 锚点必须在首帧到达时建立；上一轮留下的历史会让首帧被判迟到（C5 的结论）。
            pacer_->reset();
        }
    }
    // 无论是否保留统计，worker 计数都必须复位 —— 新线程要重新报数。
    gate_.reset_for_start();

    if (!preserve_stats) {
        run_started_at_ = SteadyClock::now();
        paused_total_ns_ = 0;
        pause_started_ns_ = 0;
        pause_count_ = 0;
    } else {
        // P16-P1B32：`finish()` 里的 `wall_micros` 是"只让第一次生效"
        // （`compare_exchange_strong(expected=0, ...)`）。pause 那一段已经写过一次
        // 5.0 s，resume 后 CAS 必然失败，`wall_us` 就永远停在 5.0 s。
        // 这里单独清零（**不是** `counters_.reset()`，那会抹掉帧计数），
        // 让 resume 后的 `finish()` 重新写入 —— 而它的 `started_at` 已含累计 pause 偏移，
        // 所以写进来的是"播放时长"。
        counters_.wall_micros.store(0, std::memory_order_relaxed);
    }

    // P14-A：三条以前没有的尺子的起点。
    //
    // CPU 起点只取一次（`valid` 后不再覆盖）：seek 会再进 `begin_run()`，
    // 每次都重置的话 G2 的账只剩"最后一次 seek 之后"。
    // 分配计数同理：已经 arm 过就不再 reset，否则稳态计数会被 seek 抹掉一半。
    if (!cpu_begin_.valid) {
        cpu_begin_ = core::metrics::sample_cpu_time();
    }
    if (!core::metrics::alloc_probe::armed()) {
        core::metrics::alloc_probe::reset();
        core::metrics::alloc_probe::arm();
    }

    DecodeSubmitWorker::Deps submit_deps;
    submit_deps.source = &source_;
    submit_deps.decoder = &decoder_;
    submit_deps.gate = &gate_;
    submit_deps.counters = &counters_;
    submit_deps.failure = &failure_;
    submit_deps.submit_cost = &submit_cost_;
    submit_deps.eos_cost = &eos_cost_;
    submit_worker_ = std::make_unique<DecodeSubmitWorker>(submit_deps);

    DecodeReceiveWorker::Deps receive_deps;
    receive_deps.decoder = &decoder_;
    receive_deps.gate = &gate_;
    receive_deps.counters = &counters_;
    receive_deps.failure = &failure_;
    receive_deps.receive_cost = &receive_cost_;
    receive_deps.release_cost = &release_cost_;
    receive_deps.pacer = pacer_.get();
    receive_deps.present_frames = present_frames_;
    receive_deps.present_schedule = present_schedule_;
    receive_deps.pace_sampler = pacer_ ? &pace_sampler_ : nullptr;
    receive_deps.rebuild_tracker = &rebuild_tracker_;
    receive_deps.surface_detached = &surface_detached_;
    // P16-P1B32：`started_at` 往后挪累计 pause 时长 —— 于是 worker 的
    // `wall_micros = elapsed_us(started_at, now)` 自动变成"播放时长"，
    // pause 那 2 秒不会被算进 `speed_vs_realtime` 的分母。
    receive_deps.started_at = run_started_at_ + std::chrono::nanoseconds(paused_total_ns_);
    receive_worker_ = std::make_unique<DecodeReceiveWorker>(receive_deps);

    started_ = true;
    launch_workers();
    return {};
}

namespace {

/// 给当前线程起名（P14-C）。
///
/// 为什么值得单独写一句：内核会把 `pthread_setname_np` 设的名字写进
/// `/proc/self/task/<tid>/comm`，而 P14-B 的分配归因正是靠 comm 把 tid 翻译成
/// 人能读的东西。不起名，证据里就只有一串 tid —— 读的人分不清哪行是收帧、哪行是送包。
///
/// Windows 上没有对应机制（`SetThreadDescription` 不进 comm），这里故意留空：
/// 桌面不是交付平台，编一个假名字反而会误导。
void name_current_thread(const char* name) noexcept {
#if defined(__ANDROID__) || defined(__linux__)
    ::pthread_setname_np(::pthread_self(), name);
#endif
    // 同时向分配探针登记：一个"零分配"的线程必须能在证据里出现，
    // 否则读者分不清"它没分配"和"它没跑"。
    core::metrics::alloc_probe::announce_thread(name);
}

}  // namespace

void DecodeSession::launch_workers() {
    // P16-P0 / D2：每个线程体退出前报一次数，让 stop() 能做有界等待。
    // 报数放在 run() 之后、线程真正结束之前；它不抛、不分配、不碰平台。
    if (mode_ == PipelineMode::kSplit) {
        // 起来顺序无关紧要：两段都靠平台队列互相驱动，谁先起都不影响正确性。
        submit_thread_ = std::thread([this] {
            name_current_thread("kivo-submit");
            submit_worker_->run();
            gate_.note_worker_exited();
        });
        receive_thread_ = std::thread([this] {
            name_current_thread("kivo-recv");
            receive_worker_->run();
            gate_.note_worker_exited();
        });
        return;
    }
    // 合并模式下送包与收帧在同一个线程里，名字就叫 `kivo-decode`。
    // 证据里看到这一个名字，就知道这一轮走的是合并管线 —— 比去翻 `pipeline.mode` 更快。
    merged_thread_ = std::thread([this] {
        name_current_thread("kivo-decode");
        run_merged();
        gate_.note_worker_exited();
    });
}

void DecodeSession::run_merged() {
    // P14-I：一圈 = 一次批量送包 + 一次 pump。两段之间的差就是这一圈的耗时，
    // 它必然由 `submit + pump + 圈外` 组成 —— 逐圈记，才知道 234 ms 花在哪。
    std::int64_t previous_iteration_ns = 0;
    while (!gate_.stop_requested()) {
        const std::int64_t iteration_begin_ns = core::pace::monotonic_now_ns();

        // 1. 连送一批（与 P12-B 的批量灌包等价）。
        int submitted = 0;
        while (submitted < kMergedSubmitBatch && !gate_.stop_requested()) {
            if (submit_worker_->pump(false) != StageStep::kProgress) {
                break;  // 输入满或送包段收工：回头收帧
            }
            ++submitted;
        }
        const std::int64_t submit_end_ns = core::pace::monotonic_now_ns();

        // 2. 收一帧。这里允许阻塞（2ms）：否则两段会在 TRY_AGAIN 上互相刷 CPU，
        //    而 P12-B/C5 的基线都是"没进展就阻塞一下"。
        const StageStep step = receive_worker_->pump(true);
        const std::int64_t iteration_end_ns = core::pace::monotonic_now_ns();

        // 三段的边界：圈起点 / 送包结束（= 收帧开始）/ 收帧结束。
        // `pace` 与 `release` 是收帧那段的**内部**，由 worker 自己报。
        if (previous_iteration_ns > 0) {
            present_gap_.note(iteration_end_ns - previous_iteration_ns,
                              submit_end_ns - iteration_begin_ns,
                              iteration_end_ns - submit_end_ns,
                              receive_worker_->last_pace_ns(),
                              receive_worker_->last_release_ns());
        }
        previous_iteration_ns = iteration_end_ns;

        if (step == StageStep::kDone) {
            break;
        }
    }
    receive_worker_->finish();
}

void DecodeSession::stop() {
    gate_.request_stop();

    // P16-P0 / D2：join 之前先做一次有界等待。
    //
    // 目的不是"打断卡死的平台调用"（那需要平台调用级超时 + 看门狗，属 P1），
    // 而是让「线程没按时退」留下一条可读原因：failure_ 的 text() 由 snapshot()
    // 写进 DecodeStats::error，再经 JNI 的 last_error / 证据落到用户可见处。
    // 正常路径下 worker 在几毫秒内退出，这段等待解析为 true，行为逐位不变。
    constexpr std::int64_t kStopJoinBudgetUs = 500'000;
    const std::int32_t expected_workers =
        started_ ? (mode_ == PipelineMode::kSplit ? 2 : 1) : 0;
    if (!gate_.wait_for_workers(expected_workers, kStopJoinBudgetUs)) {
        failure_.mark("stop join timeout: worker threads did not exit within 500 ms; "
                      "join may block on a platform call");
    }

    // join 顺序固定 submit 在前：反过来的话，送包段可能正卡在"等输入空位"上，
    // 而空位的唯一来源（收帧段的 release）已经退出 —— 靠超时也能醒，但那是运气。
    if (submit_thread_.joinable()) {
        submit_thread_.join();
    }
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    if (merged_thread_.joinable()) {
        merged_thread_.join();
    }

    if (started_) {
        // 线程都已退出，这里落最终墙钟与最后一次节拍快照是完全安全的。
        receive_worker_->finish();
        submit_worker_.reset();
        receive_worker_.reset();
    }

    // P14-A：**在渲染证据之前关掉计数**。
    //
    // 第一版没关，结果 `calls_steady` 把"把证据写成 JSON"这一段的分配也算了进去 ——
    // 一份 5 KB 的证据要几百次分配，于是每一次会话都报"稳态分配 130 次"。
    // 那个数字是真的，但它量的不是播放，是**测量本身**。
    // 关掉之后计数仍然可读（`disarm()` 只停止累加），所以 `snapshot()` 拿到的
    // 是"播放期间的分配次数"。
    core::metrics::alloc_probe::disarm();

    started_ = false;
}

core::pace::PaceSummary DecodeSession::pace_summary_after_stop() const {
    if (!pacer_) {
        return {};
    }
    return pacer_->summary();
}

DecodeStats DecodeSession::snapshot() const {
    // P14-A：**测量自己不算进被测量的东西。**
    //
    // `snapshot()` 会复制一批 `std::string`（mime / codec 名 / 时钟档…）。
    // UI 每秒读约 15 次统计，第一版没挂起，于是稳态计数稳定报 15 次/秒 ——
    // 那个数字是真的，但它量的是"读统计"，不是"播放"。
    // 挂起只影响计数，不影响任何播放行为，也不清 `steady` 标记。
    core::metrics::alloc_probe::suspend();
    DecodeStats stats = context_;
    stats.samples_submitted = counters_.samples_submitted.load(std::memory_order_relaxed);
    stats.bytes_submitted = counters_.bytes_submitted.load(std::memory_order_relaxed);
    stats.samples_rejected = counters_.samples_rejected.load(std::memory_order_relaxed);
    stats.frames_received = counters_.frames_received.load(std::memory_order_relaxed);
    stats.frames_released = counters_.frames_released.load(std::memory_order_relaxed);
    stats.frames_rendered = counters_.frames_rendered.load(std::memory_order_relaxed);
    stats.frames_dropped_late = counters_.frames_dropped_late.load(std::memory_order_relaxed);
    stats.frames_discarded_no_surface =
        counters_.frames_discarded_no_surface.load(std::memory_order_relaxed);
    stats.first_frame_latency_us =
        counters_.first_frame_latency_us.load(std::memory_order_relaxed);
    // P13-B：没 seek 过时，`counters` 里的就是冷启动值；seek 过则用单独存的那份。
    // 这两行合起来保证 `cold_start_first_frame_us` 在任何时刻都只表示"第一次起跑"。
    stats.cold_start_first_frame_us = (cold_start_first_frame_us_ >= 0)
                                          ? cold_start_first_frame_us_
                                          : stats.first_frame_latency_us;
    stats.wall_micros = counters_.wall_micros.load(std::memory_order_relaxed);
    stats.completed = counters_.completed.load(std::memory_order_relaxed);

    // P14-A：三条尺子。全部在 `snapshot()` 里现算，不另存 ——
    // 存一份"最后一次算出来的"会和计数分叉，而那正是"恒错字段"的来源。
    stats.metrics.cpu = core::metrics::make_cpu_time_facts(
        cpu_begin_, core::metrics::sample_cpu_time());
    stats.metrics.alloc = core::metrics::make_alloc_facts();
    stats.metrics.buffers = core::metrics::make_buffer_lease_facts(stats.frames_received,
                                                                   stats.frames_released);
    stats.metrics.present_gap = present_gap_.snapshot();

    stats.error = failure_.text();
    stats.pace = pace_sampler_.read();
    stats.surface_rebuild = rebuild_tracker_.snapshot();
    // P16-P1B26：当前媒体位置。`pacer_` 用 atomic 暴露，跨线程读无竞争。
    stats.position_us = pacer_ ? pacer_->position_ns() / 1000 : -1;
    // P16-P1B32：pause 账随证据一起出版 —— 否则 `clock_off` 没有上下文可读。
    stats.pause_count = pause_count_;
    stats.pause_total_us = paused_total_ns_ / 1000;
    // P12-F3B：加密事实原样带上，判定留给 renderer（与 color/seek 同一条理由）。
    stats.encryption = source_.track_encryption();
    stats.color = decoder_.color_facts();
    // seek：`first_frame_us` 用的是 C1 就有的 `first_frame_latency_us`（起跑 → 首帧交出），
    // 所以它天然只反映**最近一段**。没 seek 过时保持 -1，让 `assess_seek()` 去说"没发生"。
    stats.seek = seek_;
    if (stats.seek.requests > 0) {
        stats.seek.first_frame_us =
            counters_.first_frame_latency_us.load(std::memory_order_relaxed);
        // P12-F2：首帧 pts 与两侧落点一起进证据 —— 时延漂亮但跳错位置的运行，
        // 只有靠 pts 与落点对不上才看得出来。
        stats.seek.video_first_pts_us =
            counters_.first_frame_pts_us.load(std::memory_order_relaxed);
    }

    // P15-C：分配事实与 seek 同一条理由 —— 在会话里取一次，渲染层只负责印。
    // 它是进程级的，所以"这一轮"与"这个进程"在这里是同一个数（杀进程才会归零）。
    stats.alloc = core::metrics::make_alloc_facts();
    stats.pipeline_mode = pipeline_mode_name(mode_);
    stats.present_schedule = present_schedule_name(present_schedule_);
    stats.present_frames = present_frames_;
    stats.submit_cost = submit_cost_.load();
    stats.eos_cost = eos_cost_.load();
    stats.receive_cost = receive_cost_.load();
    stats.release_cost = release_cost_.load();

    if (stats.wall_micros > 0) {
        const double seconds = static_cast<double>(stats.wall_micros) / 1'000'000.0;
        stats.decode_fps = static_cast<double>(stats.frames_rendered) / seconds;
        if (stats.media_duration_us > 0) {
            stats.speed_vs_realtime =
                static_cast<double>(stats.media_duration_us) /
                static_cast<double>(stats.wall_micros);
        }
    }
    core::metrics::alloc_probe::resume();
    return stats;
}

}  // namespace kivo::video::mediacodec
