// audio_packet_id.hpp — P6A §7: packet identifier
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::packet {
struct AudioPacketId { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioPacketId&) const = default; };
struct AudioPacketSequence { uint64_t value{0}; bool operator==(const AudioPacketSequence&) const = default; };
struct AudioStreamId { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioStreamId&) const = default; };
struct AudioTrackId { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioTrackId&) const = default; };
}  // namespace kivo::video::audio_plane::packet
