#pragma once

#include <cstdint>
#include <cstddef>

namespace kivo::video::playback_graph {

struct PlaylistItemSnapshot {
    static constexpr std::size_t kMaxLabelBytes = 128;

    std::uint64_t index{0};
    char label[kMaxLabelBytes]{};
    bool current{false};
};

struct PlaylistPolicySnapshot {
    static constexpr std::size_t kMaxItems = 64;

    PlaylistItemSnapshot items[kMaxItems]{};
    bool valid{false};
    bool queue_connected{false};
    bool has_previous{false};
    bool has_next{false};
    bool has_next_episode{false};
    bool auto_play_next{false};
    std::uint64_t current_index{0};
    std::uint64_t item_count{0};
    std::size_t visible_item_count{0};
};

struct AutoPlayNextRequest {
    bool enabled{false};
};

struct PlaylistItemSelectRequest {
    std::uint64_t index{0};
};

}  // namespace kivo::video::playback_graph
