// audio_packet_boundary_item.hpp — GATE-023: P5A audio packet boundary item
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::output_boundary {

struct AudioPacketBoundaryItem {
    uint64_t packet_sequence{0};            // audio packet sequence number
    uint64_t media_timestamp{0};            // media timestamp of packet
    std::string audio_track_id;             // audio track identifier
    bool is_boundary{false};                // this packet is a boundary
    bool operator==(AudioPacketBoundaryItem const&) const = default;
};

}  // namespace kivo::video_plane::output_boundary
