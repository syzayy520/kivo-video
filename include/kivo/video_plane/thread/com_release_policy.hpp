// com_release_policy.hpp — GATE-025: P5A COM release policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::thread {

struct ComReleasePolicy {
    bool defer_release{true};               // defer COM release to non-realtime thread
    uint32_t max_deferred_count{64};        // max deferred releases
    bool release_on_thread_exit{true};      // release all on thread exit
    std::string release_thread_name;        // thread responsible for deferred release
    bool operator==(ComReleasePolicy const&) const = default;
};

}  // namespace kivo::video_plane::thread
