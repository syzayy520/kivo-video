#include "kivo/cinema_engine/engine_api/event_bus.hpp"

#include <utility>

namespace kivo::cinema_engine {

EventSubscriptionId EventBus::subscribe() {
  const EventSubscriptionId subscription_id = next_subscription_id_++;
  subscriptions_.emplace(subscription_id, std::vector<EngineEvent>{});
  return subscription_id;
}

bool EventBus::unsubscribe(EventSubscriptionId subscription_id) {
  return subscriptions_.erase(subscription_id) > 0;
}

void EventBus::publish(EngineEvent event) {
  for (auto& [subscription_id, events] : subscriptions_) {
    (void)subscription_id;
    events.push_back(event);
  }
}

std::vector<EngineEvent> EventBus::poll(EventSubscriptionId subscription_id) {
  auto found = subscriptions_.find(subscription_id);
  if (found == subscriptions_.end()) {
    return {};
  }

  std::vector<EngineEvent> events = std::move(found->second);
  found->second.clear();
  return events;
}

}  // namespace kivo::cinema_engine
