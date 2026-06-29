#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph::runtime {

enum class CriticalEventSeverity {
    NonFatal,
    Fatal
};

struct CriticalEvent {
    std::uint64_t sequence{0};
    CriticalEventSeverity severity{CriticalEventSeverity::NonFatal};
    PlaybackCommandId command_id{};
    PlaybackGraphError error{PlaybackGraphError::None};
};

class CriticalEventMpscRing {
public:
    CriticalEventMpscRing(std::size_t capacity, std::size_t fatal_reserved_slots);

    [[nodiscard]] bool push(CriticalEvent event) noexcept;
    [[nodiscard]] std::optional<CriticalEvent> pop_by_graph_owner() noexcept;

    [[nodiscard]] bool overflow_latched() const noexcept;
    [[nodiscard]] std::uint32_t nonfatal_drop_count() const noexcept;
    [[nodiscard]] std::uint32_t fatal_drop_count() const noexcept;
    [[nodiscard]] std::size_t fatal_reserved_count() const noexcept;

private:
    std::size_t capacity_{0};
    std::size_t fatal_reserved_slots_{0};
    std::vector<CriticalEvent> ring_;
    std::vector<CriticalEvent> fatal_reserved_;
    std::size_t read_index_{0};
    std::uint64_t next_sequence_{1};
    bool overflow_latched_{false};
    std::uint32_t nonfatal_drop_count_{0};
    std::uint32_t fatal_drop_count_{0};
};

}  // namespace kivo::video::playback_graph::runtime
