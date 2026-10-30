// audio_clock_tests.cpp — P12-C4 tests: 音频主时钟
//
// 这一组测试的目的：证明时钟在三件坏事发生时仍然可用 ——
//   ① 时间戳倒退（设备重采样/配置切换会让它退一次）
//   ② 时间戳跳变（异常大跳；真实 seek 由 P12-F 重建时钟，不走这条路）
//   ③ 采样率切换（位置必须连续；rate 一变，同一个 frame 会映射到不同时间）
// 外加把"回退三档"压一遍。正常运行时那三档一次都不会触发，
// 恰恰是这种平时不跑的分支最容易在后续改动里被悄悄写坏。
//
// 全部在桌面机器上跑，不需要 Android 设备、不需要声卡。
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_probe.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_clock_source.hpp"
#include "video/video_plane/core/audio/clock/frame_position_extrapolator.hpp"
#include "video/video_plane/core/audio/clock/monotonic_clock_guard.hpp"
#include "video/video_plane/core/audio/clock/system_clock_source.hpp"

namespace core = kivo::video::core::audio;
namespace aaudio = kivo::video::aaudio;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                        \
    do {                                                                   \
        ++g_checked;                                                       \
        if (!(expr)) {                                                     \
            ++g_failed;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);  \
        }                                                                  \
    } while (false)

#define CHECK_NEAR(actual, expected, tolerance)                                          \
    do {                                                                                 \
        ++g_checked;                                                                     \
        const long long check_actual = static_cast<long long>(actual);                   \
        const long long check_expected = static_cast<long long>(expected);               \
        const long long check_tolerance = static_cast<long long>(tolerance);             \
        if (std::llabs(check_actual - check_expected) > check_tolerance) {               \
            ++g_failed;                                                                  \
            std::printf("  FAIL %s:%d  %lld vs %lld (tol %lld)\n", __FILE__, __LINE__,   \
                        check_actual, check_expected, check_tolerance);                  \
        }                                                                                \
    } while (false)

/// 可编程的假时钟：所有读数由测试直接摆布。
class FakeProbe final : public aaudio::AaudioClockProbe {
public:
    [[nodiscard]] aaudio::AaudioTimestampReading read_timestamp() const override { return timestamp; }
    [[nodiscard]] std::int64_t read_frames_read() const override { return frames_read; }
    [[nodiscard]] std::int64_t read_frames_written() const override { return frames_written; }

    aaudio::AaudioTimestampReading timestamp;
    std::int64_t frames_read = -1;
    std::int64_t frames_written = -1;
};

std::int64_t g_now_ns = 0;
std::int64_t fake_now() noexcept { return g_now_ns; }

constexpr std::int64_t kMs = 1'000'000;  // 1 ms in ns

