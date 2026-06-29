#ifndef KIVO_VIDEO_SUBTITLE_PLANE_SNAPSHOT_SUBTITLE_SNAPSHOT_BLOB_H
#define KIVO_VIDEO_SUBTITLE_PLANE_SNAPSHOT_SUBTITLE_SNAPSHOT_BLOB_H

#include "kivo/video/subtitle_plane/evidence/subtitle_evidence.h"
#include "kivo/video/subtitle_plane/frame/subtitle_frame_lease.h"
#include "kivo/video/subtitle_plane/metrics/subtitle_metrics.h"
#include "kivo/video/subtitle_plane/state/subtitle_plane_state.h"

#define KIVO_P8_SNAPSHOT_BLOB_MAGIC 0x50385342u
#define KIVO_P8_SNAPSHOT_ENDIAN_LITTLE 1u
#define KIVO_P8_SNAPSHOT_BLOB_REVISION 1u
#define KIVO_P8_SNAPSHOT_ALIGNMENT 8u

typedef enum KivoSubtitleSnapshotLeaseStatus {
    KivoSubtitleSnapshotLeaseStatus_Ok = 0,
    KivoSubtitleSnapshotLeaseStatus_Empty = 1,
    KivoSubtitleSnapshotLeaseStatus_Error = 2,
    KivoSubtitleSnapshotLeaseStatus_Closed = 3,
    KivoSubtitleSnapshotLeaseStatus_Tombstoned = 4
} KivoSubtitleSnapshotLeaseStatus;

typedef enum KivoSubtitleSnapshotSectionKind {
    KivoSubtitleSnapshotSection_None = 0,
    KivoSubtitleSnapshotSection_TrackArray = 1,
    KivoSubtitleSnapshotSection_EvidenceSummaryArray = 2,
    KivoSubtitleSnapshotSection_FontAttachmentArray = 3,
    KivoSubtitleSnapshotSection_CommandSummaryArray = 4,
    KivoSubtitleSnapshotSection_Reserved = 0xffffffffu
} KivoSubtitleSnapshotSectionKind;

typedef struct KivoSubtitleSnapshotLease {
    KivoSubtitleSnapshotLeaseId snapshot_lease_id;
    uint64_t snapshot_lease_generation;
    KivoSubtitleSnapshotRevision snapshot_revision;
    KivoSubtitleSnapshotLeaseStatus status;
    KivoSubtitlePlaneError error;
    uint32_t reserved;
} KivoSubtitleSnapshotLease;

typedef struct KivoSubtitleSnapshotBlobSection {
    uint32_t kind;
    uint32_t element_size;
    uint64_t count;
    uint64_t offset;
    uint64_t bytes;
} KivoSubtitleSnapshotBlobSection;

typedef struct KivoSubtitleSnapshotBlobHeader {
    uint32_t magic;
    uint16_t endian;
    uint16_t header_size;
    uint32_t abi_major;
    uint32_t blob_revision;
    uint64_t total_bytes;
    uint64_t section_count;
    uint64_t section_table_offset;
    KivoSubtitlePlaneState state;
    uint32_t reserved_state;
    KivoSubtitleSnapshotRevision revision;
    KivoSubtitleFrameGeneration current_generation;
    KivoSubtitleTrackId selected_track;
    KivoSubtitleDelayOffset delay_offset;
    KivoSubtitleTime last_playback_time;
    KivoSubtitleTime last_effective_subtitle_time;
    KivoSubtitleEvidenceSummary evidence_summary;
    KivoSubtitleMetricsSnapshot metrics;
    uint32_t flags;
    uint32_t reserved;
} KivoSubtitleSnapshotBlobHeader;

#endif
