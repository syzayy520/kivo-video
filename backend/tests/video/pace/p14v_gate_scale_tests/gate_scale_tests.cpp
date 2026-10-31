// gate_scale_tests.cpp — P14-V tests: 「门限 45 ms」在这一次会话里到底有多严
//
// 这一组守七句话：
//   ① 同一个 45 ms，窗口一变，等价速率就变 —— 8 s 上 5 625 ppm，1 800 s 上 25 ppm。
//      这不是近似，是定义式 `limit × 1e6 / span`，逐位可比。
//   ② **没有时长就没有换算**：`span == 0` 时必须是"没量到"，不是 0 ppm。
//      0 ppm 读作"门严到一点点速率误差都不许有"，正好是最容易被误读的假观测。
//   ③ 没有门限就没有东西可换：`limit == 0` 两格都必须报"没量到"。
//   ④ 撞门时间是**另一个问题**：8 s 窗口上 ppm = 5 625（看着宽松），
//      但速率只偏 5 000 ppm 时 reach = 9 s **比整段还长** —— 这一轮根本撞不到门。
//      两格一起读才不会被任一个误导。
//   ⑤ 偏差取绝对值：钟慢 5 000 ppm 和钟快 5 000 ppm 撞门时间一样长。
//   ⑥ 偏差小于 1 ppm 时不给 reach（45 ms / 1 ppm = 12.5 小时，这个量程回答不了）。
//   ⑦ **反向解回得出来**：`reach × |rate − 1| × 1e9 == limit`。这条是自证 ——
//      若哪天真把公式写反了，正向测试可能仍然凑巧通过，反向一定不。
//
// 纯计算：不依赖真实时间、不需要设备。
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "video/video_plane/core/pace/pace_gate_scale.hpp"

namespace pace = kivo::video::core::pace;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                          \
    do {                                                                     \
        ++g_checked;                                                         \
        if (!(expr)) {                                                       \
            ++g_failed;                                                      \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);    \
        }                                                                    \
    } while (false)

/// double 比较。倍数取 1e-9 相对：换算只有一次除法，不该有可见误差。
#define CHECK_NEAR(actual, expected)                                                   \
    do {                                                                               \
        ++g_checked;                                                                   \
        const double a_ = (actual);                                                    \
        const double e_ = (expected);                                                  \
        const double tol_ = std::fabs(e_) * 1e-9 + 1e-12;                              \
        if (!(std::fabs(a_ - e_) <= tol_)) {                                            \
            ++g_failed;                                                                \
            std::printf("  FAIL %s:%d  %.9f vs %.9f\n", __FILE__, __LINE__, a_, e_);    \
        }                                                                              \
    } while (false)

constexpr std::int64_t kLimitNs = 45'000'000;   // 45 ms，`PacePolicy` 的默认值
constexpr std::int64_t kSecondNs = 1'000'000'000;

