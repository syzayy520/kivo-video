#pragma once

#include <vector>

#include "kivo/video/playback_graph/playback_graph_observer.hpp"

namespace kivo::video::playback_graph::runtime {

enum class ObserverDeliveryThread {
    SafeDeliveryThread,
    GraphOwnerThread,
    HotPathThread
};

struct ObserverDeliveryStats {
    std::uint32_t delivered_count{0};
    std::uint32_t overflow_count{0};
    std::uint32_t callback_failure_count{0};
    bool closed_snapshot_delivered{false};
};

class ObserverRegistry {
public:
    [[nodiscard]] SubscriptionToken subscribe(GraphObserverHandle observer,
                                              std::uint32_t queue_capacity) noexcept;
    void unsubscribe(SubscriptionToken token) noexcept;
    void detach_all_on_close() noexcept;

    [[nodiscard]] bool publish(const PlaybackGraphEvent& event) noexcept;
    [[nodiscard]] bool deliver(ObserverDeliveryThread thread) noexcept;
    [[nodiscard]] ObserverDeliveryStats stats() const noexcept;
    [[nodiscard]] std::size_t subscription_count() const noexcept;

private:
    struct Subscription {
        SubscriptionToken token{};
        GraphObserverHandle observer{};
        std::uint32_t queue_capacity{0};
        std::vector<PlaybackGraphEvent> queue{};
        bool active{true};
    };

    std::vector<Subscription> subscriptions_{};
    SubscriptionToken next_token_{1};
    ObserverDeliveryStats stats_{};
};

}  // namespace kivo::video::playback_graph::runtime
