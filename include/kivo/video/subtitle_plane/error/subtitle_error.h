#ifndef KIVO_VIDEO_SUBTITLE_PLANE_ERROR_SUBTITLE_ERROR_H
#define KIVO_VIDEO_SUBTITLE_PLANE_ERROR_SUBTITLE_ERROR_H

#include "kivo/video/subtitle_plane/foundation/p8_abi_header.h"

typedef enum KivoSubtitleResultStatus {
    KivoSubtitleResultStatus_Ok = 0,
    KivoSubtitleResultStatus_Empty = 1,
    KivoSubtitleResultStatus_NotReady = 2,
    KivoSubtitleResultStatus_Error = 3
} KivoSubtitleResultStatus;

typedef enum KivoSubtitlePlaneError {
    KivoSubtitlePlaneError_None = 0,
    KivoSubtitlePlaneError_NullPointer = 1,
    KivoSubtitlePlaneError_InvalidArgument = 2,
    KivoSubtitlePlaneError_MalformedInput = 3,
    KivoSubtitlePlaneError_IllegalEnumValue = 4,
    KivoSubtitlePlaneError_InvalidState = 5,
    KivoSubtitlePlaneError_NotReady = 6,
    KivoSubtitlePlaneError_ClosedSubtitlePlane = 7,
    KivoSubtitlePlaneError_ApiTableTooSmall = 8,
    KivoSubtitlePlaneError_UnsupportedAbiVersion = 9,
    KivoSubtitlePlaneError_ApiVersionMismatch = 10,
    KivoSubtitlePlaneError_UnsupportedSubtitleFormat = 11,
    KivoSubtitlePlaneError_UnsupportedSubtitleTrack = 12,
    KivoSubtitlePlaneError_UnsupportedOperation = 13,
    KivoSubtitlePlaneError_SubtitleSourceOpenFailed = 14,
    KivoSubtitlePlaneError_SubtitleSourceReadFailed = 15,
    KivoSubtitlePlaneError_SubtitleDiscoveryFailed = 16,
    KivoSubtitlePlaneError_SubtitleEncodingUnsupported = 17,
    KivoSubtitlePlaneError_SubtitleParseFailed = 18,
    KivoSubtitlePlaneError_SubtitleSecurityCapExceeded = 19,
    KivoSubtitlePlaneError_SubtitleDelayOutOfRange = 20,
    KivoSubtitlePlaneError_SubtitleInvalidTime = 21,
    KivoSubtitlePlaneError_SubtitleTrackNotFound = 22,
    KivoSubtitlePlaneError_SubtitleSeekFlushFailed = 23,
    KivoSubtitlePlaneError_SubtitleFrameUnavailableByPolicy = 24,
    KivoSubtitlePlaneError_SubtitleFramePoolExhausted = 25,
    KivoSubtitlePlaneError_SubtitleFrameAcquireContention = 26,
    KivoSubtitlePlaneError_SubtitleFrameLeaseInvalid = 27,
    KivoSubtitlePlaneError_SubtitleFrameLeaseDuplicateRelease = 28,
    KivoSubtitlePlaneError_SubtitleSnapshotLeaseInvalid = 29,
    KivoSubtitlePlaneError_SubtitleSnapshotBufferTooSmall = 30,
    KivoSubtitlePlaneError_SubtitleSnapshotBufferMalformed = 31,
    KivoSubtitlePlaneError_SubtitleStringInvalid = 32,
    KivoSubtitlePlaneError_SubtitleStringPoolExhausted = 33,
    KivoSubtitlePlaneError_SubtitleTextBufferNotFound = 34,
    KivoSubtitlePlaneError_SubtitleTextBufferBufferTooSmall = 35,
    KivoSubtitlePlaneError_SubtitleTextPoolExhausted = 36,
    KivoSubtitlePlaneError_SubtitleFontBlobNotFound = 37,
    KivoSubtitlePlaneError_SubtitleFontBlobBufferTooSmall = 38,
    KivoSubtitlePlaneError_SubtitleFontBlobStoreExceeded = 39,
    KivoSubtitlePlaneError_SubtitleTimelineTemporarilyUnavailable = 40,
    KivoSubtitlePlaneError_SubtitleTimelineNotStarted = 41,
    KivoSubtitlePlaneError_SubtitleTimelineInconsistent = 42,
    KivoSubtitlePlaneError_InvalidCueTimeRange = 43,
    KivoSubtitlePlaneError_InvalidRegion = 44,
    KivoSubtitlePlaneError_InvalidLanguageTag = 45,
    KivoSubtitlePlaneError_SubtitleGenerationWrapRisk = 46,
    KivoSubtitlePlaneError_SubtitleOutOfMemory = 47,
    KivoSubtitlePlaneError_SubtitleThreadCreationFailed = 48,
    KivoSubtitlePlaneError_InvalidCapValue = 49,
    KivoSubtitlePlaneError_ContractGap = 50
} KivoSubtitlePlaneError;

#endif
