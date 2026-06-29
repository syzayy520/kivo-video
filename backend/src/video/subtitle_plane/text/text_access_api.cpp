#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/resource/resource_token_validation.hpp"
#include "video/subtitle_plane/text/font_blob_store.hpp"
#include "video/subtitle_plane/text/text_buffer_store.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_copy_text_buffer(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleResourceAccessToken token,
    const KivoSubtitleTextBufferId text_buffer_id,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    if (out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    const KivoP8ApiStatus token_status =
        validate_resource_access_token(token, KivoSubtitleResourceAccessKind_FrameLease);
    if (token_status != KivoP8ApiStatus_Ok || text_buffer_id.value == 0) return KivoP8ApiStatus_InvalidArgument;
    return copy_frame_text_buffer(*record, text_buffer_id, dst, out_required_bytes);
}

KivoP8ApiStatus KIVO_P8_CALL api_copy_font_blob(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleResourceAccessToken token,
    const KivoSubtitleFontBlobId font_blob_id,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    if (out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    const KivoP8ApiStatus token_status =
        validate_resource_access_token(token, KivoSubtitleResourceAccessKind_SnapshotLease);
    if (token_status != KivoP8ApiStatus_Ok || font_blob_id.value == 0) return KivoP8ApiStatus_InvalidArgument;
    return copy_snapshot_font_blob(*record, font_blob_id, dst, out_required_bytes);
}

}  // namespace kivo::video::subtitle_plane::runtime
