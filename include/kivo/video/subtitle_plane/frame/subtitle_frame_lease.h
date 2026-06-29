#ifndef KIVO_VIDEO_SUBTITLE_PLANE_FRAME_SUBTITLE_FRAME_LEASE_H
#define KIVO_VIDEO_SUBTITLE_PLANE_FRAME_SUBTITLE_FRAME_LEASE_H

#include "kivo/video/subtitle_plane/error/subtitle_error.h"
#include "kivo/video/subtitle_plane/generation/subtitle_generation.h"

typedef enum KivoSubtitleFrameLeaseStatus {
    KivoSubtitleFrameLeaseStatus_Ok = 0,
    KivoSubtitleFrameLeaseStatus_Empty = 1,
    KivoSubtitleFrameLeaseStatus_Error = 2,
    KivoSubtitleFrameLeaseStatus_Closed = 3,
    KivoSubtitleFrameLeaseStatus_PoolExhausted = 4,
    KivoSubtitleFrameLeaseStatus_Tombstoned = 5
} KivoSubtitleFrameLeaseStatus;

typedef struct KivoSubtitleFrameLease {
    KivoSubtitleFrameId frame_id;
    KivoSubtitleSnapshotRevision snapshot_revision;
    KivoSubtitleFrameGeneration generation;
    uint64_t lease_id;
    uint64_t lease_generation;
    const void* frame;
    KivoSubtitleFrameLeaseStatus status;
    KivoSubtitlePlaneError error;
    uint32_t reserved;
} KivoSubtitleFrameLease;

#endif
