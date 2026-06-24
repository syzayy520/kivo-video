#include "kivo/cinema_engine/session_core/session_store.hpp"

#include <utility>

namespace kivo::cinema_engine {

bool SessionStore::create(PlaybackSessionAggregate session) {
  if (session.session_id.empty() || sessions_.contains(session.session_id)) {
    return false;
  }

  const std::string session_id = session.session_id;
  sessions_.emplace(session_id, std::move(session));
  return true;
}

bool SessionStore::close(const std::string& session_id, std::string closed_at, std::string close_reason) {
  auto found = sessions_.find(session_id);
  if (found == sessions_.end() || found->second.closed_at.has_value()) {
    return false;
  }

  found->second.closed_at = std::move(closed_at);
  found->second.close_reason = std::move(close_reason);
  return true;
}

std::optional<PlaybackSessionAggregate> SessionStore::find(const std::string& session_id) const {
  const auto found = sessions_.find(session_id);
  if (found == sessions_.end()) {
    return std::nullopt;
  }
  return found->second;
}

std::vector<PlaybackSessionAggregate> SessionStore::export_all() const {
  std::vector<PlaybackSessionAggregate> exported;
  exported.reserve(sessions_.size());
  for (const auto& [session_id, session] : sessions_) {
    (void)session_id;
    exported.push_back(session);
  }
  return exported;
}

std::size_t SessionStore::size() const {
  return sessions_.size();
}

}  // namespace kivo::cinema_engine

