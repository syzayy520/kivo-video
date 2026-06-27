// ffmpeg_copy_to_pool_contract.hpp — P5B B10: FFmpeg copy-to-pool default contract
#pragma once
#include <cstdint>
#include <string>
#include "../allocation/allocation_limit_policy.hpp"

namespace kivo::video_plane::input_bridge {

// FFmpeg copy-to-pool default contract
// Defines how FFmpeg demux packets are copied into DemuxBufferPool
// This is a contract only — no real FFmpeg dependency
struct FFmpegCopyToPoolContract {
    // Default: copy AVPacket data into pool-allocated buffer
    bool copy_on_demux{true};
    // Max bytes per packet copy
    uint64_t max_packet_copy_bytes{allocation::AllocationDefaults::kMaxSubtitlePacketBytes};
    // Zero source buffer after copy (security)
    bool zero_source_after_copy{true};
    // Validate packet size before copy
    bool validate_packet_size{true};
    // Reject packet if size > max_packet_copy_bytes
    bool reject_oversized{true};

    // Contract rules:
    // 1. Every AVPacket data must be copied into pool before P5 processing
    // 2. Source buffer must be zeroed after copy (no data residue)
    // 3. Packet size must be validated against max_packet_copy_bytes
    // 4. Oversized packets must be rejected with evidence
    // 5. No direct AVPacket pointer passed to P5 core (copy is mandatory)

    bool operator==(FFmpegCopyToPoolContract const&) const = default;
};

// Validate a packet against the contract
inline bool validate_packet_against_contract(
    FFmpegCopyToPoolContract const& contract,
    uint64_t packet_size) {
    if (!contract.validate_packet_size) return true;
    if (contract.reject_oversized && packet_size > contract.max_packet_copy_bytes) {
        return false;
    }
    return true;
}

}  // namespace kivo::video_plane::input_bridge
