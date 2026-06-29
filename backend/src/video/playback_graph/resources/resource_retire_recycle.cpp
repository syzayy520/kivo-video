#include "video/playback_graph/resources/resource_retire_recycle.hpp"

namespace kivo::video::playback_graph::runtime {

RetireSetId ResourceRetireRecycle::decide_retire() noexcept {
    RetireRecord record{};
    record.id.value = next_id_++;
    records_.push_back(record);
    return record.id;
}

bool ResourceRetireRecycle::mark_timeout(RetireSetId id) noexcept {
    auto* record = find(id);
    if (record == nullptr || record->state != RetireSetState::Active) {
        return false;
    }
    record->state = RetireSetState::TimedOut;
    record->terminal_error = PlaybackGraphError::CloseTimeout;
    return true;
}

bool ResourceRetireRecycle::acknowledge_recycle(RetireSetId id) noexcept {
    auto* record = find(id);
    if (record == nullptr) {
        return false;
    }
    if (record->state != RetireSetState::Active) {
        late_ack_discard_count_ += 1;
        return false;
    }
    record->state = RetireSetState::Acknowledged;
    return true;
}

void ResourceRetireRecycle::abandon_after_close(RetireSetId id) noexcept {
    auto* record = find(id);
    if (record != nullptr) {
        record->state = RetireSetState::AbandonedAfterClose;
    }
}

RetireSetState ResourceRetireRecycle::state(RetireSetId id) const noexcept {
    const auto* record = find(id);
    return record == nullptr ? RetireSetState::AbandonedAfterClose : record->state;
}

std::uint32_t ResourceRetireRecycle::late_ack_discard_count() const noexcept {
    return late_ack_discard_count_;
}

bool ResourceRetireRecycle::performs_physical_recycle() const noexcept {
    return false;
}

RetireRecord* ResourceRetireRecycle::find(RetireSetId id) noexcept {
    for (auto& record : records_) {
        if (record.id == id) {
            return &record;
        }
    }
    return nullptr;
}

const RetireRecord* ResourceRetireRecycle::find(RetireSetId id) const noexcept {
    for (const auto& record : records_) {
        if (record.id == id) {
            return &record;
        }
    }
    return nullptr;
}

}  // namespace kivo::video::playback_graph::runtime
