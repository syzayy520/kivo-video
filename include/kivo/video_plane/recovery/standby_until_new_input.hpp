// standby_until_new_input.hpp — GATE-009: P5A standby until new input
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::recovery {

struct StandbyUntilNewInput {
    std::string reason;                     // why pipeline entered standby
    uint64_t standby_timestamp{0};
    uint64_t last_known_media_time{0};
    bool auto_resume_on_new_input{true};    // resume automatically when new input arrives
    std::string correlation_id;
    bool operator==(StandbyUntilNewInput const&) const = default;
};

}  // namespace kivo::video_plane::recovery
