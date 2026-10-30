// hdr_classify_tests.cpp — P12-E2 tests: HDR 分类与"下发计划"的前置事实
//
// 这组测试守的是三件"写错不会报错、只会让证据说瞎话"的事：
//   ① 颜色键的数值是**从 android.jar 反查**的，不是背的 —— 必须钉住；
//      尤其两个反直觉值：BT601_NTSC = 4、SDR_VIDEO = 3；
//   ② `MediaFormat` 里**没有** BT2020_CONSTANT_LUMINANCE，所以 HDR 判据只认 6，
//      认 7 等于认一个在这条键上不存在的取值；
//   ③ 判成 SDR 也必须带依据 —— 空的 reason 会被读成"根本没查"。
#include <cstdint>
#include <cstdio>
#include <string>

#include "video/video_plane/backends/mediacodec/decode/color_key_names.hpp"
#include "video/video_plane/backends/mediacodec/decode/source_color_tags.hpp"
#include "video/video_plane/backends/mediacodec/decode/video_color_facts.hpp"

namespace mediacodec = kivo::video::mediacodec;
namespace color_key = kivo::video::mediacodec::color_key;

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

[[nodiscard]] mediacodec::VideoColorFacts tagged(std::int32_t standard, std::int32_t transfer) {
    mediacodec::VideoColorFacts out;
    out.mime = "video/hevc";
    out.source_profile = 2;  // Main10
    out.source_color_standard = standard;
    out.source_color_transfer = transfer;
    out.source_color_range = color_key::kRangeLimited;
    return out;
}

// ---- E2-1 常量数值钉住（来源：javap android.jar android.media.MediaFormat）----
void t1_key_values_are_pinned() {
    CHECK(color_key::kStandardBt709 == 1);
    CHECK(color_key::kStandardBt601Pal == 2);
    // 反直觉：BT601_NTSC 是 4，不是 3。
    CHECK(color_key::kStandardBt601Ntsc == 4);
    CHECK(color_key::kStandardBt2020 == 6);

    CHECK(color_key::kTransferLinear == 1);
    // 反直觉：3 是 SDR_VIDEO，不是 SMPTE_240M。E1 在无标记片源上看到的 1/3/2 就是它。
    CHECK(color_key::kTransferSdrVideo == 3);
    CHECK(color_key::kTransferSt2084 == 6);
    CHECK(color_key::kTransferHlg == 7);

    CHECK(color_key::kRangeFull == 1);
    CHECK(color_key::kRangeLimited == 2);
}

// ---- E2-2 键名写死的是平台契约，改名等于读不到 ---------------------------
void t2_key_names_are_literals() {
    CHECK(std::string(color_key::kKeyColorStandard) == "color-standard");
    CHECK(std::string(color_key::kKeyColorTransfer) == "color-transfer");
    CHECK(std::string(color_key::kKeyColorRange) == "color-range");
    CHECK(std::string(color_key::kKeyHdrStaticInfo) == "hdr-static-info");
}

// ---- E2-3 HDR10：ST2084 + BT2020 ---------------------------------------
void t3_pq_plus_bt2020_is_hdr10() {
    const auto out = mediacodec::classify_hdr(tagged(color_key::kStandardBt2020,
                                                     color_key::kTransferSt2084));
    CHECK(out.kind == mediacodec::HdrKind::kHdr10);
    // HDR 也要写依据：证据里 kind 与 reason 永远成对。
    CHECK(!out.reason.empty());
    CHECK(out.reason.find("bt2020") != std::string::npos);
    CHECK(out.reason.find("st2084") != std::string::npos);
}

// ---- E2-4 HLG：transfer 7 + BT2020 --------------------------------------
void t4_hlg_plus_bt2020_is_hlg() {
    const auto out =
        mediacodec::classify_hdr(tagged(color_key::kStandardBt2020, color_key::kTransferHlg));
    CHECK(out.kind == mediacodec::HdrKind::kHlg);
    CHECK(!out.reason.empty());
}

// ---- E2-5 PQ 却没有 BT2020：自相矛盾，判 SDR 并说明 ---------------------
void t5_pq_without_bt2020_is_sdr_with_reason() {
    const auto out = mediacodec::classify_hdr(tagged(color_key::kStandardBt709,
                                                     color_key::kTransferSt2084));
    CHECK(out.kind == mediacodec::HdrKind::kSdr);
    CHECK(out.reason.find("without BT2020") != std::string::npos);
}

