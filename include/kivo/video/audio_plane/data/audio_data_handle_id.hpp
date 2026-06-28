// audio_data_handle_id.hpp — P6A §8: handle identifier
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::data {
struct AudioDataHandleId { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioDataHandleId&) const = default; };
struct AudioPoolId { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioPoolId&) const = default; };
struct AudioPoolSlotId { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioPoolSlotId&) const = default; };
struct AudioAllocationEpoch { uint64_t value{0}; bool is_valid() const { return value != 0; } bool operator==(const AudioAllocationEpoch&) const = default; };
}  // namespace kivo::video::audio_plane::data
