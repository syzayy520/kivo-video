#ifndef KIVO_VIDEO_SUBTITLE_PLANE_OBSERVER_SUBTITLE_OBSERVER_H
#define KIVO_VIDEO_SUBTITLE_PLANE_OBSERVER_SUBTITLE_OBSERVER_H

#include "kivo/video/subtitle_plane/evidence/subtitle_evidence.h"
#include "kivo/video/subtitle_plane/snapshot/subtitle_snapshot_blob.h"

typedef struct KivoSubtitleEvent {
    KivoP8AbiHeader header;
    uint32_t kind;
    uint32_t reserved;
    KivoSubtitleSnapshotRevision revision;
    KivoSubtitleEvidenceReason reason;
    KivoSubtitleEvidenceSeverity severity;
    uint64_t payload0;
} KivoSubtitleEvent;

typedef struct KivoSubtitleObserverCallbacks {
    KivoP8AbiHeader header;
    void* user_data;
    void (KIVO_P8_CALL *on_event)(
        void* user_data,
        const KivoSubtitleEvent* event) KIVO_P8_NOEXCEPT;
} KivoSubtitleObserverCallbacks;

#endif
