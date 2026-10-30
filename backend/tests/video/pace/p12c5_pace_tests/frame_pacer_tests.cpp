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
