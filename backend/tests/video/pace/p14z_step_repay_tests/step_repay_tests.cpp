// step_repay_tests.cpp — P14-Z tests: 台阶还掉之后，误差落在谁头上
//
// 这一卡不是"改默认值"，是造一个能选臂的探针，好让真机数据来回答：
// 一次 120 ms 的钟台阶，还掉（画面平滑、A/V 差常驻）还是不还（A/V 对齐、画面跳一下）。
//
// 守四条：
//   ① A 臂（默认）：台阶进门，门照旧还它 —— `drift_rebases = 1`、单笔 ≈ 80 ms。
//   ② B 臂（探针）：台阶不进门，门一次都不动 —— `drift_rebases = 0`；
//      但**账照记**（`events = 1`、`offset = 120 ms`，与 A 臂逐位相同）。
//   ③ 没有台阶的一轮：两臂的每个数必须**逐位相同** —— 开关动了无关轮次就不是探针。
//   ④ `reset()` 之后两臂仍然各自成立（不靠残留状态）。
//
// 纯计算：假时钟 + 假墙钟，不依赖真实时间、不需要设备。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/pace/clock_position_tracker.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"

namespace pace = kivo::video::core::pace;
namespace audio = kivo::video::core::audio;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                       \
    do {                                                                  \
        ++g_checked;                                                      \
        if (!(expr)) {                                                    \
            ++g_failed;                                                   \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr); \
        }                                                                 \
    } while (false)

constexpr std::int64_t kMs = 1'000'000;
constexpr std::int64_t kFrame30 = 33'333'333;

std::int64_t g_now_ns = 0;
[[nodiscard]] std::int64_t fake_now() { return g_now_ns; }

class ScriptedClock final : public audio::AudioClockSource {
public:
    [[nodiscard]] audio::AudioClockSnapshot now() override {
        audio::AudioClockSnapshot snapshot;
        snapshot.position_ns = position_ns;
        snapshot.source = audio::AudioClockSourceKind::kAudioTimestamp;
        return snapshot;
    }
    std::int64_t position_ns = 0;
};

struct Rig {
    explicit Rig(bool step_repay) : tracker(clock, make_policy(step_repay), &fake_now) {}

    static pace::PacePolicy make_policy(bool step_repay) {
        pace::PacePolicy policy{};
        policy.drift_gate_enabled = true;
        policy.step_repay = step_repay;
        return policy;
    }

    /// 跑 n 帧正常时间，再注入一拍 120 ms 的台阶，再跑 10 帧。
    void run_with_step(int warmup_frames, std::int64_t step_ns) {
        for (int i = 0; i < warmup_frames; ++i) {
            clock.position_ns += kFrame30;
            g_now_ns += kFrame30;
            (void)tracker.read();
        }
        clock.position_ns += kFrame30 + step_ns;  // 台阶 + 这一帧的正常推进
        g_now_ns += kFrame30;
        (void)tracker.read();
        for (int i = 0; i < 10; ++i) {
            clock.position_ns += kFrame30;
            g_now_ns += kFrame30;
            (void)tracker.read();
        }
    }

    ScriptedClock clock;
    pace::ClockPositionTracker tracker;
};

// ---- ① A 臂：门还它一次 -----------------------------------------------------

void t01_arm_a_repays_the_step() {
    g_now_ns = 0;
    Rig rig(true);
    rig.run_with_step(60, 120 * kMs);  // 60 帧 = 2 s，越过启动期

    const pace::DiscontinuityTrace trace = rig.tracker.discontinuity_trace();
    CHECK(trace.events == 1);
    CHECK(trace.stepped == 1);
    CHECK(trace.offset_ns == 120 * kMs);
    CHECK(rig.tracker.drift_rebases() == 1);
    // 修正 = 120 − (45 − 5) = 80 ms
    CHECK(rig.tracker.drift_max_correction_ns() == 80 * kMs);
}

// ---- ② B 臂：门不动，账照记 -------------------------------------------------

void t02_arm_b_keeps_the_ledger_but_not_the_gate() {
    g_now_ns = 0;
    Rig rig(false);
    rig.run_with_step(60, 120 * kMs);

    const pace::DiscontinuityTrace trace = rig.tracker.discontinuity_trace();
    CHECK(trace.events == 1);
    CHECK(trace.stepped == 1);
    // 与 A 臂逐位相同：开关只影响门的输入，不影响记账。
    CHECK(trace.offset_ns == 120 * kMs);
    CHECK(trace.max.step_ns == 120 * kMs);
    // 门一次都没动。
    CHECK(rig.tracker.drift_rebases() == 0);
    CHECK(rig.tracker.drift_max_correction_ns() == 0);
}

// ---- ③ 没有台阶的一轮：两臂逐位相同 -----------------------------------------

void t03_arms_agree_without_steps() {
    g_now_ns = 0;
    Rig a(true);
    a.run_with_step(120, 0);  // step = 0：全是正常帧
    const std::int64_t a_rebases = a.tracker.drift_rebases();
    const std::int64_t a_max = a.tracker.drift_max_correction_ns();
    const pace::DiscontinuityTrace a_trace = a.tracker.discontinuity_trace();

    g_now_ns = 0;
    Rig b(false);
    b.run_with_step(120, 0);
    const std::int64_t b_rebases = b.tracker.drift_rebases();
    const std::int64_t b_max = b.tracker.drift_max_correction_ns();
    const pace::DiscontinuityTrace b_trace = b.tracker.discontinuity_trace();

    CHECK(a_rebases == b_rebases);
    CHECK(a_max == b_max);
    CHECK(a_trace.events == b_trace.events);
    CHECK(a_trace.offset_ns == b_trace.offset_ns);
    CHECK(a_trace.events == 0);  // 正常跑不应该有台阶
}

// ---- ④ reset 之后两臂各自成立 -----------------------------------------------

void t04_reset_keeps_both_arms_sound() {
    g_now_ns = 0;
    Rig rig(false);
    rig.run_with_step(60, 120 * kMs);
    CHECK(rig.tracker.drift_rebases() == 0);

    rig.tracker.reset();
    CHECK(rig.tracker.discontinuity_trace().events == 0);

    // reset 之后再来一次：B 臂仍然不动门，账仍然记一笔。
    rig.run_with_step(60, 120 * kMs);
    CHECK(rig.tracker.drift_rebases() == 0);
    CHECK(rig.tracker.discontinuity_trace().events == 1);
}

}  // namespace

int main() {
    t01_arm_a_repays_the_step();
    t02_arm_b_keeps_the_ledger_but_not_the_gate();
    t03_arms_agree_without_steps();
    t04_reset_keeps_both_arms_sound();

    if (g_failed == 0) {
        std::printf("P14-Z step_repay: %d checked, 0 failed\n", g_checked);
        return 0;
    }
    std::printf("P14-Z step_repay: %d checked, %d failed\n", g_checked, g_failed);
    return 1;
}
