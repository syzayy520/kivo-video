// seek_budget_tests.cpp — P12-F1：G8 预算判定
//
// 这组测试守的是三件"写错不会报错、只会让证据说瞎话"的事：
//   ① 没发生 seek 的一轮**不许**判通过（所有字段都是 -1，看起来"没超预算"）；
//   ② `landing > target` 即使时间达标也不算数（PREVIOUS_SYNC 应落在目标之前）；
//   ③ 失败必须带 reason，否则读的人会当成"没问题"。
#include <cstdint>
#include <cstdio>
#include <string>

#include "video/video_plane/core/seek/seek_budget.hpp"

namespace seek = kivo::video::core::seek;

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

[[nodiscard]] seek::SeekFacts facts(std::int64_t target, std::int64_t landing, std::int64_t prepare,
                                    std::int64_t first_frame) {
    seek::SeekFacts out;
    out.requests = 1;
    out.target_us = target;
    out.landing_us = landing;
    out.prepare_us = prepare;
    out.first_frame_us = first_frame;
    return out;
}

// ---- F1-1 正常达标 --------------------------------------------------------
void t1_within_budget_passes() {
    const auto out = seek::assess_seek(facts(5'000'000, 4'966'666, 20'000, 60'000));
    CHECK(out.pass);
    CHECK(out.total_us == 80'000);
    // 达标时 reason 必须为空 —— 空的失败原因才是问题，空的成功原因是对的。
    CHECK(out.reason.empty());
}

// ---- F1-2 恰好等于预算算过（<=，不是 <）--------------------------------
void t2_exactly_on_budget_passes() {
    const auto out = seek::assess_seek(facts(1'000'000, 1'000'000, 50'000, 100'000));
    CHECK(out.pass);
    CHECK(out.total_us == seek::kDefaultBudgetUs);
}

// ---- F1-3 超预算必须失败且点名两段耗时 ---------------------------------
void t3_over_budget_fails_with_breakdown() {
    const auto out = seek::assess_seek(facts(1'000'000, 900'000, 40'000, 200'000));
    CHECK(!out.pass);
    CHECK(out.total_us == 240'000);
    CHECK(out.reason.find("240000") != std::string::npos);
    CHECK(out.reason.find("prepare") != std::string::npos);
}

// ---- F1-4 没有 seek 不许判通过 -----------------------------------------
void t4_no_seek_is_not_a_pass() {
    seek::SeekFacts none;
    const auto out = seek::assess_seek(none);
    CHECK(!out.pass);
    CHECK(out.total_us == -1);
    CHECK(out.reason.find("no seek happened") != std::string::npos);
}

// ---- F1-5 落点在目标之后：时间达标也不算 -------------------------------
void t5_landing_after_target_fails() {
    const auto out = seek::assess_seek(facts(1'000'000, 1'100'000, 10'000, 20'000));
    CHECK(!out.pass);
    CHECK(out.reason.find("PREVIOUS_SYNC") != std::string::npos);
}

// ---- F1-6 落点为 -1：解封装器拒绝 -------------------------------------
void t6_rejected_seek_fails() {
    const auto out = seek::assess_seek(facts(1'000'000, -1, 10'000, 20'000));
    CHECK(!out.pass);
    CHECK(out.reason.find("rejected") != std::string::npos);
}

// ---- F1-7 seek 之后没有帧：不许当"0 ms" -------------------------------
void t7_no_first_frame_fails() {
    const auto out = seek::assess_seek(facts(1'000'000, 900'000, 10'000, -1));
    CHECK(!out.pass);
    CHECK(out.reason.find("no frame") != std::string::npos);
}

// ---- F1-8 预算可调（判据写死 150 ms，但函数不该把它焊死）--------------
void t8_budget_is_a_parameter() {
    const auto strict = seek::assess_seek(facts(1'000'000, 900'000, 10'000, 20'000), 5'000);
    CHECK(!strict.pass);
    CHECK(strict.reason.find("5000") != std::string::npos);
}

// ---- F2-1 没有音轨：不适用，不判通过 ---------------------------------
void t9_no_audio_is_not_applicable() {
    auto f = facts(1'000'000, 900'000, 10'000, 20'000);
    f.audio_present = false;
    const auto out = seek::assess_av_seek(f);
    CHECK(!out.applicable);
    CHECK(!out.pass);
    CHECK(out.reason.find("does not apply") != std::string::npos);
}

// ---- F2-2 两侧对齐：通过 ---------------------------------------------
void t10_aligned_sides_pass() {
    auto f = facts(5'000'000, 5'000'000, 20'000, 10'000);
    f.audio_present = true;
    f.audio_landing_us = 5'000'000;
    const auto out = seek::assess_av_seek(f);
    CHECK(out.applicable);
    CHECK(out.pass);
    CHECK(out.audio_gap_us == 0);
    CHECK(out.reason.empty());
}

// ---- F2-3 差一帧（33 ms）：算同步 -------------------------------------
void t11_one_frame_gap_passes() {
    auto f = facts(5'000'000, 5'000'000, 20'000, 10'000);
    f.audio_present = true;
    f.audio_landing_us = 5'033'000;
    const auto out = seek::assess_av_seek(f);
    CHECK(out.pass);
    CHECK(out.audio_gap_us == 33'000);
}

// ---- F2-4 差两帧（−66 ms）：不通过且点名两侧落点 ---------------------
void t12_two_frame_gap_fails() {
    auto f = facts(5'000'000, 5'000'000, 20'000, 10'000);
    f.audio_present = true;
    f.audio_landing_us = 4'934'000;
    const auto out = seek::assess_av_seek(f);
    CHECK(!out.pass);
    CHECK(out.audio_gap_us == -66'000);
    CHECK(out.reason.find("out of sync") != std::string::npos);
    CHECK(out.reason.find("4934000") != std::string::npos);
}

// ---- F2-5 音频没跳成：不通过 -----------------------------------------
void t13_audio_landing_missing_fails() {
    auto f = facts(5'000'000, 5'000'000, 20'000, 10'000);
    f.audio_present = true;
    f.audio_landing_us = -1;
    const auto out = seek::assess_av_seek(f);
    CHECK(!out.pass);
    CHECK(out.reason.find("audio side") != std::string::npos);
}

// ---- F2-6 视频侧没过，原因要原样带出来（不重复造）--------------------
void t14_video_failure_is_propagated() {
    auto f = facts(5'000'000, 5'000'000, 10'000, 200'000);
    f.audio_present = true;
    f.audio_landing_us = 5'000'000;
    const auto out = seek::assess_av_seek(f);
    CHECK(!out.pass);
    CHECK(out.reason.find("video side failed") != std::string::npos);
    CHECK(!out.video.pass);
}

// ---- P13B-1 冷启动 1080p（47ms）达标 ------------------------------------
void t15_cold_start_1080p_passes() {
    const auto out = seek::assess_cold_start(47'000);
    CHECK(out.applicable);
    CHECK(out.pass);
    CHECK(out.reason.empty());
    CHECK(out.budget_us == seek::kColdStartBudgetUs);
}

// ---- P13B-2 冷启动 4K 10bit（132.5ms）达标 —— 这是最关键的一例 ---------
void t16_cold_start_4k_passes() {
    // 132.5 ms 是已知最大的一档。它必须通过 —— 若判死，说明阈值定得比现实还紧。
    const auto out = seek::assess_cold_start(132'500);
    CHECK(out.applicable);
    CHECK(out.pass);
}

// ---- P13B-3 超过 200ms 判失败，且 reason 点明"这是冷启动不是 seek" ------
void t17_cold_start_over_budget() {
    // 350 ms —— 明显超过 300 ms 的 G8a 预算。
    // 注意别用 250 ms：它在预算内，会得到 pass=true（这是阈值从 200 提到 300 后的变化）。
    const auto out = seek::assess_cold_start(350'000);
    CHECK(out.applicable);
    CHECK(!out.pass);
    CHECK(out.reason.find("350000") != std::string::npos);
    CHECK(out.reason.find("not a seek") != std::string::npos);
}

// ---- P13B-4 没出帧是"没测到"，不是通过也不是超时 ------------------------
void t18_cold_start_no_frame() {
    const auto out = seek::assess_cold_start(-1);
    CHECK(!out.applicable);
    CHECK(!out.pass);
    CHECK(!out.reason.empty());
}

// ---- P13B-5 阈值确实被用上（不是装饰）----------------------------------
void t19_cold_start_budget_is_parameter() {
    // 同一个数字，换预算应得到不同结论。否则"预算"就是个摆设。
    const auto tight = seek::assess_cold_start(133'000, 100'000);
    CHECK(tight.applicable);
    CHECK(!tight.pass);
    CHECK(tight.reason.find("G8a") != std::string::npos);

    const auto relaxed = seek::assess_cold_start(133'000);
    CHECK(relaxed.pass);

    // 已观测的极大值 206 ms 必须在预算内 —— 否则判据会偶发变红，
    // 而"偶发红"等于没有判据（分不清退步和噪声）。
    const auto observed_max = seek::assess_cold_start(206'000);
    CHECK(observed_max.pass);

    // 明显超标（350 ms）要红。
    const auto way_over = seek::assess_cold_start(350'000);
    CHECK(!way_over.pass);

    // 注意：132.5 ms **在 150 ms 的 seek 预算内**。
    // 所以 P13-A 那句"150 ms 在 4K 上不够"是错的 —— 不仅混了两种口径，
    // 连"超预算"这个事实都不成立。真实的说法是：余量从 3.2× 缩到 1.13×，很紧但没超。
    const auto with_seek_budget = seek::assess_cold_start(132'500, seek::kDefaultBudgetUs);
    CHECK(with_seek_budget.pass);
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"F1-1 达标通过", t1_within_budget_passes},
    {"F1-2 恰好等于预算算过", t2_exactly_on_budget_passes},
    {"F1-3 超预算失败并给拆分", t3_over_budget_fails_with_breakdown},
    {"F1-4 没有 seek 不算通过", t4_no_seek_is_not_a_pass},
    {"F1-5 落点在目标之后不算", t5_landing_after_target_fails},
    {"F1-6 落点 -1 = 被拒绝", t6_rejected_seek_fails},
    {"F1-7 没有首帧不算 0ms", t7_no_first_frame_fails},
    {"F1-8 预算可传参", t8_budget_is_a_parameter},
    {"F2-1 没有音轨 = 不适用不通过", t9_no_audio_is_not_applicable},
    {"F2-2 两侧对齐通过", t10_aligned_sides_pass},
    {"F2-3 差一帧算同步", t11_one_frame_gap_passes},
    {"F2-4 差两帧失败", t12_two_frame_gap_fails},
    {"F2-5 音频没跳成失败", t13_audio_landing_missing_fails},
    {"F2-6 视频失败原因透传", t14_video_failure_is_propagated},
    {"P13B-1 冷启动 1080p 达标", t15_cold_start_1080p_passes},
    {"P13B-2 冷启动 4K 达标", t16_cold_start_4k_passes},
    {"P13B-3 冷启动超预算", t17_cold_start_over_budget},
    {"P13B-4 没出帧不算通过", t18_cold_start_no_frame},
    {"P13B-5 阈值可传参", t19_cold_start_budget_is_parameter},
};

}  // namespace

int main() {
    std::printf("=== P12-F1/F2 + P13-B seek budget tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
