#include "video/subtitle_plane/format/ass_semantics.hpp"

#include <string_view>

namespace kivo::video::subtitle_plane::runtime {
namespace {

uint32_t hex_value(const char value) noexcept {
    if (value >= '0' && value <= '9') return static_cast<uint32_t>(value - '0');
    if (value >= 'A' && value <= 'F') return static_cast<uint32_t>(value - 'A' + 10);
    if (value >= 'a' && value <= 'f') return static_cast<uint32_t>(value - 'a' + 10);
    return 16;
}

bool parse_hex(const std::string_view digits, uint32_t* const out_value) noexcept {
    uint32_t value = 0;
    for (const char digit : digits) {
        const uint32_t nibble = hex_value(digit);
        if (nibble > 15) return false;
        value = (value << 4u) | nibble;
    }
    *out_value = value;
    return true;
}

std::string_view view_of(const KivoSubtitleInputStringView text) noexcept {
    return text.utf8 == nullptr ? std::string_view{} :
        std::string_view(reinterpret_cast<const char*>(text.utf8),
                         static_cast<size_t>(text.utf8_bytes));
}

}  // namespace

KivoP8ApiStatus classify_ass_semantics(
    const KivoSubtitleInputStringView text,
    KivoSubtitleStyleSemanticsReport* const out_report) noexcept {
    const std::string_view value = view_of(text);
    out_report->format = KivoSubtitleFormat_Ass;
    if (value.find("\\N") != std::string_view::npos) {
        out_report->flags |= kP8StyleFlagExplicitLineBreak;
    }
    const size_t marker = value.find("&H");
    if (marker != std::string_view::npos) {
        size_t end = marker + 2;
        while (end < value.size() && hex_value(value[end]) < 16) ++end;
        const std::string_view digits = value.substr(marker + 2, end - marker - 2);
        if (digits.size() != 6 && digits.size() != 8) {
            out_report->reason = KivoSubtitleEvidenceReason_AssColorMalformed;
            return KivoP8ApiStatus_Failed;
        }
        uint32_t color = 0;
        if (!parse_hex(digits, &color)) {
            out_report->reason = KivoSubtitleEvidenceReason_AssColorMalformed;
            return KivoP8ApiStatus_Failed;
        }
        if (digits.size() == 8) {
            const uint32_t inverse_alpha = (color >> 24u) & 0xffu;
            out_report->primary_alpha = 255u - inverse_alpha;
            out_report->primary_bgr = color & 0x00ffffffu;
        } else {
            out_report->primary_alpha = 255u;
            out_report->primary_bgr = color;
        }
    }
    if (value.find("PlayResX") == std::string_view::npos ||
        value.find("PlayResY") == std::string_view::npos) {
        out_report->reason = KivoSubtitleEvidenceReason_AssPlayResMissing;
        out_report->flags |= kP8StyleFlagNormalizedFallback;
        out_report->normalized_x = 384;
        out_report->normalized_y = 288;
    }
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
