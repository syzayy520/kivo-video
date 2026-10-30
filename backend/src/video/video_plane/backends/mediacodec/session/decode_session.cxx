#include "video/video_plane/backends/mediacodec/session/decode_session.hpp"

#include "video/video_plane/backends/mediacodec/decode/mediaformat_color_io.hpp"
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
    const HdrAssessment hdr = classify_hdr(source_facts);

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
    const DecodeOutcome configured =
        decoder_.configure(info.mime, source_.track_format(), window);
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
    pace_sampler_.publish(pacer_->summary());
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

SessionOutcome DecodeSession::begin_run() {
    counters_.reset();
    gate_.reset_for_start();
    if (pacer_) {
        // 锚点必须在首帧到达时建立；上一轮留下的历史会让首帧被判迟到（C5 的结论）。
        pacer_->reset();
    }

    run_started_at_ = SteadyClock::now();

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
    receive_deps.started_at = run_started_at_;
    receive_worker_ = std::make_unique<DecodeReceiveWorker>(receive_deps);

    started_ = true;
    launch_workers();
    return {};
}

void DecodeSession::launch_workers() {
    if (mode_ == PipelineMode::kSplit) {
        // 起来顺序无关紧要：两段都靠平台队列互相驱动，谁先起都不影响正确性。
        submit_thread_ = std::thread([this] { submit_worker_->run(); });
        receive_thread_ = std::thread([this] { receive_worker_->run(); });
        return;
    }
    merged_thread_ = std::thread([this] { run_merged(); });
}

void DecodeSession::run_merged() {
    while (!gate_.stop_requested()) {
        // 1. 连送一批（与 P12-B 的批量灌包等价）。
        int submitted = 0;
        while (submitted < kMergedSubmitBatch && !gate_.stop_requested()) {
            if (submit_worker_->pump(false) != StageStep::kProgress) {
                break;  // 输入满或送包段收工：回头收帧
            }
            ++submitted;
        }
        // 2. 收一帧。这里允许阻塞（2ms）：否则两段会在 TRY_AGAIN 上互相刷 CPU，
        //    而 P12-B/C5 的基线都是"没进展就阻塞一下"。
        if (receive_worker_->pump(true) == StageStep::kDone) {
            break;
        }
    }
    receive_worker_->finish();
}

void DecodeSession::stop() {
    gate_.request_stop();

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

    stats.error = failure_.text();
    stats.pace = pace_sampler_.read();
    stats.surface_rebuild = rebuild_tracker_.snapshot();
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
