#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/command/command_lifecycle.hpp"
#include "video/subtitle_plane/core/plane_store.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_query_command(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleCommandId command_id,
    KivoSubtitleCommandLifecycleSnapshot* const out_snapshot) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (out_snapshot == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    *out_snapshot = query_command_snapshot(*record, command_id);
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
