#include <cassert>
#include <iostream>
#include <string>

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

    // Test 4: Convert a decoded audio frame
    DecodedFrame frame;
    frame.stream_kind = "audio";
    frame.sample_rate = 44100;
    frame.channels = 2;
    frame.sample_format = "fltp";
    frame.pts = 0;

    PcmFormat target_format;
    target_format.sample_rate = 44100;
    target_format.channels = 2;
    target_format.bit_depth = 16;
    target_format.sample_format = "s16";

    ConversionResult result = converter.convert(frame, target_format);
    assert(result.success);
    assert(result.sample_count == 1024);
    assert(result.bytes_per_sample == 2);
    assert(result.timestamp_us == 0);
    assert(!result.pcm_data.empty());
    std::cout << "  PASS: Conversion successful, samples=" << result.sample_count
              << ", bytes=" << result.pcm_data.size() << "\n";

    // Test 5: Convert with different format
    PcmFormat target_format_32;
    target_format_32.sample_rate = 48000;
    target_format_32.channels = 1;
    target_format_32.bit_depth = 32;
    target_format_32.sample_format = "s32";

    ConversionResult result_32 = converter.convert(frame, target_format_32);
    assert(result_32.success);
    assert(result_32.bytes_per_sample == 4);
    std::cout << "  PASS: Conversion to 32-bit successful\n";

    // Test 6: Release
    converter.release();
    assert(!converter.is_ready());
    std::cout << "  PASS: Converter released\n";

    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}
