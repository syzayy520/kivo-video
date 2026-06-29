#include "video/subtitle_plane/evidence/evidence_summary.hpp"

namespace kivo::video::subtitle_plane::runtime {

void record_evidence(PlaneStateRecord& record,
                     const KivoSubtitleEvidenceReason reason,
                     const KivoSubtitleEvidenceSeverity severity) noexcept {
    KivoSubtitleEvidenceSummary& summary = record.evidence_summary;
    if (summary.primary_evidence_id.value == 0) {
        summary.primary_evidence_id.value = 1;
        summary.primary_reason = reason;
    }
    if (severity > summary.max_severity) {
        summary.max_severity = severity;
    }
    if (severity == KivoSubtitleEvidenceSeverity_Info) ++summary.info_count;
    if (severity == KivoSubtitleEvidenceSeverity_Warning) ++summary.warning_count;
    if (severity == KivoSubtitleEvidenceSeverity_Recoverable) ++summary.recoverable_count;
    if (severity == KivoSubtitleEvidenceSeverity_Unsupported) ++summary.unsupported_count;
    if (severity == KivoSubtitleEvidenceSeverity_FatalSubtitlePlane) ++summary.fatal_count;
}

}  // namespace kivo::video::subtitle_plane::runtime
