#include <atomic>
#include <cstdio>

#include "video/video_plane/core/pace/frame_pacer.hpp"

namespace pace = kivo::video::core::pace;
namespace audio = kivo::video::core::audio;

namespace {
int checks = 0, failures = 0;
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; \
    std::printf("FAIL line %d: %s\n", __LINE__, #expr); } } while (false)

constexpr std::int64_t start = 1'000'000'000;
constexpr std::int64_t frame = 40'000'000;
std::int64_t wall = start;
std::atomic<bool> stopped{false};
int waits = 0;
bool cancel_on_wait = false;
std::int64_t longest_wait = 0;
std::int64_t now() { return wall; }
void wait(std::int64_t deadline) {
    ++waits;
    if (deadline - wall > longest_wait) longest_wait = deadline - wall;
    wall = deadline;
    if (cancel_on_wait) stopped.store(true);
}

struct Clock final : audio::AudioClockSource {
    int reads = 0;
    std::int64_t paused = 0;
    std::int64_t offset = 0;
    audio::AudioClockSnapshot now() override {
        ++reads;
        audio::AudioClockSnapshot out;
        out.position_ns = wall - start - paused + offset;
        out.source = audio::AudioClockSourceKind::kAudioTimestamp;
        return out;
    }
};

void reset() {
    wall = start;
    stopped.store(false);
    waits = 0;
    longest_wait = 0;
    cancel_on_wait = false;
}

pace::FramePacer::Setup setup(Clock& clock) {
    pace::FramePacer::Setup out;
    out.clock = &clock;
    out.now = &now;
    out.wait = &wait;
    out.stop_flag = &stopped;
    out.sleep_before_present = false;
    out.policy.max_single_wait_ns = 5'000'000;
    return out;
}

void cancellable_wait() {
    reset();
    Clock clock;
    auto config = setup(clock);
    pace::FramePacer pacer(config);
    cancel_on_wait = true;
    CHECK(!pacer.wait_until(wall + 30'000'000'000));
    CHECK(waits == 1);
    CHECK(longest_wait <= config.policy.max_single_wait_ns);
    const auto cancelled_at = wall;
    CHECK(!pacer.wait_until(wall - 1)); // Stop wins even when deadline is past.
    CHECK(waits == 1);
    stopped.store(false);
    cancel_on_wait = false;
    CHECK(pacer.wait_until(cancelled_at + frame));
    CHECK(wall == cancelled_at + frame);
}

struct Result { pace::PaceSummary summary; int reads; };

Result run(bool pause_pending, bool pause_without_pending, bool scheduled) {
    reset();
    Clock clock;
    pace::FramePacer pacer(setup(clock));
    auto deliver = [&](const pace::PaceDecision& decision) {
        CHECK(pacer.wait_until(decision.due_at_ns));
        if (scheduled) pacer.note_scheduled(decision, wall);
        else pacer.note_presented(decision, wall);
    };
    auto first = pacer.arrive(0);
    deliver(first);
    wall += 10'000'000;
    auto pending = pacer.arrive(frame);
    CHECK(pending.action == pace::PaceAction::kWait);
    const auto initial_due = pending.due_at_ns;
    const auto initial_reads = clock.reads;
    if (pause_pending) {
        for (auto pause : {2'000'000'000LL, 3'000'000'000LL}) {
            cancel_on_wait = true;
            CHECK(!pacer.wait_until(pending.due_at_ns));
            cancel_on_wait = false;
            wall += pause;
            clock.paused += pause;
            pacer.resume_pending(pending, pause, wall);
            stopped.store(false);
            CHECK(clock.reads == initial_reads);
            CHECK(pacer.summary().frames_decided == 2);
            CHECK(pacer.summary().presented == 1);
        }
        CHECK(pending.due_at_ns == initial_due + clock.paused);
        CHECK(pending.media_ns == frame);
    }
    deliver(pending);
    if (pause_without_pending) {
        wall += 2'000'000'000;
        clock.paused += 2'000'000'000;
        pacer.resume_at(wall);
    }
    for (int i = 2; i <= 6; ++i) {
        wall = start + clock.paused + i * frame - 10'000'000;
        deliver(pacer.arrive(i * frame));
    }
    return {pacer.summary(), clock.reads};
}

void compare(bool without_pending, bool scheduled) {
    auto control = run(false, false, scheduled);
    auto resumed = run(!without_pending, without_pending, scheduled);
    const auto& a = control.summary;
    const auto& b = resumed.summary;
    CHECK(resumed.reads == control.reads);
    CHECK(b.frames_decided == 7);
    CHECK(b.presented == a.presented);
    CHECK(b.scheduled_samples == a.scheduled_samples);
    CHECK(b.dropped == 0);
    CHECK(b.undelivered_frames == 0);
    CHECK(b.span_segments == 2);
    CHECK(b.span_frames == (without_pending ? 5 : 6));
    CHECK(b.clock_lead_ns == 0);
    CHECK(b.elapsed_span_ns == b.arrive_mono_span_ns);
    CHECK(b.due_span_ns == b.pts_span_ns);
    CHECK(b.last_present_mono_ns - b.first_present_mono_ns == b.pts_span_ns);
    CHECK(b.interval.count() == a.interval.count());
    CHECK(b.over_budget_frames == 0);
}

void seek_dispositions_survive_reset() {
    reset();
    Clock clock;
    pace::FramePacer pacer(setup(clock));
    for (int run = 0; run < 3; ++run) {
        const auto delivered = pacer.arrive(0);
        CHECK(delivered.action == pace::PaceAction::kPresent);
        pacer.note_presented(delivered, wall);
        const auto stopped_frame = pacer.arrive(frame);
        CHECK(stopped_frame.action == pace::PaceAction::kWait);
        pacer.note_discarded_on_stop(); // Worker has successfully returned this slot.
        const auto without_surface = pacer.arrive(2 * frame);
        CHECK(without_surface.action == pace::PaceAction::kWait);
        pacer.note_discarded_no_surface();
        const auto summary = pacer.summary();
        CHECK(summary.frames_decided == 3 * (run + 1));
        CHECK(summary.presented == run + 1);
        CHECK(summary.discarded_on_stop == run + 1);
        CHECK(summary.discarded_no_surface == run + 1);
        CHECK(summary.undelivered_frames == 2 * (run + 1));
        CHECK(summary.frames_decided == summary.presented + summary.dropped +
              summary.discarded_on_stop + summary.discarded_no_surface);
        if (run != 2) pacer.reset(); // Two seeks; final run ends with stop.
    }
}

void seek_clock_diagnostics_survive_reset(bool repay) {
    reset();
    Clock clock;
    auto config = setup(clock);
    config.policy.step_repay = repay;
    pace::FramePacer pacer(config);
    std::int64_t first_event = 0;
    for (int segment = 0; segment < 3; ++segment) {
        clock.offset = 0;
        pacer.arrive(0);
        CHECK(pacer.summary().clock_lead_ns == 0);
        wall += 1'100'000'000;
        pacer.arrive(1'100'000'000);
        // Four observable clock steps after startup in each seek segment.
        for (int event = 1; event <= 4; ++event) {
            wall += frame;
            clock.offset += 50'000'000;
            pacer.arrive(1'100'000'000 + event * frame);
        }
        const auto summary = pacer.summary();
        const auto trace = summary.drift_rebase_trace;
        const auto disc = summary.drift_discontinuity;
        const int events = 4 * (segment + 1);
        const int sampled = events < 8 ? events : 8;
        CHECK(disc.events == events);
        CHECK(disc.sampled == sampled);
        CHECK(disc.offset_ns == events * 50'000'000LL);
        CHECK(disc.sampled_offset_ns == sampled * 50'000'000LL);
        CHECK(trace.rebases == (repay ? events : 0));
        CHECK(trace.sampled == (repay ? sampled : 0));
        CHECK(trace.rebases == summary.drift_rebases);
        CHECK(trace.max.amount_ns == summary.drift_max_correction_ns);
        if (segment == 0) first_event = disc.first_at_ns;
        CHECK(disc.first_at_ns == first_event);
        for (int i = 1; i < disc.sampled; ++i)
            CHECK(disc.recent[i].at_ns > disc.recent[i - 1].at_ns);
        for (int i = 1; i < trace.sampled; ++i)
            CHECK(trace.recent[i].at_ns > trace.recent[i - 1].at_ns);
        if (segment != 2) {
            pacer.reset();
            CHECK(pacer.summary().drift_discontinuity.events == events);
            CHECK(pacer.summary().drift_rebase_trace.sampled == trace.sampled);
        }
    }
}
}

int main() {
    cancellable_wait();
    seek_dispositions_survive_reset();
    seek_clock_diagnostics_survive_reset(true);
    seek_clock_diagnostics_survive_reset(false);
    for (bool scheduled : {false, true}) {
        compare(false, scheduled);
        compare(true, scheduled);
    }
    std::printf("pause/resume: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
