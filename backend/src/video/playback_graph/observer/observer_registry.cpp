#include "video/playback_graph/observer/observer_registry.hpp"

namespace kivo::video::playback_graph::runtime {

SubscriptionToken ObserverRegistry::subscribe(GraphObserverHandle observer,
                                              std::uint32_t queue_capacity) noexcept {
    if (observer.callback == nullptr || queue_capacity == 0) {
        stats_.callback_failure_count += 1;
        return SubscriptionToken{};
    }
    Subscription subscription{};
    subscription.token = next_token_;
    next_token_.value += 1;
    subscription.observer = observer;
    subscription.queue_capacity = queue_capacity;
    subscriptions_.push_back(subscription);
    return subscription.token;
}

void ObserverRegistry::unsubscribe(SubscriptionToken token) noexcept {
    for (auto& subscription : subscriptions_) {
        if (subscription.token == token) {
            subscription.active = false;
        }
    }
}

void ObserverRegistry::detach_all_on_close() noexcept {
    for (auto& subscription : subscriptions_) {
        subscription.active = false;
    }
}

bool ObserverRegistry::publish(const PlaybackGraphEvent& event) noexcept {
    bool queued = false;
    for (auto& subscription : subscriptions_) {
        if (!subscription.active) {
            continue;
        }
        if (subscription.queue.size() >= subscription.queue_capacity) {
            stats_.overflow_count += 1;
            continue;
        }
        subscription.queue.push_back(event);
        queued = true;
        if (event.kind == PlaybackGraphEventKind::GraphStateChanged &&
            event.state == PlaybackGraphState::Closed) {
            stats_.closed_snapshot_delivered = true;
        }
    }
    return queued;
}

bool ObserverRegistry::deliver(ObserverDeliveryThread thread) noexcept {
    if (thread != ObserverDeliveryThread::SafeDeliveryThread) {
        return false;
    }
    for (auto& subscription : subscriptions_) {
        while (!subscription.queue.empty()) {
            const auto event = subscription.queue.front();
            subscription.queue.erase(subscription.queue.begin());
            subscription.observer.callback(subscription.observer.user_data, event);
            stats_.delivered_count += 1;
        }
    }
    return true;
}

ObserverDeliveryStats ObserverRegistry::stats() const noexcept {
    return stats_;
}

std::size_t ObserverRegistry::subscription_count() const noexcept {
    return subscriptions_.size();
}

}  // namespace kivo::video::playback_graph::runtime
