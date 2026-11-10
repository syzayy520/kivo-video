// discontinuity_tests.cpp — P14-Y tests: 钟的不连续那本账
//
// 这一组守九句话：
//   ① 账要对：`Σ step` 就是 `offset`，分类计数加起来就是 `events`。
//   ② 环只留 8 笔，但**总账不许被环截断** —— 第 9 笔进来时，
//      `sampled_offset` 扣掉被挤走那一笔，`offset` 一分不少。
//   ③ `max` 按绝对值取：一笔 −300 ms 必须赢过 +5 ms，否则"最大台阶"是假的。
//   ④ 钟停一次、再补回来：账上**两笔都记下来了**（外推 + 补回），
//      而门**照旧还它一次** —— 这是 P13-S/AB 的契约（S5–S7 守着），本卡不推翻，
//      只把那笔"一共多少、什么时候、长什么样"出版出来。
//   ⑤ 有 `monotonic_rejected` 标志时，分类是它，不是 `position_step`（一帧只记一笔）。
//   ⑥ 启动期（首帧后 1 s）的台阶不记账，与"启动期不拦"同一个门。
//   ⑦ 门关着时账照记 —— 记账不该被门的状态污染。
//   ⑧ 稳态一轮：零事件、零 offset、门零动作。改了记账不许影响正常轮次。
//   ⑨ `reset()` 把账清干净：换片源之后"上一段吃过多少台阶"不能算进这一段。
//   ⑪ 丢帧要有现场（P14-AG）：环留 16 笔、`truncated` 在 17 笔置位、
//      按时间升序、`reset` 清零。计数之外还要"在哪儿"，否则"丢了 24 帧"没法判读。
//   ⑩ 跳变被丢掉多少要单独记（P14-AF）：`jump_max_ns` 取**绝对值最大**的那一笔、
//      **带符号**保留方向，且只有跳变进这一格（台阶走 `offset_ns`，两者不许混）。
//
// 纯计算：假时钟 + 假墙钟，不依赖真实时间、不需要设备。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/pace/clock_position_tracker.hpp"
#include "video/video_plane/core/pace/pace_discontinuity_ledger.hpp"
#include "video/video_plane/core/pace/pace_drop_trace.hpp"
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
constexpr std::int64_t kFrame30 = 33'333'333;  // 30 fps

// ---- 假时钟 / 假墙钟 -------------------------------------------------------

std::int64_t g_now_ns = 0;

[[nodiscard]] std::int64_t fake_now() { return g_now_ns; }

class ScriptedClock final : public audio::AudioClockSource {
public:
    [[nodiscard]] audio::AudioClockSnapshot now() override {
        audio::AudioClockSnapshot snapshot;
        snapshot.position_ns = position_ns;
        snapshot.source = audio::AudioClockSourceKind::kAudioTimestamp;
        snapshot.stale = stale;
        snapshot.monotonic_rejected = rejected;
        return snapshot;
    }

    std::int64_t position_ns = 0;
    bool stale = false;
    bool rejected = false;
};

/// 一台节拍器所需的全部东西：可编排的钟 + 策略 + 被测量的 tracker。
struct Rig {
    explicit Rig(bool gate) : tracker(clock, make_policy(gate), &fake_now) {}

    static pace::PacePolicy make_policy(bool gate) {
        pace::PacePolicy policy{};
        policy.drift_gate_enabled = gate;
        return policy;
    }

    ScriptedClock clock;
    pace::ClockPositionTracker tracker;
};

/// 跑 n 帧"钟与墙同步"的正常帧。
void run_normal(Rig& rig, int frames) {
    for (int i = 0; i < frames; ++i) {
        rig.clock.position_ns += kFrame30;
        g_now_ns += kFrame30;
        (void)rig.tracker.read();
    }
}

// ---- ① 账要对 ---------------------------------------------------------------

