// color_facts_tests.cpp — P12-E1 tests: 颜色格式名与位深判定
//
// 这组测试守的是三件"写错不会报错、只会让证据说瞎话"的事：
//   ① NDK 不导出 `COLOR_Format*` 常量，所以那些数值是我们自己写的 —— 必须钉住；
//   ② `surface` 输出**不携带位深**，谁把它当 8bit 就是把"看不见"写成"确认是 8bit"；
//   ③ 10bit 源在任何非 10bit 结果上都必须带 reason —— 卡片那句"绝不静默受 NV12"。
#include <cstdint>
#include <cstdio>
#include <string>

#include "video/video_plane/backends/mediacodec/decode/color_format_names.hpp"
#include "video/video_plane/backends/mediacodec/decode/video_color_facts.hpp"

namespace mediacodec = kivo::video::mediacodec;
namespace color_format = kivo::video::mediacodec::color_format;

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

[[nodiscard]] mediacodec::VideoColorFacts facts(const char* mime, std::int32_t profile,
                                                std::int32_t format) {
    mediacodec::VideoColorFacts out;
    out.mime = mime;
    out.source_profile = profile;
    out.negotiated_format = format;
    return out;
}

// ---- E1-1 常量数值钉住（来源 AOSP MediaCodecInfo.java）-------------------
void t1_constants_are_pinned() {
    CHECK(color_format::kYuv420Planar == 19);
    CHECK(color_format::kYuv420SemiPlanar == 21);
    CHECK(color_format::kYuvP010 == 54);
    CHECK(color_format::kSurface == 0x7F000789);
    CHECK(color_format::kYuv420Flexible == static_cast<std::int32_t>(0x7F420888));
}

// ---- E1-2 名字与位深映射 ------------------------------------------------
void t2_names_and_depths() {
    CHECK(color_format::format_name(color_format::kYuvP010) == "yuvp010");
    CHECK(color_format::format_name(color_format::kYuv420SemiPlanar) == "nv12");
    CHECK(color_format::format_name(color_format::kYuv420Planar) == "yuv420p");
    CHECK(color_format::format_name(color_format::kSurface) == "surface");

    CHECK(color_format::format_bit_depth(color_format::kYuvP010) == 10);
    CHECK(color_format::format_bit_depth(color_format::kYuv420SemiPlanar) == 8);
    CHECK(color_format::format_bit_depth(color_format::kYuv420Planar) == 8);

    // surface / flexible 不携带位深 —— 必须返回 0（未知），不许返回 8。
    CHECK(color_format::format_bit_depth(color_format::kSurface) == 0);
    CHECK(color_format::format_bit_depth(color_format::kYuv420Flexible) == 0);

    // 厂商私有值：给得出名字，但位深必须是"未知"（0），不许猜成 8。
    CHECK(color_format::format_name(0x7FA30C06) == "qcom_private");
    CHECK(color_format::format_bit_depth(0x7FA30C06) == 0);
    CHECK(color_format::is_vendor_private(0x7FA30C06));
    CHECK(color_format::is_vendor_private(0x7FA30CFF));
    CHECK(!color_format::is_vendor_private(color_format::kSurface));

    // 未知值要能被看见，不能返回空串。
    const std::string unknown = color_format::format_name(0x1234);
    CHECK(unknown.rfind("unknown(", 0) == 0);
    CHECK(color_format::format_bit_depth(0x1234) == 0);
}

// ---- E1-3 10bit 源 + Surface 输出 → 不确定，且必须解释 -------------------
void t3_ten_bit_source_on_surface_is_indeterminate() {
    const auto out = mediacodec::assess_color(
        facts("video/hevc", /*Main10=*/2, color_format::kSurface));
    CHECK(out.source_bit_depth_known);
    CHECK(out.source_is_10bit);
    CHECK(out.verdict == mediacodec::ColorVerdict::kIndeterminate);
    CHECK(!out.reason.empty());
    CHECK(out.reason.find("bit depth is not expressed") != std::string::npos);
}

// ---- E1-4 10bit 源 + NV12 → 8bit，且点名是降级 ---------------------------
void t4_ten_bit_source_on_nv12_is_a_downgrade() {
    const auto out = mediacodec::assess_color(
        facts("video/hevc", 2, color_format::kYuv420SemiPlanar));
    CHECK(out.source_is_10bit);
    CHECK(out.verdict == mediacodec::ColorVerdict::kEightBit);
    CHECK(!out.reason.empty());
    CHECK(out.reason.find("silent downgrade") != std::string::npos);
}

