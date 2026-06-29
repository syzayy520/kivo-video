#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kivo::video::playback_graph::runtime {

struct alignas(64) RealtimeEventSlot {
    std::uint64_t sequence{0};
    std::uint32_t payload_size{0};
    bool in_use{false};
    std::byte payload[48]{};
};

static_assert(alignof(RealtimeEventSlot) >= 64,
              "RealtimeEventSlot must be cacheline aligned");
static_assert(sizeof(RealtimeEventSlot) % 64 == 0,
              "RealtimeEventSlot size must avoid adjacent-slot false sharing");

class RealtimeEventAllocator {
public:
    explicit RealtimeEventAllocator(std::size_t capacity) {
        slots_.resize(capacity);
        free_stack_.reserve(capacity);
        for (std::size_t index = 0; index < capacity; ++index) {
            free_stack_.push_back(capacity - index - 1);
        }
    }

    void mark_graph_started() noexcept {
        graph_started_ = true;
    }

    [[nodiscard]] RealtimeEventSlot* allocate() noexcept {
        if (free_stack_.empty()) {
            overflow_count_ += 1;
            return nullptr;
        }
        const auto index = free_stack_.back();
        free_stack_.pop_back();
        auto& slot = slots_[index];
        slot.in_use = true;
        slot.sequence = next_sequence_++;
        return &slot;
    }

    void release(RealtimeEventSlot* slot) noexcept {
        if (slot == nullptr || !slot->in_use) {
            return;
        }
        const auto index = static_cast<std::size_t>(slot - slots_.data());
        slot->in_use = false;
        slot->payload_size = 0;
        free_stack_.push_back(index);
    }

    [[nodiscard]] bool debug_dynamic_allocation_allowed() const noexcept {
        return !graph_started_;
    }

    [[nodiscard]] std::uint32_t overflow_count() const noexcept {
        return overflow_count_;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return slots_.size();
    }

private:
    std::vector<RealtimeEventSlot> slots_{};
    std::vector<std::size_t> free_stack_{};
    std::uint64_t next_sequence_{1};
    std::uint32_t overflow_count_{0};
    bool graph_started_{false};
};

}  // namespace kivo::video::playback_graph::runtime
