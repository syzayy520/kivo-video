#ifndef KIVO_VIDEO_SUBTITLE_PLANE_INPUT_SUBTITLE_INPUT_VIEWS_H
#define KIVO_VIDEO_SUBTITLE_PLANE_INPUT_SUBTITLE_INPUT_VIEWS_H

#include "kivo/video/subtitle_plane/foundation/p8_abi_header.h"

typedef struct KivoSubtitleInputStringView {
    const uint8_t* utf8;
    uint64_t utf8_bytes;
} KivoSubtitleInputStringView;

typedef struct KivoSubtitleInputBytesView {
    const uint8_t* data;
    uint64_t bytes;
} KivoSubtitleInputBytesView;

typedef struct KivoMutableByteSpan {
    uint8_t* data;
    uint64_t bytes;
} KivoMutableByteSpan;

#endif
