#pragma once

#include "kivo/video/playback_graph/playback_graph_event.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph {

using GraphObserverCallback = void (*)(void* user_data, const PlaybackGraphEvent& event) noexcept;

struct GraphObserverHandle {
    void* user_data{nullptr};
    GraphObserverCallback callback{nullptr};
};

struct ObserverDeliveryPolicy {
    bool allow_closed_subscription{true};
    std::uint32_t queue_capacity{64};
};

}  // namespace kivo::video::playback_graph
