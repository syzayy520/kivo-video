#ifndef KIVO_VIDEO_SUBTITLE_PLANE_TYPES_SUBTITLE_IDS_H
#define KIVO_VIDEO_SUBTITLE_PLANE_TYPES_SUBTITLE_IDS_H

#include "kivo/video/subtitle_plane/foundation/p8_abi_header.h"

typedef struct KivoSubtitlePlaneHandle { uint64_t value; } KivoSubtitlePlaneHandle;
typedef struct KivoSubtitleCommandId { uint64_t value; } KivoSubtitleCommandId;
typedef struct KivoSubtitleCorrelationId { uint64_t value; } KivoSubtitleCorrelationId;
typedef struct KivoSubtitleTrackId { uint64_t value; } KivoSubtitleTrackId;
typedef struct KivoSubtitleCueId { uint64_t value; } KivoSubtitleCueId;
typedef struct KivoSubtitleFrameId { uint64_t value; } KivoSubtitleFrameId;
typedef struct KivoSubtitleSnapshotId { uint64_t value; } KivoSubtitleSnapshotId;
typedef struct KivoSubtitleSourceId { uint64_t value; } KivoSubtitleSourceId;
typedef struct KivoSubtitlePacketId { uint64_t value; } KivoSubtitlePacketId;
typedef struct KivoSubtitleTextBufferId { uint64_t value; } KivoSubtitleTextBufferId;
typedef struct KivoSubtitleFontBlobId { uint64_t value; } KivoSubtitleFontBlobId;
typedef struct KivoSubtitleStringId { uint64_t value; } KivoSubtitleStringId;
typedef struct KivoSubtitleEvidenceId { uint64_t value; } KivoSubtitleEvidenceId;
typedef struct KivoSubtitleSnapshotLeaseId { uint64_t value; } KivoSubtitleSnapshotLeaseId;
typedef struct KivoSubtitleDiscoveryId { uint64_t value; } KivoSubtitleDiscoveryId;
typedef struct KivoSubtitleSubscriptionToken { uint64_t value; } KivoSubtitleSubscriptionToken;
typedef struct KivoSubtitleSeekEpoch { uint64_t value; } KivoSubtitleSeekEpoch;
typedef struct KivoP8PlaybackRevision { uint64_t value; } KivoP8PlaybackRevision;
typedef struct KivoP8GraphSnapshotRevision { uint64_t value; } KivoP8GraphSnapshotRevision;
typedef struct KivoSubtitleSnapshotRevision { uint64_t value; } KivoSubtitleSnapshotRevision;

#endif
