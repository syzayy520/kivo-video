#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "kivo/cinema_engine/engine_api/engine_event.hpp"

namespace kivo::cinema_engine {

using EventSubscriptionId = std::uint64_t;

class EventBus {
 public:
  EventSubscriptionId subscribe();
  bool unsubscribe(EventSubscriptionId subscription_id);
  void publish(EngineEvent event);
  std::vector<EngineEvent> poll(EventSubscriptionId subscription_id);

 private:
  EventSubscriptionId next_subscription_id_{1};
  std::unordered_map<EventSubscriptionId, std::vector<EngineEvent>> subscriptions_;
};

}  // namespace kivo::cinema_engine

