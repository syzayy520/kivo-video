#pragma once

#include <atomic>
#include <cstdint>

#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph {

enum class GenerationStampState {
    PendingBarrier,
    Active,
    Retired
};

struct GenerationStamp {
    GenerationTupleId tuple_id{};
    std::uint64_t stream_stamp_id{0};
    GenerationStampState state{GenerationStampState::PendingBarrier};
    bool operator==(const GenerationStamp&) const = default;
};

#if defined(_WIN64)
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "P7 requires lock-free uint64 atomics on Windows x64");
#endif

}  // namespace kivo::video::playback_graph
