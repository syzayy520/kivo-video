#include "video/video_plane/backends/mediacodec/session/decode_receive_worker.hpp"

#include "video/video_plane/backends/mediacodec/decode/mediacodec_video_decoder.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_failure_slot.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pace_sampler.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pipeline_counters.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pipeline_gate.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stage_cost.hpp"
#include "video/video_plane/backends/mediacodec/session/surface_rebuild_tracker.hpp"
#include "video/video_plane/core/metrics/alloc_probe.hpp"
#include "video/video_plane/core/pace/frame_pacer.hpp"
#include "video/video_plane/core/pace/pace_now.hpp"

namespace kivo::video::mediacodec {
namespace {

using SteadyClock = std::chrono::steady_clock;

/// 没帧时在平台上等多久。与 C5 相同（2000us）：
/// 这个值同时是"多久空转一次"的上限，太大则停止反应慢，太小则烧 CPU。
///
/// **别把它当成"呈现回压"来调（C6B 已查过，`21-p12c6b-present-backpressure.md`）。**
/// 无节拍时每次 TRY_AGAIN 平均等 1563us，看着像浪费；但那是"解码器确实没解完"的真实等待，
/// 把超时调小只会把同样长的等待切成更多次空转，吞吐不变、CPU 更忙。
/// 而在真实节拍下（24/30/60fps）TRY_AGAIN 只有 0.02–0.31 次/帧、receive 占 wall 不到 3%，
/// 这个超时根本不在关键路径上。改它之前先读那份文档的 §二与 §三。
constexpr std::int64_t kReceiveWaitUs = 2000;

/// EOS 之后若这么久还没有新帧，判定解码器卡死并显式失败。
constexpr std::int64_t kEosStallLimitUs = 5'000'000;

/// 每多少帧把节拍器状态发布一次。取 30：30fps 下每秒一次，
/// 对"看进度"足够，又不至于让锁出现在热路径上。
constexpr std::int64_t kPaceSnapshotIntervalFrames = 30;

/// `kSurfaceAtTime` 路径允许的最大提前量（按帧数计）。
///
/// 取 3 的理由：Surface 的 BufferQueue 通常 3–4 个槽，提前 3 帧正好填满而不溢出。
/// 提前更多不会更快 —— 槽位满了 release 就会阻塞，反而退化成 CPU 等待。
/// 这个数是推出来的，不是量出来的；若实测显示 2 更稳或 4 更好，改这里并记进证据。
constexpr std::int64_t kMaxScheduleLeadFrames = 3;

[[nodiscard]] std::int64_t elapsed_us(SteadyClock::time_point from, SteadyClock::time_point to) {
    return std::chrono::duration_cast<std::chrono::microseconds>(to - from).count();
}

[[nodiscard]] std::int64_t nanos_since(SteadyClock::time_point from) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(SteadyClock::now() - from).count();
}

}  // namespace

DecodeReceiveWorker::DecodeReceiveWorker(Deps deps)
    : deps_(deps), last_progress_at_(deps.started_at) {}

StageStep DecodeReceiveWorker::pump(bool allow_wait) {
    DecodedFrame frame;
    const auto begin = SteadyClock::now();
    // 每圈重置：`last_*` 描述的是"这一圈"，不是"上一圈留下的值"。
    last_pace_ns_ = 0;
    last_release_ns_ = 0;

    const DecodeOutcome outcome =
        deps_.decoder->receive(frame, allow_wait ? kReceiveWaitUs : 0);
    deps_.receive_cost->record(elapsed_us(begin, SteadyClock::now()));

    StageStep step;
    if (outcome.ok() || outcome.end_of_stream()) {
        step = handle_frame(frame);
    } else if (outcome.try_again()) {
        step = check_eos_stall();
    } else {
        step = abort(outcome.message);
    }

    last_pump_ns_ = nanos_since(begin);
    return step;
}

void DecodeReceiveWorker::run() {
    while (!deps_.gate->stop_requested()) {
        if (pump(true) == StageStep::kDone) {
            break;
        }
    }
    finish();
}

