// decoder_contract.hpp — P5D D1: Decoder contract
#pragma once
#include <cstdint>
#include <string>
#include "../stamp/core_video_stamp.hpp"

namespace kivo::video_plane::decoder {

using stamp::CoreVideoStamp;

enum class DecoderKind {
    Software,
    HardwareD3D11VA,
    HardwareUnknown,
    Fake
};

struct DecoderContract {
    DecoderKind kind{DecoderKind::Fake};
    uint64_t max_frame_queue_depth{16};
    uint64_t frame_timeout_ms{1000};
    bool supports_drain{true};
    bool supports_flush{true};
    bool supports_interruption{true};
    bool operator==(DecoderContract const&) const = default;
};

}  // namespace kivo::video_plane::decoder
