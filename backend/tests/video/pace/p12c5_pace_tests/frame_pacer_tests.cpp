// frame_pacer_tests.cpp — P12-C5 tests: 呈现节拍器
//
// 这一组测试的目的：证明"什么时候该呈现这一帧"这个判断在四种坏事发生时仍然成立 ——
//   ① pts 重复或倒退（坏片源、首帧重复、时间基切换）
//   ② 时钟位置跳变（换采样率 / seek / 设备重建）
//   ③ 时钟停更（stale 一个 burst 是正常的，停更半秒就是不正常）
//   ④ 连续落后（解不动了，必须丢帧追上，而不是越拖越远）
// 外加把"抖动分布"本身压一遍：分位数算错，G6 的数字就是假的。
//
// 全部在桌面机器上跑：注入假时钟 + 假睡眠，不依赖真实时间、不需要设备。
// 真机只回答"这套判断在真实时钟上跑出来的数是多少"（见 18-p12c5-status.md）。
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/audio/clock/clock_offset_source.hpp"
#include "video/video_plane/core/pace/clock_position_tracker.hpp"
#include "video/video_plane/core/pace/frame_pacer.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"

namespace core = kivo::video::core;
namespace pace = kivo::video::core::pace;
namespace audio = kivo::video::core::audio;

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

#define CHECK_NEAR(actual, expected, tolerance)                                        \
    do {                                                                               \
        ++g_checked;                                                                   \
        const long long check_actual = static_cast<long long>(actual);                 \
        const long long check_expected = static_cast<long long>(expected);             \
        const long long check_tolerance = static_cast<long long>(tolerance);           \
        if (std::llabs(check_actual - check_expected) > check_tolerance) {             \
            ++g_failed;                                                                \
            std::printf("  FAIL %s:%d  %lld vs %lld (tol %lld)\n", __FILE__, __LINE__, \
                        check_actual, check_expected, check_tolerance);                \
        }                                                                              \
    } while (false)

// ---- 假时间轴 --------------------------------------------------------------
// 用全局量而不是对象：`NowFn` 是函数指针，这样注入不需要 `std::function`。

std::int64_t g_now_ns = 0;
std::int64_t g_overshoot_ns = 0;  ///< 模拟"睡过头"，用来制造真实抖动
std::int64_t g_wait_calls = 0;

[[nodiscard]] std::int64_t fake_now() { return g_now_ns; }

void fake_wait(std::int64_t deadline_ns) {
    ++g_wait_calls;
    g_now_ns = deadline_ns + g_overshoot_ns;
}

void reset_time() {
    g_now_ns = 0;
    g_overshoot_ns = 0;
    g_wait_calls = 0;
}

/// 可编程假时钟。位置、陈旧标志全部由测试摆布。
class FakeClock final : public audio::AudioClockSource {
public:
    [[nodiscard]] audio::AudioClockSnapshot now() override {
        ++reads;
        if (follow_wall) {
            // 模拟"音频时钟按实时推进"：位置 = 墙钟 + 固定偏移。
            // 真实的 AAudio 时钟就是这样，只不过它的推进来自设备侧时间戳。
            position_ns = g_now_ns + wall_offset_ns;
        }
        audio::AudioClockSnapshot snapshot;
        snapshot.position_ns = position_ns;
        snapshot.source = kind;
        snapshot.stale = stale;
        snapshot.monotonic_rejected = monotonic_rejected;
        return snapshot;
    }

    std::int64_t position_ns = 0;
    bool follow_wall = false;        ///< true 时位置跟着 g_now_ns 走
    std::int64_t wall_offset_ns = 0; ///< 墙钟与时钟位置的固定差
    audio::AudioClockSourceKind kind = audio::AudioClockSourceKind::kSystem;
    bool stale = false;
    bool monotonic_rejected = false;
    int reads = 0;
};

struct Harness {
    FakeClock clock;
    pace::FramePacer pacer;

    Harness()
        : pacer(pace::FramePacer::Setup{&clock, pace::PacePolicy{}, &fake_now, &fake_wait,
                                        nullptr}) {}
};

constexpr std::int64_t k_ms = 1'000'000;
constexpr std::int64_t kFrame30 = 33'333'333;  // 30fps
constexpr std::int64_t kFrame24 = 41'666'667;  // 24fps

// ---- C5 用例 ---------------------------------------------------------------

void t1_first_frame_is_due_now() {
    Harness h;
    reset_time();
    const pace::PaceDecision first = h.pacer.arrive(0);
    CHECK(first.action == pace::PaceAction::kPresent);
    CHECK(first.due_at_ns == 0);
    CHECK(first.lateness_ns == 0);
    CHECK(first.wait_ns == 0);
}

void t2_early_frame_waits_until_due() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);
    // 墙钟没走、时钟没走，下一帧 pts=20ms → 正好早到 20ms
    const pace::PaceDecision decision = h.pacer.arrive(20 * k_ms);
    CHECK(decision.action == pace::PaceAction::kWait);
    CHECK_NEAR(decision.wait_ns, 20 * k_ms, 1'000);
    CHECK_NEAR(decision.lateness_ns, -20 * k_ms, 1'000);
    CHECK_NEAR(g_now_ns, 20 * k_ms, 1'000);  // 真的睡到了到期时刻
    CHECK(g_wait_calls >= 1);
}

void t3_within_margin_presents_without_waiting() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);
    // pts 前进 1ms、时钟不动 → 早到 1ms，落在 2ms 容差内，不该睡
    const pace::PaceDecision decision = h.pacer.arrive(1 * k_ms);
    CHECK(decision.action == pace::PaceAction::kPresent);
    CHECK(decision.wait_ns == 0);
    CHECK(g_wait_calls == 0);
}

void t4_late_50ms_presents_but_counts() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);
    h.clock.position_ns = kFrame30 + 50 * k_ms;  // 时钟比媒体快 50ms
    const pace::PaceDecision decision = h.pacer.arrive(kFrame30);
    CHECK(decision.action == pace::PaceAction::kPresent);
    CHECK_NEAR(decision.lateness_ns, 50 * k_ms, 1'000);
    CHECK(h.pacer.summary().dropped == 0);
}

void t5_late_150ms_drops_with_reason() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);
    h.clock.position_ns = kFrame30 + 150 * k_ms;
    const pace::PaceDecision decision = h.pacer.arrive(kFrame30);
    CHECK(decision.action == pace::PaceAction::kDrop);
    CHECK(decision.reason[0] != '\0');
    CHECK(h.pacer.summary().dropped == 1);
}

