// blocking_call_detector.hpp — GATE-025: P5A blocking call detector
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::thread {

struct BlockingCallDetector {
    std::string call_site;                  // where blocking call was detected
    uint64_t blocked_duration_ms{0};        // how long the call blocked
    bool is_realtime_thread{false};         // call was on a realtime thread
    std::string recommendation;             // recommendation to fix
    bool operator==(BlockingCallDetector const&) const = default;
};

}  // namespace kivo::video_plane::thread
