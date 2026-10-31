// clock_lead_tests.cpp — P14-T tests: 「钟比墙快了多少」能不能从报告里读出来
//
// 这一组守的是四句话：
//   ① `clock_lead_ns` **逐位等于** `elapsed_span − arrive_mono_span`，
//      没有"近似"这一说 —— 它不是测量，是两个已出版的数相减。
//   ② 钟按墙走（1.0×）时它是 0，而**不是**"看起来像 0"：正的、负的都要能测出来。
//   ③ 钟快就正、钟慢就负，方向不许反 —— 反了会把"视频跑快"和"视频跑慢"
//      两个完全不同的病报成同一个。
//   ④ 一帧都没有时它是 0，而那个 0 **不可读作"钟很准"**（可测性由
//      `clock_supplied_frames` 守，判据那边判）。这里只钉住"它不会变成垃圾"。
//
// 纯桌面计算：注入假时钟、假睡眠，不依赖真实时间、不需要设备。
//
// 为什么值得一个独立目标：这一格是 P14-S 那 4 份"成因未查"的答案，
// 而它一旦算错（比如拿 `due_span` 去减），报出来的仍然是"看起来合理的小时数"。
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/pace/frame_pacer.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"

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

std::int64_t g_now_ns = 0;

[[nodiscard]] std::int64_t fake_now() { return g_now_ns; }

/// 睡就是"把时间拨到截止点"。真实世界里 0.3 ms 的过头这里一律不计 ——
/// 本组测的是"钟与墙的差"，不是调度抖动。
void fake_wait(std::int64_t deadline_ns) { g_now_ns = deadline_ns; }

/// 可以跑得比墙快的假时钟。`lead_ppm` = 每百万纳秒多走多少纳秒。
///
/// 真实的那一只（AAudio）没有这么理想：它按块更新、会陈旧、会被外推。
/// 这里刻意给一个**连续**的偏快量，因为要证的是"差能被算出来"，
/// 不是"某种具体的抖动能被算出来"——后者是 `clock_position_tracker` 的活。
class SkewClock final : public audio::AudioClockSource {
public:
    [[nodiscard]] audio::AudioClockSnapshot now() override {
        audio::AudioClockSnapshot snapshot;
        snapshot.position_ns = g_now_ns + g_now_ns / 1'000'000 * lead_ppm;
        snapshot.source = audio::AudioClockSourceKind::kAudioTimestamp;
        return snapshot;
    }

    std::int64_t lead_ppm = 0;
};

struct Harness {
    SkewClock clock;
    pace::FramePacer pacer;

    explicit Harness(bool drift_gate = false) : pacer(make(drift_gate)) {}

    [[nodiscard]] pace::FramePacer make(bool drift_gate) {
        pace::PacePolicy policy{};
        policy.drift_gate_enabled = drift_gate;
        return pace::FramePacer(
            pace::FramePacer::Setup{&clock, policy, &fake_now, &fake_wait, nullptr});
    }
};

constexpr std::int64_t kFrame30 = 33'333'333;
constexpr std::int64_t kFrames = 120;  // 4 秒

/// 跑 n 帧，返回 summary。`lead_ppm` 决定钟快多少，`drift_gate` 决定门开不开。
///
/// **默认门关**：本组要测的是"领先量能不能被算出来"，而不是"门能不能夹住它"。
/// 门开着时钟一快就被夹回 45 ms 以内（`t5` 专门钉这一条），于是 t1 量到的
/// 会是门限而不是钟的快慢 —— 一个测试测错了对象，比没测更糟。
[[nodiscard]] pace::PaceSummary run(std::int64_t lead_ppm, int frames = kFrames,
                                   bool drift_gate = false) {
    g_now_ns = 0;
    Harness h(drift_gate);
    h.clock.lead_ppm = lead_ppm;
    for (int index = 0; index < frames; ++index) {
        const std::int64_t pts = index * kFrame30;
        const pace::PaceDecision decision = h.pacer.arrive(pts);
        if (decision.action != pace::PaceAction::kDrop) {
            h.pacer.note_presented(decision, g_now_ns);
        }
    }
    return h.pacer.summary();
}