// ---- E2-6 HLG 却没有 BT2020：同上 ---------------------------------------
void t6_hlg_without_bt2020_is_sdr_with_reason() {
    const auto out =
        mediacodec::classify_hdr(tagged(color_key::kStandardBt709, color_key::kTransferHlg));
    CHECK(out.kind == mediacodec::HdrKind::kSdr);
    CHECK(out.reason.find("without BT2020") != std::string::npos);
}

// ---- E2-7 普通 SDR 片源（E1 实测的 1/3/2）------------------------------
void t7_plain_sdr_is_sdr_with_reason() {
    const auto out = mediacodec::classify_hdr(tagged(color_key::kStandardBt709,
                                                     color_key::kTransferSdrVideo));
    CHECK(out.kind == mediacodec::HdrKind::kSdr);
    CHECK(!out.reason.empty());
    CHECK(out.reason.find("neither ST2084") != std::string::npos);
}

// ---- E2-8 一个颜色键都没有：不许说"这是 SDR" ----------------------------
// 事实是"没有可判的东西"，而 E1 已证解码器的 1/3/2 只是默认值。
void t8_untagged_source_says_nothing_to_classify() {
    const auto out = mediacodec::classify_hdr(tagged(-1, -1));
    CHECK(out.kind == mediacodec::HdrKind::kSdr);
    CHECK(out.reason.find("neither color-standard nor color-transfer") != std::string::npos);
    CHECK(out.reason.find("not a claim") != std::string::npos);
}

// ---- E2-9 只有 standard 没有 transfer：不能升级成 HDR ------------------
void t9_bt2020_without_transfer_stays_sdr() {
    const auto out = mediacodec::classify_hdr(tagged(color_key::kStandardBt2020, -1));
    CHECK(out.kind == mediacodec::HdrKind::kSdr);
    CHECK(!out.reason.empty());
}

// ---- E2-10 名字映射：未知值不许假装认识 --------------------------------
void t10_names_do_not_pretend() {
    CHECK(std::string(color_key::standard_name(color_key::kStandardBt2020)) == "bt2020");
    CHECK(std::string(color_key::transfer_name(color_key::kTransferSt2084)) == "st2084(pq)");
    CHECK(std::string(color_key::transfer_name(color_key::kTransferSdrVideo)) == "sdr_video");
    // 5 在 color-standard 上没有定义（BT601_NTSC 是 4，BT2020 是 6）——
    // 正好是设计稿写错的那个值，钉在这里防止有人"顺手补上 BT2020_CL = 5"。
    CHECK(std::string(color_key::standard_name(5)) == "unknown");
    CHECK(std::string(color_key::standard_name(-1)) == "unknown(-1)");
}

// ---- E2-11 有标记才值得下发 -------------------------------------------
void t11_tags_any_is_the_gate_for_delivery() {
    mediacodec::SourceColorTags empty;
    CHECK(!empty.any());

    mediacodec::SourceColorTags only_range;
    only_range.range = color_key::kRangeLimited;
    CHECK(only_range.any());

    mediacodec::SourceColorTags only_static;
    only_static.static_info_bytes = 24;
    CHECK(only_static.any());
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"E2-1 颜色键数值钉住", t1_key_values_are_pinned},
    {"E2-2 键名是平台契约", t2_key_names_are_literals},
    {"E2-3 PQ + BT2020 = hdr10", t3_pq_plus_bt2020_is_hdr10},
    {"E2-4 HLG + BT2020 = hlg", t4_hlg_plus_bt2020_is_hlg},
    {"E2-5 PQ 无 BT2020 = sdr 且说明", t5_pq_without_bt2020_is_sdr_with_reason},
    {"E2-6 HLG 无 BT2020 = sdr 且说明", t6_hlg_without_bt2020_is_sdr_with_reason},
    {"E2-7 普通 SDR = sdr 且说明", t7_plain_sdr_is_sdr_with_reason},
    {"E2-8 无标记 = 没有可判的东西", t8_untagged_source_says_nothing_to_classify},
    {"E2-9 只有 standard = 不升级成 HDR", t9_bt2020_without_transfer_stays_sdr},
    {"E2-10 名字映射不假装认识", t10_names_do_not_pretend},
    {"E2-11 有标记才下发", t11_tags_any_is_the_gate_for_delivery},
};

}  // namespace

int main() {
    std::printf("=== P12-E2 hdr classify tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