// ---- E1-5 10bit 源 + P010 → 10bit，无需解释 ------------------------------
void t5_ten_bit_source_on_p010_matches() {
    const auto out = mediacodec::assess_color(facts("video/hevc", 2, color_format::kYuvP010));
    CHECK(out.source_is_10bit);
    CHECK(out.verdict == mediacodec::ColorVerdict::kTenBit);
    CHECK(out.reason.empty());
}

// ---- E1-6 8bit 源 + NV12 → 匹配，无需解释 --------------------------------
void t6_eight_bit_source_matches() {
    const auto out = mediacodec::assess_color(
        facts("video/hevc", /*Main=*/1, color_format::kYuv420SemiPlanar));
    CHECK(out.source_bit_depth_known);
    CHECK(!out.source_is_10bit);
    CHECK(out.verdict == mediacodec::ColorVerdict::kEightBit);
    CHECK(out.reason.empty());
}

// ---- E1-7 8bit 源 + Surface → 不确定，但要说明"为什么不影响判定" --------
void t7_eight_bit_source_on_surface_explains_why_it_matters_not() {
    const auto out = mediacodec::assess_color(facts("video/hevc", 1, color_format::kSurface));
    CHECK(out.verdict == mediacodec::ColorVerdict::kIndeterminate);
    CHECK(!out.reason.empty());
    CHECK(out.reason.find("does not affect this verdict") != std::string::npos);
}

// ---- E1-8 AVC High10 也算 10bit ------------------------------------------
void t8_avc_high10_counts_as_ten_bit() {
    const auto out = mediacodec::assess_color(
        facts("video/avc", /*High10=*/110, color_format::kYuvP010));
    CHECK(out.source_is_10bit);
    CHECK(out.verdict == mediacodec::ColorVerdict::kTenBit);

    const auto plain = mediacodec::assess_color(
        facts("video/avc", /*High=*/8, color_format::kYuv420SemiPlanar));
    CHECK(plain.source_bit_depth_known);
    CHECK(!plain.source_is_10bit);
}

// ---- E1-9 未知 codec：源位深不可判定，但输出仍照报 -----------------------
void t9_unknown_codec_keeps_output_fact() {
    auto f = facts("video/av01", -1, color_format::kYuv420SemiPlanar);
    const auto out = mediacodec::assess_color(f);
    CHECK(!out.source_bit_depth_known);
    CHECK(!out.source_is_10bit);
    // 源侧未知不该把已观测到的输出位深也拖成未知。
    CHECK(out.verdict == mediacodec::ColorVerdict::kEightBit);
    CHECK(!out.reason.empty());

    f.source_profile = 0;  // 未知 mime 下即使有 profile 也不认
    CHECK(!mediacodec::assess_color(f).source_bit_depth_known);
}

// ---- E1-10 verdict 名字稳定（证据里就是这几个词）------------------------
void t10_verdict_names_are_stable() {
    CHECK(std::string(mediacodec::color_verdict_name(mediacodec::ColorVerdict::kTenBit)) ==
          "ten_bit");
    CHECK(std::string(mediacodec::color_verdict_name(mediacodec::ColorVerdict::kEightBit)) ==
          "eight_bit");
    CHECK(std::string(mediacodec::color_verdict_name(mediacodec::ColorVerdict::kIndeterminate)) ==
          "indeterminate");
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"E1-1 常量数值钉住", t1_constants_are_pinned},
    {"E1-2 名字与位深映射", t2_names_and_depths},
    {"E1-3 10bit 源 + surface = 不确定且解释", t3_ten_bit_source_on_surface_is_indeterminate},
    {"E1-4 10bit 源 + NV12 = 点名降级", t4_ten_bit_source_on_nv12_is_a_downgrade},
    {"E1-5 10bit 源 + P010 = 匹配", t5_ten_bit_source_on_p010_matches},
    {"E1-6 8bit 源 + NV12 = 匹配", t6_eight_bit_source_matches},
    {"E1-7 8bit 源 + surface = 不确定且说明为何不影响", t7_eight_bit_source_on_surface_explains_why_it_matters_not},
    {"E1-8 AVC High10 也算 10bit", t8_avc_high10_counts_as_ten_bit},
    {"E1-9 未知 codec 仍照报输出", t9_unknown_codec_keeps_output_fact},
    {"E1-10 verdict 名字稳定", t10_verdict_names_are_stable},
};

}  // namespace

int main() {
    std::printf("=== P12-E1 color facts tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
