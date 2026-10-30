#include "kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <cmath>
#include <vector>

namespace kivo::cinema_engine {

namespace {

std::string packed_format(std::string format) {
    if (!format.empty() && format.back() == 'p') {
        format.pop_back();
    }
    return format;
}

bool is_planar_format(const std::string& format) {
    return !format.empty() && format.back() == 'p';
}

int bytes_per_sample_from_format(const std::string& format) {
    const std::string packed = packed_format(format);
    if (packed == "s16" || packed == "u16") return 2;
    if (packed == "s32" || packed == "u32" || packed == "flt") return 4;
    if (packed == "s64" || packed == "u64" || packed == "dbl") return 8;
    if (packed == "s8" || packed == "u8") return 1;
    return 2;  // default to 16-bit
}

int16_t float_to_int16(float sample) {
    sample = std::max(-1.0f, std::min(1.0f, sample));
    return static_cast<int16_t>(sample * 32767.0f);
}

int32_t float_to_int32(float sample) {
    sample = std::max(-1.0f, std::min(1.0f, sample));
    return static_cast<int32_t>(sample * 2147483647.0f);
}

float read_sample_as_float(const uint8_t* ptr, const std::string& format) {
    const std::string packed = packed_format(format);
    if (packed == "flt") {
        float value = 0.0f;
        std::memcpy(&value, ptr, sizeof(value));
        return value;
    }
    if (packed == "dbl") {
        double value = 0.0;
        std::memcpy(&value, ptr, sizeof(value));
        return static_cast<float>(value);
    }
    if (packed == "s16") {
        int16_t value = 0;
        std::memcpy(&value, ptr, sizeof(value));
        return static_cast<float>(value) / 32767.0f;
    }
    if (packed == "s32") {
        int32_t value = 0;
        std::memcpy(&value, ptr, sizeof(value));
        return static_cast<float>(value) / 2147483647.0f;
    }
    if (packed == "u8") {
        uint8_t value = 0;
        std::memcpy(&value, ptr, sizeof(value));
        return (static_cast<float>(value) - 128.0f) / 127.0f;
    }
    return 0.0f;
}

void write_sample_from_float(float sample, uint8_t* ptr, const std::string& format) {
    const std::string packed = packed_format(format);
    if (packed == "flt") {
        std::memcpy(ptr, &sample, sizeof(sample));
        return;
    }
    if (packed == "dbl") {
        const double value = static_cast<double>(sample);
        std::memcpy(ptr, &value, sizeof(value));
        return;
    }
    if (packed == "s32") {
        const int32_t value = float_to_int32(sample);
        std::memcpy(ptr, &value, sizeof(value));
        return;
    }
    if (packed == "u8") {
        const auto clamped = std::max(-1.0f, std::min(1.0f, sample));
        const auto value = static_cast<uint8_t>((clamped * 127.0f) + 128.0f);
        std::memcpy(ptr, &value, sizeof(value));
        return;
    }
    const int16_t value = float_to_int16(sample);
    std::memcpy(ptr, &value, sizeof(value));
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

    int src_channels = frame.channels;
    std::string src_sample_format = frame.sample_format;
    int src_bytes_per_sample = bytes_per_sample_from_format(src_sample_format);
    const bool src_planar = is_planar_format(src_sample_format);

    int dst_channels = target_format.channels;
    std::string dst_sample_format =
        target_format.sample_format.empty() ? "s16" : target_format.sample_format;
    int dst_bytes_per_sample = bytes_per_sample_from_format(dst_sample_format);
    if (target_format.bit_depth > 0 && packed_format(dst_sample_format) != "flt") {
        dst_bytes_per_sample = target_format.bit_depth / 8;
    }

    if (src_channels <= 0 || dst_channels <= 0 || src_bytes_per_sample <= 0 ||
        dst_bytes_per_sample <= 0) {
        result.success = false;
        result.error_message = "Invalid audio format";
        return result;
    }

    int src_sample_count = 0;
    std::vector<size_t> plane_offsets(static_cast<size_t>(src_channels), 0);
    if (src_planar) {
        size_t offset = 0;
        for (int ch = 0; ch < src_channels; ++ch) {
            plane_offsets[static_cast<size_t>(ch)] = offset;
            const int line_size = ch < 8 && frame.linesize[ch] > 0
                                      ? frame.linesize[ch]
                                      : 0;
            if (ch == 0 && line_size > 0) {
                src_sample_count = line_size / src_bytes_per_sample;
            }
            const size_t bytes =
                line_size > 0
                    ? static_cast<size_t>(line_size)
                    : static_cast<size_t>(std::max(0, src_sample_count)) *
                          static_cast<size_t>(src_bytes_per_sample);
            offset += bytes;
        }
        if (src_sample_count <= 0 && src_channels > 0) {
            src_sample_count = static_cast<int>(
                frame.frame_data.size() /
                (static_cast<size_t>(src_channels) * src_bytes_per_sample));
        }
    } else {
        src_sample_count = static_cast<int>(
            frame.frame_data.size() /
            (static_cast<size_t>(src_channels) * src_bytes_per_sample));
    }

    if (src_sample_count == 0) {
        result.success = false;
        result.error_message = "No samples in frame";
        return result;
    }

    int dst_sample_count = src_sample_count;
    const int dst_frame_bytes = dst_channels * dst_bytes_per_sample;
    result.pcm_data.resize(dst_sample_count * dst_frame_bytes);
    result.sample_count = dst_sample_count;
    result.bytes_per_sample = dst_bytes_per_sample;
    result.timestamp_us = frame.pts;

    auto read_source_sample = [&](int sample, int channel) -> float {
        int src_channel = channel;
        if (src_channel >= src_channels) {
            src_channel = src_channels == 1 ? 0 : -1;
        }
        if (src_channel < 0) {
            return 0.0f;
        }

        size_t byte_offset = 0;
        if (src_planar) {
            byte_offset = plane_offsets[static_cast<size_t>(src_channel)] +
                          static_cast<size_t>(sample) *
                              static_cast<size_t>(src_bytes_per_sample);
        } else {
            byte_offset =
                (static_cast<size_t>(sample) * static_cast<size_t>(src_channels) +
                 static_cast<size_t>(src_channel)) *
                static_cast<size_t>(src_bytes_per_sample);
        }
        if (byte_offset + static_cast<size_t>(src_bytes_per_sample) >
            frame.frame_data.size()) {
            return 0.0f;
        }
        return read_sample_as_float(frame.frame_data.data() + byte_offset,
                                    src_sample_format);
    };

    if (target_format.is_planar) {
        result.pcm_data.resize(dst_sample_count * dst_channels * dst_bytes_per_sample);
        for (int ch = 0; ch < dst_channels; ++ch) {
            for (int sample = 0; sample < dst_sample_count; ++sample) {
                uint8_t* dst_ptr = result.pcm_data.data() +
                    (static_cast<size_t>(ch) * static_cast<size_t>(dst_sample_count) +
                     static_cast<size_t>(sample)) *
                        static_cast<size_t>(dst_bytes_per_sample);
                write_sample_from_float(read_source_sample(sample, ch), dst_ptr,
                                        dst_sample_format);
            }
        }
    } else {
        for (int sample = 0; sample < dst_sample_count; ++sample) {
            for (int ch = 0; ch < dst_channels; ++ch) {
                uint8_t* dst_ptr = result.pcm_data.data() +
                    (static_cast<size_t>(sample) * static_cast<size_t>(dst_channels) +
                     static_cast<size_t>(ch)) *
                        static_cast<size_t>(dst_bytes_per_sample);
                write_sample_from_float(read_source_sample(sample, ch), dst_ptr,
                                        dst_sample_format);
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
