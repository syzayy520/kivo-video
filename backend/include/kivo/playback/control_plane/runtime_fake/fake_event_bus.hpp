#pragma once

#include <vector>
#include <mutex>
#include "kivo/playback/control_plane/events/session_event.hpp"
#include "kivo/playback/control_plane/events/event_bus_policy.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::runtime_fake {

/// FakeEventBus collects events for testing.
class FakeEventBus
{
public:
    void publish(const events::SessionEvent& event)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
        if (event.delivery_class == events::EventDeliveryClass::Critical)
            ++critical_count_;
    }

    events::EventBusPolicy policy() const
    {
        events::EventBusPolicy p;
        p.max_pending_events_per_subscriber = primitives::Count{1024};
        return p;
    }

    size_t total_events() const { std::lock_guard<std::mutex> lock(mutex_); return events_.size(); }
    size_t critical_events() const { std::lock_guard<std::mutex> lock(mutex_); return critical_count_; }
    void clear() { std::lock_guard<std::mutex> lock(mutex_); events_.clear(); critical_count_ = 0; }

private:
    mutable std::mutex mutex_;
    std::vector<events::SessionEvent> events_;
    size_t critical_count_{0};
};

} // namespace kivo::playback::control_plane::runtime_fake
