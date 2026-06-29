#ifndef KIVO_VIDEO_SUBTITLE_PLANE_STATE_SUBTITLE_PLANE_STATE_H
#define KIVO_VIDEO_SUBTITLE_PLANE_STATE_SUBTITLE_PLANE_STATE_H

#include "kivo/video/subtitle_plane/foundation/p8_abi_header.h"

typedef enum KivoSubtitlePlaneState {
    KivoSubtitlePlaneState_NotCreated = 0,
    KivoSubtitlePlaneState_Created = 1,
    KivoSubtitlePlaneState_Discovering = 2,
    KivoSubtitlePlaneState_OpeningSource = 3,
    KivoSubtitlePlaneState_Reading = 4,
    KivoSubtitlePlaneState_Parsing = 5,
    KivoSubtitlePlaneState_Indexing = 6,
    KivoSubtitlePlaneState_Ready = 7,
    KivoSubtitlePlaneState_Priming = 8,
    KivoSubtitlePlaneState_Active = 9,
    KivoSubtitlePlaneState_Paused = 10,
    KivoSubtitlePlaneState_Seeking = 11,
    KivoSubtitlePlaneState_Flushing = 12,
    KivoSubtitlePlaneState_TrackSwitching = 13,
    KivoSubtitlePlaneState_EosReached = 14,
    KivoSubtitlePlaneState_TimelineUnavailable = 15,
    KivoSubtitlePlaneState_TimelineNotStarted = 16,
    KivoSubtitlePlaneState_Recovering = 17,
    KivoSubtitlePlaneState_Closing = 18,
    KivoSubtitlePlaneState_Closed = 19,
    KivoSubtitlePlaneState_Failed = 20
} KivoSubtitlePlaneState;

typedef enum KivoSubtitleTimelineAvailability {
    KivoSubtitleTimelineAvailability_Available = 0,
    KivoSubtitleTimelineAvailability_TemporarilyUnavailable = 1,
    KivoSubtitleTimelineAvailability_NotYetStarted = 2,
    KivoSubtitleTimelineAvailability_Seeking = 3,
    KivoSubtitleTimelineAvailability_Buffering = 4,
    KivoSubtitleTimelineAvailability_Recovering = 5,
    KivoSubtitleTimelineAvailability_Closed = 6,
    KivoSubtitleTimelineAvailability_Failed = 7
} KivoSubtitleTimelineAvailability;

#endif
