#include "video/video_plane/backends/mediacodec/decode/dv_config_facts.hpp"

namespace kivo::video::mediacodec {
namespace {

/// 2 的幂判定的位技巧：`v & (v-1) == 0`。0 不是 2 的幂（它会让表达式也为 0，先排除）。
[[nodiscard]] bool is_power_of_two(std::int32_t value) noexcept {
    return value > 0 && (value & (value - 1)) == 0;
}

/// 平台键是 `1<<n` 位标志，n 最大到 30（int32 内）。循环有界，不依赖内建函数
/// （`__builtin_ctz` 在 MSVC 上没有）。
[[nodiscard]] std::int32_t power_two_exponent(std::int32_t value) noexcept {
    for (std::int32_t p = 0; p < 31; ++p) {
        if ((1 << p) == value) {
            return p;
        }
    }
    return -1;
}

}  // namespace

const char* dv_verdict_name(DvVerdict verdict) noexcept {
    switch (verdict) {
        case DvVerdict::kNotDolbyVision:
            return "not_dolby_vision";
        case DvVerdict::kPassthroughHdr10:
            return "passthrough_hdr10";
        case DvVerdict::kUnsupported:
            return "unsupported";
    }
    return "unknown";
}

bool is_dolby_vision_mime(std::string_view mime) {
    return mime == "video/dolby-vision";
}

std::int32_t dv_profile_from_key(std::int32_t profile_key) {
    if (!is_power_of_two(profile_key)) {
        return -1;
    }
    return power_two_exponent(profile_key);
}

std::int32_t dv_level_from_key(std::int32_t level_key) {
    if (!is_power_of_two(level_key)) {
        return -1;
    }
    const std::int32_t exponent = power_two_exponent(level_key);
    return exponent < 0 ? -1 : exponent + 1;  // 键是 1<<(level-1)
}

BlCodec sniff_bl_codec(const std::uint8_t* csd0, std::size_t size) {
    if (csd0 == nullptr || size < 4) {
        return BlCodec::kUnknown;
    }
    // MediaCodec 的 csd-0 是 Annex-B：以 00 00 01（或 00 00 00 01）起始码开头。
    // 先找起始码，再读第一个 NAL 的头字节 —— 直接取 csd0[0] 会读到起始码的 0x00。
    for (std::size_t i = 0; i + 3 < size && i < 8; ++i) {
        if (csd0[i] == 0x00 && csd0[i + 1] == 0x00 && csd0[i + 2] == 0x01) {
            const std::uint8_t b0 = csd0[i + 3];
            if ((b0 & 0x80) != 0) {
                return BlCodec::kUnknown;  // forbidden_zero_bit 必须为 0
            }
            // HEVC：forbidden(1) | type(6) | …。VPS=32（0x40）、SPS=33（0x42）。
            const std::uint8_t hevc_type = static_cast<std::uint8_t>(b0 & 0x7E);
            if (hevc_type == 0x40 || hevc_type == 0x42) {
                return BlCodec::kHevc;
            }
            // AVC：forbidden(1) | ref_idc(2) | type(5)。SPS type=7。
            if ((b0 & 0x1F) == 7) {
                return BlCodec::kAvc;
            }
            return BlCodec::kUnknown;
        }
    }
    return BlCodec::kUnknown;
}

const char* bl_codec_mime(BlCodec codec) noexcept {
    switch (codec) {
        case BlCodec::kHevc:
            return "video/hevc";
        case BlCodec::kAvc:
            return "video/avc";
        case BlCodec::kUnknown:
            break;
    }
    return "";
}

DvAssessment assess_dv(const std::string& mime, const DvTrackKeys& keys, BlCodec bl_codec) {
    DvAssessment out;
    if (!is_dolby_vision_mime(mime)) {
        out.reason = "mime is " + mime + "; not a Dolby Vision track";
        return out;
    }

    out.dv_profile = dv_profile_from_key(keys.profile_key);
    out.dv_level = dv_level_from_key(keys.level_key);

    if (out.dv_profile < 0) {
        out.verdict = DvVerdict::kUnsupported;
        out.reason = "mime is video/dolby-vision but the platform profile key (raw " +
                     std::to_string(keys.profile_key) +
                     ") is not a recognized power-of-two encoding; refusing to guess";
        return out;
    }
    if (out.dv_profile == 5) {
        out.verdict = DvVerdict::kUnsupported;
        out.reason = "dv_profile=5 (IPT-coded BL, not backward-compatible with HDR10); "
                     "refused before configure so the failure names the profile instead of a "
                     "generic decoder error";
        return out;
    }
    if (out.dv_profile != 8) {
        out.verdict = DvVerdict::kUnsupported;
        out.reason = "dv_profile=" + std::to_string(out.dv_profile) +
                     " is outside tier-A scope (8.x passthrough only)";
        return out;
    }

    // profile 8.x：BL 是 HDR10 兼容的 HEVC 流。
    if (bl_codec != BlCodec::kHevc) {
        out.verdict = DvVerdict::kUnsupported;
        out.reason = std::string("dv_profile=8 but csd-0 does not start with an HEVC VPS/SPS "
                                 "(sniffed ") +
                     (bl_codec == BlCodec::kAvc ? "AVC" : "unknown") +
                     "); tier-A passthrough only covers HEVC BL";
        return out;
    }
    out.verdict = DvVerdict::kPassthroughHdr10;
    out.bl_mime = bl_codec_mime(bl_codec);
    out.reason =
        "dv_profile=8 (HDR10-compatible BL); this device has no dolby-vision decoder (probe "
        "track_format_dump: createDecoderByType returned null), so the BL is handed to the HEVC "
        "decoder with the mime rewritten; RPU NALs stay in-stream and are dropped by the "
        "decoder; DV metadata itself is not interpreted (tier A has no mapping layer)";
    return out;
}

}  // namespace kivo::video::mediacodec
