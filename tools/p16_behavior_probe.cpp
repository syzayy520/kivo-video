#include <cmath>
#include <cstdio>
#include <cstring>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_source.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.hpp"
#include "video/video_plane/core/audio/clock/clock_rate_estimator.hpp"
#include "video/video_plane/core/audio/clock/micro_resampler.hpp"
#include "video/video_plane/core/pace/clock_position_tracker.hpp"
#include "video/video_plane/core/pace/frame_pacer.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"

namespace {
// P16-P1B5：固定墙钟，让"同一时刻切采样率"成为确定性场景，不用 sleep、不碰 AAudio。
std::int64_t g_now_ns = 1'000'000'000;
std::int64_t now_fn() noexcept { return g_now_ns; }
/// T16 用：`FramePacer` 在 `sleep_before_present=false` 时不调它，但 `Setup` 要求非空。
void noop_wait(std::int64_t) noexcept {}

class FakeProbe final : public kivo::video::aaudio::AaudioClockProbe {
public:
    explicit FakeProbe(bool ok) noexcept : ok_(ok) {}

    [[nodiscard]] kivo::video::aaudio::AaudioTimestampReading read_timestamp() const override {
        kivo::video::aaudio::AaudioTimestampReading reading;
        if (ok_) {
            reading.status = kivo::video::aaudio::AaudioTimestampStatus::kOk;
            reading.frame_position = 48000;      // 48 kHz 下正好 1000 ms
            reading.time_nanos = g_now_ns;
        } else {
            reading.status = kivo::video::aaudio::AaudioTimestampStatus::kNotStarted;
        }
        return reading;
    }
    [[nodiscard]] std::int64_t read_frames_read() const override { return -1; }
    [[nodiscard]] std::int64_t read_frames_written() const override { return -1; }

private:
    bool ok_;
};
}  // namespace

