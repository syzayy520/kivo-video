#ifndef KIVO_VIDEO_SUBTITLE_PLANE_EVIDENCE_SUBTITLE_EVIDENCE_H
#define KIVO_VIDEO_SUBTITLE_PLANE_EVIDENCE_SUBTITLE_EVIDENCE_H

#include "kivo/video/subtitle_plane/types/subtitle_ids.h"

typedef enum KivoSubtitleEvidenceSeverity {
    KivoSubtitleEvidenceSeverity_Info = 0,
    KivoSubtitleEvidenceSeverity_Warning = 1,
    KivoSubtitleEvidenceSeverity_Recoverable = 2,
    KivoSubtitleEvidenceSeverity_Unsupported = 3,
    KivoSubtitleEvidenceSeverity_CommandRejected = 4,
    KivoSubtitleEvidenceSeverity_FatalSubtitlePlane = 5,
    KivoSubtitleEvidenceSeverity_DiagnosticsOnly = 6,
    KivoSubtitleEvidenceSeverity_ArchitectureOnly = 7,
    KivoSubtitleEvidenceSeverity_TestOnly = 8
} KivoSubtitleEvidenceSeverity;

typedef enum KivoSubtitleEvidenceReason {
    KivoSubtitleEvidenceReason_None = 0,
    KivoSubtitleEvidenceReason_NotReady = 1,
    KivoSubtitleEvidenceReason_TimelineInconsistent = 2,
    KivoSubtitleEvidenceReason_InvalidLanguageTag = 3,
    KivoSubtitleEvidenceReason_InvalidRegion = 4,
    KivoSubtitleEvidenceReason_InvalidCueTimeRange = 5,
    KivoSubtitleEvidenceReason_SnapshotBufferMalformed = 6,
    KivoSubtitleEvidenceReason_AssPlayResMissing = 7,
    KivoSubtitleEvidenceReason_AssColorMalformed = 8,
    KivoSubtitleEvidenceReason_WebVttCueSettingUnsupported = 9,
    KivoSubtitleEvidenceReason_ExecutableScriptSemanticsRejected = 10,
    KivoSubtitleEvidenceReason_SourceOpenFailed = 11,
    KivoSubtitleEvidenceReason_SourceReadFailed = 12,
    KivoSubtitleEvidenceReason_DiscoveryFailed = 13,
    KivoSubtitleEvidenceReason_EmbeddedPacketWaitTimeout = 14,
    KivoSubtitleEvidenceReason_PacketPtsDiscontinuity = 15,
    KivoSubtitleEvidenceReason_PacketGenerationMismatch = 16,
    KivoSubtitleEvidenceReason_PrivacyRedacted = 17,
    KivoSubtitleEvidenceReason_ContractGap = 18
} KivoSubtitleEvidenceReason;

typedef struct KivoSubtitleEvidenceSummary {
    KivoSubtitleEvidenceId primary_evidence_id;
    KivoSubtitleEvidenceReason primary_reason;
    KivoSubtitleEvidenceSeverity max_severity;
    uint32_t info_count;
    uint32_t warning_count;
    uint32_t recoverable_count;
    uint32_t unsupported_count;
    uint32_t fatal_count;
    uint32_t flags;
    uint32_t reserved;
} KivoSubtitleEvidenceSummary;

#endif