void t6_consecutive_late_triggers_catch_up() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);

    int drops = 0;
    pace::PaceDecision last;
    for (int index = 1; index <= 11; ++index) {
        const std::int64_t pts = static_cast<std::int64_t>(index) * kFrame30;
        h.clock.position_ns = pts + 150 * k_ms;
        g_now_ns = 0;
        last = h.pacer.arrive(pts);
        if (last.action == pace::PaceAction::kDrop) {
            ++drops;
        }
    }
    CHECK(drops == 10);
    CHECK(last.action == pace::PaceAction::kCatchUp);
    CHECK(h.pacer.summary().catch_up_events == 1);
    CHECK(h.pacer.summary().catch_up_frames == 1);

    // 追帧态不会因为"下一帧还是迟到"就退出：仍在追，动作应继续是 catch_up，
    // 而不是退回 drop —— 否则每一帧都要重新累计 10 次才追一次，等于没追。
    const std::int64_t pts = 12 * kFrame30;
    h.clock.position_ns = pts + 150 * k_ms;
    g_now_ns = 0;
    const pace::PaceDecision after = h.pacer.arrive(pts);
    CHECK(after.action == pace::PaceAction::kCatchUp);
    CHECK(h.pacer.summary().catch_up_frames == 2);
}

void t7_catch_up_exits_when_close_enough() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);

    for (int index = 1; index <= 11; ++index) {
        const std::int64_t pts = static_cast<std::int64_t>(index) * kFrame30;
        h.clock.position_ns = pts + 150 * k_ms;
        g_now_ns = 0;
        (void)h.pacer.arrive(pts);
    }
    // 追帧的物理含义：媒体时间跳得比时钟快（中间若干帧被跳过）。
    // 时钟走了 43ms、媒体走了 216ms，落后收窄到 10ms → 应当退出并呈现。
    const std::int64_t pts = 550 * k_ms;
    h.clock.position_ns = 560 * k_ms;
    g_now_ns = 0;
    const pace::PaceDecision escaped = h.pacer.arrive(pts);
    CHECK(escaped.action == pace::PaceAction::kPresent);
    CHECK(h.pacer.summary().catch_up_events == 1);
}

void t8_duplicate_pts_never_goes_backwards() {
    Harness h;
    reset_time();
    const pace::PaceDecision first = h.pacer.arrive(0);
    (void)first;

    const pace::PaceDecision duplicate = h.pacer.arrive(0);
    CHECK(duplicate.action == pace::PaceAction::kWait);
    CHECK_NEAR(duplicate.due_at_ns, 40 * k_ms, 1'000);  // 首帧时长兜底 40ms
    CHECK(duplicate.wait_ns > 0);

    // 倒退的 pts 也不许把 due 拉回去
    const std::int64_t due_before = duplicate.due_at_ns;
    g_now_ns = duplicate.due_at_ns;
    const pace::PaceDecision backwards = h.pacer.arrive(0 - 5 * k_ms);
    CHECK(backwards.due_at_ns >= due_before);
}

void t9_clock_jump_is_rebased() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);

    g_now_ns = 4 * k_ms;              // 墙钟只走 4ms
    h.clock.position_ns = 1500 * k_ms;  // 时钟跳了 1.5s
    const pace::PaceDecision decision = h.pacer.arrive(kFrame24);

    CHECK(h.pacer.tracker().jump_rebases() == 1);
    // 重锚后 due 仍按 pts 走（≈41.67ms），不该因为跳变整体平移
    CHECK_NEAR(decision.due_at_ns, kFrame24, 500'000);
    CHECK(decision.action != pace::PaceAction::kDrop);
}

void t10_stale_short_is_extrapolated() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);

    std::int64_t pts = 0;
    for (int index = 0; index < 8; ++index) {
        g_now_ns += 4 * k_ms;
        pts += 4 * k_ms;
        h.clock.stale = true;  // 位置不动，但不该被当成"时钟停了"
        (void)h.pacer.arrive(pts + 20 * k_ms);
    }
    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.extrapolated_frames == 8);
    CHECK(summary.free_run_frames == 0);
    CHECK(summary.dropped == 0);
}

void t11_long_stale_degrades_to_free_run() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);

    h.clock.stale = true;
    g_now_ns += 40 * k_ms;
    (void)h.pacer.arrive(40 * k_ms);  // 第一次 stale：开始计时

    g_now_ns += 600 * k_ms;
    const pace::PaceDecision degraded = h.pacer.arrive(80 * k_ms);  // 超 500ms：降级

    CHECK(degraded.health == pace::ClockHealth::kFreeRun);
    CHECK(h.pacer.summary().free_run_frames >= 1);
    // 降级当帧从现在起算，不许把陈旧期算成迟到（否则降级自己制造丢帧风暴）
    CHECK_NEAR(degraded.due_at_ns, g_now_ns, 1'000);
    CHECK(degraded.action != pace::PaceAction::kDrop);
}

