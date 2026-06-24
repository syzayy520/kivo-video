#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "kivo/cinema_engine/session_core/playback_session_aggregate.hpp"

namespace kivo::cinema_engine {

class SessionStore {
 public:
  bool create(PlaybackSessionAggregate session);
  bool close(const std::string& session_id, std::string closed_at, std::string close_reason);
  std::optional<PlaybackSessionAggregate> find(const std::string& session_id) const;
  std::vector<PlaybackSessionAggregate> export_all() const;
  std::size_t size() const;

 private:
  std::unordered_map<std::string, PlaybackSessionAggregate> sessions_;
};

}  // namespace kivo::cinema_engine
