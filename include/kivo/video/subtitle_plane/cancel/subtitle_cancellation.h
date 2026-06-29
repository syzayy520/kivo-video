#ifndef KIVO_VIDEO_SUBTITLE_PLANE_CANCEL_SUBTITLE_CANCELLATION_H
#define KIVO_VIDEO_SUBTITLE_PLANE_CANCEL_SUBTITLE_CANCELLATION_H

#include "kivo/video/subtitle_plane/foundation/p8_status.h"

typedef struct KivoSubtitleCancellationView {
    uint64_t operation_id;
    void* user_data;
    KivoP8CallbackStatus (KIVO_P8_CALL *is_cancelled)(
        void* user_data,
        uint64_t operation_id,
        uint32_t* out_cancelled) KIVO_P8_NOEXCEPT;
} KivoSubtitleCancellationView;

#endif