void t12_follows_clock_when_it_advances() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);
    g_now_ns = kFrame24;
    h.clock.position_ns = kFrame24;
    const pace::PaceDecision decision = h.pacer.arrive(kFrame24);
    CHECK(decision.action == pace::PaceAction::kPresent);
    CHECK_NEAR(decision.due_at_ns, kFrame24, 1'000);
    CHECK_NEAR(decision.lateness_ns, 0, 1'000);
}

void t13_policy_boundaries_are_exact() {
    pace::PacePolicy policy;

    const pace::PaceVerdict at_margin =
        pace::decide(pace::PaceInput{-policy.early_margin_ns, 0, false}, policy);
    CHECK(at_margin.action == pace::PaceAction::kPresent);  // 正好 2ms：不睡

    const pace::PaceVerdict past_margin =
        pace::decide(pace::PaceInput{-policy.early_margin_ns - 1, 0, false}, policy);
    CHECK(past_margin.action == pace::PaceAction::kWait);

    const pace::PaceVerdict at_drop =
        pace::decide(pace::PaceInput{policy.late_drop_ns, 0, false}, policy);
    CHECK(at_drop.action == pace::PaceAction::kPresent);  // 正好 100ms：不丢

    const pace::PaceVerdict past_drop =
        pace::decide(pace::PaceInput{policy.late_drop_ns + 1, 0, false}, policy);
    CHECK(past_drop.action == pace::PaceAction::kDrop);

    CHECK(policy.sane());
    pace::PacePolicy broken;
    broken.early_margin_ns = broken.late_drop_ns;  // 自相矛盾
    CHECK(!broken.sane());
}

void t14_histogram_percentiles_are_accurate() {
    pace::SignedHistogram histogram;
    for (int index = 0; index < 100; ++index) {
        histogram.add(static_cast<std::int64_t>(index) * k_ms);
    }
    CHECK(histogram.count() == 100);
    CHECK_NEAR(histogram.min_ns(), 0, 1);
    CHECK_NEAR(histogram.max_ns(), 99 * k_ms, 1);
    // 桶宽 0.25ms，误差上限 0.125ms；给 0.5ms 余量
    CHECK_NEAR(histogram.percentile_ns(0.50), 49 * k_ms + 125'000, 500'000);
    CHECK_NEAR(histogram.percentile_ns(0.95), 94 * k_ms + 125'000, 500'000);
    CHECK_NEAR(histogram.percentile_ns(0.99), 98 * k_ms + 125'000, 500'000);
}

void t15_histogram_overflow_keeps_extremes_exact() {
    pace::SignedHistogram histogram;
    histogram.add(200 * k_ms);
    histogram.add(-300 * k_ms);
    histogram.add(1 * k_ms);
    CHECK(histogram.above_count() == 1);
    CHECK(histogram.below_count() == 1);
    CHECK_NEAR(histogram.max_ns(), 200 * k_ms, 1);
    CHECK_NEAR(histogram.min_ns(), -300 * k_ms, 1);
    // 溢出时给精确极值，不编桶中心
    CHECK_NEAR(histogram.percentile_ns(0.999), 200 * k_ms, 1);
    CHECK_NEAR(histogram.percentile_ns(0.001), -300 * k_ms, 1);
}

void t16_presented_overshoot_lands_in_jitter() {
    Harness h;
    reset_time();
    const pace::PaceDecision first = h.pacer.arrive(0);
    h.pacer.note_presented(first, 0);

    g_overshoot_ns = 500'000;  // 每次睡都过头 0.5ms
    const pace::PaceDecision second = h.pacer.arrive(20 * k_ms);
    CHECK(second.action == pace::PaceAction::kWait);
    h.pacer.note_presented(second, g_now_ns);

    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.latency_samples == 2);
    CHECK_NEAR(summary.lateness.percentile_ns(0.99), 500'000, 300'000);
    CHECK_NEAR(summary.lateness.max_ns(), 500'000, 300'000);
    // 间隔也应当被记成"多了半毫秒"，而不是"差了一整帧"
    CHECK_NEAR(summary.interval.percentile_ns(0.99), 500'000, 300'000);
}

