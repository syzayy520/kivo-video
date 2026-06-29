#ifndef KIVO_VIDEO_SUBTITLE_PLANE_STREAM_SUBTITLE_STREAM_END_H
#define KIVO_VIDEO_SUBTITLE_PLANE_STREAM_SUBTITLE_STREAM_END_H

#include "kivo/video/subtitle_plane/generation/subtitle_generation.h"

typedef struct KivoEmbeddedSubtitleStreamEndRequest {
    KivoP8AbiHeader header;
    KivoSubtitleTrackId track_id;
    KivoSubtitlePacketGeneration packet_generation;
    KivoSubtitleCorrelationId correlation_id;
    uint32_t flags;
    uint32_t reserved;
} KivoEmbeddedSubtitleStreamEndRequest;

#endif
