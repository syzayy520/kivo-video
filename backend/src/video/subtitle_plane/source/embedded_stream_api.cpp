#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/command/command_lifecycle.hpp"
#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/evidence/evidence_summary.hpp"
#include "video/subtitle_plane/observer/observer_registry.hpp"
#include "video/subtitle_plane/source/embedded_stream_end.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_end_embedded_stream(
    const KivoSubtitlePlaneHandle plane,
    const KivoEmbeddedSubtitleStreamEndRequest* const request,
    KivoSubtitleCommandToken* const out_token) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (observer_mutation_blocked(*record)) {
        return KivoP8ApiStatus_UnsupportedOperation;
    }
    if (request == nullptr || out_token == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    const KivoSubtitleCommandImmediateStatus status = apply_embedded_stream_end(*record, *request);
    const KivoSubtitleEvidenceReason reason =
        status == KivoSubtitleCommandImmediateStatus_NoOp
            ? KivoSubtitleEvidenceReason_PacketGenerationMismatch
            : KivoSubtitleEvidenceReason_PacketPtsDiscontinuity;
    record_evidence(*record, reason, KivoSubtitleEvidenceSeverity_Info);
    dispatch_observer_event(*record, reason, KivoSubtitleEvidenceSeverity_Info);
    *out_token = accept_command(*record, request->correlation_id, status);
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