void t17_percentile_never_exceeds_max() {
    // 全部样本落在同一个桶里时，桶中心可能比真实极值更大。
    // 不钳住就会出现 "p99 = 0.125ms 而 max = 0.079ms" 这种自相矛盾的证据。
    pace::SignedHistogram histogram;
    for (int index = 0; index < 50; ++index) {
        histogram.add(79'000);  // 79us，落在 [0, 0.25ms) 桶
    }
    CHECK_NEAR(histogram.max_ns(), 79'000, 1);
    CHECK(histogram.percentile_ns(0.99) <= histogram.max_ns());
    CHECK(histogram.percentile_ns(0.50) >= histogram.min_ns());
}

// ---- C7 用例 ---------------------------------------------------------------
//
// C7 的唯一改动是"谁来等"：C5 是 CPU 睡到 due_at，C7 是把 due_at 交给合成器。
// 所以这里要钉死三件事：
//   ① 不睡的时候，due_at 必须和睡的时候算得一模一样（否则排程时刻就是错的）；
//   ② 提前量要记进独立直方图，不许污染 lateness / over_budget（否则证据会自相矛盾）；
//   ③ 默认值必须还是 C5 的行为（一个不设就改行为，等于把老路径悄悄换掉）。

void t18_at_time_skips_sleep_but_keeps_due() {
    // 与 C5-2 完全相同的场景，唯一差别是关了自睡。
    Harness h;
    reset_time();
    h.pacer.set_sleep_before_present(false);
    (void)h.pacer.arrive(0);
    const std::int64_t waits_before = g_wait_calls;

    const pace::PaceDecision decision = h.pacer.arrive(20 * k_ms);
    CHECK(decision.action == pace::PaceAction::kWait);
    CHECK(decision.due_at_ns == 20 * k_ms);  // 到期时刻与 C5-2 一致
    CHECK(decision.wait_ns == 0);            // 没睡就没有等待量
    CHECK(g_wait_calls == waits_before);     // 假 wait 一次都没被调
    CHECK(g_now_ns == 0);                    // 时间根本没往前走
}

void t19_schedule_lead_does_not_pollute_lateness() {
    // 核心风险：把"提前 3 帧"记进 lateness，over_budget_frames 会把所有正常帧
    // 判成"超预算"，读证据的人会以为 C7 把播放搞坏了。
    Harness h;
    reset_time();
    h.pacer.set_sleep_before_present(false);

    const pace::PaceDecision first = h.pacer.arrive(0);
    h.pacer.note_scheduled(first, first.due_at_ns - 100 * k_ms);
    const pace::PaceDecision second = h.pacer.arrive(kFrame30);
    h.pacer.note_scheduled(second, second.due_at_ns - 100 * k_ms);

    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.scheduled_samples == 2);
    CHECK(summary.presented == 2);        // 仍然算"已呈现"，drop_ratio 才不会失真
    CHECK(summary.latency_samples == 0);  // lateness 一条都没进
    CHECK_NEAR(summary.schedule_lead.percentile_ns(0.50), -100 * k_ms, 500'000);
    CHECK(summary.over_budget_frames == 0);  // 没有被误判成超预算
}

void t20_default_still_sleeps() {
    Harness h;
    reset_time();
    (void)h.pacer.arrive(0);
    const std::int64_t waits_before = g_wait_calls;
    const pace::PaceDecision decision = h.pacer.arrive(20 * k_ms);
    CHECK(decision.action == pace::PaceAction::kWait);
    CHECK(decision.wait_ns > 0);
    CHECK(g_wait_calls > waits_before);
}

void t21_schedule_lead_spread_ignores_startup_transient() {
    // 判据是"稳态跨度小"，不是"均值大"；也不能用 p99−p01 ——
    // 开播头几帧锚点还没建立，提前量是 0，只占百分之几，却会把 p99 抬起来，
    // 算出来的跨度变成几十毫秒，读者会以为排程不稳。p10–p90 把这段排除。
    Harness h;
    reset_time();
    h.pacer.set_sleep_before_present(false);

    constexpr int kFrames = 100;
    constexpr int kTransient = 5;  // 头 5 帧模拟"锚点未建立"，提前量为 0
    for (int index = 0; index < kFrames; ++index) {
        const pace::PaceDecision decision = h.pacer.arrive(index * kFrame30);
        const std::int64_t lead_ns = index < kTransient ? 0 : -100 * k_ms;
        h.pacer.note_scheduled(decision, decision.due_at_ns + lead_ns);
    }

    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.scheduled_samples == kFrames);

    // 稳健跨度应当接近 0：稳态全是 -100ms。
    const std::int64_t spread = summary.schedule_lead.percentile_ns(0.90) -
                                summary.schedule_lead.percentile_ns(0.10);
    CHECK_NEAR(spread, 0, 300'000);

    // 而 p99−p01 会被那 5% 瞬态撑到一整帧的宽度 —— 这正是不能用它的理由。
    const std::int64_t naive = summary.schedule_lead.percentile_ns(0.99) -
                               summary.schedule_lead.percentile_ns(0.01);
    CHECK(naive > spread);
    CHECK(naive >= 50 * k_ms);
}

// ---- C8 用例 ---------------------------------------------------------------

void t22_lateness_at_present_equals_av_drift() {
    // C8 全部 A/V 论证的地基：
    //   `note_presented()` 回填的 lateness ≡ (音频已走) − (视频 pts 已走)
    // 因为 `due_at = now + (pts − anchor_pts − elapsed)`，两式相减就得到它。
    // 等式成立，"把 lateness 当音画偏差读"才成立；不成立的话 C8 的数字全是别的东西。
    Harness h;
    reset_time();
    h.clock.kind = audio::AudioClockSourceKind::kAudioTimestamp;
    h.clock.follow_wall = true;  // 音频时钟按实时推进（真实 AAudio 时钟就是这样）

    const pace::PaceDecision first = h.pacer.arrive(0);
    h.pacer.note_presented(first, g_now_ns);

    // 睡过头 2ms：模拟负载下"醒了但没立刻交出去"。这就是一轮真实的音画偏差。
    g_overshoot_ns = 2 * k_ms;
    const std::int64_t pts = kFrame30;
    const pace::PaceDecision second = h.pacer.arrive(pts);
    h.pacer.note_presented(second, g_now_ns);

    // 必须走 `now()` 现取：`follow_wall` 时位置是在查询里算出来的，
    // 直接读字段只会拿到上一次查询的值（这里会是 0，然后断言莫名其妙地失败）。
    const std::int64_t audio_elapsed = h.clock.now().position_ns;  // 锚点位置 0
    const std::int64_t video_elapsed = pts - 0;                    // 锚点 pts 0
    const std::int64_t expected = audio_elapsed - video_elapsed;
    CHECK_NEAR(expected, 2 * k_ms, 1'000);
    CHECK_NEAR(h.pacer.summary().lateness.max_ns(), expected, 1'000);

    // 播放速率要用到首末时刻：首帧写一次、之后只更新末帧。
    // 这两个数错位会让速率算成 0 或无穷，而速率是唯一不与节拍器结论循环的检查。
    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.first_present_mono_ns == 0);              // 首帧在 g_now=0 时交出
    CHECK_NEAR(summary.last_present_mono_ns, g_now_ns, 1);  // 末帧就是刚才那次
    CHECK(summary.last_present_mono_ns >= summary.first_present_mono_ns);
}

void t23_at_time_has_no_present_samples() {
    // 反面守卫：`surface_at_time` 路径下 `note_presented()` 根本不被调用，
    // 所以 `lateness` 一条样本都没有。谁把 lateness 当"音画偏差"读，
    // 在默认配置下就会读到一片 0 并宣布完美 —— C8 第一轮真机差点就这么错。
    Harness h;
    reset_time();
    h.clock.kind = audio::AudioClockSourceKind::kAudioTimestamp;
    h.pacer.set_sleep_before_present(false);

    const pace::PaceDecision first = h.pacer.arrive(0);
    h.pacer.note_scheduled(first, first.due_at_ns - 3 * kFrame30);
    const pace::PaceDecision second = h.pacer.arrive(kFrame30);
    h.pacer.note_scheduled(second, second.due_at_ns - 3 * kFrame30);

    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.presented == 2);        // 确实呈现了
    CHECK(summary.latency_samples == 0);  // 但没有一条"呈现时刻"样本
    CHECK(summary.scheduled_samples == 2);
}

void t24_at_time_arrive_lateness_is_submit_lead() {
    // 把 C8 第一轮踩的坑钉成断言：不睡的路径上，`arrive()` 算出来的 lateness
    // 就是"提前多久交出去"的负值，量级 = 提交提前量。它长得像音画偏差，其实不是。
    Harness h;
    reset_time();
    h.pacer.set_sleep_before_present(false);

    (void)h.pacer.arrive(0);
    const pace::PaceDecision decision = h.pacer.arrive(kFrame30);
    // 时钟没走、墙钟没走 → 早到整整一帧
    CHECK_NEAR(decision.lateness_ns, -kFrame30, 1'000);

    const std::int64_t lead = -100 * k_ms;
    h.pacer.note_scheduled(decision, decision.due_at_ns + lead);
    const pace::PaceSummary summary = h.pacer.summary();
    // 两个数说的是同一件事：这一帧被提前了多少。
    CHECK_NEAR(decision.lateness_ns, -1 * kFrame30, 1'000);
    CHECK_NEAR(summary.schedule_lead.percentile_ns(1.0), lead, 1'000);
}

void t25_histogram_bins_replay_percentile() {
    // C8 验收的一半：JSON 里的 p99 必须能被第三方从**分布**重算出来。
    // 这里先在 C++ 里把"只看 bins 重放 percentile"实现一遍，证明映射闭合；
    // `tools/verify_p12c_sync.py` 用完全相同的规则。
    pace::SignedHistogram hist;
    const std::int64_t values[] = {0, -1, 2, 7, -13, 250'000, 900'000, -3'000'000, 12'345'678};
    for (const std::int64_t value : values) {
        hist.add(value);
    }

    // 与 pace_histogram.cxx 逐行对应，只许喂公开的统计量。
    const auto replay = [&hist](double q) -> std::int64_t {
        const std::int64_t count = hist.count();
        if (count == 0) {
            return 0;
        }
        if (q <= 0.0) {
            return hist.min_ns();
        }
        if (q >= 1.0) {
            return hist.max_ns();
        }
        std::int64_t rank = static_cast<std::int64_t>(q * static_cast<double>(count));
        if (static_cast<double>(rank) < q * static_cast<double>(count)) {
            ++rank;
        }
        if (rank < 1) {
            rank = 1;
        }
        const auto clamp_to_range = [&hist](std::int64_t value) {
            if (value < hist.min_ns()) {
                return hist.min_ns();
            }
            if (value > hist.max_ns()) {
                return hist.max_ns();
            }
            return value;
        };
        if (rank <= hist.below_count()) {
            return hist.min_ns();
        }
        std::int64_t accumulated = hist.below_count();
        const auto& bins = hist.bins();
        for (std::int32_t index = 0; index < pace::SignedHistogram::kBucketCount; ++index) {
            accumulated += bins[static_cast<std::size_t>(index)];
            if (rank <= accumulated) {
                return clamp_to_range(pace::SignedHistogram::bucket_center_ns(index));
            }
        }
        return hist.max_ns();
    };

    const double quantiles[] = {0.01, 0.10, 0.50, 0.90, 0.95, 0.99};
    for (const double q : quantiles) {
        CHECK(replay(q) == hist.percentile_ns(q));
    }
    // 导出侧的自洽：桶计数 + 两个溢出桶必须等于样本总数。
    std::int64_t bucket_total = hist.below_count() + hist.above_count();
    for (const std::int64_t bucket : hist.bins()) {
        bucket_total += bucket;
    }
    CHECK(bucket_total == hist.count());
}

void t26_clock_mix_counts_every_frame() {
    // 三者之和必须等于 `frames_decided`。少了就说明有帧漏记，
    // 而漏记会让回退比例算小 —— 正好把"时钟降级"这件事藏起来。
    Harness h;
    reset_time();
    h.clock.kind = audio::AudioClockSourceKind::kAudioTimestamp;
    (void)h.pacer.arrive(0);
    h.clock.kind = audio::AudioClockSourceKind::kAudioFramesRead;
    h.clock.position_ns = kFrame30;
    (void)h.pacer.arrive(kFrame30);
    h.clock.kind = audio::AudioClockSourceKind::kSystem;
    h.clock.position_ns = 2 * kFrame30;
    (void)h.pacer.arrive(2 * kFrame30);

    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.frames_decided == 3);
    CHECK(summary.clock_primary_frames == 1);
    CHECK(summary.clock_fallback_frames == 1);
    CHECK(summary.clock_system_frames == 1);
    CHECK(summary.clock_primary_frames + summary.clock_fallback_frames +
              summary.clock_system_frames ==
          summary.frames_decided);
}

struct TestCase {
    const char* name;
    void (*run)();
};

// ---- S1 时钟"只快一点"也要被拦住（P13-S）----------------------------------
//
// `jump_limit_ns` 只看单次跳变（1 s），于是 1.5% 的**速率**误差永远不触发它：
// 每帧只多走 250 µs，跑一万帧也只是 250 µs。真机满载 8 秒攒了 118.79 ms，
// 而 `jump_rebases` 全程 0 —— 门根本没响（`91-p13r-status.md` §一）。
//
// 这一条钉住：累积漂移超过 45 ms 就必须按墙钟推进一次，且位置与墙钟的距离
// 不能超过"一个门限 + 一帧"。
void s1_cumulative_drift_is_rebased() {
    reset_time();
    FakeClock clock;
    pace::PacePolicy policy;
    pace::ClockPositionTracker tracker(clock, policy, &fake_now);

    const std::int64_t frame_ns = 16'666'667;
    const std::int64_t fast_ns = 16'916'667;  // 每帧多 250 µs ≈ 1.5%
    std::int64_t position = 0;
    pace::ClockPositionTracker::Reading last{};
    for (int i = 0; i < 400; ++i) {
        g_now_ns += frame_ns;
        position += fast_ns;
        clock.position_ns = position;
        last = tracker.read();
    }

    // 8 秒里累积约 60 ms，必须至少触发一次
    CHECK(tracker.drift_rebases() >= 1);
    // 位置的超前量不能超过"门限 + 一帧多走的量"
    CHECK(last.position_ns - g_now_ns <= policy.drift_limit_ns + fast_ns);
    // 门限以内不该误触发（这个假时钟只快 1.5%，不是跳变）
    CHECK(tracker.jump_rebases() == 0);

    // 反例：时钟与墙同步时一次都不该触发
    reset_time();
    FakeClock steady;
    pace::ClockPositionTracker tracker2(steady, policy, &fake_now);
    position = 0;
    for (int i = 0; i < 400; ++i) {
        g_now_ns += frame_ns;
        position += frame_ns;
        steady.position_ns = position;
        (void)tracker2.read();
    }
    CHECK(tracker2.drift_rebases() == 0);
}

// ---- S3 计数真的能从 tracker 走到 summary（P13-T）--------------------------
//
// P13-S 的结题里写过一句"`pace.clock_health.jump_rebases` 是死字段"。核了一遍
// `FramePacer::summary()`：它确实把 tracker 的四个计数搬进 `PaceSummary`（jump / stale /
// backward / monotonic），**不是死字段** —— 那 442 份证据里全 0，是因为 tracker 级
// 的跳变一次都没发生（音频时钟自己在更下层就钳掉了）。
//
// 这一条把整条链钉住：假快钟 → tracker 计数 → `pacer.summary()` → 证据字段。
void s3_drift_counter_reaches_summary() {
    reset_time();
    FakeClock clock;
    pace::FramePacer::Setup setup;
    setup.clock = &clock;
    setup.now = &fake_now;
    setup.wait = &fake_wait;
    pace::FramePacer pacer(setup);

    const std::int64_t frame_ns = 16'666'667;
    const std::int64_t fast_ns = 16'916'667;
    std::int64_t position = 0;
    for (int i = 0; i < 400; ++i) {
        g_now_ns += frame_ns;
        position += fast_ns;
        clock.position_ns = position;
        const pace::PaceDecision decision = pacer.arrive(static_cast<std::int64_t>(i) * frame_ns);
        pacer.note_presented(decision, g_now_ns);
    }
    const pace::PaceSummary summary = pacer.summary();
    CHECK(summary.drift_rebases >= 1);
    CHECK(summary.presented == 400);
}

// ---- S4 断断续续的超前也要被拦住（P13-V）----------------------------------
//
// P13-U 的真机形状：8 秒领先 91.72 ms，但**中间夹着零漂**，最差帧的 `due_step`
// 只有 −3.8 µs。旧判据要求"连续 60 帧正漂"，夹一次零漂就归零，永远到不了门限。
//
// 这一条造同样的形状：每 20 帧超前 300 µs，随后 5 帧不动。旧判据 0 次，
// 新判据（按单帧贡献累积，30 ms 以上才算台阶）必须触发。
void s4_bursty_lead_is_still_rebased() {
    reset_time();
    FakeClock clock;
    pace::PacePolicy policy;
    pace::ClockPositionTracker tracker(clock, policy, &fake_now);

    const std::int64_t frame_ns = 16'666'667;
    std::int64_t position = 0;
    for (int i = 0; i < 400; ++i) {
        g_now_ns += frame_ns;
        const bool burst = (i % 25) < 20;
        position += frame_ns + (burst ? 300'000 : 0);
        clock.position_ns = position;
        (void)tracker.read();
    }
    // 400 帧里 320 帧各多 300 µs ⇒ 累积 96 ms，必须被拦
    CHECK(tracker.drift_rebases() >= 1);
    CHECK(tracker.jump_rebases() == 0);
}

// ---- S5 启动期之外的"中台阶"必须被吃掉（P13-W）----------------------------
//
// 30 ms–1 s 这一档以前两条门都不管：`jump_limit_ns` 要 > 1 s，
// 漂移门要"单帧 < 30 ms 才计入累积"。真机反复出现的正是这一档（P13-R/U/V 各 1/3 轮）。
//
// 新规则：启动期（首帧后 1 s）之外的正向台阶直接按墙钟吃掉。
// 启动期内的台阶**不拦** —— C5-4/C5-5 就在那一段压 50 ms，语义不能改。
void s5_mid_step_is_absorbed_after_startup() {
    reset_time();
    FakeClock clock;
    pace::PacePolicy policy;
    pace::ClockPositionTracker tracker(clock, policy, &fake_now);

    const std::int64_t frame_ns = 16'666'667;
    std::int64_t position = 0;
    // 先走 2 s 的正常时间（越过启动期）
    for (int i = 0; i < 120; ++i) {
        g_now_ns += frame_ns;
        position += frame_ns;
        clock.position_ns = position;
        (void)tracker.read();
    }
    CHECK(tracker.drift_rebases() == 0);

    // 一个 60 ms 的台阶：> 30 ms（台阶门），< 1 s（跳变门不管）
    g_now_ns += frame_ns;
    position += frame_ns + 60000000;
    clock.position_ns = position;
    const pace::ClockPositionTracker::Reading stepped = tracker.read();

    CHECK(stepped.drift_rebased);
    CHECK(tracker.drift_rebases() == 1);
    CHECK(tracker.jump_rebases() == 0);  // 没到 1 s，不该算跳变
}

// ---- S6 领先量必须等于"位置 −（原点 + 墙钟）"（P13-AA）--------------------
//
// P13-Z 在真机上发现：`clock_ahead +73.30 ms` 而 `drift_rebases = 0` ——
// 逐帧累加那本账漏掉了 stale 外推 / 跳变重锚 / 倒退钳制三条推进分支。
// 改成派生（两端之差）之后，这条恒等式**按定义**成立，单测把它钉死：
// 无论位置是被谁推的，领先量都能被同一把尺子量到。
void s6_lead_equals_position_minus_wall() {
    reset_time();
    FakeClock clock;
    pace::PacePolicy policy;
    pace::ClockPositionTracker tracker(clock, policy, &fake_now);

    const std::int64_t frame_ns = 16'666'667;
    std::int64_t position = 0;
    std::int64_t origin_position = 0;
    std::int64_t origin_mono = 0;
    std::int64_t previous_lead = 0;
    std::int64_t final_lead = 0;
    bool first = true;
    for (int i = 0; i < 300; ++i) {
        g_now_ns += frame_ns;
        // 中段混入一次 80 ms 台阶（< 1 s，走正常分支）与几帧停更（走 stale 外推）
        std::int64_t extra = 0;
        if (i == 200) {
            extra = 80'000'000;
        }
        clock.stale = (i >= 120 && i < 125);
        position += frame_ns + extra;
        clock.position_ns = position;
        const auto reading = tracker.read();
        if (first) {
            origin_position = reading.position_ns;
            origin_mono = g_now_ns;
            first = false;
        }
        // 恒等式：这次读到的领先量必须等于两端之差（容差一帧）。
        //
        // 台阶（i = 200）之后**不能**再要求"领先量立刻 ≤ 门限"：P13-AC 之后
        // 修正限速 2 ms/帧，80 ms 的台阶要 20 帧才收敛（那时这条断言会把
        // 正常的收敛过程判成失败 —— 本卡第一版就是这么红的）。
        // 台阶之后该守的是"不增"：每帧最多补一帧的墙钟增量，修正只会让它变小。
        const std::int64_t lead = reading.position_ns - (origin_position + (g_now_ns - origin_mono));
        if (i < 200) {
            CHECK(lead <= policy.drift_limit_ns + frame_ns);
        } else if (i > 200) {
            CHECK(lead <= previous_lead + frame_ns);
        }
        previous_lead = lead;
        final_lead = lead;
    }
    CHECK(tracker.drift_rebases() >= 1);
    // 收敛之后必须回到门限以内 —— 限速改的是"怎么收敛"，不是"收敛到哪"。
    CHECK(final_lead <= policy.drift_limit_ns);
}

// ---- S7 时钟台阶注入：按需触发，且门只吃一次（P13-AB）----------------------
//
// P13-S 到 P13-AA 靠负载撞"时钟跑偏"，20 多轮命中率约 1/3 —— 撞不上就分不清
// "门没用"和"这次没撞上"，于是一整条链从 P13-S 欠到现在。这一条用注入把
// 那件事变成确定的：位置从第 2 s 起持续超前 80 ms，其余全是真的。
//
// 钉两件事：
//   ① 装饰器语义 —— 2 s 之前一毫秒不加，之后**每一帧**都带着这 80 ms
//      （持续偏移，不是一次性脉冲）；
//   ② 门的收敛 —— 复位一次把领先量压到门限，之后**不再反复复位**。
void s7_clock_offset_is_injected_and_gated_once() {
    reset_time();
    FakeClock clock;
    clock.follow_wall = true;  // 位置跟着墙钟走：真实音频时钟的近似
    audio::ClockOffsetSource injected(clock, 80'000'000, 2'000'000'000);

    // ① 起点定在首次读到的那一刻
    g_now_ns += 1'000'000;
    const audio::AudioClockSnapshot first = injected.now();
    const std::int64_t origin = first.position_ns;
    CHECK(!injected.applied());

    // 2 s 之前：位置与内层一致，一分不加
    bool clean_before = true;
    for (int i = 0; i < 100; ++i) {  // 100 × 16.7 ms ≈ 1.67 s
        g_now_ns += 16'666'667;
        const audio::AudioClockSnapshot snapshot = injected.now();
        if (snapshot.position_ns != clock.position_ns) {
            clean_before = false;
        }
    }
    CHECK(clean_before);
    CHECK(!injected.applied());

    // 越过 2 s：从这里开始每一帧都带 80 ms
    g_now_ns += 400'000'000;
    const audio::AudioClockSnapshot armed = injected.now();
    CHECK(injected.applied());
    CHECK(injected.armed_at_position_ns() >= origin + 2'000'000'000);
    CHECK(armed.position_ns == clock.position_ns + 80'000'000);

    bool all_biased = true;
    for (int i = 0; i < 100; ++i) {
        g_now_ns += 16'666'667;
        const audio::AudioClockSnapshot snapshot = injected.now();
        if (snapshot.position_ns != clock.position_ns + 80'000'000) {
            all_biased = false;
        }
    }
    CHECK(all_biased);  // 持续偏移，不是脉冲

    // ② 同一条台阶喂给 tracker：门只吃一次，领先量停在门限上
    reset_time();
    FakeClock clock2;
    clock2.follow_wall = true;
    audio::ClockOffsetSource injected2(clock2, 80'000'000, 2'000'000'000);
    pace::PacePolicy policy;
    pace::ClockPositionTracker tracker(injected2, policy, &fake_now);

    std::int64_t tracker_origin_position = 0;
    std::int64_t tracker_origin_mono = 0;
    for (int i = 0; i < 240; ++i) {  // 4 s
        g_now_ns += 16'666'667;
        const pace::ClockPositionTracker::Reading reading = tracker.read();
        if (i == 0) {
            tracker_origin_position = reading.position_ns;
            tracker_origin_mono = g_now_ns;
        }
    }
    const std::int64_t lead =
        tracker.read().position_ns - (tracker_origin_position + (g_now_ns - tracker_origin_mono));
    CHECK(injected2.applied());
    // P13-AC：一次台阶**一次还完**，且还到 `limit − band` 而不是 `limit`。
    // （第一版在这里限速 2 ms/帧，真机否掉了 —— 事件帧因此从 −45 变 −78。）
    CHECK(tracker.drift_rebases() == 1);
    CHECK(tracker.drift_max_correction_ns() == tracker.drift_corrected_total_ns());
    CHECK(lead <= policy.drift_limit_ns);  // 收敛到门限以内
    CHECK(lead > 0);                       // 没有像 P13-X 那样翻负（过冲）
}

// ---- S8 迟滞带：一次还完，但还到「门限 − 带」，且稳态不再触发（P13-AC）----
//
// P13-AB 的残差停在 45.00 ms：门限同时当"触发阈值"和"收敛目标"，于是下一帧
// 钟再挤 50 µs 就又触发一次，`drift_rebases` 是 5/6/10 而总修正只有 35.2 ms。
//
// 这一条钉三件事：
//   ① 一次台阶只修一次（`rebases == 1`，max == total）—— 没有反复微调；
//   ② 收敛到 `limit − band`（40 ms），不是停在门限下方一点（44）；
//   ③ 收敛之后不再触发（迟滞带把稳态推到阈值以下）。
//
// ⚠️ 第一版这里是"限速 2 ms/帧"。真机把它否了：事件帧 `due_step` 从 −45 变 −78、
// 迟到从 29 变 62、200 ms 台阶多丢 5 帧 —— 限速只在**瞬时台阶**上起作用，且更差；
// 在渐变漂移上触发时超出量只有几毫秒，根本跑不满。
void s8_hysteresis_band_settles_once() {
    reset_time();
    FakeClock clock;
    clock.follow_wall = true;
    audio::ClockOffsetSource injected(clock, 200'000'000, 2'000'000'000);  // 200 ms 大台阶
    pace::PacePolicy policy;
    pace::ClockPositionTracker tracker(injected, policy, &fake_now);

    std::int64_t origin_position = 0;
    std::int64_t origin_mono = 0;
    for (int i = 0; i < 600; ++i) {  // 10 s
        g_now_ns += 16'666'667;
        const pace::ClockPositionTracker::Reading reading = tracker.read();
        if (i == 0) {
            origin_position = reading.position_ns;
            origin_mono = g_now_ns;
        }
    }
    const std::int64_t lead =
        tracker.read().position_ns - (origin_position + (g_now_ns - origin_mono));

    CHECK(injected.applied());
    CHECK(tracker.drift_rebases() == 1);                                   // ① 一次还完
    CHECK(tracker.drift_max_correction_ns() == tracker.drift_corrected_total_ns());
    // 修正总量 = 台阶 − (limit − band) = 200 − 40 = 160 ms
    CHECK(tracker.drift_corrected_total_ns() == 160'000'000);
    // ② 收敛到迟滞带下沿
    CHECK(lead <= policy.drift_limit_ns - policy.drift_band_ns + 1'000'000);
    CHECK(lead > policy.drift_limit_ns - policy.drift_band_ns - 1'000'000);
    // ③ 稳态零新触发
    const std::int64_t rebases_after_settle = tracker.drift_rebases();
    for (int i = 0; i < 120; ++i) {
        g_now_ns += 16'666'667;
        (void)tracker.read();
    }
    CHECK(tracker.drift_rebases() == rebases_after_settle);
}

// ---- S9 速率误差：门能追住，关掉门就一路涨（P13-AD）------------------------
//
// P13-AB/AC 注入的都是**瞬时台阶**，可 8 次真实事件**全是渐变**：8 秒里
// `clock_ahead` 从 0 长到 73–165 ms（1.5%–5% 的速率差）。两者对门的要求不一样 ——
// 台阶考验"当帧还多少"，速率考验"能不能一直追住、不越攒越多"。
//
// 这一条同时跑"门开/门关"两组，因为只有对照组能证明那组数字是门的功劳：
//   · 门开：领先量被压在 [limit−band, limit] 的锯齿里，`drift_rebases` 多次；
//   · 门关：`drift_rebases = 0`，领先量按时钟的速率一路涨到 ppm × 时长。
void s9_rate_error_is_tracked_only_with_gate() {
    constexpr std::int32_t kPpm = 15'000;  // 1.5%，与 P13-R r4 同量级
    const std::int64_t frame_ns = 16'666'667;

    // ── 门开 ──
    reset_time();
    FakeClock clock;
    clock.follow_wall = true;
    audio::ClockOffsetSource injected(clock, 0, 1'000'000'000, kPpm);
    pace::PacePolicy policy;  // drift_gate_enabled 默认 true
    pace::ClockPositionTracker tracker(injected, policy, &fake_now);

    std::int64_t origin_position = 0;
    std::int64_t origin_mono = 0;
    std::int64_t peak_lead = 0;
    for (int i = 0; i < 360; ++i) {  // 6 s
        g_now_ns += frame_ns;
        const auto reading = tracker.read();
        if (i == 0) {
            origin_position = reading.position_ns;
            origin_mono = g_now_ns;
        }
        const std::int64_t lead =
            reading.position_ns - (origin_position + (g_now_ns - origin_mono));
        if (lead > peak_lead) {
            peak_lead = lead;
        }
    }
    CHECK(injected.applied());
    CHECK(tracker.drift_rebases() >= 5);  // 5 s × 1.5% = 75 ms，每 5 ms 越线一次
    // 锯齿的上沿不超过门限 + 一帧（越线那一帧才会修，下一帧才落到 40）
    CHECK(peak_lead <= policy.drift_limit_ns + frame_ns);

    // ── 门关（对照组）──
    reset_time();
    FakeClock clock2;
    clock2.follow_wall = true;
    audio::ClockOffsetSource injected2(clock2, 0, 1'000'000'000, kPpm);
    pace::PacePolicy open_policy;
    open_policy.drift_gate_enabled = false;
    pace::ClockPositionTracker tracker2(injected2, open_policy, &fake_now);

    std::int64_t origin2 = 0;
    std::int64_t mono2 = 0;
    for (int i = 0; i < 360; ++i) {  // 6 s
        g_now_ns += frame_ns;
        const auto reading = tracker2.read();
        if (i == 0) {
            origin2 = reading.position_ns;
            mono2 = g_now_ns;
        }
    }
    const std::int64_t lead_off =
        tracker2.read().position_ns - (origin2 + (g_now_ns - mono2));

    CHECK(tracker2.drift_rebases() == 0);  // 门关着，一次都不该触发
    // 5 s × 1.5% = 75 ms，允许 ±10 ms 的取整与首帧误差
    CHECK(lead_off > 65'000'000);
    CHECK(lead_off < 85'000'000);
}

const TestCase kCases[] = {
    {"C5-1 首帧即刻到期", t1_first_frame_is_due_now},
    {"C5-2 早到则睡到到期", t2_early_frame_waits_until_due},
    {"C5-3 容差内不睡", t3_within_margin_presents_without_waiting},
    {"C5-4 迟到 50ms 仍呈现", t4_late_50ms_presents_but_counts},
    {"C5-5 迟到 150ms 丢弃并记原因", t5_late_150ms_drops_with_reason},
    {"C5-6 连续落后触发追帧", t6_consecutive_late_triggers_catch_up},
    {"C5-7 追到即退出追帧", t7_catch_up_exits_when_close_enough},
    {"C5-8 pts 重复/倒退不倒退", t8_duplicate_pts_never_goes_backwards},
    {"C5-9 时钟跳变重锚", t9_clock_jump_is_rebased},
    {"C5-10 短暂停更按墙钟外推", t10_stale_short_is_extrapolated},
    {"C5-11 停更过久降为自走", t11_long_stale_degrades_to_free_run},
    {"C5-12 随时钟推进正常呈现", t12_follows_clock_when_it_advances},
    {"C5-13 阈值边界精确", t13_policy_boundaries_are_exact},
    {"C5-14 分位数准确", t14_histogram_percentiles_are_accurate},
    {"C5-15 溢出桶保留极值", t15_histogram_overflow_keeps_extremes_exact},
    {"C5-16 睡过头计入抖动", t16_presented_overshoot_lands_in_jitter},
    {"C5-17 分位数不超过极值", t17_percentile_never_exceeds_max},
    {"C7-1 不睡但到期时刻不变", t18_at_time_skips_sleep_but_keeps_due},
    {"C7-2 提前量不污染 lateness", t19_schedule_lead_does_not_pollute_lateness},
    {"C7-3 默认仍然睡", t20_default_still_sleeps},
    {"C7-4 跨度排除启动瞬态", t21_schedule_lead_spread_ignores_startup_transient},
    {"C8-1 呈现时刻 lateness 等于音画偏差", t22_lateness_at_present_equals_av_drift},
    {"C8-2 at_time 路径没有呈现样本", t23_at_time_has_no_present_samples},
    {"C8-3 at_time 的 lateness 是提交提前量", t24_at_time_arrive_lateness_is_submit_lead},
    {"C8-4 分布可独立重放分位数", t25_histogram_bins_replay_percentile},
    {"C8-5 时钟分档逐帧不漏", t26_clock_mix_counts_every_frame},
    {"S1 累积漂移超过 45ms 才重锚", s1_cumulative_drift_is_rebased},
    {"S3 漂移计数走到 summary", s3_drift_counter_reaches_summary},
    {"S4 断断续续的超前也被拦", s4_bursty_lead_is_still_rebased},
    {"S5 启动期外的中台阶被吃掉", s5_mid_step_is_absorbed_after_startup},
    {"S6 领先量等于位置减墙钟", s6_lead_equals_position_minus_wall},
    {"S7 时钟台阶注入且门只吃一次", s7_clock_offset_is_injected_and_gated_once},
    {"S8 迟滞带一次还完且稳态不触发", s8_hysteresis_band_settles_once},
    {"S9 速率误差：门开追住/门关涨到 ppm", s9_rate_error_is_tracked_only_with_gate},
};

}  // namespace

int main() {
    std::printf("=== P12-C5 FramePacer tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
