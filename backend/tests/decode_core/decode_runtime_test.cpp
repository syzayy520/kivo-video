#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/decode_core/decode_request.hpp"
#include "kivo/cinema_engine/decode_core/fake_decode_runtime.hpp"

using namespace kivo::cinema_engine;

static void test_successful_decode() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    DecodedFrame frame;
    frame.frame_id = "frame-001";
    frame.stream_kind = "video";
    frame.pts = 1000;
    frame.dts = 900;
    frame.duration = 33;
    frame.width = 1920;
    frame.height = 1080;
    frame.pixel_format = "yuv420p";
    
    auto result = build_successful_decode_result(metadata, frame);
    assert(result.success == true);
    assert(result.eof == false);
    assert(result.needs_more_input == false);
    assert(result.frame.frame_id == "frame-001");
    assert(result.frame.stream_kind == "video");
    assert(result.frame.width == 1920);
    assert(result.frame.height == 1080);
    std::cout << "  PASS: successful_decode\n";
}

static void test_eof_detection() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_eof_decode_result(metadata);
    assert(result.success == true);
    assert(result.eof == true);
    assert(result.needs_more_input == false);
    assert(result.frame.is_eof == true);
    std::cout << "  PASS: eof_detection\n";
}

static void test_needs_more_input() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_needs_more_input_result(metadata);
    assert(result.success == false);
    assert(result.eof == false);
    assert(result.needs_more_input == true);
    std::cout << "  PASS: needs_more_input\n";
}

static void test_failed_decode() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_failed_decode_result(metadata, "codec error: invalid bitstream");
    assert(result.success == false);
    assert(result.eof == false);
    assert(result.needs_more_input == false);
    assert(result.error_message == "codec error: invalid bitstream");
    std::cout << "  PASS: failed_decode\n";
}

static void test_custom_frame() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    DecodedFrame frame;
    frame.frame_id = "audio-001";
    frame.stream_kind = "audio";
    frame.pts = 5000;
    frame.sample_rate = 48000;
    frame.channels = 2;
    frame.sample_format = "s16";
    
    auto result = build_decode_result_with_frame(metadata, frame);
    assert(result.success == true);
    assert(result.frame.frame_id == "audio-001");
    assert(result.frame.stream_kind == "audio");
    assert(result.frame.sample_rate == 48000);
    assert(result.frame.channels == 2);
    std::cout << "  PASS: custom_frame\n";
}

static void test_decode_request_fields() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    DecodeRequest request;
    request.metadata = metadata;
    request.packet.packet_id = "pkt-001";
    request.packet.stream_index = 0;
    request.packet.pts = 100;
    request.packet.is_key_frame = true;
    request.force_software = true;
    
    assert(request.packet.packet_id == "pkt-001");
    assert(request.packet.stream_index == 0);
    assert(request.packet.pts == 100);
    assert(request.packet.is_key_frame == true);
    assert(request.force_software == true);
    std::cout << "  PASS: decode_request_fields\n";
}

static void test_decode_result_fields() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    DecodeResult result;
    result.metadata = metadata;
    result.success = true;
    result.eof = false;
    result.needs_more_input = false;
    result.error_message = "";
    
    assert(result.success == true);
    assert(result.eof == false);
    assert(result.needs_more_input == false);
    assert(result.error_message.empty());
    std::cout << "  PASS: decode_result_fields\n";
}

int main() {
    std::cout << "decode_runtime_test:\n";
    test_successful_decode();
    test_eof_detection();
    test_needs_more_input();
    test_failed_decode();
    test_custom_frame();
    test_decode_request_fields();
    test_decode_result_fields();
    std::cout << "ALL 7 TESTS PASSED\n";
    return 0;
}