// ── ① 时间戳倒退：必须拒绝，且返回值不回退 ──
void t1_backward_timestamp_is_rejected() {
    core::MonotonicClockGuard guard;
    CHECK(guard.observe(1'000 * kMs).position_ns == 1'000 * kMs);

    const auto decision = guard.observe(900 * kMs);
    CHECK(decision.position_ns == 1'000 * kMs);
    CHECK(decision.rejected_backward);
    CHECK(!decision.clamped_jump);
    CHECK(guard.backward_rejects() == 1);
    CHECK(guard.max_position_ns() == 1'000 * kMs);
}

// ── ② 时间戳跳变：钳制到阈值，不是直接接受 ──
void t2_jump_is_clamped() {
    core::MonotonicClockGuard guard;  // 默认阈值 200 ms
    (void)guard.observe(0);

    const auto decision = guard.observe(5'000 * kMs);  // 跳 5 秒
    CHECK(decision.position_ns == 200 * kMs);
    CHECK(decision.clamped_jump);
    CHECK(!decision.rejected_backward);
    CHECK(guard.jump_clamps() == 1);
}

// 阈值内的正常推进不能被误伤：一个视频帧 16–42 ms，远小于 200 ms。
void t2b_normal_advance_is_not_clamped() {
    core::MonotonicClockGuard guard;
    (void)guard.observe(0);
    for (int i = 1; i <= 10; ++i) {
        const auto decision = guard.observe(i * 41 * kMs);  // 每步 41 ms
        CHECK(!decision.clamped_jump);
        CHECK(decision.position_ns == i * 41 * kMs);
    }
    CHECK(guard.jump_clamps() == 0);
}

// ── ③ 切换采样率：位置必须连续 ──
// 48000 帧在 48 kHz 下是 1000 ms；切到 44.1 kHz 后同一个帧位置
// 若直接重算会变成 1088 ms —— 那 88 ms 的跳变就是这么来的。
void t3_sample_rate_switch_keeps_position_continuous() {
    core::FramePositionExtrapolator extrapolator(48000, 8 * kMs);

    const auto before = extrapolator.extrapolate({48000, 0, 48000}, 0);
    CHECK(before.position_ns == 1'000 * kMs);

    // 同一帧位置、同一时刻，只是采样率变了 → 位置必须原地不动
    const auto after_switch = extrapolator.extrapolate({48000, 0, 44100}, 0);
    CHECK_NEAR(after_switch.position_ns, 1'000 * kMs, 1);

    // 之后按新速率继续走：再 48000 帧 = 48000/44100 s ≈ 1088.4 ms
    const auto advanced = extrapolator.extrapolate({96000, 0, 44100}, 0);
    CHECK_NEAR(advanced.position_ns, 1'000 * kMs + 1'088'435'374, 100);
    CHECK(extrapolator.sample_rate() == 44100);
}

// ── 外推正确性：采样点 + 经过时间 ──
void t4_extrapolation_adds_elapsed_time() {
    core::FramePositionExtrapolator extrapolator(48000, 8 * kMs);

    const auto result = extrapolator.extrapolate({48000, 1'000 * kMs, 48000}, 1'001 * kMs);
    CHECK(result.position_ns == 1'001 * kMs);
    CHECK(result.extrapolated_ns == 1 * kMs);
    CHECK(!result.stale);
}

// ── 陈旧刹车：两个 burst 没更新就停止外推 ──
// 否则时钟会无中生有地往前跑，同步器去追一个根本不存在的进度。
void t5_stale_timestamp_stops_extrapolation() {
    core::FramePositionExtrapolator extrapolator(48000, 8 * kMs);

    const auto result = extrapolator.extrapolate({48000, 1'000 * kMs, 48000}, 1'100 * kMs);
    CHECK(result.stale);
    CHECK(result.extrapolated_ns == 0);
    CHECK(result.position_ns == 1'000 * kMs);  // 冻结在采样点位置
    CHECK(extrapolator.stale_freezes() == 1);
}

// ── 启动期：位置必须是 0，不算失败 ──
// NDK 原文：requestStart 是异步的，这段时间 getTimestamp 返回 INVALID_STATE，
// "should not be considered a fatal error, just try calling again later"。
void t6_startup_returns_zero_without_counting_failure() {
    FakeProbe probe;
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kNotStarted;
    g_now_ns = 0;

    aaudio::AaudioClockSource clock(probe, 48000, 192, &fake_now);
    const auto snapshot = clock.now();

    CHECK(snapshot.position_ns == 0);
    CHECK(!snapshot.stale);
    CHECK(clock.stats().timestamp_not_started == 1);
    CHECK(clock.stats().timestamp_fail == 0);  // 启动期不是故障
}

// ── 启动期不喂守卫：首次成功读数不能被判成跳变 ──
// 若启动期用 0 给守卫打基准，首个真实读数（设备可能已播了 250 ms）
// 会撞上 200 ms 阈值被钳制，凭空砍掉 50 ms 真实进度。
void t7_first_successful_reading_is_not_clamped() {
    FakeProbe probe;
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kNotStarted;
    g_now_ns = 0;

    aaudio::AaudioClockSource clock(probe, 48000, 192, &fake_now);
    (void)clock.now();  // 启动期

    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOk;
    probe.timestamp.frame_position = 12000;  // 250 ms 的位置
    probe.timestamp.time_nanos = 1'000 * kMs;
    g_now_ns = 1'000 * kMs;

    const auto snapshot = clock.now();
    CHECK(!snapshot.jump_clamped);
    CHECK(snapshot.position_ns == 250 * kMs);
    CHECK(clock.stats().jumps_clamped == 0);
}

// ── 回退档 1：主源失败时用 getFramesRead，语义同为"已呈现"，无需补偿 ──
void t8_fallback_to_frames_read() {
    FakeProbe probe;
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kNotStarted;
    g_now_ns = 0;
    aaudio::AaudioClockSource clock(probe, 48000, 192, &fake_now);

    // 先让主源成功一次，脱离"启动期"状态
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOk;
    probe.timestamp.frame_position = 384;  // 8 ms
    probe.timestamp.time_nanos = 5 * kMs;
    g_now_ns = 5 * kMs;
    (void)clock.now();

    // 主源失效（真的失败，不是启动期）
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOther;
    probe.frames_read = 768;  // 16 ms
    g_now_ns = 20 * kMs;

    const auto snapshot = clock.now();
    CHECK(snapshot.source == core::AudioClockSourceKind::kAudioFramesRead);
    CHECK(snapshot.position_ns == 16 * kMs);
    CHECK(clock.stats().fallback_frames_read == 1);
    CHECK(clock.stats().timestamp_fail == 1);
}

// ── 回退档 2：getFramesWritten 必须减 latency，否则会突然超前 ──
void t9_fallback_to_frames_written_subtracts_latency() {
    FakeProbe probe;
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kNotStarted;
    g_now_ns = 0;
    aaudio::AaudioClockSource clock(probe, 48000, 192, &fake_now);

    // 成功一次，同时给出"已写入 − 已呈现"= 1536 帧（32 ms）的管线深度
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOk;
    probe.timestamp.frame_position = 384;
    probe.timestamp.time_nanos = 5 * kMs;
    probe.frames_written = 1920;  // 1920 − 384 = 1536
    g_now_ns = 5 * kMs;
    (void)clock.now();
    CHECK(clock.stats().latency_frames == 1536);

    // 主源与 framesRead 都不可用，只剩 framesWritten
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOther;
    probe.frames_read = -1;
    probe.frames_written = 2304;  // 若不减 latency，会报 48 ms
    g_now_ns = 10 * kMs;

    const auto snapshot = clock.now();
    CHECK(snapshot.source == core::AudioClockSourceKind::kAudioFramesWritten);
    // 2304 − 1536 = 768 帧 = 16 ms
    CHECK(snapshot.position_ns == 16 * kMs);
    CHECK(clock.stats().fallback_frames_written == 1);
}

// ── 三档全废：冻结并标 stale，不许凭空造进度 ──
void t10_all_sources_dead_freezes() {
    FakeProbe probe;
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kNotStarted;
    g_now_ns = 0;
    aaudio::AaudioClockSource clock(probe, 48000, 192, &fake_now);

    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOk;
    probe.timestamp.frame_position = 480;  // 10 ms
    probe.timestamp.time_nanos = 0;
    g_now_ns = 0;
    (void)clock.now();

    probe.timestamp.status = aaudio::AaudioTimestampStatus::kDisconnected;
    probe.frames_read = -1;
    probe.frames_written = -1;

    const auto snapshot = clock.now();
    CHECK(snapshot.stale);
    CHECK(snapshot.position_ns == 10 * kMs);  // 冻结在最后已知位置
    CHECK(clock.stats().published_frozen == 1);
}

// ── clock_source 必须说实话：靠回退撑住就不能报主源 ──
void t11_dominant_source_reflects_reality() {
    FakeProbe probe;
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kNotStarted;
    g_now_ns = 0;
    aaudio::AaudioClockSource clock(probe, 48000, 192, &fake_now);

    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOk;
    probe.timestamp.time_nanos = 0;
    for (int i = 0; i < 5; ++i) {
        probe.timestamp.frame_position = i * 192;
        g_now_ns = i * 4 * kMs;
        (void)clock.now();
    }
    CHECK(clock.stats().dominant_source() == core::AudioClockSourceKind::kAudioTimestamp);

    // 之后连续 10 次只能靠回退 → 主导来源必须变成 frames_read
    probe.timestamp.status = aaudio::AaudioTimestampStatus::kOther;
    probe.frames_read = 960;
    for (int i = 0; i < 10; ++i) {
        (void)clock.now();
    }
    CHECK(clock.stats().dominant_source() == core::AudioClockSourceKind::kAudioFramesRead);
}

// ── 无音轨时的系统时钟：单调、且自报 kSystem（不许冒充音频时钟）──
void t12_system_clock_is_monotonic_and_labelled() {
    core::SystemClockSource system_clock;
    const auto first = system_clock.now();
    CHECK(first.source == core::AudioClockSourceKind::kSystem);

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto second = system_clock.now();

    CHECK(second.position_ns >= first.position_ns);
    CHECK(second.position_ns >= 1 * kMs);  // 至少睡了 2 ms 的一半
    CHECK(second.source == core::AudioClockSourceKind::kSystem);
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"C1 时间戳倒退被拒绝", t1_backward_timestamp_is_rejected},
    {"C2 时间戳跳变被钳制", t2_jump_is_clamped},
    {"C2b 正常推进不被误伤", t2b_normal_advance_is_not_clamped},
    {"C3 切换采样率位置连续", t3_sample_rate_switch_keeps_position_continuous},
    {"C4 外推加入经过时间", t4_extrapolation_adds_elapsed_time},
    {"C5 陈旧时间戳停止外推", t5_stale_timestamp_stops_extrapolation},
    {"C6 启动期返回 0 不计失败", t6_startup_returns_zero_without_counting_failure},
    {"C7 首个成功读数不被钳制", t7_first_successful_reading_is_not_clamped},
    {"C8 回退档1 getFramesRead", t8_fallback_to_frames_read},
    {"C9 回退档2 减 latency", t9_fallback_to_frames_written_subtracts_latency},
    {"C10 三档全废则冻结", t10_all_sources_dead_freezes},
    {"C11 主导来源反映真实", t11_dominant_source_reflects_reality},
    {"C12 系统时钟单调且自报", t12_system_clock_is_monotonic_and_labelled},
};

}  // namespace

int main() {
    std::printf("=== P12-C4 AaudioClockSource tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
