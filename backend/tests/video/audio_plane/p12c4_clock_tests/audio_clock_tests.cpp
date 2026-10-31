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
#include "video/video_plane/backends/aaudio/clock/aaudio_clock_tail.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_clock_trace.hpp"
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

// ── P14-E：异常窗口 ────────────────────────────────────────────────

void t13_trace_window_ends_at_the_event() {
    aaudio::ClockTraceRing ring;
    aaudio::ClockTrace trace;

    // 喂 16 个正常点（每点 +10ms），最后一个是"倒退"点。
    for (int index = 0; index < 15; ++index) {
        aaudio::ClockTraceSample sample;
        sample.mono_ns = index * 10'000'000LL;
        sample.candidate_ns = index * 10'000'000LL;
        sample.accepted_ns = index * 10'000'000LL;
        sample.kind = 0;
        ring.push(sample);
    }
    aaudio::ClockTraceSample bad;
    bad.mono_ns = 15 * 10'000'000LL;
    bad.candidate_ns = 15 * 10'000'000LL;
    bad.accepted_ns = 14 * 10'000'000LL;  // 被守卫压回上一个位置
    bad.kind = 0;
    ring.push(bad);
    ring.snapshot_into(trace, 1);

    CHECK(trace.event_count == 1);
    CHECK(trace.events[0].flags == 1);
    CHECK(trace.events[0].samples == 16);
    // 窗格以事件结尾：pivot 必须是最后一个样本。
    CHECK(trace.events[0].pivot == 15);
    // 最后一行必须**同时**保留设备原值与守卫输出 —— 两者不等才是这一格的证据。
    CHECK(trace.events[0].data[15].candidate_ns == 150'000'000);
    CHECK(trace.events[0].data[15].accepted_ns == 140'000'000);
    // 时间轴必须是连续的：窗口里 mono 严格递增。
    for (int index = 1; index < trace.events[0].samples; ++index) {
        CHECK(trace.events[0].data[index].mono_ns > trace.events[0].data[index - 1].mono_ns);
    }
}

void t14_trace_keeps_at_most_three_events() {
    aaudio::ClockTraceRing ring;
    aaudio::ClockTrace trace;
    for (int event = 0; event < 5; ++event) {
        aaudio::ClockTraceSample sample;
        sample.mono_ns = event;
        ring.push(sample);
        ring.snapshot_into(trace, 2);
    }
    // 超过三次说明"这不是异常而是常态"，那时该改判据而不是加容量 ——
    // 所以只留三格，第 4、5 次**明确丢弃**（而不是悄悄覆盖第一个）。
    CHECK(trace.event_count == 3);
}

// ── P14-G：残差尾部 ────────────────────────────────────────────────

void t15_tail_first_sample_only_sets_baseline() {
    aaudio::ClockTailTracker tracker;
    tracker.note(0, 1000);
    const aaudio::ClockTailFacts facts = tracker.snapshot();
    // 第一个观测没有"上一步"可比 —— 计入样本就等于凭空造一个 0 残差，
    // 而 0 残差是"最健康"的那个读数。
    CHECK(facts.samples == 0);
    CHECK(facts.forward == 0);
    CHECK(facts.backward == 0);
}

void t16_tail_equal_steps_land_in_the_healthiest_bucket() {
    aaudio::ClockTailTracker tracker;
    tracker.note(0, 1000);
    tracker.note(33'000'000, 33'001'000);  // 两者都走了 33 ms
    const aaudio::ClockTailFacts facts = tracker.snapshot();
    CHECK(facts.samples == 1);
    CHECK(facts.forward == 1);
    CHECK(facts.buckets[0] == 1);  // < 1 ms
    CHECK(facts.max_forward_ns == 0);
    CHECK(facts.beyond_threshold() == 0);
}

void t17_tail_clamp_sized_residual_is_counted_beyond_threshold() {
    aaudio::ClockTailTracker tracker;
    tracker.note(60'000'000'000, 1'000);
    // 实测形状（P14-E）：候选跳 +226 ms，墙钟只走 0.5 ms → 残差 225.5 ms。
    // `mono_ns` 是**绝对时刻**，因此第二个观测的墙钟步长 = 501000 − 1000 = 0.5 ms。
    tracker.note(60'226'000'000, 501'000);
    const aaudio::ClockTailFacts facts = tracker.snapshot();
    CHECK(facts.samples == 1);
    CHECK(facts.max_forward_ns == 225'500'000);
    // 225.5 ms 落在 [128, 256) 那一档（档号 8：第 0 档是 [0,1)，
    // 之后每档的下界依次是 1/2/4/…/512 ms）。**档的下界 128 ms 低于阈值 200 ms**，
    // 所以"从档位反推越过数"会漏掉这一批 —— 这条断言钉死"必须逐次判定"。
    CHECK(facts.buckets[8] == 1);
    CHECK(facts.beyond_threshold() == 1);
    CHECK(facts.clamp_threshold_ns == 200'000'000);
}

void t18_tail_backward_residual_is_recorded_as_magnitude() {
    aaudio::ClockTailTracker tracker;
    tracker.note(0, 1000);
    tracker.note(29'240'000, 33'001'000);  // 设备只走 29.24 ms，墙钟走 33 ms
    const aaudio::ClockTailFacts facts = tracker.snapshot();
    CHECK(facts.backward == 1);
    CHECK(facts.forward == 0);
    CHECK(facts.max_backward_ns == 3'760'000);  // 3.76 ms，与 P14-F r2 同量级
    CHECK(facts.buckets[2] == 1);               // [2, 4) ms
    CHECK(facts.beyond_threshold() == 0);
}

