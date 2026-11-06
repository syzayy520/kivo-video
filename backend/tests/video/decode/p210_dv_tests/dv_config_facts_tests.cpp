// dv_config_facts_tests.cpp — 210：DV 判定层的单测
//
// 判定规则必须能在桌面上钉住，真机只负责提供平台键值（`mediaformat_dv_io` 读数）。
// 测的重点（对应 `210-dv-a-detect-passthrough-design.md` §六）：
//   AC1：8.1 判 passthrough 且 bl_mime = video/hevc；
//   AC3：profile 5 判 unsupported 且 reason 点名 dv_profile=5；
//   AC4（桌面半边）：既有 HdrKind 三值的数值与名字一个不变；
//   位标志解码的两条实测规则（1<<profile、1<<(level-1)）与非 2 幂的"不猜"。
#include <cstdio>
#include <string>
#include <vector>

#include "video/video_plane/backends/mediacodec/decode/dv_config_facts.hpp"
#include "video/video_plane/backends/mediacodec/decode/hdr_kind.hpp"

namespace {

namespace mediacodec = kivo::video::mediacodec;

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* expr, const char* test, int line) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL %s:%d  %s\n", test, line, expr);
    }
}

#define CHECK(expr) check((expr), #expr, __func__, __LINE__)

mediacodec::DvTrackKeys keys(std::int32_t profile, std::int32_t level, std::int32_t bl_sig) {
    mediacodec::DvTrackKeys out;
    out.profile_key = profile;
    out.level_key = level;
    out.bl_compatibility_id = bl_sig;
    return out;
}

void test_mime() {
    CHECK(mediacodec::is_dolby_vision_mime("video/dolby-vision"));
    CHECK(!mediacodec::is_dolby_vision_mime("video/hevc"));
    CHECK(!mediacodec::is_dolby_vision_mime("video/avc"));
    CHECK(!mediacodec::is_dolby_vision_mime(""));
    // 精确匹配：带尾随内容不算（不猜）。
    CHECK(!mediacodec::is_dolby_vision_mime("video/dolby-vision.x"));
}

void test_profile_key_decode() {
    CHECK(mediacodec::dv_profile_from_key(256) == 8);   // 真机实测：profile 8 → 256
    CHECK(mediacodec::dv_profile_from_key(32) == 5);    // 真机实测：profile 5 → 32
    CHECK(mediacodec::dv_profile_from_key(1) == 0);
    CHECK(mediacodec::dv_profile_from_key(2) == 1);
    CHECK(mediacodec::dv_profile_from_key(512) == 9);
    CHECK(mediacodec::dv_profile_from_key(1 << 30) == 30);
    // 非 2 的幂、0、负值：不猜。
    CHECK(mediacodec::dv_profile_from_key(3) == -1);
    CHECK(mediacodec::dv_profile_from_key(6) == -1);
    CHECK(mediacodec::dv_profile_from_key(0) == -1);
    CHECK(mediacodec::dv_profile_from_key(-1) == -1);
}

void test_level_key_decode() {
    CHECK(mediacodec::dv_level_from_key(1) == 1);    // 真机实测：level 1 → 1
    CHECK(mediacodec::dv_level_from_key(32) == 6);   // 真机实测：level 6 → 32 = 1<<(6-1)
    CHECK(mediacodec::dv_level_from_key(2) == 2);
    CHECK(mediacodec::dv_level_from_key(1024) == 11);
    CHECK(mediacodec::dv_level_from_key(0) == -1);
    CHECK(mediacodec::dv_level_from_key(48) == -1);
    CHECK(mediacodec::dv_level_from_key(-1) == -1);
}

