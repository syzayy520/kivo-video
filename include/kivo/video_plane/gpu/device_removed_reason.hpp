// device_removed_reason.hpp — GATE-024: P5A device removed reason
#pragma once
#include <string>

namespace kivo::video_plane::gpu {

enum class DeviceRemovedReason {
    None,                                   // device not removed
    Hung,                                   // device hung
    OutOfMemory,                            // device out of memory
    DriverError,                            // driver error
    HardwareFailure,                        // hardware failure
    Unknown                                 // unknown reason
};

struct DeviceRemovedInfo {
    DeviceRemovedReason reason{DeviceRemovedReason::None};
    std::string description;                // human-readable description
    bool operator==(DeviceRemovedInfo const&) const = default;
};

}  // namespace kivo::video_plane::gpu