void t19_tail_rebase_drops_baseline_but_keeps_counts() {
    aaudio::ClockTailTracker tracker;
    tracker.note(0, 1000);
    tracker.note(33'000'000, 33'001'000);
    CHECK(tracker.snapshot().samples == 1);

    // seek：候选位置整体挪到 90 s。若不丢基线，下一个观测会算出一个
    // 等于 seek 距离的残差，把尾部统计污染成"一次 90 秒的跳变"。
    tracker.drop_baseline();
    tracker.note(90'000'000'000, 33'002'000);
    const aaudio::ClockTailFacts facts = tracker.snapshot();
    CHECK(facts.samples == 1);            // 没有多出一个假样本
    CHECK(facts.max_forward_ns == 0);     // 也没有出现一个巨大的正残差

    // 基线已重建：下一个正常观测继续被采样。
    tracker.note(90'033'000'000, 66'002'000);
    CHECK(tracker.snapshot().samples == 2);
}


void t20_long_gap_with_zero_residual_is_a_false_clamp_not_a_jump() {
    aaudio::ClockTailTracker tracker;
    tracker.note(59'994'238'594, 23'747'823'150'678);

    // 烟测实测形状：查询循环被卡住 287.4 ms，设备位置**同量**前进 287.4 ms。
    // 残差 ≈ −9.7 µs，即"设备完全正常"；但守卫只看位置距上轮最大值，
    // 287 ms > 200 ms 阈值 → 钳掉 200 ms。
    tracker.note(60'281'678'541, 23'748'110'600'365);
    const aaudio::ClockTailFacts facts = tracker.snapshot();

    CHECK(facts.samples == 1);
    CHECK(facts.max_forward_ns == 0);          // 没有正残差
    CHECK(facts.max_backward_ns == 9'740);     // 负残差只有 9.7 µs
    CHECK(facts.beyond_threshold() == 0);      // 尾部里**没有**越阈值的样本

    // 间隔必须自己说出来 —— 否则这次钳制在证据里就成了"设备跳了 287 ms"的假案。
    CHECK(facts.intervals == 1);
    CHECK(facts.long_gaps == 1);
    CHECK(facts.max_mono_step_ns == 287'449'687);
}

void t21_long_gap_is_counted_even_when_candidate_regresses() {
    aaudio::ClockTailTracker tracker;
    tracker.note(1'000'000'000, 5'000'000'000);

    // 间隔 250 ms，但候选位置**倒退**：两个异常同时发生。
    // 间隔若记在"候选无效就早退"之后，这条最该看的组合就丢了。
    tracker.note(900'000'000, 5'250'000'000);
    const aaudio::ClockTailFacts facts = tracker.snapshot();
    CHECK(facts.intervals == 1);
    CHECK(facts.long_gaps == 1);
    CHECK(facts.samples == 0);   // 倒退的样本不进残差分布（各有专职计数）
    CHECK(facts.backward == 0);
}

void t22_gap_plus_residual_can_exceed_threshold_together() {
    aaudio::ClockTailTracker tracker;
    tracker.note(0, 1'000'000);

    // 首轮 30 分钟里的第三次钳制：间隔 190 ms（不到阈值）、残差 20 ms（也不到阈值），
    // 但守卫比的是**候选步长** 210 ms → 照样钳制。
    // "残差越阈值 OR 间隔越阈值"这条判据会漏掉它 —— 所以候选步长必须自己记。
    tracker.note(210'000'000, 191'000'000);
    const aaudio::ClockTailFacts facts = tracker.snapshot();

    CHECK(facts.long_gaps == 0);               // 间隔 190 ms，没越阈值
    CHECK(facts.beyond_threshold() == 0);      // 残差 20 ms，也没越阈值
    CHECK(facts.buckets[5] == 1);              // 第 5 档是 [16, 32) ms
    CHECK(facts.long_candidate_steps == 1);    // 但候选步长 210 ms 越了
    CHECK(facts.max_candidate_step_ns == 210'000'000);
}

void t23_first_mono_is_the_start_of_the_first_interval() {
    aaudio::ClockTailTracker tracker;
    tracker.note(0, 5'000'000'000);
    tracker.note(33'000'000, 5'033'000'000);
    // 起点必须是**第一个间隔的起点**（5.000 s），不是第二个观测的时刻（5.033 s）——
    // 差一个步长看似无害，但事件时刻表全靠这个零点。
    CHECK(tracker.snapshot().first_mono_ns == 5'000'000'000);
}

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
    {"C13 异常窗口以事件结尾", t13_trace_window_ends_at_the_event},
    {"C14 窗口最多三次", t14_trace_keeps_at_most_three_events},
    {"C15 首次观测只建基线", t15_tail_first_sample_only_sets_baseline},
    {"C16 同步前进落在最低档", t16_tail_equal_steps_land_in_the_healthiest_bucket},
    {"C17 钳制幅度计入越阈值", t17_tail_clamp_sized_residual_is_counted_beyond_threshold},
    {"C18 负残差按幅度记录", t18_tail_backward_residual_is_recorded_as_magnitude},
    {"C19 seek 丢基线不清计数", t19_tail_rebase_drops_baseline_but_keeps_counts},
    {"C20 长间隔零残差是假钳制", t20_long_gap_with_zero_residual_is_a_false_clamp_not_a_jump},
    {"C21 候选倒退也记间隔", t21_long_gap_is_counted_even_when_candidate_regresses},
    {"C22 间隔加残差可以一起越线", t22_gap_plus_residual_can_exceed_threshold_together},
    {"C23 零点取第一个间隔的起点", t23_first_mono_is_the_start_of_the_first_interval},
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
