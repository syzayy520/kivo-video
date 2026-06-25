#include "kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp"

#include <iostream>

namespace kivo::cinema_engine {

ConversionResult DecodedAudioFrameConverter::convert(const DecodedFrame& frame, const PcmFormat& target_format) {
    std::cout << "[DecodedAudioFrameConverter] convert: stream_kind=" << frame.stream_kind
              << ", sample_rate=" << frame.sample_rate
              << ", channels=" << frame.channels
              << ", sample_format=" << frame.sample_format
              << " -> target sample_rate=" << target_format.sample_rate
              << ", channels=" << target_format.channels
              << ", bit_depth=" << target_format.bit_depth
              << ", sample_format=" << target_format.sample_format << std::endl;

    ConversionResult result;
    result.success = true;
    result.sample_count = 1024;  // stub: assume 1024 samples per frame
    result.bytes_per_sample = target_format.bit_depth / 8;
    result.timestamp_us = frame.pts;

    // Generate dummy PCM data (silence)
    const int32_t total_bytes = result.sample_count * target_format.channels * result.bytes_per_sample;
    result.pcm_data.resize(total_bytes, 0);

    return result;
}

bool DecodedAudioFrameConverter::is_ready() const { return initialized_; }

std::string DecodedAudioFrameConverter::last_error() const { return last_error_; }

void DecodedAudioFrameConverter::release() {
    last_error_.clear();
    initialized_ = false;
    std::cout << "[DecodedAudioFrameConverter] release" << std::endl;
}

}  // namespace kivo::cinema_engine
