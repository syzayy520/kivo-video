#include "video/subtitle_plane/format/webvtt_semantics.hpp"

#include <string_view>

namespace kivo::video::subtitle_plane::runtime {
namespace {

std::string_view view_of(const KivoSubtitleInputStringView text) noexcept {
    return text.utf8 == nullptr ? std::string_view{} :
        std::string_view(reinterpret_cast<const char*>(text.utf8),
                         static_cast<size_t>(text.utf8_bytes));
}

}  // namespace

KivoP8ApiStatus classify_webvtt_semantics(
    const KivoSubtitleInputStringView text,
    KivoSubtitleStyleSemanticsReport* const out_report) noexcept {
    const std::string_view value = view_of(text);
    out_report->format = KivoSubtitleFormat_WebVtt;
    if (value.find("line:") != std::string_view::npos) {
        out_report->flags |= kP8StyleFlagWebVttPosition;
    }
    if (value.find("position:") != std::string_view::npos ||
        value.find("size:") != std::string_view::npos ||
        value.find("align:") != std::string_view::npos) {
        out_report->flags |= kP8StyleFlagWebVttPosition;
    }
    if (value.find("vertical:") != std::string_view::npos ||
        value.find("snap-to-lines:false") != std::string_view::npos) {
        out_report->flags |= kP8StyleFlagWebVttVertical;
    }
    if (value.find("region:") != std::string_view::npos) {
        out_report->reason = KivoSubtitleEvidenceReason_WebVttCueSettingUnsupported;
        return KivoP8ApiStatus_UnsupportedOperation;
    }
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
