#ifndef KIVO_VIDEO_SUBTITLE_PLANE_RESOURCE_SUBTITLE_RESOURCE_TOKEN_H
#define KIVO_VIDEO_SUBTITLE_PLANE_RESOURCE_SUBTITLE_RESOURCE_TOKEN_H

#include "kivo/video/subtitle_plane/snapshot/subtitle_snapshot_blob.h"

typedef enum KivoSubtitleResourceAccessKind {
    KivoSubtitleResourceAccessKind_None = 0,
    KivoSubtitleResourceAccessKind_FrameLease = 1,
    KivoSubtitleResourceAccessKind_SnapshotLease = 2
} KivoSubtitleResourceAccessKind;

typedef struct KivoSubtitleResourceAccessToken {
    KivoSubtitleResourceAccessKind kind;
    uint32_t reserved;
    uint64_t lease_id;
    uint64_t lease_generation;
    KivoSubtitleSnapshotLeaseId snapshot_lease_id;
    uint64_t snapshot_lease_generation;
    KivoSubtitleSnapshotRevision snapshot_revision;
} KivoSubtitleResourceAccessToken;

#endif
