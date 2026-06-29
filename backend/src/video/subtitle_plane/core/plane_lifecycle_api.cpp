#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/core/runtime_caps.hpp"
#include "video/subtitle_plane/threading/close_tombstone_policy.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_create_plane(const KivoSubtitlePlaneConfig* const config,
                                              KivoSubtitlePlaneHandle* const out_plane) KIVO_P8_NOEXCEPT {
    if (out_plane == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    KivoSubtitlePlaneRuntimeCaps caps{};
    const KivoP8ApiStatus cap_status = merge_runtime_caps(config, &caps);
    if (cap_status != KivoP8ApiStatus_Ok) {
        out_plane->value = 0;
        return cap_status;
    }
    return create_plane_record(caps, out_plane) != nullptr ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_Failed;
}

KivoP8ApiStatus KIVO_P8_CALL api_close_plane(const KivoSubtitlePlaneHandle plane) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    apply_close_tombstones(*record);
    return KivoP8ApiStatus_Ok;
}

KivoP8ApiStatus KIVO_P8_CALL api_destroy_plane(const KivoSubtitlePlaneHandle plane) KIVO_P8_NOEXCEPT {
    return destroy_plane_record(plane) ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_InvalidArgument;
}

}  // namespace kivo::video::subtitle_plane::runtime
