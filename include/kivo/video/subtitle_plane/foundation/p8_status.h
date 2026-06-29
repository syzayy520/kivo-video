#ifndef KIVO_VIDEO_SUBTITLE_PLANE_FOUNDATION_P8_STATUS_H
#define KIVO_VIDEO_SUBTITLE_PLANE_FOUNDATION_P8_STATUS_H

#include "kivo/video/subtitle_plane/foundation/p8_version.h"

typedef enum KivoP8ApiStatus {
    KivoP8ApiStatus_Ok = 0,
    KivoP8ApiStatus_NullPointer = 1,
    KivoP8ApiStatus_InvalidArgument = 2,
    KivoP8ApiStatus_ApiTableTooSmall = 3,
    KivoP8ApiStatus_UnsupportedAbiVersion = 4,
    KivoP8ApiStatus_UnsupportedHostEndian = 5,
    KivoP8ApiStatus_NotReady = 6,
    KivoP8ApiStatus_Failed = 7,
    KivoP8ApiStatus_UnsupportedOperation = 8
} KivoP8ApiStatus;

typedef enum KivoP8CallbackStatus {
    KivoP8CallbackStatus_Ok = 0,
    KivoP8CallbackStatus_Cancelled = 1,
    KivoP8CallbackStatus_Failed = 2,
    KivoP8CallbackStatus_InvalidArgument = 3
} KivoP8CallbackStatus;

#endif
