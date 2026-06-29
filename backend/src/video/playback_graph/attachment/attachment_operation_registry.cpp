#include "video/playback_graph/attachment/attachment_operation_registry.hpp"

#include <algorithm>

namespace kivo::video::playback_graph::runtime {

AttachmentOperationRegistry::AttachmentOperationRegistry(
    std::size_t active_capacity,
    std::size_t completion_capacity,
    std::size_t reserved_close_or_recovery_capacity)
    : active_capacity_(active_capacity),
      completion_capacity_(completion_capacity),
      reserved_close_or_recovery_capacity_(
          reserved_close_or_recovery_capacity > active_capacity
              ? active_capacity
              : reserved_close_or_recovery_capacity) {
    active_.reserve(active_capacity_);
    completions_.reserve(completion_capacity_);
    tombstones_.reserve(active_capacity_ + completion_capacity_);
}

bool AttachmentOperationRegistry::start_operation(const AttachmentAsyncToken& token,
                                                  bool close_or_recovery) noexcept {
    if (has_tombstone(token.key) || !has_active_capacity(close_or_recovery)) {
        return false;
    }
    active_.push_back(token);
    return true;
}

bool AttachmentOperationRegistry::enqueue_completion(const AttachmentCompletionEvent& event) noexcept {
    if (has_tombstone(event.token.key)) {
        late_completion_discard_count_ += 1;
        return false;
    }
    if (completions_.size() >= completion_capacity_) {
        queue_full_drop_count_ += 1;
        return false;
    }
    completions_.push_back(event);
    return true;
}

std::optional<AttachmentCompletionEvent> AttachmentOperationRegistry::pop_completion() noexcept {
    if (completions_.empty()) {
        return std::nullopt;
    }
    auto event = completions_.front();
    completions_.erase(completions_.begin());
    return event;
}

bool AttachmentOperationRegistry::mark_timeout(const OperationKey& key,
                                               AttachmentTimeoutReason reason,
                                               PlaybackGraphError error) noexcept {
    if (has_tombstone(key)) {
        return false;
    }

    auto it = std::find_if(active_.begin(), active_.end(), [&key](const auto& token) {
        return token.key == key;
    });
    if (it != active_.end()) {
        active_.erase(it);
    }

    tombstones_.push_back(AttachmentOperationTombstone{key, reason, error});
    return true;
}

bool AttachmentOperationRegistry::contains_active(const OperationKey& key) const noexcept {
    return std::any_of(active_.begin(), active_.end(), [&key](const auto& token) {
        return token.key == key;
    });
}

bool AttachmentOperationRegistry::has_tombstone(const OperationKey& key) const noexcept {
    return std::any_of(tombstones_.begin(), tombstones_.end(), [&key](const auto& tombstone) {
        return tombstone.key == key;
    });
}

std::size_t AttachmentOperationRegistry::active_count() const noexcept {
    return active_.size();
}

std::size_t AttachmentOperationRegistry::completion_count() const noexcept {
    return completions_.size();
}

std::size_t AttachmentOperationRegistry::tombstone_count() const noexcept {
    return tombstones_.size();
}

std::uint32_t AttachmentOperationRegistry::late_completion_discard_count() const noexcept {
    return late_completion_discard_count_;
}

std::uint32_t AttachmentOperationRegistry::queue_full_drop_count() const noexcept {
    return queue_full_drop_count_;
}

bool AttachmentOperationRegistry::has_active_capacity(bool close_or_recovery) const noexcept {
    if (active_.size() >= active_capacity_) {
        return false;
    }
    if (close_or_recovery) {
        return true;
    }
    const auto normal_capacity = active_capacity_ - reserved_close_or_recovery_capacity_;
    return active_.size() < normal_capacity;
}

}  // namespace kivo::video::playback_graph::runtime
