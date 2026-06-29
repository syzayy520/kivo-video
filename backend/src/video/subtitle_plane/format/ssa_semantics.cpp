#include "video/subtitle_plane/format/ssa_semantics.hpp"

#include <string_view>

namespace kivo::video::subtitle_plane::runtime {
namespace {

std::string_view view_of(const KivoSubtitleInputStringView text) noexcept {
    return text.utf8 == nullptr ? std::string_view{} :
        std::string_view(reinterpret_cast<const char*>(text.utf8),
                         static_cast<size_t>(text.utf8_bytes));
}

}  // namespace

KivoP8ApiStatus classify_ssa_semantics(
    const KivoSubtitleInputStringView text,
    KivoSubtitleStyleSemanticsReport* const out_report) noexcept {
    const std::string_view value = view_of(text);
    out_report->format = KivoSubtitleFormat_Ssa;
    if (value.find("\\pos(") != std::string_view::npos ||
        value.find("\\move(") != std::string_view::npos ||
        value.find("\\fad(") != std::string_view::npos) {
        out_report->reason = KivoSubtitleEvidenceReason_ExecutableScriptSemanticsRejected;
        return KivoP8ApiStatus_UnsupportedOperation;
    }
    if (value.find("\\N") != std::string_view::npos) {
        out_report->flags |= kP8StyleFlagExplicitLineBreak;
    }
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
