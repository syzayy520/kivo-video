#include "video/video_plane/backends/mediacodec/decode/video_color_facts.hpp"

#include "video/video_plane/backends/mediacodec/decode/color_format_names.hpp"
#include "video/video_plane/backends/mediacodec/decode/color_key_names.hpp"

namespace kivo::video::mediacodec {
namespace {

/// HEVC profile 里属于 >8bit 的那几个。数值来源 AOSP `MediaCodecInfo.CodecProfileLevel`：
/// HEVCProfileMain10 = 2、HEVCProfileMain10HDR10 = 4096、HEVCProfileMain10HDR10Plus = 8192。
[[nodiscard]] bool hevc_profile_is_10bit(std::int32_t profile) {
    return profile == 2 || profile == 4096 || profile == 8192;
}

/// AVC 的 High10 / High422 / High444 = 110 / 122 / 244。
[[nodiscard]] bool avc_profile_is_10bit(std::int32_t profile) {
    return profile == 110 || profile == 122 || profile == 244;
}

[[nodiscard]] bool source_bit_depth_known(const VideoColorFacts& facts) {
    if (facts.source_profile < 0) {
        return false;
    }
    return facts.mime == "video/hevc" || facts.mime == "video/avc";
}

[[nodiscard]] bool source_is_10bit(const VideoColorFacts& facts) {
    if (facts.mime == "video/hevc") {
        return hevc_profile_is_10bit(facts.source_profile);
    }
    if (facts.mime == "video/avc") {
        return avc_profile_is_10bit(facts.source_profile);
    }
    return false;
}

[[nodiscard]] std::string describe_output(const VideoColorFacts& facts) {
    std::string out = color_format::format_name(facts.negotiated_format) + "(" +
                      std::to_string(facts.negotiated_format) + ")";
    if (color_format::is_vendor_private(facts.negotiated_format)) {
        // 点明"厂商私有"很重要：读的人第一反应会是"这是不是一个我们没映射的常量"，
        // 而事实是"这个值根本不表达位深"。实测同一台设备上 8bit 与 10bit 源给出同一个值。
        out += " [vendor-private: does not express bit depth]";
    }
    return out;
}

}  // namespace

const char* color_verdict_name(ColorVerdict verdict) noexcept {
    switch (verdict) {
        case ColorVerdict::kTenBit:
            return "ten_bit";
        case ColorVerdict::kEightBit:
            return "eight_bit";
        case ColorVerdict::kIndeterminate:
            return "indeterminate";
    }
    return "unknown";
}

ColorAssessment assess_color(const VideoColorFacts& facts) {
    ColorAssessment out;
    out.source_bit_depth_known = source_bit_depth_known(facts);
    out.source_is_10bit = out.source_bit_depth_known && source_is_10bit(facts);

    const std::int32_t depth = color_format::format_bit_depth(facts.negotiated_format);
    out.verdict = depth == 10 ? ColorVerdict::kTenBit
                              : (depth == 8 ? ColorVerdict::kEightBit : ColorVerdict::kIndeterminate);

    if (!out.source_bit_depth_known) {
        // mime/profile 不在已知表里（VP9/AV1 等）。此时连"该不该是 10bit"都无从判断，
        // 但**输出位深仍然照报** —— 那是观测到的事实，不该被源侧的未知拖累。
        out.reason = "source codec/profile is not in the known table (mime=" + facts.mime +
                     ", profile=" + std::to_string(facts.source_profile) +
                     "); cannot say whether 10-bit is expected";
        return out;
    }

    if (out.source_is_10bit) {
        if (out.verdict == ColorVerdict::kTenBit) {
            return out;  // 匹配，无需解释
        }
        if (out.verdict == ColorVerdict::kEightBit) {
            out.reason = "source is 10-bit (profile " + std::to_string(facts.source_profile) +
                         ") but decoder negotiated 8-bit output " + describe_output(facts) +
                         " — this is a silent downgrade";
            return out;
        }
        out.reason = "source is 10-bit (profile " + std::to_string(facts.source_profile) +
                     ") but the negotiated output is " + describe_output(facts) +
                     " — bit depth is not expressed by this value, so 10-bit delivery is "
                     "unverified on this path";
        return out;
    }

    // 8 bit 源
    if (out.verdict == ColorVerdict::kTenBit) {
        out.reason = "source is 8-bit but decoder negotiated 10-bit output " +
                     describe_output(facts) + " (unexpected, not harmful)";
        return out;
    }
    if (out.verdict == ColorVerdict::kIndeterminate) {
        // 这里必须解释，否则读的人会以为"8bit 源也没查"。
        out.reason = "source is 8-bit; the negotiated output is " + describe_output(facts) +
                     " — bit depth is not expressed by this value, which does not affect this "
                     "verdict";
        return out;
    }
    return out;  // 8bit 源 + 8bit 输出：匹配，无需解释
}

HdrAssessment classify_hdr(const VideoColorFacts& facts) {
    const std::int32_t standard = facts.source_color_standard;
    const std::int32_t transfer = facts.source_color_transfer;

    // 依据串只造一次：四个分支都要用它，手写四遍迟早有一处少带一个键。
    const std::string evidence = "standard=" + std::to_string(standard) + "(" +
                                 color_key::standard_name(standard) + "), transfer=" +
                                 std::to_string(transfer) + "(" +
                                 color_key::transfer_name(transfer) + ")";

    HdrAssessment out;

    // 两个键都不存在：**不能写成"这是 SDR"**。轨道格式里没有标记，
    // 事实是"没有可判的东西"，而 E1 已经演示过解码器的默认值会把无标记片源
    // 报成 BT709/SDR —— 那个默认值不是片源信息。
    if (standard < 0 && transfer < 0) {
        out.kind = HdrKind::kSdr;
        out.reason =
            "track format carries neither color-standard nor color-transfer; nothing to classify "
            "or deliver (this is not a claim that the source is SDR)";
        return out;
    }

    if (transfer == color_key::kTransferSt2084) {
        if (standard == color_key::kStandardBt2020) {
            out.kind = HdrKind::kHdr10;
            out.reason = evidence;
            return out;
        }
        // PQ 却没有 BT2020 原色：这是自相矛盾的标记。宁可判 SDR 并说明，
        // 也不能把 SDR 内容按 PQ 送进显示链路 —— 那会把画面弄成又灰又暗。
        out.kind = HdrKind::kSdr;
        out.reason = evidence + " — PQ transfer without BT2020 primaries; refusing to treat it as HDR10";
        return out;
    }

    if (transfer == color_key::kTransferHlg) {
        if (standard == color_key::kStandardBt2020) {
            out.kind = HdrKind::kHlg;
            out.reason = evidence;
            return out;
        }
        out.kind = HdrKind::kSdr;
        out.reason = evidence + " — HLG transfer without BT2020 primaries; refusing to treat it as HLG";
        return out;
    }

    out.kind = HdrKind::kSdr;
    out.reason = evidence + " — transfer is neither ST2084(PQ) nor HLG";
    return out;
}

}  // namespace kivo::video::mediacodec
