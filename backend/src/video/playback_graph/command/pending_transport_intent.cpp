#include "video/playback_graph/command/pending_transport_intent.hpp"

namespace kivo::video::playback_graph::runtime {

void PendingTransportIntent::request(GraphCommandKind command) noexcept {
    if (command == GraphCommandKind::Pause) {
        latest_ = TransportIntentKind::Pause;
    } else if (command == GraphCommandKind::Resume) {
        latest_ = TransportIntentKind::Resume;
    }
}

void PendingTransportIntent::clear_for_close() noexcept {
    latest_ = TransportIntentKind::None;
}

bool PendingTransportIntent::has_pending() const noexcept {
    return latest_ != TransportIntentKind::None;
}

TransportIntentKind PendingTransportIntent::latest() const noexcept {
    return latest_;
}

}  // namespace kivo::video::playback_graph::runtime