bool same_text(const char* a, const char* b) {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

/// ① 三个真实窗口。份数与 ppm 取自 `157-p14v-design.md` §2 的全仓普查。
void test_three_real_windows() {
    const auto win8 = pace::gate_scale(kLimitNs, 8 * kSecondNs, 1.0, true);
    CHECK(win8.ppm.measured);
    CHECK_NEAR(win8.limit_ppm_equivalent, 5'625.0);   // 45 ms / 8 s = 0.5625%

    const auto win60 = pace::gate_scale(kLimitNs, 60 * kSecondNs, 1.0, true);
    CHECK(win60.ppm.measured);
    CHECK_NEAR(win60.limit_ppm_equivalent, 750.0);    // 0.075%

    const auto long18 = pace::gate_scale(kLimitNs, 1'800 * kSecondNs, 1.0, true);
    CHECK(long18.ppm.measured);
    CHECK_NEAR(long18.limit_ppm_equivalent, 25.0);    // 0.0025% —— 晶振精度区
}

/// ② `span == 0`：没记录时长，换算无从谈起。**不许给 0。**
void test_no_span_is_not_zero() {
    const auto out = pace::gate_scale(kLimitNs, 0, 1.0, true);
    CHECK(!out.ppm.measured);
    CHECK_NEAR(out.limit_ppm_equivalent, 0.0);   // 字段值仍是 0，但 measured 说它不可读
    CHECK(same_text(out.ppm.reason, pace::kNoSpanReason));
    CHECK(out.ppm.reason != pace::kUnspecifiedReason);  // 说清了，不是"忘了填"
}

/// ③ `limit == 0`：门没配置，两格都没东西可换。
void test_no_limit() {
    const auto out = pace::gate_scale(0, 8 * kSecondNs, 1.005, true);
    CHECK(!out.ppm.measured);
    CHECK(!out.reach.measured);
    CHECK(same_text(out.ppm.reason, pace::kNoGateLimitReason));
    CHECK(same_text(out.reach.reason, pace::kNoGateLimitReason));
}

/// ④ 撞门时间，以及"整段都撞不到"这种情形。
void test_reach_beyond_the_session() {
    // 5 000 ppm：45000 ns / (0.005 × 1e9) = 9 s。
    const auto out = pace::gate_scale(kLimitNs, 8 * kSecondNs, 1.005, true);
    CHECK(out.reach.measured);
    CHECK_NEAR(out.limit_reach_s, 9.0);
    // **reach 比整段 8 s 还长** —— 这一轮撞不到门。
    // 也就是说 8 s 片源上 5 000 ppm 的速率误差，门从头到尾不会响。
    CHECK(out.limit_reach_s > 8.0);
}

/// ⑤ 偏差取绝对值：钟慢与钟快撞门时间一样长。
void test_reach_is_symmetric() {
    const auto fast = pace::gate_scale(kLimitNs, 8 * kSecondNs, 1.005, true);
    const auto slow = pace::gate_scale(kLimitNs, 8 * kSecondNs, 0.995, true);
    CHECK(fast.reach.measured);
    CHECK(slow.reach.measured);
    CHECK_NEAR(fast.limit_reach_s, slow.limit_reach_s);
    CHECK_NEAR(slow.limit_reach_s, 9.0);
}

/// ⑥ 偏差小于 1 ppm：不给 reach。
void test_reach_too_small() {
    const auto out = pace::gate_scale(kLimitNs, 8 * kSecondNs, 1.0000005, true);
    CHECK(out.ppm.measured);       // ppm 与速率无关，照样算得出来
    CHECK(!out.reach.measured);
    CHECK(same_text(out.reach.reason, pace::kRateTooCloseReason));
}

/// 速率没测到：reach 不可算，但 ppm 仍可算 —— 两格的可测性**互相独立**。
void test_rate_unmeasured() {
    const auto out = pace::gate_scale(kLimitNs, 8 * kSecondNs, 0.0, false);
    CHECK(out.ppm.measured);
    CHECK(!out.reach.measured);
    CHECK(same_text(out.reach.reason, pace::kRateUnmeasuredReason));
}

/// 门已经动作过：reach 必须判"不可算"，并给出专门的理由（P14-W）。
///
/// 60 s 片源上跑 5 000 ppm，门第 9 秒就动手，`media_rate` 被压回 1.0007 ——
/// 拿这个净速率去算"多久撞门"会得到 63.6 s，而**门实际已经动作了 49 次**。
/// 这个数描述的是"残余速率还要多久"，不是"注入的速率多久撞门"。
/// 所以只要 `drift_rebases > 0`，这一格就必须闭嘴 —— 不是算得不准，是问题问错了。
void test_gate_acted_hides_reach() {
    const auto out = pace::gate_scale(kLimitNs, 60 * kSecondNs, 1.000707, true, 49);
    CHECK(out.ppm.measured);   // ppm 与门动没动过无关，照样能算
    CHECK(!out.reach.measured);
    CHECK(same_text(out.reach.reason, pace::kGateActedReason));

    // 同一份数据、门没动过 ⇒ reach 照算。证明这不是"门开着一律关闭"，
    // 而是"门动过才闭嘴"。
    const auto clean = pace::gate_scale(kLimitNs, 60 * kSecondNs, 1.000707, true, 0);
    CHECK(clean.reach.measured);
    CHECK(clean.limit_reach_s > 60.0);
}

/// ⑦ 反向自证：`reach × |rate − 1| × 1e9 == limit`。
///
/// 正向断言可能凑巧通过（比如把分母写成了 span），反向解不会。
void test_reach_inverts_back_to_limit() {
    const double rates[] = {1.005, 0.9985, 1.0002, 1.02};
    const std::int64_t spans[] = {8 * kSecondNs, 300 * kSecondNs, 1'800 * kSecondNs};
    for (double rate : rates) {
        for (std::int64_t span : spans) {
            const auto out = pace::gate_scale(kLimitNs, span, rate, true);
            CHECK(out.reach.measured);
            const double dev = std::fabs(rate - 1.0);
            const double back = out.limit_reach_s * dev * 1e9;
            CHECK_NEAR(back, static_cast<double>(kLimitNs));
        }
    }
}

/// ⑧ 单调性：窗口翻倍，等价速率减半。
///
/// 这条守的是"方向"——公式写成 `span / limit` 的话，正向的单个数值可能仍然对得上
/// （如果恰好挑了 45 ms / 8 s），但翻倍关系一定反。
void test_ppm_halves_when_window_doubles() {
    const auto a = pace::gate_scale(kLimitNs, 60 * kSecondNs, 1.0, true);
    const auto b = pace::gate_scale(kLimitNs, 120 * kSecondNs, 1.0, true);
    CHECK(a.ppm.measured && b.ppm.measured);
    CHECK_NEAR(b.limit_ppm_equivalent, a.limit_ppm_equivalent / 2.0);
}

}  // namespace

int main() {
    std::printf("P14-V gate scale tests\n");
    test_three_real_windows();
    test_no_span_is_not_zero();
    test_no_limit();
    test_reach_beyond_the_session();
    test_reach_is_symmetric();
    test_reach_too_small();
    test_rate_unmeasured();
    test_gate_acted_hides_reach();
    test_reach_inverts_back_to_limit();
    test_ppm_halves_when_window_doubles();

    std::printf("P14-V: %d checks, %d failed\n", g_checked, g_failed);
    if (g_failed == 0) {
        std::printf("PASS\n");
    }
    return g_failed == 0 ? 0 : 1;
}
