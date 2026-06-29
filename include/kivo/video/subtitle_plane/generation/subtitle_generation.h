#ifndef KIVO_VIDEO_SUBTITLE_PLANE_GENERATION_SUBTITLE_GENERATION_H
#define KIVO_VIDEO_SUBTITLE_PLANE_GENERATION_SUBTITLE_GENERATION_H

#include "kivo/video/subtitle_plane/types/subtitle_time.h"

typedef struct KivoSubtitleContentGeneration {
    uint64_t source_generation;
    uint64_t track_generation;
    uint64_t parser_generation;
} KivoSubtitleContentGeneration;

typedef struct KivoSubtitlePacketGeneration {
    uint64_t source_generation;
    uint64_t track_generation;
    KivoSubtitleSeekEpoch seek_epoch;
    uint64_t packet_generation;
} KivoSubtitlePacketGeneration;

typedef struct KivoSubtitleTimelineGeneration {
    KivoSubtitleSeekEpoch seek_epoch;
    uint64_t delay_epoch;
    KivoP8PlaybackRevision playback_revision;
    KivoP8GraphSnapshotRevision graph_snapshot_revision;
} KivoSubtitleTimelineGeneration;

typedef struct KivoSubtitleFrameGeneration {
    KivoSubtitleContentGeneration content;
    KivoSubtitleTimelineGeneration timeline;
} KivoSubtitleFrameGeneration;

#endif
