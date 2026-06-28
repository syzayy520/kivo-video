// audio_packet_pool_contract.hpp — P6A §9: packet pool memory contract
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::packet {
struct AudioPacketPoolContract {
    ContractMetadata metadata;
    int64_t max_bytes{0};
    int32_t max_packets{0};
    int64_t max_duration_ms{0};
    int32_t max_frames{0};
    bool hot_path_allocation_forbidden{true};
    bool allocation_phase_stream_open_only{true};
    bool max_duration_hard_enforced{true};
    bool max_duration_not_diagnostic_only{true};
};
}  // namespace kivo::video::audio_plane::packet