void test_sniff_bl_codec() {
    const std::vector<std::uint8_t> hevc_vps = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x00};
    const std::vector<std::uint8_t> hevc_sps = {0x00, 0x00, 0x01, 0x42, 0x01, 0x00};
    const std::vector<std::uint8_t> avc_sps = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00};
    const std::vector<std::uint8_t> garbage = {0x12, 0x34, 0x56, 0x78};
    const std::vector<std::uint8_t> hevc_idr = {0x00, 0x00, 0x00, 0x01, 0x26, 0x01, 0x00};

    CHECK(mediacodec::sniff_bl_codec(hevc_vps.data(), hevc_vps.size()) ==
          mediacodec::BlCodec::kHevc);
    CHECK(mediacodec::sniff_bl_codec(hevc_sps.data(), hevc_sps.size()) ==
          mediacodec::BlCodec::kHevc);
    CHECK(mediacodec::sniff_bl_codec(avc_sps.data(), avc_sps.size()) ==
          mediacodec::BlCodec::kAvc);
    CHECK(mediacodec::sniff_bl_codec(garbage.data(), garbage.size()) ==
          mediacodec::BlCodec::kUnknown);
    CHECK(mediacodec::sniff_bl_codec(hevc_idr.data(), hevc_idr.size()) ==
          mediacodec::BlCodec::kUnknown);  // 第一个 NAL 不是 VPS/SPS：不猜
    CHECK(mediacodec::sniff_bl_codec(nullptr, 0) == mediacodec::BlCodec::kUnknown);
    std::vector<std::uint8_t> too_short = {0x00, 0x00, 0x01};
    CHECK(mediacodec::sniff_bl_codec(too_short.data(), too_short.size()) ==
          mediacodec::BlCodec::kUnknown);
}

void test_bl_codec_mime() {
    CHECK(std::string(mediacodec::bl_codec_mime(mediacodec::BlCodec::kHevc)) == "video/hevc");
    CHECK(std::string(mediacodec::bl_codec_mime(mediacodec::BlCodec::kAvc)) == "video/avc");
    CHECK(std::string(mediacodec::bl_codec_mime(mediacodec::BlCodec::kUnknown)).empty());
}

void test_assess_not_dv() {
    const mediacodec::DvAssessment out =
        mediacodec::assess_dv("video/hevc", keys(-1, -1, -1), mediacodec::BlCodec::kUnknown);
    CHECK(out.verdict == mediacodec::DvVerdict::kNotDolbyVision);
    CHECK(!out.reason.empty());  // 成对铁律：判成"不是 DV"也要说依据
    CHECK(out.dv_profile == -1);
    CHECK(out.bl_mime.empty());
}

void test_assess_profile81_passthrough() {
    const mediacodec::DvAssessment out =
        mediacodec::assess_dv("video/dolby-vision", keys(256, 1, 1), mediacodec::BlCodec::kHevc);
    CHECK(out.verdict == mediacodec::DvVerdict::kPassthroughHdr10);
    CHECK(out.dv_profile == 8);
    CHECK(out.dv_level == 1);
    CHECK(out.bl_mime == "video/hevc");
    CHECK(!out.reason.empty());
}

void test_assess_level_key_absent_still_passthrough() {
    // level 键缺席不该挡住透传 —— 8.x 的判定依据是 profile + BL 是 HEVC。
    const mediacodec::DvAssessment out =
        mediacodec::assess_dv("video/dolby-vision", keys(256, -1, 1), mediacodec::BlCodec::kHevc);
    CHECK(out.verdict == mediacodec::DvVerdict::kPassthroughHdr10);
    CHECK(out.dv_level == -1);
}

void test_assess_profile5_reject() {
    const mediacodec::DvAssessment out =
        mediacodec::assess_dv("video/dolby-vision", keys(32, 32, -1), mediacodec::BlCodec::kHevc);
    CHECK(out.verdict == mediacodec::DvVerdict::kUnsupported);
    CHECK(out.dv_profile == 5);
    CHECK(out.reason.find("dv_profile=5") != std::string::npos);  // AC3：点名 profile
    CHECK(out.bl_mime.empty());
}

void test_assess_out_of_scope_profile() {
    // 128 = 1<<7 → profile 7（双层盘源那档，A 档不收）。
    const mediacodec::DvAssessment out =
        mediacodec::assess_dv("video/dolby-vision", keys(128, -1, -1), mediacodec::BlCodec::kHevc);
    CHECK(out.verdict == mediacodec::DvVerdict::kUnsupported);
    CHECK(out.dv_profile == 7);
    CHECK(out.reason.find("outside tier-A") != std::string::npos);
}

