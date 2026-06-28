// output_device_clock_snapshot.hpp — P6A clock: OutputDeviceClockSnapshot
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct OutputDeviceClockSnapshot {
    ContractMetadata metadata;
    int64_t device_consumed_media_time_ms{0};
    int64_t device_padding_duration_ms{0};
    int64_t qpc_timestamp{0};
    uint64_t device_generation{0};
    bool operator==(const OutputDeviceClockSnapshot&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
