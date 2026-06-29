#include "video/subtitle_plane/threading/cancellation_poll.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus poll_cancelled(const KivoSubtitleCancellationView& view,
                               uint32_t* const out_cancelled) noexcept {
    if (out_cancelled == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    *out_cancelled = 0;
    if (view.is_cancelled == nullptr) {
        return KivoP8ApiStatus_Ok;
    }
    const KivoP8CallbackStatus status =
        view.is_cancelled(view.user_data, view.operation_id, out_cancelled);
    return status == KivoP8CallbackStatus_Ok ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_Failed;
}

}  // namespace kivo::video::subtitle_plane::runtime
