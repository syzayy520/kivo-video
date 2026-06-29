#include "video/playback_graph/events/critical_event_mpsc_ring.hpp"

namespace kivo::video::playback_graph::runtime {

CriticalEventMpscRing::CriticalEventMpscRing(std::size_t capacity,
                                             std::size_t fatal_reserved_slots)
    : capacity_(capacity), fatal_reserved_slots_(fatal_reserved_slots) {
    ring_.reserve(capacity_);
    fatal_reserved_.reserve(fatal_reserved_slots_);
}

bool CriticalEventMpscRing::push(CriticalEvent event) noexcept {
    event.sequence = next_sequence_++;
    if (ring_.size() - read_index_ < capacity_) {
        ring_.push_back(event);
        return true;
    }

    overflow_latched_ = true;
    if (event.severity == CriticalEventSeverity::Fatal &&
        fatal_reserved_.size() < fatal_reserved_slots_) {
        fatal_reserved_.push_back(event);
        return true;
    }
    if (event.severity == CriticalEventSeverity::Fatal) {
        fatal_drop_count_ += 1;
    } else {
        nonfatal_drop_count_ += 1;
    }
    return false;
}

std::optional<CriticalEvent> CriticalEventMpscRing::pop_by_graph_owner() noexcept {
    if (read_index_ < ring_.size()) {
        return ring_[read_index_++];
    }
    if (!fatal_reserved_.empty()) {
        const auto event = fatal_reserved_.front();
        fatal_reserved_.erase(fatal_reserved_.begin());
        return event;
    }
    return std::nullopt;
}

bool CriticalEventMpscRing::overflow_latched() const noexcept {
    return overflow_latched_;
}

std::uint32_t CriticalEventMpscRing::nonfatal_drop_count() const noexcept {
    return nonfatal_drop_count_;
}

std::uint32_t CriticalEventMpscRing::fatal_drop_count() const noexcept {
    return fatal_drop_count_;
}

std::size_t CriticalEventMpscRing::fatal_reserved_count() const noexcept {
    return fatal_reserved_.size();
}

}  // namespace kivo::video::playback_graph::runtime
