#include "kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp"

#include <iostream>
#include <cstring>
#include <cmath>

namespace kivo::cinema_engine {

namespace {

// Helper: Convert sample format string to bytes per sample
int bytes_per_sample_from_format(const std::string& format) {
    if (format == "s16" || format == "u16") return 2;
    if (format == "s32" || format == "u32" || format == "flt") return 4;
    if (format == "dbl") return 8;
    if (format == "s8" || format == "u8") return 1;
    return 2;  // default to 16-bit
}

// Helper: Convert float sample to int16
int16_t float_to_int16(float sample) {
    // Clamp to [-1.0, 1.0]
    sample = std::max(-1.0f, std::min(1.0f, sample));
    // Scale to int16 range
    return static_cast<int16_t>(sample * 32767.0f);
}

// Helper: Convert int16 sample to float
float int16_to_float(int16_t sample) {
    return static_cast<float>(sample) / 32767.0f;
}

// Helper: Convert float sample to int32
int32_t float_to_int32(float sample) {
    sample = std::max(-1.0f, std::min(1.0f, sample));
    return static_cast<int32_t>(sample * 2147483647.0f);
}

// Helper: Convert int32 sample to float
float int32_to_float(int32_t sample) {
    return static_cast<float>(sample) / 2147483647.0f;
}

}  // namespace

ConversionResult DecodedAudioFrameConverter::convert(const DecodedFrame& frame, const PcmFormat& target_format) {
    ConversionResult result;
    
    // Validate input
    if (frame.stream_kind != "audio") {
        result.success = false;
        result.error_message = "Not an audio frame";
        return result;
    }

    if (frame.frame_data.empty()) {
        result.success = false;
        result.error_message = "No frame data";
        return result;
    }

    // Get source format info
    int src_sample_rate = frame.sample_rate;
    int src_channels = frame.channels;
    std::string src_sample_format = frame.sample_format;
    int src_bytes_per_sample = bytes_per_sample_from_format(src_sample_format);

    // Get target format info
    int dst_sample_rate = target_format.sample_rate;
    int dst_channels = target_format.channels;
    std::string dst_sample_format = target_format.sample_format;
    int dst_bytes_per_sample = target_format.bit_depth / 8;

    // Calculate source sample count
    int src_frame_bytes = src_channels * src_bytes_per_sample;
    if (src_frame_bytes == 0) {
        result.success = false;
        result.error_message = "Invalid source format";
        return result;
    }

    int src_sample_count = static_cast<int>(frame.frame_data.size()) / src_frame_bytes;
    if (src_sample_count == 0) {
        result.success = false;
        result.error_message = "No samples in frame";
        return result;
    }

    // For now, we'll do a simple format conversion without resampling
    // In a real implementation, this would use libsamplerate or similar

    // Calculate destination sample count (same as source without resampling)
    int dst_sample_count = src_sample_count;

    // Calculate destination buffer size
    int dst_frame_bytes = dst_channels * dst_bytes_per_sample;
    result.pcm_data.resize(dst_sample_count * dst_frame_bytes);
    result.sample_count = dst_sample_count;
    result.bytes_per_sample = dst_bytes_per_sample;
    result.timestamp_us = frame.pts;

    // Simple sample-by-sample conversion
    // This handles interleaved audio only (planar would need separate logic)
    if (target_format.is_planar) {
        // Planar output: separate channels
        result.pcm_data.resize(dst_sample_count * dst_channels * dst_bytes_per_sample);
        
        for (int ch = 0; ch < dst_channels; ++ch) {
            for (int sample = 0; sample < dst_sample_count; ++sample) {
                // Get source sample (interleaved)
                int src_idx = sample * src_channels + ch;
                if (src_idx >= src_sample_count * src_channels) continue;
                
                const uint8_t* src_ptr = frame.frame_data.data() + src_idx * src_bytes_per_sample;
                uint8_t* dst_ptr = result.pcm_data.data() + 
                    (ch * dst_sample_count + sample) * dst_bytes_per_sample;

                // Convert based on formats
                if (src_sample_format == "flt" && dst_sample_format == "s16") {
                    float src_val = *reinterpret_cast<const float*>(src_ptr);
                    int16_t dst_val = float_to_int16(src_val);
                    memcpy(dst_ptr, &dst_val, sizeof(int16_t));
                } else if (src_sample_format == "s16" && dst_sample_format == "flt") {
                    int16_t src_val = *reinterpret_cast<const int16_t*>(src_ptr);
                    float dst_val = int16_to_float(src_val);
                    memcpy(dst_ptr, &dst_val, sizeof(float));
                } else if (src_sample_format == "flt" && dst_sample_format == "flt") {
                    memcpy(dst_ptr, src_ptr, sizeof(float));
                } else if (src_sample_format == "s16" && dst_sample_format == "s16") {
                    memcpy(dst_ptr, src_ptr, sizeof(int16_t));
                } else {
                    // Unsupported conversion - copy as bytes
                    memcpy(dst_ptr, src_ptr, std::min(src_bytes_per_sample, dst_bytes_per_sample));
                }
            }
        }
    } else {
        // Interleaved output
        for (int sample = 0; sample < dst_sample_count; ++sample) {
            for (int ch = 0; ch < dst_channels; ++ch) {
                // Get source sample
                int src_idx = sample * src_channels + ch;
                if (src_idx >= src_sample_count * src_channels) continue;
                
                const uint8_t* src_ptr = frame.frame_data.data() + src_idx * src_bytes_per_sample;
                uint8_t* dst_ptr = result.pcm_data.data() + 
                    (sample * dst_channels + ch) * dst_bytes_per_sample;

                // Convert based on formats
                if (src_sample_format == "flt" && dst_sample_format == "s16") {
                    float src_val = *reinterpret_cast<const float*>(src_ptr);
                    int16_t dst_val = float_to_int16(src_val);
                    memcpy(dst_ptr, &dst_val, sizeof(int16_t));
                } else if (src_sample_format == "s16" && dst_sample_format == "flt") {
                    int16_t src_val = *reinterpret_cast<const int16_t*>(src_ptr);
                    float dst_val = int16_to_float(src_val);
                    memcpy(dst_ptr, &dst_val, sizeof(float));
                } else if (src_sample_format == "flt" && dst_sample_format == "flt") {
                    memcpy(dst_ptr, src_ptr, sizeof(float));
                } else if (src_sample_format == "s16" && dst_sample_format == "s16") {
                    memcpy(dst_ptr, src_ptr, sizeof(int16_t));
                } else {
                    // Unsupported conversion - copy as bytes
                    memcpy(dst_ptr, src_ptr, std::min(src_bytes_per_sample, dst_bytes_per_sample));
                }
            }
        }
    }

    result.success = true;
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