#include "kivo/video/providers/local_file/local_file_session_store.hpp"

namespace kivo::video::providers::local_file {

source_core::SourceSessionId LocalFileSessionStore::allocate_id() {
    std::lock_guard<std::mutex> lock(mutex_);
    return source_core::SourceSessionId{next_id_++};
}

void LocalFileSessionStore::insert(LocalFileSessionRecord record) {
    auto id = record.session.session_id.value;
    std::lock_guard<std::mutex> lock(mutex_);
    records_[id] = std::move(record);
}

LocalFileSessionRecord* LocalFileSessionStore::find(source_core::SourceSessionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(id.value);
    return (it != records_.end()) ? &it->second : nullptr;
}

const LocalFileSessionRecord* LocalFileSessionStore::find(source_core::SourceSessionId id) const {
    const_cast<std::mutex&>(mutex_).lock();
    auto it = records_.find(id.value);
    const auto* ptr = (it != records_.end()) ? &it->second : nullptr;
    const_cast<std::mutex&>(mutex_).unlock();
    return ptr;
}

void LocalFileSessionStore::update_offset(source_core::SourceSessionId id, std::uint64_t offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(id.value);
    if (it != records_.end()) {
        it->second.current_offset = offset;
    }
}

void LocalFileSessionStore::append_evidence(source_core::SourceSessionId id, source_core::SourceEvidenceItem item) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(id.value);
    if (it != records_.end()) {
        item.sequence_number = static_cast<std::uint64_t>(it->second.evidence.items.size() + 1);
        it->second.evidence.items.push_back(std::move(item));
    }
}

source_core::SourceError LocalFileSessionStore::close_session(source_core::SourceSessionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Check tombstones first for idempotent close
    auto tomb_it = tombstones_.find(id.value);
    if (tomb_it != tombstones_.end()) {
        return source_core::SourceError::ok();  // already closed
    }
    auto it = records_.find(id.value);
    if (it == records_.end()) {
        return {source_core::SourceErrorCode::session_not_found, "session not found"};
    }

    it->second.session.session_state = source_core::SourceSessionState::closed;
    tombstones_[id.value] = std::move(it->second);
    records_.erase(it);
    return source_core::SourceError::ok();
}

}  // namespace kivo::video::providers::local_file
