#ifndef KIVO_VIDEO_SUBTITLE_PLANE_TYPES_SUBTITLE_TIME_H
#define KIVO_VIDEO_SUBTITLE_PLANE_TYPES_SUBTITLE_TIME_H

#include "kivo/video/subtitle_plane/types/subtitle_ids.h"

#define KIVO_SUBTITLE_TIME_INVALID INT64_MIN

typedef struct KivoSubtitleTime {
    int64_t microseconds;
} KivoSubtitleTime;

typedef struct KivoSubtitleDelayOffset {
    int64_t microseconds;
} KivoSubtitleDelayOffset;

#endif