// ---- 用例 -----------------------------------------------------------------

void t1_lead_is_exactly_the_difference_of_two_spans() {
    // 钟快 5%（50000 ppm）：4 秒的媒体应由钟在 ~3.81 秒的墙里走完。
    const pace::PaceSummary summary = run(50'000);
    CHECK(summary.presented > 100);
    // 逐位相等 —— 这一格是派生量，任何偏差都说明有人手改过它。
    CHECK(summary.clock_lead_ns ==
          summary.elapsed_span_ns - summary.arrive_mono_span_ns);
    // 钟真的领先，而且量级对（4 秒的 5% ≈ 200 ms）。
    CHECK(summary.clock_lead_ns > 100'000'000);
    CHECK_NEAR(summary.clock_lead_ns, 190'000'000, 60'000'000);
}

void t2_wall_clock_is_zero() {
    // 钟按墙走：领先量必须是 0。1 ppm 都不许有 —— 这里没有噪声源，
    // 有噪声就说明两个跨度不是同一批帧算出来的。
    const pace::PaceSummary summary = run(0);
    CHECK(summary.presented > 100);
    CHECK(summary.clock_lead_ns == 0);
    CHECK(summary.elapsed_span_ns == summary.arrive_mono_span_ns);
}

void t3_direction_is_not_flipped() {
    // 钟慢 3%：领先量为**负**，且同样逐位等于两跨度之差。
    // 方向反了会把"视频跑快"和"视频跑慢"报成同一句话 —— 修法完全不同。
    const pace::PaceSummary summary = run(-30'000);
    CHECK(summary.presented > 100);
    CHECK(summary.clock_lead_ns < 0);
    CHECK(summary.clock_lead_ns ==
          summary.elapsed_span_ns - summary.arrive_mono_span_ns);
    CHECK_NEAR(summary.clock_lead_ns, -120'000'000, 60'000'000);
}

void t5_gate_caps_the_lead() {
    // 门开着的同一只快钟：领先量被夹在门限附近，而不是一路跑到 190 ms。
    //
    // 这一条解释了一整批真机证据（`docs/evidence` 里 58 份门开的轮次，
    // 领先量最大 44.938 ms / 门限 45 ms；门关的 3 份是 103 / 332 / 636 ms）。
    // 也就是说：**P14-S 那 4 份越界，全部落在"门还没出生"或"门关着"的轮次里。**
    const pace::PaceSummary gated = run(50'000, kFrames, true);
    const pace::PaceSummary free_running = run(50'000);
    CHECK(gated.presented > 100);
    CHECK(gated.clock_lead_ns >= 0);
    CHECK(gated.clock_lead_ns < 50'000'000);  // 门限 45 ms + 一帧余量
    // 门确实起作用：自由跑的那一轮至少是它的三倍。
    CHECK(free_running.clock_lead_ns > gated.clock_lead_ns * 3);
}

void t4_empty_session_is_zero_not_garbage() {
    // 一帧都没跑：四跨度全 0，领先量也必然是 0。
    // **这个 0 不可读作"钟很准"** —— 可测性由 `clock_supplied_frames` 守（判据 T1/T2）。
    g_now_ns = 0;
    Harness h;
    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.presented == 0);
    CHECK(summary.clock_supplied_frames == 0);
    CHECK(summary.clock_lead_ns == 0);
}

}  // namespace

int main() {
    t1_lead_is_exactly_the_difference_of_two_spans();
    t2_wall_clock_is_zero();
    t3_direction_is_not_flipped();
    t4_empty_session_is_zero_not_garbage();
    t5_gate_caps_the_lead();

    // 把两个量级打出来：状态卡里引用的就是这两个数，将来有人改策略时
    // 一眼能看出"门还在不在起作用"。
    const pace::PaceSummary free_run = run(50'000);
    const pace::PaceSummary gated = run(50'000, kFrames, true);
    std::printf("  lead: 钟快 5%% 自由跑 %.3f ms / 门开 %.3f ms\n",
                static_cast<double>(free_run.clock_lead_ns) / 1e6,
                static_cast<double>(gated.clock_lead_ns) / 1e6);
    std::printf("p14t clock_lead: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