StageStep DecodeReceiveWorker::handle_frame(const DecodedFrame& frame) {
    if (frame.end_of_stream) {
        // EOS 缓冲**不是画面**：不进节拍判定、不算呈现，但必须 release 把槽还回去
        // （与"丢帧也必须 release"同一条规矩）。
        //
        // 为什么不计入 `frames_rendered`：它占了一个输出槽但不是一帧内容。
        // C5 的证据里 `samples_submitted=192 / frames_rendered=193` 就是这个 +1，
        // 当时混在"多解出一帧"里没被认出来 —— 那会让 fps 与帧数断言同时偏 1。
        deps_.counters->frames_received.fetch_add(1, std::memory_order_relaxed);

        const auto release_begin = SteadyClock::now();
        const DecodeOutcome released = deps_.decoder->release_frame(frame.index, false);
        deps_.release_cost->record(elapsed_us(release_begin, SteadyClock::now()));
        last_release_ns_ = nanos_since(release_begin);
        if (!released.ok()) {
            return abort(released.message);
        }
        deps_.counters->frames_released.fetch_add(1, std::memory_order_relaxed);
        deps_.gate->signal_input_room();
        deps_.counters->completed.store(true, std::memory_order_relaxed);
        return StageStep::kDone;
    }

    deps_.counters->frames_received.fetch_add(1, std::memory_order_relaxed);

    // ---- 呈现决策 ----
    // 没有节拍器时与 P12-B 一致：收到就交（render=true）。
    // 有节拍器时先问"现在该不该呈现"：
    //  - kCpuPaced：arrive() 内部睡到 due_at，醒了立刻交（C5 结构）；
    //  - kSurfaceAtTime：arrive() 不睡，把 due_at 交给 releaseOutputBufferAtTime，
    //    由合成器在合成时按时间显示（C7）。
    const bool at_time = deps_.present_schedule == PresentSchedule::kSurfaceAtTime;
    bool render = deps_.present_frames;
    core::pace::PaceDecision decision;
    std::int64_t present_at_ns = 0;
    if (deps_.pacer != nullptr && deps_.present_frames) {
        const auto pace_begin = SteadyClock::now();
        decision = deps_.pacer->arrive(frame.presentation_time_us * 1000);
        render = decision.action != core::pace::PaceAction::kDrop &&
                 decision.action != core::pace::PaceAction::kCatchUp;
        if (at_time && render) {
            // 提前量必须受 BufferQueue 深度约束：Surface 只有 3–4 个槽，
            // 一路提前提交会把槽填满，release 反而阻塞在队列上 —— 那就退化成 C5
            // 还多付一层延迟。睡到"最早可以提交的时刻"再交，提前量恒定。
            const std::int64_t cap_ns =
                kMaxScheduleLeadFrames * decision.frame_duration_ns;
            const std::int64_t lead_ns =
                decision.due_at_ns - core::pace::monotonic_now_ns();
            if (lead_ns > cap_ns) {
                core::pace::wait_until_ns(decision.due_at_ns - cap_ns);
            }
        }
        // 呈现（或排程）在场的时刻：release 调用之前的瞬间。晚几微秒无所谓，
        // 但不能用 arrive() 内部睡醒的时刻 —— 那是"睡完了"，不是"交出去了"。
        present_at_ns = core::pace::monotonic_now_ns();
        // 这一圈里"节拍决策"花了多久（含按节拍睡眠）。它是**有意的等待** ——
        // 判读时要先看它有没有吃掉整圈，再谈别的。
        last_pace_ns_ = nanos_since(pace_begin);
    }

    // P12-D1：面摘掉期间一律不渲染。
    // 节拍照跑（时间轴不能停），但缓冲只还回去、不碰 Surface ——
    // 旧 BufferQueue 已经 disconnect，往上交付轻则丢帧、重则在驱动里崩。
    // 单独计数：混进 `frames_dropped_late` 会让换面看起来像"节拍崩了"。
    bool discarded_no_surface = false;
    if (render && deps_.surface_detached != nullptr &&
        deps_.surface_detached->load(std::memory_order_acquire)) {
        render = false;
        discarded_no_surface = true;
        deps_.counters->frames_discarded_no_surface.fetch_add(1, std::memory_order_relaxed);
    }

    const auto release_begin = SteadyClock::now();
    const DecodeOutcome released =
        (at_time && render) ? deps_.decoder->release_frame_at(frame.index, decision.due_at_ns)
                            : deps_.decoder->release_frame(frame.index, render);
    deps_.release_cost->record(elapsed_us(release_begin, SteadyClock::now()));
    last_release_ns_ = nanos_since(release_begin);

    if (!released.ok()) {
        // 这一帧没交出去：输出槽不会回收，继续跑只会越积越死。直接失败。
        return abort(released.message);
    }
    deps_.counters->frames_released.fetch_add(1, std::memory_order_relaxed);

    // P12-D1：换面之后第一帧什么时候真正交出去。
    // 时刻用 `present_at_ns`（release 之前那一瞬）而不是这里的时间 ——
    // 中间隔着一次平台调用，那个差值会算进"恢复耗时"里，而我们想知道的是
    // "画面什么时候回来"，不是"我们自己记账用了多久"。
    if (deps_.rebuild_tracker != nullptr) {
        // `present_at_ns` 只在开了节拍器时才有值；没开节拍时现场取一个，
        // 否则 0 会让"恢复耗时"算成一个荒谬的负数。
        const std::int64_t observed_ns =
            present_at_ns > 0 ? present_at_ns : core::pace::monotonic_now_ns();
        deps_.rebuild_tracker->note_release(observed_ns, render);
    }

    // 输出槽归还 = 解码器又能吐帧 = 输入槽大概率也能再塞一包。
    deps_.gate->signal_input_room();

    const auto now = SteadyClock::now();
    last_progress_at_ = now;

    if (deps_.pacer != nullptr && deps_.present_frames) {
        if (render) {
            // 只有真的呈现了才回填时刻：丢了还记样本会把 p99 算到"丢帧的迟到"上。
            // 两条路径的语义不同，直方图也必须分开 —— 混记会让 over_budget_frames
            // 把"正常提前 3 帧"判成"超预算"。
            if (at_time) {
                deps_.pacer->note_scheduled(decision, present_at_ns);
            } else {
                deps_.pacer->note_presented(decision, present_at_ns);
            }
        }
    }

    if (render) {
        const std::int64_t previous_rendered =
            deps_.counters->frames_rendered.fetch_add(1, std::memory_order_relaxed);
        // P14-A：首帧渲染之后才开始算"稳态分配"。
        // 之前的分配（解码器初始化、vector 扩容、Surface 首建）是启动成本，
        // G3 判的是稳态循环，不是开播那一刻。
        if (previous_rendered == 0) {
            core::metrics::alloc_probe::mark_steady();
        }
        std::int64_t expected = -1;
        deps_.counters->first_frame_latency_us.compare_exchange_strong(
            expected, elapsed_us(deps_.started_at, now), std::memory_order_relaxed);
        // P12-F2：同时记首帧 pts。时延说明"多快"，pts 说明"是不是那一刻的帧"，
        // 只记时延的话，seek 跳错位置（比如落回片头）也会得到漂亮的时延数字。
        std::int64_t expected_pts = -1;
        deps_.counters->first_frame_pts_us.compare_exchange_strong(
            expected_pts, frame.presentation_time_us, std::memory_order_relaxed);
    } else if (discarded_no_surface) {
        // 已经在上面记进 `frames_discarded_no_surface`。
        // **不能再记一次 `frames_dropped_late`** —— 那不是节拍判它太晚，
        // 是当时没有面可上。混记的直接后果：一次 500ms 的切后台会凭空多出 15 个丢帧，
        // 看起来像节拍在换面后崩了。第一轮真机就是这么错的。
    } else {
        // 丢帧也必须 release（render=false）—— 上面已经交出去了，这里只记账。
        deps_.counters->frames_dropped_late.fetch_add(1, std::memory_order_relaxed);
    }

    if (deps_.pacer != nullptr && deps_.present_frames && deps_.pace_sampler != nullptr) {
        ++frames_since_sample_;
        if (frames_since_sample_ >= kPaceSnapshotIntervalFrames) {
            frames_since_sample_ = 0;
            deps_.pace_sampler->publish(deps_.pacer->summary(), deps_.pacer->last_rate_ppm(),
                                    deps_.pacer->last_phase_error_ns());
        }
    }

    return StageStep::kProgress;
}

