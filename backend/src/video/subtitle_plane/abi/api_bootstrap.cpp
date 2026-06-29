#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include <cstring>

#include "video/subtitle_plane/abi/api_table_builder.hpp"

extern "C" KivoP8ApiStatus KIVO_P8_CALL kivo_subtitle_plane_get_api_v1(
    const uint32_t requested_abi_revision,
    KivoSubtitlePlaneApiV1* const out_api) KIVO_P8_NOEXCEPT {
    if (out_api == nullptr) return KivoP8ApiStatus_NullPointer;
    const uint32_t caller_size = out_api->header.struct_size;
    if (caller_size < KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE) {
        std::memset(out_api, 0, caller_size);
        return KivoP8ApiStatus_ApiTableTooSmall;
    }
    if (requested_abi_revision != KIVO_P8_ABI_REVISION) {
        std::memset(out_api, 0, caller_size);
        return KivoP8ApiStatus_UnsupportedAbiVersion;
    }
    if (!kivo::video::subtitle_plane::runtime::host_is_little_endian()) {
        std::memset(out_api, 0, caller_size);
        return KivoP8ApiStatus_UnsupportedHostEndian;
    }
    const KivoSubtitlePlaneApiV1 table = kivo::video::subtitle_plane::runtime::make_api_table();
    std::memset(out_api, 0, caller_size);
    std::memcpy(out_api, &table, KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE);
    out_api->header.struct_size = caller_size;
    return KivoP8ApiStatus_Ok;
}
