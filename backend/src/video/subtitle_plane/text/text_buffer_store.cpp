#include "video/subtitle_plane/text/text_buffer_store.hpp"

#include <cstring>

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus copy_frame_text_buffer(
    PlaneStateRecord& record,
    const KivoSubtitleResourceAccessToken token,
    const KivoSubtitleTextBufferId text_buffer_id,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) noexcept {
    if (out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    if (text_buffer_id.value != 1) return KivoP8ApiStatus_InvalidArgument;
    uint64_t cue_slot = 0;
    for (uint64_t index = 0; index < record.metrics.active_frame_lease_count; ++index) {
        if (record.active_frame_lease_ids[index] == token.lease_id) {
            cue_slot = record.active_frame_lease_text_cue_slots[index];
            break;
        }
    }
    if (cue_slot == 0 || cue_slot > PlaneStateRecord::kMaxTextCues) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    const std::size_t cue_index = static_cast<std::size_t>(cue_slot - 1u);
    if (!record.text_cue_valid[cue_index] || record.text_cue_bytes[cue_index] == 0) {
        return KivoP8ApiStatus_NotReady;
    }
    *out_required_bytes = record.text_cue_bytes[cue_index];
    record.metrics.text_pool_bytes = record.text_cue_bytes[cue_index];
    if (dst.data == nullptr) return dst.bytes == 0 ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_InvalidArgument;
    if (dst.bytes < record.text_cue_bytes[cue_index]) return KivoP8ApiStatus_InvalidArgument;
    std::memcpy(dst.data,
                record.text_cue_text[cue_index],
                static_cast<std::size_t>(record.text_cue_bytes[cue_index]));
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