void t01_ledger_adds_up() {
    pace::DiscontinuityLedger ledger;
    ledger.note(pace::DiscontinuityKind::kMonotonicRejected, 100 * kMs, 33 * kMs, 0,
                -33 * kMs, 40 * kMs, 7 * kMs);
    ledger.note(pace::DiscontinuityKind::kPositionStep, 200 * kMs, 33 * kMs, 146 * kMs,
                113 * kMs, 7 * kMs, 120 * kMs);
    ledger.note(pace::DiscontinuityKind::kExtrapolated, 300 * kMs, 33 * kMs, 0, 0, 120 * kMs,
                120 * kMs);

    const pace::DiscontinuityTrace trace = ledger.trace();
    CHECK(trace.events == 3);
    CHECK(trace.rejected == 1);
    CHECK(trace.stepped == 1);
    CHECK(trace.extrapolated == 1);
    CHECK(trace.clamped == 0);
    CHECK(trace.jumped == 0);
    CHECK(trace.offset_ns == -33 * kMs + 113 * kMs);
    CHECK(trace.sampled == 3);
    CHECK(trace.sampled_offset_ns == trace.offset_ns);
    CHECK(trace.first_at_ns == 100 * kMs);
    // ③ max 按绝对值：+113 ms 比 −33 ms 大。
    CHECK(trace.max.step_ns == 113 * kMs);
    CHECK(trace.max.at_ns == 200 * kMs);
    // 升序展开。
    CHECK(trace.samples_used == 3);
    CHECK(trace.recent[0].at_ns == 100 * kMs);
    CHECK(trace.recent[1].at_ns == 200 * kMs);
    CHECK(trace.recent[2].at_ns == 300 * kMs);
}

// ---- ② 环回绕：总账不许被环截断 ----------------------------------------------

void t02_ring_does_not_truncate_the_total() {
    pace::DiscontinuityLedger ledger;
    for (int i = 0; i < 10; ++i) {
        ledger.note(pace::DiscontinuityKind::kPositionStep, (i + 1) * kMs, kFrame30, kFrame30 + 10 * kMs,
                    10 * kMs, 0, 10 * kMs);
    }
    const pace::DiscontinuityTrace trace = ledger.trace();
    CHECK(trace.events == 10);
    CHECK(trace.sampled == 8);
    CHECK(trace.samples_used == 8);
    CHECK(trace.offset_ns == 100 * kMs);       // 10 笔全算
    CHECK(trace.sampled_offset_ns == 80 * kMs);  // 环里只剩 8 笔
    // 最先的两笔已被挤出去：环里第一笔是第 3 笔（3 ms）。
    CHECK(trace.recent[0].at_ns == 3 * kMs);
    CHECK(trace.recent[7].at_ns == 10 * kMs);
}

// ---- ⑨ reset 清干净 ---------------------------------------------------------

void t03_reset_clears_everything() {
    pace::DiscontinuityLedger ledger;
    ledger.note(pace::DiscontinuityKind::kJumpRebased, kMs, kFrame30, 0, 0, 0, 0);
    ledger.reset();
    const pace::DiscontinuityTrace trace = ledger.trace();
    CHECK(trace.events == 0);
    CHECK(trace.offset_ns == 0);
    CHECK(trace.sampled == 0);
    CHECK(trace.max.step_ns == 0);
    CHECK(trace.first_at_ns == 0);
}

// ---- ⑩ 被跳变丢掉的量：取绝对最大、带符号、只认跳变 --------------------------

