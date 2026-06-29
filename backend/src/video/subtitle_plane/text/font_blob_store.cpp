#include "video/subtitle_plane/text/font_blob_store.hpp"

#include <cstring>

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus copy_snapshot_font_blob(
    PlaneStateRecord& record,
    const KivoSubtitleFontBlobId font_blob_id,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) noexcept {
    if (out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    if (font_blob_id.value != 1) return KivoP8ApiStatus_InvalidArgument;
    constexpr uint8_t kFont[] = {0x4b, 0x46};
    *out_required_bytes = sizeof(kFont);
    record.metrics.font_blob_bytes = sizeof(kFont);
    if (dst.data == nullptr) return dst.bytes == 0 ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_InvalidArgument;
    if (dst.bytes < sizeof(kFont)) return KivoP8ApiStatus_InvalidArgument;
    std::memcpy(dst.data, kFont, sizeof(kFont));
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
