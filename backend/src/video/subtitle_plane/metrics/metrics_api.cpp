#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/core/plane_store.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_get_metrics(
    const KivoSubtitlePlaneHandle plane,
    KivoSubtitleMetricsSnapshot* const out_metrics) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    if (out_metrics == nullptr) return KivoP8ApiStatus_NullPointer;
    *out_metrics = record->metrics;
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
