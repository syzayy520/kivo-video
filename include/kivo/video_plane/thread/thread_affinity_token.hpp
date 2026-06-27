// thread_affinity_token.hpp — GATE-025: P5A thread affinity token
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::thread {

struct ThreadAffinityToken {
    uint64_t thread_id{0};                  // thread identifier
    std::string thread_name;                // human-readable thread name
    bool is_pinned{false};                  // thread is pinned to specific core
    uint32_t core_mask{0};                  // core affinity mask
    bool operator==(ThreadAffinityToken const&) const = default;
};

}  // namespace kivo::video_plane::thread
