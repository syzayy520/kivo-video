#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/format/ass_semantics.hpp"
#include "video/subtitle_plane/format/ssa_semantics.hpp"
#include "video/subtitle_plane/format/webvtt_semantics.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_classify_style_semantics(
    const KivoSubtitleFormat format,
    const KivoSubtitleInputStringView text,
    KivoSubtitleStyleSemanticsReport* const out_report) KIVO_P8_NOEXCEPT {
    if (out_report == nullptr) return KivoP8ApiStatus_NullPointer;
    if (text.utf8 == nullptr && text.utf8_bytes > 0) return KivoP8ApiStatus_InvalidArgument;
    *out_report = KivoSubtitleStyleSemanticsReport{};
    out_report->header.struct_size = sizeof(KivoSubtitleStyleSemanticsReport);
    out_report->format = format;
    if (format == KivoSubtitleFormat_Ssa) {
        return classify_ssa_semantics(text, out_report);
    }
    if (format == KivoSubtitleFormat_Ass) {
        return classify_ass_semantics(text, out_report);
    }
    if (format == KivoSubtitleFormat_WebVtt) {
        return classify_webvtt_semantics(text, out_report);
    }
    if (format == KivoSubtitleFormat_Srt) {
        return KivoP8ApiStatus_Ok;
    }
    out_report->reason = KivoSubtitleEvidenceReason_ContractGap;
    return KivoP8ApiStatus_UnsupportedOperation;
}

}  // namespace kivo::video::subtitle_plane::runtime
