#ifndef KIVO_VIDEO_SUBTITLE_PLANE_FORMAT_SUBTITLE_FORMAT_H
#define KIVO_VIDEO_SUBTITLE_PLANE_FORMAT_SUBTITLE_FORMAT_H

#include "kivo/video/subtitle_plane/foundation/p8_abi_header.h"

typedef enum KivoSubtitleFormat {
    KivoSubtitleFormat_Unknown = 0,
    KivoSubtitleFormat_Srt = 1,
    KivoSubtitleFormat_Ass = 2,
    KivoSubtitleFormat_Ssa = 3,
    KivoSubtitleFormat_WebVtt = 4,
    KivoSubtitleFormat_Pgs = 5,
    KivoSubtitleFormat_VobSub = 6,
    KivoSubtitleFormat_DvbSub = 7,
    KivoSubtitleFormat_Unsupported = 8
} KivoSubtitleFormat;

typedef enum KivoSubtitleTrackKind {
    KivoSubtitleTrackKind_ExternalFile = 0,
    KivoSubtitleTrackKind_ExternalMemory = 1,
    KivoSubtitleTrackKind_EmbeddedPacket = 2,
    KivoSubtitleTrackKind_UnsupportedBitmap = 3
} KivoSubtitleTrackKind;

typedef enum KivoSubtitleEmbeddedCodecKind {
    KivoSubtitleEmbeddedCodec_Unknown = 0,
    KivoSubtitleEmbeddedCodec_MatroskaSrt = 1,
    KivoSubtitleEmbeddedCodec_MatroskaAssDialogue = 2,
    KivoSubtitleEmbeddedCodec_MatroskaUtf8Text = 3,
    KivoSubtitleEmbeddedCodec_Mp4WebVttSample = 4,
    KivoSubtitleEmbeddedCodec_Mp4Tx3g = 5,
    KivoSubtitleEmbeddedCodec_MovText = 6,
    KivoSubtitleEmbeddedCodec_Pgs = 7,
    KivoSubtitleEmbeddedCodec_VobSub = 8,
    KivoSubtitleEmbeddedCodec_DvbSub = 9,
    KivoSubtitleEmbeddedCodec_Unsupported = 10
} KivoSubtitleEmbeddedCodecKind;

#endif
