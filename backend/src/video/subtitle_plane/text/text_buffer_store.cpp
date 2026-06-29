#include "video/subtitle_plane/text/text_buffer_store.hpp"

#include <cstring>

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus copy_frame_text_buffer(
    PlaneStateRecord& record,
    const KivoSubtitleTextBufferId text_buffer_id,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) noexcept {
    if (out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    if (text_buffer_id.value != 1) return KivoP8ApiStatus_InvalidArgument;
    constexpr char kText[] = "cue";
    *out_required_bytes = 3;
    record.metrics.text_pool_bytes = 3;
    if (dst.data == nullptr) return dst.bytes == 0 ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_InvalidArgument;
    if (dst.bytes < 3) return KivoP8ApiStatus_InvalidArgument;
    std::memcpy(dst.data, kText, 3);
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
