#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Current state of an Emby session.
enum class EmbySessionState {
    Created,
    Playing,
    Paused,
    Stopped,
    TornDown,
};

// Runtime lifecycle and state for an Emby session.
struct EmbySessionRuntime {
    ContractMetadata metadata;
    std::string session_id;
    EmbySessionState state{EmbySessionState::Created};
    int64_t created_at{0};
    int64_t last_activity_at{0};
    int64_t playback_position_ticks{0};  // 10 million ticks = 1 second
    bool is_paused{false};
};

}  // namespace kivo::cinema_engine
