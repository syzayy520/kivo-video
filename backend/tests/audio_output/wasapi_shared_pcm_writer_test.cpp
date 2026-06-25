#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp"
#include "kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp"

using namespace kivo::cinema_engine;

int main() {
    std::cout << "wasapi_shared_pcm_writer_test:\n";

    // Test 1: Create writer
    WasapiSharedPcmWriter writer;
    std::cout << "  PASS: Writer created\n";

    // Test 2: Check if ready (not ready initially)
    assert(!writer.is_ready());
    std::cout << "  PASS: Writer not ready initially\n";

    // Test 3: Get last error (empty initially)
    assert(writer.last_error().empty());
    std::cout << "  PASS: No error initially\n";

    // Test 4: Initialize with audio endpoint
    AudioEndpointRuntimeContract endpoint;
    endpoint.endpoint_id = "test-endpoint";
    endpoint.endpoint_name = "Speakers (Realtek Audio)";
    endpoint.output_mode = "shared_pcm";
    endpoint.sample_rate = 44100;
    endpoint.channels = 2;
    endpoint.bit_depth = 16;

    bool init_result = writer.initialize(endpoint);
    assert(init_result);
    assert(writer.is_ready());
    std::cout << "  PASS: Writer initialized with endpoint " << endpoint.endpoint_name << "\n";

    // Test 5: Write PCM data
    std::vector<uint8_t> pcm_data(4096, 0);  // 1024 frames * 2 channels * 2 bytes
    WriteResult write_result = writer.write(pcm_data.data(), static_cast<int32_t>(pcm_data.size()));
    assert(write_result.success);
    assert(write_result.samples_written == 1024);
    assert(write_result.frames_written == 512);
    assert(!write_result.is_buffer_full);
    std::cout << "  PASS: PCM data written, samples=" << write_result.samples_written
              << ", frames=" << write_result.frames_written << "\n";

    // Test 6: Check available frames
    int32_t available = writer.available_frames();
    assert(available == 1024);
    std::cout << "  PASS: Available frames=" << available << "\n";

    // Test 7: Start playback
    bool start_result = writer.start();
    assert(start_result);
    std::cout << "  PASS: Playback started\n";

    // Test 8: Stop playback
    bool stop_result = writer.stop();
    assert(stop_result);
    std::cout << "  PASS: Playback stopped\n";

    // Test 9: Release
    writer.release();
    assert(!writer.is_ready());
    std::cout << "  PASS: Writer released\n";

    std::cout << "ALL 9 TESTS PASSED\n";
    return 0;
}
