// live_timestamp_origin_resolver.hpp — GATE-020: P5A live timestamp origin resolver
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::timebase {

enum class LiveTimestampOrigin {
    WallClock,                              // wall clock based origin
    FirstPacket,                            // first packet arrival time
    ServerReported,                         // server-reported origin
    Estimated                               // estimated origin
};

struct LiveTimestampOriginResolver {
    LiveTimestampOrigin origin{LiveTimestampOrigin::FirstPacket};
    uint64_t origin_timestamp{0};           // resolved origin timestamp
    bool is_stable{false};                  // origin is stable
    std::string instability_reason;         // reason if unstable
    bool operator==(LiveTimestampOriginResolver const&) const = default;
};

}  // namespace kivo::video_plane::timebase
