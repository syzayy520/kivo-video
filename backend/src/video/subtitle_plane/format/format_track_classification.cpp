#include "video/subtitle_plane/format/format_track_classification.hpp"

namespace kivo::video::subtitle_plane::runtime {

bool subtitle_format_is_text(const KivoSubtitleFormat format) noexcept {
    return format == KivoSubtitleFormat_Srt ||
           format == KivoSubtitleFormat_Ass ||
           format == KivoSubtitleFormat_Ssa ||
           format == KivoSubtitleFormat_WebVtt;
}

bool subtitle_format_is_bitmap(const KivoSubtitleFormat format) noexcept {
    return format == KivoSubtitleFormat_Pgs ||
           format == KivoSubtitleFormat_VobSub ||
           format == KivoSubtitleFormat_DvbSub;
}

KivoSubtitleFormat subtitle_format_for_embedded_codec(
    const KivoSubtitleEmbeddedCodecKind codec) noexcept {
    switch (codec) {
        case KivoSubtitleEmbeddedCodec_MatroskaSrt: return KivoSubtitleFormat_Srt;
        case KivoSubtitleEmbeddedCodec_MatroskaAssDialogue: return KivoSubtitleFormat_Ass;
        case KivoSubtitleEmbeddedCodec_MatroskaUtf8Text: return KivoSubtitleFormat_Srt;
        case KivoSubtitleEmbeddedCodec_Mp4WebVttSample: return KivoSubtitleFormat_WebVtt;
        case KivoSubtitleEmbeddedCodec_Pgs: return KivoSubtitleFormat_Pgs;
        case KivoSubtitleEmbeddedCodec_VobSub: return KivoSubtitleFormat_VobSub;
        case KivoSubtitleEmbeddedCodec_DvbSub: return KivoSubtitleFormat_DvbSub;
        default: return KivoSubtitleFormat_Unsupported;
    }
}

KivoSubtitleTrackKind subtitle_track_kind_for_source(
    const KivoSubtitleSourceKind source_kind,
    const KivoSubtitleFormat format) noexcept {
    if (subtitle_format_is_bitmap(format) || !subtitle_format_is_text(format)) {
        return KivoSubtitleTrackKind_UnsupportedBitmap;
    }
    if (source_kind == KivoSubtitleSourceKind_FilePathUtf8) {
        return KivoSubtitleTrackKind_ExternalFile;
    }
    if (source_kind == KivoSubtitleSourceKind_MemoryBytes ||
        source_kind == KivoSubtitleSourceKind_Reader ||
        source_kind == KivoSubtitleSourceKind_ProviderByteStream) {
        return KivoSubtitleTrackKind_ExternalMemory;
    }
    if (source_kind == KivoSubtitleSourceKind_EmbeddedTrack) {
        return KivoSubtitleTrackKind_EmbeddedPacket;
    }
    return KivoSubtitleTrackKind_UnsupportedBitmap;
}

}  // namespace kivo::video::subtitle_plane::runtime