void test_assess_unrecognized_profile_key() {
    const mediacodec::DvAssessment out =
        mediacodec::assess_dv("video/dolby-vision", keys(3, -1, -1), mediacodec::BlCodec::kHevc);
    CHECK(out.verdict == mediacodec::DvVerdict::kUnsupported);
    CHECK(out.dv_profile == -1);
    CHECK(out.reason.find("power-of-two") != std::string::npos);
}

void test_assess_bl_codec_guards() {
    const mediacodec::DvTrackKeys eight = keys(256, 1, 1);
    const mediacodec::DvAssessment avc =
        mediacodec::assess_dv("video/dolby-vision", eight, mediacodec::BlCodec::kAvc);
    CHECK(avc.verdict == mediacodec::DvVerdict::kUnsupported);
    CHECK(avc.reason.find("AVC") != std::string::npos);

    const mediacodec::DvAssessment unknown =
        mediacodec::assess_dv("video/dolby-vision", eight, mediacodec::BlCodec::kUnknown);
    CHECK(unknown.verdict == mediacodec::DvVerdict::kUnsupported);
    CHECK(unknown.reason.find("unknown") != std::string::npos);
}

void test_verdict_names() {
    CHECK(std::string(mediacodec::dv_verdict_name(mediacodec::DvVerdict::kNotDolbyVision)) ==
          "not_dolby_vision");
    CHECK(std::string(mediacodec::dv_verdict_name(mediacodec::DvVerdict::kPassthroughHdr10)) ==
          "passthrough_hdr10");
    CHECK(std::string(mediacodec::dv_verdict_name(mediacodec::DvVerdict::kUnsupported)) ==
          "unsupported");
}

void test_hdr_kind_regression() {
    // AC4（桌面半边）：既有三值的数值与名字必须一个不变 —— 证据里存的是旧数值。
    CHECK(static_cast<std::int32_t>(mediacodec::HdrKind::kSdr) == 0);
    CHECK(static_cast<std::int32_t>(mediacodec::HdrKind::kHdr10) == 1);
    CHECK(static_cast<std::int32_t>(mediacodec::HdrKind::kHlg) == 2);
    CHECK(std::string(mediacodec::hdr_kind_name(mediacodec::HdrKind::kSdr)) == "sdr");
    CHECK(std::string(mediacodec::hdr_kind_name(mediacodec::HdrKind::kHdr10)) == "hdr10");
    CHECK(std::string(mediacodec::hdr_kind_name(mediacodec::HdrKind::kHlg)) == "hlg");
    // 新增值在最后，名字点明来源。
    CHECK(static_cast<std::int32_t>(mediacodec::HdrKind::kDolbyVision) == 3);
    CHECK(std::string(mediacodec::hdr_kind_name(mediacodec::HdrKind::kDolbyVision)) ==
          "dolby_vision");
}

void test_dv_facts_defaults() {
    const mediacodec::DvFacts facts;
    CHECK(facts.verdict == mediacodec::DvVerdict::kNotDolbyVision);
    CHECK(facts.dv_profile == -1);
    CHECK(facts.dv_level == -1);
    CHECK(facts.bl_compatibility_id == -1);
    CHECK(!facts.configure_format_rebuilt);
}

}  // namespace

int main() {
    std::printf("== 210 dv_config_facts_tests ==\n");
    test_mime();
    test_profile_key_decode();
    test_level_key_decode();
    test_sniff_bl_codec();
    test_bl_codec_mime();
    test_assess_not_dv();
    test_assess_profile81_passthrough();
    test_assess_level_key_absent_still_passthrough();
    test_assess_profile5_reject();
    test_assess_out_of_scope_profile();
    test_assess_unrecognized_profile_key();
    test_assess_bl_codec_guards();
    test_verdict_names();
    test_hdr_kind_regression();
    test_dv_facts_defaults();
    std::printf("%d checks / %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
