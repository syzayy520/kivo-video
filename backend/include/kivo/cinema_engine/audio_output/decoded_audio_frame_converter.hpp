#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// PcmFormat describes the target PCM format for conversion.
struct PcmFormat {
    int32_t sample_rate{0};
    int32_t channels{0};
    int32_t bit_depth{16};  // 16-bit or 32-bit
    std::string sample_format;  // "s16", "s32", "flt", "dbl"
    bool is_planar{false};  // false=interleaved, true=planar
};

// ConversionResult contains the result of PCM conversion.
struct ConversionResult {
    bool success{false};
    std::vector<uint8_t> pcm_data;
    int32_t sample_count{0};
    int32_t bytes_per_sample{0};
    int64_t timestamp_us{0};  // presentation timestamp in microseconds
    std::string error_message;
};

// DecodedAudioFrameConverter converts decoded audio frames to PCM format.
// This interface is platform-neutral; actual sample conversion stays inside adapter boundary.
struct DecodedAudioFrameConverter {
    ContractMetadata metadata;

    // Convert a decoded audio frame to target PCM format.
    ConversionResult convert(const DecodedFrame& frame, const PcmFormat& target_format);

    // Check if converter is initialized and ready.
    bool is_ready() const;

    // Get the last error message (empty if no error).
    std::string last_error() const;

    // Release all resources.
    void release();

private:
    std::string last_error_;
    bool initialized_{true};
};

}  // namespace kivo::cinema_engine
