#pragma once

#include <cstdint>
#include <vector>

#include "kivo/video/playback_graph/playback_graph_error.hpp"

namespace kivo::video::playback_graph::runtime {

struct RetireSetId {
    std::uint64_t value{0};
    bool operator==(const RetireSetId&) const = default;
};

enum class RetireSetState {
    Active,
    TimedOut,
    Acknowledged,
    AbandonedAfterClose
};

struct RetireRecord {
    RetireSetId id{};
    RetireSetState state{RetireSetState::Active};
    PlaybackGraphError terminal_error{PlaybackGraphError::None};
};

class ResourceRetireRecycle {
public:
    [[nodiscard]] RetireSetId decide_retire() noexcept;
    [[nodiscard]] bool mark_timeout(RetireSetId id) noexcept;
    [[nodiscard]] bool acknowledge_recycle(RetireSetId id) noexcept;
    void abandon_after_close(RetireSetId id) noexcept;

    [[nodiscard]] RetireSetState state(RetireSetId id) const noexcept;
    [[nodiscard]] std::uint32_t late_ack_discard_count() const noexcept;
    [[nodiscard]] bool performs_physical_recycle() const noexcept;

private:
    [[nodiscard]] RetireRecord* find(RetireSetId id) noexcept;
    [[nodiscard]] const RetireRecord* find(RetireSetId id) const noexcept;

    std::uint64_t next_id_{1};
    std::vector<RetireRecord> records_{};
    std::uint32_t late_ack_discard_count_{0};
};

}  // namespace kivo::video::playback_graph::runtime