StageStep DecodeReceiveWorker::check_eos_stall() {
    if (!deps_.gate->submit_finished()) {
        // 送包段还在跑，"暂时没帧"是正常的（解码中）。
        return StageStep::kProgress;
    }
    if (elapsed_us(last_progress_at_, SteadyClock::now()) <= kEosStallLimitUs) {
        return StageStep::kProgress;
    }
    return abort("decoder stalled after end of stream (no frame for " +
                 std::to_string(kEosStallLimitUs / 1'000'000) + "s)");
}

StageStep DecodeReceiveWorker::abort(const std::string& reason) {
    deps_.failure->mark(reason);
    deps_.gate->request_stop();
    return StageStep::kDone;
}

void DecodeReceiveWorker::finish() {
    // 只让第一次生效：谁先结束收帧段，谁就是真实的 wall。
    std::int64_t expected = 0;
    deps_.counters->wall_micros.compare_exchange_strong(
        expected, elapsed_us(deps_.started_at, SteadyClock::now()), std::memory_order_relaxed);

    if (deps_.pacer != nullptr && deps_.pace_sampler != nullptr) {
        // 收尾补采一次：否则最后不足 30 帧的那一截不会出现在证据里。
        frames_since_sample_ = 0;
        deps_.pace_sampler->publish(deps_.pacer->summary(), deps_.pacer->last_rate_ppm(),
                                    deps_.pacer->last_phase_error_ns());
    }
}

}  // namespace kivo::video::mediacodec
