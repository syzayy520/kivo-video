#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/command/command_lifecycle.hpp"
#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/observer/observer_registry.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_begin_discovery(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleDiscoveryBeginRequest* const request,
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
    record->state = KivoSubtitlePlaneState_Discovering;
    *out_token = accept_command(*record, request->correlation_id);
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
