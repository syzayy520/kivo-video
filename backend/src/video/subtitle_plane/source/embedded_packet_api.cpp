#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include "video/subtitle_plane/command/command_lifecycle.hpp"
#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/source/embedded_packet_ingest.hpp"

extern "C" KivoP8ApiStatus KIVO_P8_CALL kivo_subtitle_plane_submit_embedded_packet_v1(
    const KivoSubtitlePlaneHandle plane,
    const KivoEmbeddedSubtitlePacketSubmitRequest* const request,
    KivoSubtitleCommandToken* const out_token) KIVO_P8_NOEXCEPT {
    using namespace kivo::video::subtitle_plane::runtime;

    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (request == nullptr || out_token == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    if (record->state == KivoSubtitlePlaneState_Closed) {
        *out_token = accept_command(*record,
                                    request->correlation_id,
                                    KivoSubtitleCommandImmediateStatus_Closed,
                                    KivoSubtitlePlaneError_ClosedSubtitlePlane);
        return KivoP8ApiStatus_Ok;
    }
    return ingest_embedded_packet(*record, *request, out_token);
}
