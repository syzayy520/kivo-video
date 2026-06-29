#ifndef KIVO_VIDEO_SUBTITLE_PLANE_REQUEST_SUBTITLE_PLANE_REQUESTS_H
#define KIVO_VIDEO_SUBTITLE_PLANE_REQUEST_SUBTITLE_PLANE_REQUESTS_H

#include "kivo/video/subtitle_plane/cancel/subtitle_cancellation.h"
#include "kivo/video/subtitle_plane/config/subtitle_plane_config.h"
#include "kivo/video/subtitle_plane/evidence/subtitle_evidence.h"
#include "kivo/video/subtitle_plane/format/subtitle_format.h"
#include "kivo/video/subtitle_plane/source/subtitle_source.h"

typedef struct KivoSubtitleDiscoveryBeginRequest {
    KivoP8AbiHeader header;
    uint32_t flags;
    uint32_t reserved;
    KivoSubtitleInputStringView preferred_language_utf8;
    KivoSubtitleCorrelationId correlation_id;
} KivoSubtitleDiscoveryBeginRequest;

typedef struct KivoSubtitleFrameBuildRequest {
    KivoP8AbiHeader header;
    KivoSubtitleTime playback_time;
    KivoP8PlaybackRevision playback_revision;
    KivoP8GraphSnapshotRevision graph_snapshot_revision;
    KivoSubtitleSeekEpoch seek_epoch;
    KivoSubtitleCancellationView cancellation;
    uint32_t flags;
    uint32_t hazard_retry_budget;
} KivoSubtitleFrameBuildRequest;

typedef struct KivoSubtitleSnapshotAcquireRequest {
    KivoP8AbiHeader header;
    KivoP8PlaybackRevision playback_revision;
    KivoP8GraphSnapshotRevision graph_snapshot_revision;
    KivoSubtitleCancellationView cancellation;
    uint32_t flags;
    uint32_t hazard_retry_budget;
} KivoSubtitleSnapshotAcquireRequest;

typedef struct KivoSubtitleStyleSemanticsReport {
    KivoP8AbiHeader header;
    KivoSubtitleFormat format;
    KivoSubtitleEvidenceReason reason;
    uint32_t flags;
    uint32_t reserved;
    uint64_t normalized_x;
    uint64_t normalized_y;
    uint32_t primary_bgr;
    uint32_t primary_alpha;
} KivoSubtitleStyleSemanticsReport;

#endif
