#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/video_render_core/fake_flip_presenter_service.hpp"

using namespace kivo::cinema_engine;

static void test_default_config() {
    auto config = build_default_flip_presenter_first_frame_config("config-001");
    assert(config.metadata.schema_version == "1.1");
    assert(config.config_id == "config-001");
    assert(config.enable_hdr == false);
    assert(config.enable_zero_copy == true);
    assert(config.width == 1920);
    assert(config.height == 1080);
    assert(config.pixel_format == "rgba8");
    assert(config.vsync_enabled == true);
    assert(config.max_latency == 2);
    std::cout << "  PASS: default_config\n";
}

static void test_successful_first_frame() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_successful_first_frame_result(metadata, "config-001", 16667);
    assert(result.success == true);
    assert(result.first_frame_presented == true);
    assert(result.presentation_time_us == 16667);
    assert(result.hdr_active == false);
    assert(result.zero_copy_active == true);
    assert(result.error_message.empty());
    std::cout << "  PASS: successful_first_frame\n";
}

static void test_failed_first_frame() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_failed_first_frame_result(metadata, "config-002", "device lost");
    assert(result.success == false);
    assert(result.first_frame_presented == false);
    assert(result.presentation_time_us == 0);
    assert(result.error_message == "device lost");
    assert(result.hdr_active == false);
    assert(result.zero_copy_active == false);
    std::cout << "  PASS: failed_first_frame\n";
}

static void test_hdr_first_frame() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_hdr_first_frame_result(metadata, "config-003", 16667);
    assert(result.success == true);
    assert(result.first_frame_presented == true);
    assert(result.presentation_time_us == 16667);
    assert(result.hdr_active == true);
    assert(result.zero_copy_active == true);
    std::cout << "  PASS: hdr_first_frame\n";
}

static void test_zero_copy_first_frame() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_zero_copy_first_frame_result(metadata, "config-004", 16667);
    assert(result.success == true);
    assert(result.first_frame_presented == true);
    assert(result.presentation_time_us == 16667);
    assert(result.hdr_active == false);
    assert(result.zero_copy_active == true);
    std::cout << "  PASS: zero_copy_first_frame\n";
}

static void test_custom_config() {
    auto config = build_default_flip_presenter_first_frame_config("config-005");
    config.enable_hdr = true;
    config.width = 3840;
    config.height = 2160;
    config.pixel_format = "rgba16f";
    
    assert(config.config_id == "config-005");
    assert(config.enable_hdr == true);
    assert(config.width == 3840);
    assert(config.height == 2160);
    assert(config.pixel_format == "rgba16f");
    std::cout << "  PASS: custom_config\n";
}

static void test_config_validation() {
    auto config = build_default_flip_presenter_first_frame_config("config-006");
    assert(config.width > 0);
    assert(config.height > 0);
    assert(config.max_latency > 0);
    assert(!config.pixel_format.empty());
    std::cout << "  PASS: config_validation\n";
}

int main() {
    std::cout << "flip_presenter_first_frame_test:\n";
    test_default_config();
    test_successful_first_frame();
    test_failed_first_frame();
    test_hdr_first_frame();
    test_zero_copy_first_frame();
    test_custom_config();
    test_config_validation();
    std::cout << "ALL 7 TESTS PASSED\n";
    return 0;
}