void t10_jump_max_keeps_the_largest_discarded() {
    pace::DiscontinuityLedger ledger;
    // 一次往前跳 1500 ms（被丢掉），再一次往回跳 800 ms（绝对值更小，不该换）。
    ledger.note(pace::DiscontinuityKind::kJumpRebased, 20 * kMs, kFrame30, kFrame30 + 1500 * kMs,
                0, 0, 0);
    CHECK(ledger.trace().jump_max_ns == 1500 * kMs);
    ledger.note(pace::DiscontinuityKind::kJumpRebased, 30 * kMs, kFrame30, kFrame30 - 800 * kMs,
                0, 0, 0);
    CHECK(ledger.trace().jump_max_ns == 1500 * kMs);

    // 再来一次 −2000 ms：绝对值更大 ⇒ 替换，且**符号保留**（负 = 钟往回跳）。
    ledger.note(pace::DiscontinuityKind::kJumpRebased, 40 * kMs, kFrame30, kFrame30 - 2000 * kMs,
                0, 0, 0);
    CHECK(ledger.trace().jump_max_ns == -2000 * kMs);

    // 台阶不该污染这一格：它走 `offset_ns`，两份账不能混。
    ledger.note(pace::DiscontinuityKind::kPositionStep, 50 * kMs, kFrame30, kFrame30 + 900 * kMs,
                900 * kMs, 0, 900 * kMs);
    CHECK(ledger.trace().jump_max_ns == -2000 * kMs);
    CHECK(ledger.trace().offset_ns == 900 * kMs);

    // reset 之后归零 —— 否则换片源会把上一段的最大跳变带进来。
    ledger.reset();
    CHECK(ledger.trace().jump_max_ns == 0);
}

// ---- ⑪ 丢帧现场：环 16 笔、truncated、升序 ----------------------------------

void t11_drop_ring_keeps_the_shape() {
    pace::DropRing ring;
    ring.note(20 * kMs, 130 * kMs);
    ring.note(21 * kMs, 140 * kMs);
    auto trace = ring.trace();
    CHECK(trace.total == 2);
    CHECK(trace.sampled == 2);
    CHECK(!trace.truncated);
    CHECK(trace.recent[0].media_ns == 20 * kMs);
    CHECK(trace.recent[1].lateness_ns == 140 * kMs);

    // 写到 17 笔：环里只剩最后 16 笔，`truncated` 必须置位 ——
    // 不置位就等于拿"看得见的 16 笔"冒充全部 17 笔。
    for (std::int32_t i = 0; i < 15; ++i) {
        ring.note((22 + i) * kMs, 120 * kMs);
    }
    trace = ring.trace();
    CHECK(trace.total == 17);
    CHECK(trace.sampled == 16);
    CHECK(trace.truncated);
    // 升序：最旧的一笔是第 2 笔（21 ms），最新的是第 17 笔（36 ms）。
    CHECK(trace.recent[0].media_ns == 21 * kMs);
    CHECK(trace.recent[15].media_ns == 36 * kMs);
    for (std::int32_t i = 1; i < 16; ++i) {
        CHECK(trace.recent[i].media_ns >= trace.recent[i - 1].media_ns);
    }

    ring.reset();
    trace = ring.trace();
    CHECK(trace.total == 0);
    CHECK(trace.sampled == 0);
    CHECK(!trace.truncated);
}

// ---- ④ 恢复只补实际缺额，门仍偿还真实领先量 ----------------------------------

void t04_stall_then_catch_up_is_booked_but_still_repaid() {
    g_now_ns = 0;
    Rig rig(true);
    run_normal(rig, 40);  // 越过启动期（1 s）

    // 钟停一拍：位置不动，墙照走。tracker 按墙外推（step = 0），这一次只留现场。
    const std::int64_t position_at_stall = rig.clock.position_ns;
    rig.clock.stale = true;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();

    // 恢复：设备给的是**真实位置** —— 停更期间它自己往前走了 113 ms，再叠这一帧。
    rig.clock.stale = false;
    rig.clock.position_ns = position_at_stall + 113 * kMs + kFrame30;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();

    run_normal(rig, 10);

    const pace::DiscontinuityTrace trace = rig.tracker.discontinuity_trace();
    CHECK(trace.extrapolated >= 1);
    CHECK(trace.stepped >= 1);
    // r9：stale 已外推过一帧，恢复必须扣掉这笔，不能重复加到设备绝对位置上。
    // 真实领先 = 113ms - 一帧；旧预期 113ms 把重复计时也算成了钟的台阶。
    CHECK(trace.offset_ns == 113 * kMs - kFrame30);
    CHECK(rig.tracker.drift_rebases() == 1);
    CHECK(rig.tracker.drift_max_correction_ns() == 113 * kMs - kFrame30 - 40 * kMs);
}

// ---- ⑤ 有拒绝标志时分类是它 -------------------------------------------------

