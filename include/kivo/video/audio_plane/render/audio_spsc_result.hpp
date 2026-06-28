// audio_spsc_result.hpp — P6A §15: strong typed results (NOT bool)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::render {
enum class AudioSpscPushResult : int32_t {
    Pushed = 0, Full = 1, Closed = 2, GenerationMismatch = 3, InvalidPacket = 4
};
enum class AudioSpscPopResult : int32_t {
    Popped = 0, Empty = 1, Closed = 2, GenerationMismatch = 3
};
struct AudioSpscDepthSnapshot {
    uint32_t approximate_depth{0};
    uint32_t capacity{0};
    bool is_approximate{true};
};
}  // namespace kivo::video::audio_plane::render
