#ifndef KIVO_VIDEO_SUBTITLE_PLANE_COMMAND_SUBTITLE_COMMAND_H
#define KIVO_VIDEO_SUBTITLE_PLANE_COMMAND_SUBTITLE_COMMAND_H

#include "kivo/video/subtitle_plane/error/subtitle_error.h"
#include "kivo/video/subtitle_plane/types/subtitle_ids.h"

typedef enum KivoSubtitleCommandImmediateStatus {
    KivoSubtitleCommandImmediateStatus_Accepted = 0,
    KivoSubtitleCommandImmediateStatus_Rejected = 1,
    KivoSubtitleCommandImmediateStatus_NoOp = 2,
    KivoSubtitleCommandImmediateStatus_Superseded = 3,
    KivoSubtitleCommandImmediateStatus_Closed = 4
} KivoSubtitleCommandImmediateStatus;

typedef struct KivoSubtitleCommandToken {
    KivoSubtitleCommandId command_id;
    KivoSubtitleCorrelationId correlation_id;
    KivoSubtitleCommandImmediateStatus immediate_status;
    KivoSubtitlePlaneError immediate_error;
    KivoSubtitleSnapshotRevision accepted_snapshot_revision;
} KivoSubtitleCommandToken;

typedef enum KivoSubtitleCommandLifecycleState {
    KivoSubtitleCommandLifecycleState_Unknown = 0,
    KivoSubtitleCommandLifecycleState_Accepted = 1,
    KivoSubtitleCommandLifecycleState_Queued = 2,
    KivoSubtitleCommandLifecycleState_Running = 3,
    KivoSubtitleCommandLifecycleState_Completed = 4,
    KivoSubtitleCommandLifecycleState_Failed = 5,
    KivoSubtitleCommandLifecycleState_Cancelled = 6,
    KivoSubtitleCommandLifecycleState_Superseded = 7,
    KivoSubtitleCommandLifecycleState_Expired = 8
} KivoSubtitleCommandLifecycleState;

typedef struct KivoSubtitleCommandLifecycleSnapshot {
    KivoSubtitleCommandId command_id;
    KivoSubtitleCorrelationId correlation_id;
    KivoSubtitleCommandLifecycleState state;
    KivoSubtitlePlaneError error;
    KivoSubtitleSnapshotRevision accepted_snapshot_revision;
    KivoSubtitleSnapshotRevision last_update_revision;
    uint64_t accepted_tick;
    uint64_t completed_tick;
} KivoSubtitleCommandLifecycleSnapshot;

#endif