int main() {
    using kivo::video::aaudio::AaudioClockSource;
    FakeProbe quiet(false);
    FakeProbe running(true);
    AaudioClockSource device(quiet, 48000, 192);
    AaudioClockSource fallback(quiet, 48000, 0);

    int failures = 0;
    const auto check = [&failures](bool ok, const char* name) {
        std::printf("  %-46s %s\n", name, ok ? "PASS" : "FAIL");
        if (!ok) { ++failures; }
    };

    // ---- P16-P1B3：兜底可查询 ----
    check(!device.used_fallback(), "T1a device(192) used_fallback == false");
    check(std::strcmp(device.fallback_reason(), "device") == 0,
          "T1b device(192) reason == device");
    check(fallback.used_fallback(), "T2a fallback(0) used_fallback == true");
    const char* reason = fallback.fallback_reason();
    check(reason != nullptr && std::strlen(reason) > 0 &&
              std::strstr(reason, "unavailable") != nullptr,
          "T2b fallback(0) reason nonempty + explicit");
    const auto snapshot = fallback.now();
    (void)snapshot;
    check(fallback.stats().sample_rate > 0, "T3a fallback sample_rate > 0");
    check(std::strcmp(reason, "device") != 0, "T3b fallback reason differs from device");

    // ---- P16-P1B5：采样率切换 ----
    // T4：set_sample_rate 生效，且 <=0 是防御性空操作。
    device.set_sample_rate(44100);
    check(device.stats().sample_rate == 44100, "T4a set_sample_rate(44100) applied");
    device.set_sample_rate(0);
    check(device.stats().sample_rate == 44100, "T4b set_sample_rate(0) is a no-op");

    // T5：兜底实例切换后，兜底事实不能被擦掉。
    fallback.set_sample_rate(44100);
    check(fallback.used_fallback(), "T5a fallback survives rate switch");
    check(std::strstr(fallback.fallback_reason(), "unavailable") != nullptr,
          "T5b fallback reason survives rate switch");
    (void)fallback.now();
    check(fallback.stats().sample_rate == 44100, "T5c fallback now() after rate switch");

    // T6：位置连续性。frame_position=48000 在 48k 是 1000 ms，在 44.1k 是 1088 ms。
    // 若切换时没把基准挪到"最后已知帧位置"，同一墙钟时刻会凭空跳 88 ms，
    // 足以越过 G7 的 40 ms 阈值，却小到不会触发 jump_limit_ns(1s) 守卫。
    g_now_ns = 1'000'000'000;
    AaudioClockSource switching(running, 48000, 192, &now_fn);
    const auto before = switching.now();
    switching.set_sample_rate(44100);
    const auto after = switching.now();
    std::int64_t delta = after.position_ns - before.position_ns;
    if (delta < 0) { delta = -delta; }
    std::printf("  T6 position delta after 48k->44.1k: %lld ns\n",
                static_cast<long long>(delta));
    check(delta < 1'000'000, "T6 no 88ms jump at fixed wall clock (<1ms)");

    // ---- P16-P1B10：频差估计器（纯逻辑，合成输入）----
    using kivo::video::core::audio::ClockRateEstimator;
    constexpr std::int64_t kStepNs = 16'666'667;  // ~60 fps

    // T7：位置每步比墙钟快 0.5% → 应收敛到 5000 ppm，且相位误差为正。
    ClockRateEstimator fast;
    std::int64_t mono = 0;
    std::int64_t pos = 0;
    ClockRateEstimator::Estimate est{};
    for (int i = 0; i < 300; ++i) {
        mono += kStepNs;
        pos += static_cast<std::int64_t>(static_cast<double>(kStepNs) * 1.005);
        est = fast.note({mono, pos});
    }
    check(std::fabs(est.rate_ppm - 5000.0) < 50.0, "T7a rate_ppm converges to ~5000");
    check(est.phase_error_ns > 0, "T7b phase error positive when clock is fast");

    // T8：1.000 → 应落在 ±10 ppm，相位误差有界。
    ClockRateEstimator steady;
    mono = 0;
    pos = 0;
    for (int i = 0; i < 300; ++i) {
        mono += kStepNs;
        pos += kStepNs;
        est = steady.note({mono, pos});
    }
    check(std::fabs(est.rate_ppm) < 10.0, "T8a rate_ppm ~= 0 for a steady clock");
    check(est.phase_error_ns > -1'000'000 && est.phase_error_ns < 1'000'000,
          "T8b phase error bounded (<1ms)");

    // T9：reset 后必须回到"不可判"，而不是带着上一段的基准继续算。
    fast.reset();
    const auto after_reset = fast.note({0, 0});
    check(!after_reset.valid && after_reset.reason != nullptr && after_reset.reason[0] != '\0',
          "T9a reset -> invalid with non-empty reason");
    check(fast.samples() == 0, "T9b reset clears sample count");

    // ---- P16-P1B11：接线后的 tracker 应把频差估出来（只算、不出版）----
    class RampClock final : public kivo::video::core::audio::AudioClockSource {
    public:
        std::int64_t position = 0;
        bool stale = false;
        kivo::video::core::audio::AudioClockSnapshot now() override {
            kivo::video::core::audio::AudioClockSnapshot snapshot;
            snapshot.position_ns = position;
            snapshot.source = kivo::video::core::audio::AudioClockSourceKind::kAudioTimestamp;
            snapshot.stale = stale;
            return snapshot;
        }
    };

    RampClock ramp;
    const kivo::video::core::pace::PacePolicy policy;
    kivo::video::core::pace::ClockPositionTracker tracker(ramp, policy, &now_fn);
    g_now_ns = 0;
    std::int64_t ramp_pos = 0;
    kivo::video::core::pace::ClockPositionTracker::Reading reading{};
    for (int i = 0; i < 200; ++i) {
        g_now_ns += kStepNs;
        ramp_pos += static_cast<std::int64_t>(static_cast<double>(kStepNs) * 1.005);
        ramp.position = ramp_pos;
        reading = tracker.read();
    }
    check(std::fabs(reading.rate_ppm - 5000.0) < 100.0,
          "T10 tracker reading.rate_ppm ~= 5000");
    check(reading.phase_error_ns > 0, "T11 tracker phase error positive");

    // ---- P16-P1B20：微调重采样内核（纯逻辑，不接线）----
    using kivo::video::core::audio::MicroResampler;
    constexpr std::int32_t kChannels = 2;
    constexpr std::int32_t kFrames = 192;
    float src[kFrames * kChannels];
    float dst[kFrames * kChannels * 2];
    for (std::int32_t i = 0; i < kFrames * kChannels; ++i) {
        src[i] = static_cast<float>(i);
    }

    // T12：ratio=1.0 必须逐位透传、帧数不变。
    MicroResampler passthrough;
    passthrough.set_ratio(1.0);
    const std::int32_t n1 = passthrough.process(src, kFrames, dst, kFrames * 2, kChannels);
    bool same = (n1 == kFrames);
    for (std::int32_t i = 0; same && i < kFrames * kChannels; ++i) {
        same = (dst[i] == src[i]);
    }
    check(same, "T12 ratio=1.0 is bit-exact passthrough");

    // T13/T14：小数比率跨调用累积，总输出 ≈ 输入 × ratio（容差 2 帧）。
    constexpr std::int32_t kCalls = 520;
    const std::int64_t in_total = static_cast<std::int64_t>(kCalls) * kFrames;

    MicroResampler faster;
    faster.set_ratio(1.0002);  // 输出多 0.02%
    std::int64_t out_fast = 0;
    for (int i = 0; i < kCalls; ++i) {
        out_fast += faster.process(src, kFrames, dst, kFrames * 2, kChannels);
    }
    const std::int64_t expect_fast =
        static_cast<std::int64_t>(static_cast<double>(in_total) * 1.0002);
    const std::int64_t diff_fast = out_fast > expect_fast ? out_fast - expect_fast
                                                          : expect_fast - out_fast;
    check(diff_fast <= 2, "T13 ratio=1.0002 accumulates correctly");

    MicroResampler slower;
    slower.set_ratio(0.9998);  // 输出少 0.02%
    std::int64_t out_slow = 0;
    for (int i = 0; i < kCalls; ++i) {
        out_slow += slower.process(src, kFrames, dst, kFrames * 2, kChannels);
    }
    const std::int64_t expect_slow =
        static_cast<std::int64_t>(static_cast<double>(in_total) * 0.9998);
    const std::int64_t diff_slow = out_slow > expect_slow ? out_slow - expect_slow
                                                          : expect_slow - out_slow;
    check(diff_slow <= 2, "T14 ratio=0.9998 accumulates correctly");

    // T15：实时接口 fill() 必须**输出恰好** out_frames 帧 —— AAudio 回调填不满就是断续。
    {
        MicroResampler filler;
        float inbuf[512 * 2];
        float outbuf[192 * 2];
        for (int i = 0; i < 512 * 2; ++i) {
            inbuf[i] = static_cast<float>(i);
        }
        bool all_exact = true;
        const double ratios[3] = {1.0, 1.002, 0.998};
        for (double ratio : ratios) {
            filler.set_ratio(ratio);
            filler.reset();
            const std::int32_t need =
                static_cast<std::int32_t>(std::ceil(192.0 / ratio)) + 2;
            for (int k = 0; k < 50; ++k) {
                if (filler.fill(inbuf, need, outbuf, 192, 2) != 192) {
                    all_exact = false;
                    break;
                }
            }
        }
        check(all_exact, "T15 fill() outputs exactly out_frames");
    }

    // ---- P16-P1B29：pause 恢复的 due 锚点 ----
    {
        RampClock pacer_clock;
        const kivo::video::core::pace::PacePolicy pacer_policy;
        kivo::video::core::pace::FramePacer::Setup pacer_setup;
        pacer_setup.clock = &pacer_clock;
        pacer_setup.policy = pacer_policy;
        pacer_setup.now = &now_fn;
        pacer_setup.wait = &noop_wait;
        pacer_setup.sleep_before_present = false;
        kivo::video::core::pace::FramePacer pacer(pacer_setup);

        g_now_ns = 0;
        pacer_clock.position = 0;
        (void)pacer.arrive(0);
        g_now_ns = 33'000'000;
        pacer_clock.position = 33'000'000;
        (void)pacer.arrive(33'000'000);

        // pause 600 ms：时钟 stale -> FreeRun，due 走 `last_due + fd`。
        pacer_clock.stale = true;
        g_now_ns += 600'000'000;
        const auto paused = pacer.arrive(33'000'000);
        check(paused.lateness_ns > 100'000'000, "T16a pause causes lateness under FreeRun");

        pacer.resume_at(g_now_ns);
        const auto resumed = pacer.arrive(33'000'000);
        // resume 后第一帧会"早到整整一帧"（due = now + fd），这是正常的；
        // 边界放宽 1 ms，否则 `-33ms` 正好卡在严格大于上被判失败。
        check(resumed.lateness_ns > -34'000'000 && resumed.lateness_ns < 33'000'000,
              "T16b resume_at keeps first frame on time");
    }

    // T17：pause 后第一帧不能把 pause 时长"外推进位置"。
    //
    // 真机实测（P16-P1B32，`b6e950ee`）：不做 `skip_pause` 时，
    // `drift_discontinuity` 里会出现 `kind=extrapolated`、`wall_delta=2.044s`、
    // `raw_delta=32ms` 的一条 —— stale 外推把位置凭空推前 2 秒，
    // tracker 从此永久超前，视频为了追上丢掉 58 帧。
    //
    // 这里用**同一个构造**跑两遍：一遍调 `resume_at`（应当不膨胀），
    // 一遍不调（应当膨胀约 2 s）。只测"调了之后很小"是不够的 ——
    // 那样即使 `skip_pause` 整个被删掉、外推量恰好很小，断言也会误判为 PASS。
    {
        const auto measure_pause_inflation = [](bool call_resume_at) -> std::int64_t {
            RampClock clock;
            kivo::video::core::pace::PacePolicy policy;
            kivo::video::core::pace::FramePacer::Setup setup;
            setup.clock = &clock;
            setup.policy = policy;
            setup.now = &now_fn;
            setup.wait = &noop_wait;
            setup.sleep_before_present = false;
            kivo::video::core::pace::FramePacer pacer(setup);

            g_now_ns = 0;
            clock.position = 0;
            (void)pacer.arrive(0);
            g_now_ns = 33'000'000;
            clock.position = 33'000'000;
            (void)pacer.arrive(33'000'000);
            const std::int64_t before = pacer.position_ns();

            // pause 2 s：视频线程停了，所以这里**不调用** arrive（真机就是这样）。
            g_now_ns += 2'000'000'000;
            if (call_resume_at) {
                pacer.resume_at(g_now_ns);
            }
            // resume 后第一帧：音频 timestamp 仍 stale、位置还冻在 33 ms。
            clock.stale = true;
            (void)pacer.arrive(33'000'000);
            return pacer.position_ns() - before;
        };

        const std::int64_t inflated = measure_pause_inflation(false);
        const std::int64_t skipped = measure_pause_inflation(true);
        check(inflated > 1'500'000'000,
              "T17a without resume_at the pause inflates position (bug is real)");
        check(skipped >= 0 && skipped < 100'000'000,
              "T17b resume_at keeps position from absorbing the pause");
    }

    std::printf("RESULT: %s (failures=%d)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
