#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

struct AudioOutputPolicySnapshot {
    double volume{1.0};
    bool muted{false};
    char device_id[64]{};
    std::int64_t delay_ms{0};
    bool valid{false};
};

struct AudioVolumeRequest {
    double volume{1.0};
};

struct AudioMuteRequest {
    bool muted{false};
};

struct AudioDeviceSelectRequest {
    char device_id[64]{};
};

struct AudioDelayRequest {
    std::int64_t delay_ms{0};
};

}  // namespace kivo::video::playback_graph