#pragma once

#include "video/playback_graph/command/command_conflict_matrix.hpp"

namespace kivo::video::playback_graph::runtime {

enum class TransportIntentKind {
    None,
    Pause,
    Resume
};

class PendingTransportIntent {
public:
    void request(GraphCommandKind command) noexcept;
    void clear_for_close() noexcept;

    [[nodiscard]] bool has_pending() const noexcept;
    [[nodiscard]] TransportIntentKind latest() const noexcept;

private:
    TransportIntentKind latest_{TransportIntentKind::None};
};

}  // namespace kivo::video::playback_graph::runtime
