#define _USE_MATH_DEFINES
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp"
#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"

using namespace kivo::cinema_engine;

int main() {
    std::cout << "decoded_audio_frame_converter_test:\n";

    // Test 1: Create converter
    DecodedAudioFrameConverter converter;
    std::cout << "  PASS: Converter created\n";

    // Test 2: Check if ready
    assert(converter.is_ready());
    std::cout << "  PASS: Converter is ready\n";

    // Test 3: Get last error (empty initially)
    assert(converter.last_error().empty());
    std::cout << "  PASS: No error initially\n";

    // Test 4: Convert a decoded audio frame with real data
    DecodedFrame frame;
    frame.stream_kind = "audio";
    frame.sample_rate = 44100;
    frame.channels = 2;
    frame.sample_format = "flt";  // float planar format
    frame.pts = 0;

    // Create real audio data: 1024 samples, 2 channels, float format
    // Each sample is a float between -1.0 and 1.0
    const int sample_count = 1024;
    const int channels = 2;
    frame.frame_data.resize(sample_count * channels * sizeof(float));
    
    // Fill with a simple sine wave
    float* data = reinterpret_cast<float*>(frame.frame_data.data());
    for (int i = 0; i < sample_count; ++i) {
        float sample = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / 44100.0));
        for (int ch = 0; ch < channels; ++ch) {
            data[i * channels + ch] = sample;
        }
    }

    PcmFormat target_format;
    target_format.sample_rate = 44100;
    target_format.channels = 2;
    target_format.bit_depth = 16;
    target_format.sample_format = "s16";

    ConversionResult result = converter.convert(frame, target_format);
    assert(result.success);
    assert(result.sample_count == sample_count);
    assert(result.bytes_per_sample == 2);
    assert(result.timestamp_us == 0);
    assert(!result.pcm_data.empty());
    std::cout << "  PASS: Conversion successful, samples=" << result.sample_count
              << ", bytes=" << result.pcm_data.size() << "\n";

    // Verify the conversion actually converted data (not all zeros)
    bool has_non_zero = false;
    const int16_t* pcm_samples = reinterpret_cast<const int16_t*>(result.pcm_data.data());
    for (int i = 0; i < result.sample_count * channels; ++i) {
        if (pcm_samples[i] != 0) {
            has_non_zero = true;
            break;
        }
    }
    assert(has_non_zero);
    std::cout << "  PASS: Conversion produced non-zero data\n";

    // Test 5: Convert with different format
    PcmFormat target_format_32;
    target_format_32.sample_rate = 48000;
    target_format_32.channels = 1;
    target_format_32.bit_depth = 32;
    target_format_32.sample_format = "flt";

    ConversionResult result_32 = converter.convert(frame, target_format_32);
    assert(result_32.success);
    assert(result_32.bytes_per_sample == 4);
    std::cout << "  PASS: Conversion to 32-bit float successful\n";

    // Test 6: Convert empty frame
    DecodedFrame empty_frame;
    empty_frame.stream_kind = "audio";
    empty_frame.frame_data.empty();
    
    ConversionResult empty_result = converter.convert(empty_frame, target_format);
    assert(!empty_result.success);
    assert(!empty_result.error_message.empty());
    std::cout << "  PASS: Empty frame conversion fails correctly\n";

    // Test 7: Convert video frame
    DecodedFrame video_frame;
    video_frame.stream_kind = "video";
    video_frame.frame_data = {0x00, 0x00, 0x00, 0xFF};  // Dummy data
    
    ConversionResult video_result = converter.convert(video_frame, target_format);
    assert(!video_result.success);
    assert(!video_result.error_message.empty());
    std::cout << "  PASS: Video frame conversion fails correctly\n";

    // Test 8: Release
    converter.release();
    assert(!converter.is_ready());
    std::cout << "  PASS: Converter released\n";

    std::cout << "ALL 8 TESTS PASSED\n";
    return 0;
}