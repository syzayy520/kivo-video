#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include "video/subtitle_plane/format/ssa_semantics.hpp"

namespace {

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

KivoSubtitleInputStringView text_view(const char* value) {
    KivoSubtitleInputStringView view{};
    view.utf8 = reinterpret_cast<const uint8_t*>(value);
    uint64_t bytes = 0;
    while (value[bytes] != '\0') ++bytes;
    view.utf8_bytes = bytes;
    return view;
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();
    KivoSubtitleStyleSemanticsReport report{};

    if (api.classify_style_semantics(
            KivoSubtitleFormat_Ssa,
            text_view("Dialogue: 0,0:00:01,0:00:02,Default,,0,0,0,,A\\NB"),
            &report) != KivoP8ApiStatus_Ok) {
        return 1;
    }
    if ((report.flags & kivo::video::subtitle_plane::runtime::kP8StyleFlagExplicitLineBreak) == 0u) {
        return 2;
    }

    if (api.classify_style_semantics(
            KivoSubtitleFormat_Ssa,
            text_view("{\\pos(10,20)}move"),
            &report) != KivoP8ApiStatus_UnsupportedOperation) {
        return 3;
    }
    if (report.reason != KivoSubtitleEvidenceReason_ExecutableScriptSemanticsRejected) return 4;

    if (api.classify_style_semantics(
            KivoSubtitleFormat_Ass,
            text_view("PlayResX: 640\nPlayResY: 480\nStyle: Default,&H112233&"),
            &report) != KivoP8ApiStatus_Ok) {
        return 5;
    }
    if (report.primary_bgr != 0x112233u || report.primary_alpha != 255u) return 6;
    if (report.reason == KivoSubtitleEvidenceReason_AssPlayResMissing) return 7;

    if (api.classify_style_semantics(
            KivoSubtitleFormat_Ass,
            text_view("PlayResX: 640\nPlayResY: 480\nStyle: Default,&H80112233&"),
            &report) != KivoP8ApiStatus_Ok) {
        return 8;
    }
    if (report.primary_bgr != 0x112233u || report.primary_alpha != 127u) return 9;

    if (api.classify_style_semantics(
            KivoSubtitleFormat_Ass,
            text_view("Style: Default,&H112233&"),
            &report) != KivoP8ApiStatus_Ok) {
        return 10;
    }
    if (report.reason != KivoSubtitleEvidenceReason_AssPlayResMissing) return 11;
    if ((report.flags & kivo::video::subtitle_plane::runtime::kP8StyleFlagNormalizedFallback) == 0u) {
        return 12;
    }
    if (report.normalized_x != 384 || report.normalized_y != 288) return 13;

    if (api.classify_style_semantics(
            KivoSubtitleFormat_Ass,
            text_view("PlayResX: 640\nPlayResY: 480\nStyle: Default,&H12Z&"),
            &report) != KivoP8ApiStatus_Failed) {
        return 14;
    }
    if (report.reason != KivoSubtitleEvidenceReason_AssColorMalformed) return 15;

    if (api.classify_style_semantics(
            KivoSubtitleFormat_WebVtt,
            text_view("00:00.000 --> 00:01.000 line:10% position:40% vertical:rl"),
            &report) != KivoP8ApiStatus_Ok) {
        return 16;
    }
    if ((report.flags & kivo::video::subtitle_plane::runtime::kP8StyleFlagWebVttPosition) == 0u) {
        return 17;
    }
    if ((report.flags & kivo::video::subtitle_plane::runtime::kP8StyleFlagWebVttVertical) == 0u) {
        return 18;
    }

    if (api.classify_style_semantics(
            KivoSubtitleFormat_WebVtt,
            text_view("00:00.000 --> 00:01.000 region:credits"),
            &report) != KivoP8ApiStatus_UnsupportedOperation) {
        return 19;
    }
    if (report.reason != KivoSubtitleEvidenceReason_WebVttCueSettingUnsupported) return 20;

    KivoSubtitleInputStringView bad{};
    bad.utf8_bytes = 2;
    if (api.classify_style_semantics(KivoSubtitleFormat_Srt, bad, &report) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 21;
    }
    if (api.classify_style_semantics(KivoSubtitleFormat_Pgs, text_view("bitmap"), &report) !=
        KivoP8ApiStatus_UnsupportedOperation) {
        return 22;
    }

    return 0;
}
