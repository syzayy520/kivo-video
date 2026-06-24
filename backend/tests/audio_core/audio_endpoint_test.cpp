#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/audio_core/fake_audio_endpoint_service.hpp"

using namespace kivo::cinema_engine;

static void test_shared_pcm_endpoint() {
    auto endpoint = build_shared_pcm_endpoint("ep-001", 44100, 2, 16);
    assert(endpoint.metadata.schema_version == "1.1");
    assert(endpoint.endpoint_id == "ep-001");
    assert(endpoint.endpoint_name == "Shared PCM Endpoint");
    assert(endpoint.output_mode == "shared_pcm");
    assert(endpoint.sample_rate == 44100);
    assert(endpoint.channels == 2);
    assert(endpoint.bit_depth == 16);
    assert(endpoint.is_initialized == true);
    assert(endpoint.is_playing == false);
    assert(endpoint.samples_written == 0);
    assert(endpoint.buffer_duration_ms == 10.0);
    std::cout << "  PASS: shared_pcm_endpoint\n";
}

static void test_uninitialized_endpoint() {
    auto endpoint = build_uninitialized_endpoint("ep-002");
    assert(endpoint.metadata.schema_version == "1.1");
    assert(endpoint.endpoint_id == "ep-002");
    assert(endpoint.is_initialized == false);
    assert(endpoint.is_playing == false);
    assert(endpoint.sample_rate == 0);
    assert(endpoint.channels == 0);
    assert(endpoint.bit_depth == 0);
    std::cout << "  PASS: uninitialized_endpoint\n";
}

static void test_playing_endpoint() {
    auto endpoint = build_playing_endpoint("ep-003", 12345, 20.5);
    assert(endpoint.metadata.schema_version == "1.1");
    assert(endpoint.endpoint_id == "ep-003");
    assert(endpoint.is_initialized == true);
    assert(endpoint.is_playing == true);
    assert(endpoint.samples_written == 12345);
    assert(endpoint.buffer_duration_ms == 20.5);
    assert(endpoint.sample_rate == 48000);
    assert(endpoint.channels == 2);
    assert(endpoint.bit_depth == 16);
    std::cout << "  PASS: playing_endpoint\n";
}

static void test_exclusive_pcm_endpoint() {
    auto endpoint = build_exclusive_pcm_endpoint("ep-004", 96000, 6, 24);
    assert(endpoint.metadata.schema_version == "1.1");
    assert(endpoint.endpoint_id == "ep-004");
    assert(endpoint.output_mode == "exclusive_pcm");
    assert(endpoint.sample_rate == 96000);
    assert(endpoint.channels == 6);
    assert(endpoint.bit_depth == 24);
    assert(endpoint.is_initialized == true);
    assert(endpoint.is_playing == false);
    std::cout << "  PASS: exclusive_pcm_endpoint\n";
}

static void test_passthrough_endpoint() {
    auto endpoint = build_passthrough_endpoint("ep-005", 48000, 8, 32);
    assert(endpoint.metadata.schema_version == "1.1");
    assert(endpoint.endpoint_id == "ep-005");
    assert(endpoint.output_mode == "passthrough");
    assert(endpoint.sample_rate == 48000);
    assert(endpoint.channels == 8);
    assert(endpoint.bit_depth == 32);
    assert(endpoint.is_initialized == true);
    assert(endpoint.is_playing == false);
    std::cout << "  PASS: passthrough_endpoint\n";
}

int main() {
    std::cout << "audio_endpoint_test:\n";
    test_shared_pcm_endpoint();
    test_uninitialized_endpoint();
    test_playing_endpoint();
    test_exclusive_pcm_endpoint();
    test_passthrough_endpoint();
    std::cout << "ALL 5 TESTS PASSED\n";
    return 0;
}