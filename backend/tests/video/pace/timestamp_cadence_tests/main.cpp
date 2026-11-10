#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_source.hpp"
#include "video/video_plane/core/pace/clock_position_tracker.hpp"

namespace audio = kivo::video::core::audio;
namespace pace = kivo::video::core::pace;
namespace aaudio = kivo::video::aaudio;
namespace {
constexpr std::int64_t ms = 1'000'000;
constexpr std::int64_t start = 1'000 * ms;
std::int64_t wall = start;
std::int64_t now() noexcept { return wall; }
int checks = 0, failures = 0;
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; \
    if (failures < 12) std::printf("FAIL line %d: %s\n", __LINE__, #expr); } } while (false)

struct Probe final : aaudio::AaudioClockProbe {
    aaudio::AaudioTimestampReading value;
    aaudio::AaudioTimestampReading read_timestamp() const override { return value; }
    std::int64_t read_frames_read() const override { return -1; }
    std::int64_t read_frames_written() const override { return -1; }
    void at(std::int64_t media_ms) {
        value.status = aaudio::AaudioTimestampStatus::kOk;
        value.frame_position = media_ms * 48;
        value.time_nanos = start + media_ms * ms;
    }
};

void slow_timestamp_publication() {
    wall = start;
    Probe probe;
    probe.at(0);
    aaudio::AaudioClockSource source(probe, 48000, 96, &now);
    pace::ClockPositionTracker tracker(source, {}, &now);
    auto first = tracker.read();
    tracker.anchor(0, first);
    for (int t = 7; t <= 18'000; t += 7) {
        wall = start + t * ms;
        probe.at(t / 50 * 50); // 2ms callback burst; 50ms hardware timestamp publication.
        auto reading = tracker.read();
        CHECK(reading.health == pace::ClockHealth::kFresh);
        CHECK(std::llabs(reading.elapsed_ns - t * ms) <= 1);
    }
    CHECK(tracker.free_run_readings() == 0);
    CHECK(source.stats().monotonic_rejects == 0);
    CHECK(source.stats().timestamp_age_budget_ns == 100 * ms);
    CHECK(source.stats().max_timestamp_age_ns == 49 * ms);
    CHECK(source.stats().timestamp_stale == 0);
    // Truly frozen device: no extrapolation beyond the explicitly bounded budget.
    wall = start + 18'500 * ms;
    CHECK(source.now().stale);
    CHECK(source.stats().timestamp_stale == 1);
}

struct Clock final : audio::AudioClockSource {
    std::int64_t position = 0;
    bool stale = false;
    audio::AudioClockSnapshot now() override {
        audio::AudioClockSnapshot out;
        out.position_ns = position;
        out.stale = stale;
        return out;
    }
};

void recovery(bool long_outage, bool raw_keeps_advancing) {
    wall = start;
    Clock clock;
    pace::ClockPositionTracker tracker(clock, {}, &now);
    tracker.anchor(0, tracker.read());
    const int duration = long_outage ? 1200 : 200;
    for (int t = 20; t <= duration; t += 20) {
        wall = start + t * ms;
        clock.stale = true;
        clock.position = raw_keeps_advancing ? t * ms : 0;
        (void)tracker.read();
    }
    if (long_outage) CHECK(tracker.free_run_readings() > 0);
    wall += 20 * ms;
    clock.position = (duration + 20) * ms;
    clock.stale = false;
    const auto recovered = tracker.read();
    CHECK(recovered.health == pace::ClockHealth::kFresh);
    CHECK(recovered.position_ns == (duration + 20) * ms);
    // The next raw delta must continue from the recovered position exactly once.
    wall += 20 * ms;
    clock.position += 20 * ms;
    CHECK(tracker.read().position_ns == (duration + 40) * ms);
}

void recovery_preserves_rebase() {
    wall = start;
    Clock clock;
    pace::ClockPositionTracker tracker(clock, {}, &now);
    tracker.anchor(0, tracker.read());
    wall += 20 * ms;
    clock.position = 1520 * ms;
    CHECK(tracker.read().position_ns == 20 * ms); // Genuine 1.5s jump removed.
    wall += 20 * ms;
    clock.stale = true;
    (void)tracker.read();
    wall += 20 * ms;
    clock.stale = false;
    clock.position = 1560 * ms;
    CHECK(tracker.read().position_ns == 60 * ms); // Preserve only the 1.5s correction.
    tracker.reset();
    CHECK(tracker.read().position_ns == 1560 * ms);
}

void pause_keeps_lead_origin() {
    wall = start;
    Clock clock;
    pace::ClockPositionTracker tracker(clock, {}, &now);
    tracker.anchor(0, tracker.read());
    wall += 2000 * ms;
    clock.position = 2000 * ms;
    CHECK(tracker.read().position_ns == 2000 * ms);
    wall += 2000 * ms; // Audio and video paused together.
    tracker.skip_pause(wall);
    wall += 20 * ms;
    clock.position += 20 * ms;
    const auto resumed = tracker.read();
    CHECK(!resumed.jump_rebased && !resumed.drift_rebased);
    CHECK(resumed.position_ns == 2020 * ms);
    CHECK(tracker.discontinuity_trace().offset_ns == 0);
}
}

int main() {
    slow_timestamp_publication();
    for (bool long_outage : {false, true})
        for (bool raw_advances : {false, true}) recovery(long_outage, raw_advances);
    recovery_preserves_rebase();
    pause_keeps_lead_origin();
    std::printf("timestamp cadence/recovery: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
