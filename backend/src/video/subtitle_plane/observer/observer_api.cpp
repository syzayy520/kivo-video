#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/observer/observer_registry.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_subscribe_events(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleObserverCallbacks* const callbacks,
    KivoSubtitleSubscriptionToken* const out_token) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    if (out_token == nullptr) return KivoP8ApiStatus_NullPointer;
    if (callbacks == nullptr || callbacks->on_event == nullptr) {
        out_token->value = 0;
        return KivoP8ApiStatus_InvalidArgument;
    }
    return subscribe_observer(*record, *callbacks, out_token);
}

KivoP8ApiStatus KIVO_P8_CALL api_unsubscribe_events(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSubscriptionToken token) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    return unsubscribe_observer(*record, token);
}

}  // namespace kivo::video::subtitle_plane::runtime