void t05_rejected_wins_over_position_step() {
    g_now_ns = 0;
    Rig rig(true);
    run_normal(rig, 40);

    // 源拒了一个回退候选：位置停住，且带着标志。
    rig.clock.rejected = true;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();

    const pace::DiscontinuityTrace trace = rig.tracker.discontinuity_trace();
    CHECK(trace.rejected == 1);
    CHECK(trace.stepped == 0);  // 一帧只记一笔
    CHECK(trace.events == 1);
    CHECK(trace.offset_ns == -kFrame30);  // 停了一帧，领先量少一帧
}

// ---- ⑥ 启动期不记账 ---------------------------------------------------------

void t06_startup_steps_are_not_booked() {
    g_now_ns = 0;
    Rig rig(true);
    // 前 10 帧：一笔 200 ms 的台阶压在启动期里（首帧后不到 1 s）。
    rig.clock.position_ns = 200 * kMs;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();
    CHECK(rig.tracker.discontinuity_trace().events == 0);
}

// ---- ⑦ 门关着时账照记 -------------------------------------------------------

void t07_ledger_ignores_gate_state() {
    g_now_ns = 0;
    Rig rig(false);  // 门关
    run_normal(rig, 40);

    const std::int64_t position_at_stall = rig.clock.position_ns;
    rig.clock.stale = true;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();
    rig.clock.stale = false;
    rig.clock.position_ns = position_at_stall + 113 * kMs + kFrame30;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();

    const pace::DiscontinuityTrace trace = rig.tracker.discontinuity_trace();
    CHECK(trace.stepped >= 1);
    CHECK(trace.offset_ns == 113 * kMs - kFrame30);
    CHECK(rig.tracker.drift_rebases() == 0);  // 门关着本来也不动，账却记了
}

// ---- ⑧ 稳态一轮：零事件 -----------------------------------------------------

void t08_steady_clock_books_nothing() {
    g_now_ns = 0;
    Rig rig(true);
    run_normal(rig, 120);  // 4 s
    const pace::DiscontinuityTrace trace = rig.tracker.discontinuity_trace();
    CHECK(trace.events == 0);
    CHECK(trace.offset_ns == 0);
    CHECK(trace.sampled == 0);
    CHECK(rig.tracker.drift_rebases() == 0);
}

// ---- ⑨ reset 保留会话诊断，新位置仍从新段起算 -------------------------------

void t09_tracker_reset_preserves_diagnostics() {
    g_now_ns = 0;
    Rig rig(true);
    run_normal(rig, 40);
    const std::int64_t position_at_stall = rig.clock.position_ns;
    rig.clock.stale = true;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();
    rig.clock.stale = false;
    rig.clock.position_ns = position_at_stall + 113 * kMs + kFrame30;
    g_now_ns += kFrame30;
    (void)rig.tracker.read();
    const auto before = rig.tracker.discontinuity_trace();
    CHECK(before.events > 0);

    rig.tracker.reset();
    CHECK(rig.tracker.discontinuity_trace().events == before.events);
    CHECK(rig.tracker.discontinuity_trace().offset_ns == before.offset_ns);
    const auto fresh = rig.tracker.read();
    CHECK(fresh.position_ns == rig.clock.position_ns);
    CHECK(fresh.elapsed_ns == 0);
}

}  // namespace

int main() {
    t01_ledger_adds_up();
    t02_ring_does_not_truncate_the_total();
    t03_reset_clears_everything();
    t04_stall_then_catch_up_is_booked_but_still_repaid();
    t05_rejected_wins_over_position_step();
    t06_startup_steps_are_not_booked();
    t07_ledger_ignores_gate_state();
    t08_steady_clock_books_nothing();
    t09_tracker_reset_preserves_diagnostics();
    t10_jump_max_keeps_the_largest_discarded();
    t11_drop_ring_keeps_the_shape();

    if (g_failed == 0) {
        std::printf("P14-Y discontinuity: %d checked, 0 failed\n", g_checked);
        return 0;
    }
    std::printf("P14-Y discontinuity: %d checked, %d failed\n", g_checked, g_failed);
    return 1;
}
