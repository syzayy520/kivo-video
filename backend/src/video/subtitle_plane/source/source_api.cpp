#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/command/command_lifecycle.hpp"
#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/evidence/evidence_summary.hpp"
#include "video/subtitle_plane/observer/observer_registry.hpp"
#include "video/subtitle_plane/privacy/redacted_identity.hpp"
#include "video/subtitle_plane/source/external_reader_lifecycle.hpp"
#include "video/subtitle_plane/source/source_descriptor_validation.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_set_source(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSourceDescriptor* const source,
    const KivoSubtitleCorrelationId correlation_id,
    KivoSubtitleCommandToken* const out_token) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (observer_mutation_blocked(*record)) {
        return KivoP8ApiStatus_UnsupportedOperation;
    }
    if (source == nullptr || out_token == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    const KivoSubtitlePlaneError validation_error = validate_source_descriptor(*source);
    if (validation_error != KivoSubtitlePlaneError_None) {
        record_evidence(*record, KivoSubtitleEvidenceReason_ContractGap,
                        KivoSubtitleEvidenceSeverity_CommandRejected);
        *out_token = reject_command(correlation_id, validation_error);
        return KivoP8ApiStatus_InvalidArgument;
    }
    const KivoSubtitlePlaneError reader_error = probe_external_reader_once(*source);
    if (reader_error != KivoSubtitlePlaneError_None) {
        record_evidence(*record,
                        reader_error == KivoSubtitlePlaneError_SubtitleSourceOpenFailed
                            ? KivoSubtitleEvidenceReason_SourceOpenFailed
                            : KivoSubtitleEvidenceReason_SourceReadFailed,
                        KivoSubtitleEvidenceSeverity_Recoverable);
        *out_token = reject_command(correlation_id, reader_error);
        return KivoP8ApiStatus_Failed;
    }
    KivoRedactedSourceIdentity identity{};
    (void)compute_redacted_identity(nullptr, *source, &identity);
    record->state = KivoSubtitlePlaneState_Ready;
    record->timeline = KivoSubtitleTimelineAvailability_Available;
    record->metrics.packet_queue_bytes = source->memory_bytes.bytes;
    record->metrics.peak_packet_queue_bytes = source->memory_bytes.bytes;
    *out_token = accept_command(*record, correlation_id);
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
