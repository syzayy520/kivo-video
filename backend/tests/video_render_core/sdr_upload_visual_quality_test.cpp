#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/video_render_core/fake_sdr_upload_service.hpp"
#include "kivo/cinema_engine/video_render_core/fake_visual_quality_service.hpp"

using namespace kivo::cinema_engine;

static void test_default_sdr_upload_config() {
    auto config = build_default_sdr_upload_config("upload-001");
    assert(config.metadata.schema_version == "1.1");
    assert(config.upload_id == "upload-001");
    assert(config.enable_color_correction == true);
    assert(config.enable_sharpening == false);
    assert(config.brightness_adjustment == 0.0f);
    assert(config.contrast_adjustment == 1.0f);
    assert(config.saturation_adjustment == 1.0f);
    std::cout << "  PASS: default_sdr_upload_config\n";
}

static void test_successful_sdr_upload_result() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_successful_sdr_upload_result(metadata, "upload-001", 100);
    assert(result.success == true);
    assert(result.upload_id == "upload-001");
    assert(result.frames_processed == 100);
    assert(result.frames_skipped == 0);
    assert(result.error_message.empty());
    std::cout << "  PASS: successful_sdr_upload_result\n";
}

static void test_failed_sdr_upload_result() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_failed_sdr_upload_result(metadata, "upload-002", "encoding error");
    assert(result.success == false);
    assert(result.upload_id == "upload-002");
    assert(result.frames_processed == 0);
    assert(result.frames_skipped == 0);
    assert(result.error_message == "encoding error");
    std::cout << "  PASS: failed_sdr_upload_result\n";
}

static void test_custom_sdr_upload_result() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_custom_sdr_upload_result(metadata, "upload-003", true, 50, 5);
    assert(result.success == true);
    assert(result.upload_id == "upload-003");
    assert(result.frames_processed == 50);
    assert(result.frames_skipped == 5);
    std::cout << "  PASS: custom_sdr_upload_result\n";
}

static void test_excellent_visual_quality() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_excellent_visual_quality_result(metadata, "metrics-001");
    assert(result.meets_baseline == true);
    assert(result.quality_tier == "excellent");
    assert(result.metrics.psnr == 45.0f);
    assert(result.metrics.ssim == 0.98f);
    assert(result.metrics.vmaf == 95.0f);
    assert(result.metrics.bitrate_efficiency == 0.95f);
    assert(result.metrics.frame_count == 1000);
    assert(result.metrics.is_hdr == false);
    std::cout << "  PASS: excellent_visual_quality\n";
}

static void test_good_visual_quality() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_good_visual_quality_result(metadata, "metrics-002");
    assert(result.meets_baseline == true);
    assert(result.quality_tier == "good");
    assert(result.metrics.psnr == 38.0f);
    assert(result.metrics.ssim == 0.92f);
    assert(result.metrics.vmaf == 85.0f);
    assert(result.metrics.bitrate_efficiency == 0.85f);
    std::cout << "  PASS: good_visual_quality\n";
}

static void test_acceptable_visual_quality() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_acceptable_visual_quality_result(metadata, "metrics-003");
    assert(result.meets_baseline == true);
    assert(result.quality_tier == "acceptable");
    assert(result.metrics.psnr == 30.0f);
    assert(result.metrics.ssim == 0.85f);
    assert(result.metrics.vmaf == 70.0f);
    assert(result.metrics.bitrate_efficiency == 0.70f);
    std::cout << "  PASS: acceptable_visual_quality\n";
}

static void test_poor_visual_quality() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto result = build_poor_visual_quality_result(metadata, "metrics-004");
    assert(result.meets_baseline == false);
    assert(result.quality_tier == "poor");
    assert(result.metrics.psnr == 20.0f);
    assert(result.metrics.ssim == 0.70f);
    assert(result.metrics.vmaf == 50.0f);
    assert(result.metrics.bitrate_efficiency == 0.50f);
    assert(result.warnings.size() == 1);
    assert(result.warnings[0] == "Visual quality below baseline threshold");
    std::cout << "  PASS: poor_visual_quality\n";
}

static void test_custom_visual_quality() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    std::vector<std::string> warnings = {"Low bitrate", "High compression"};
    auto result = build_custom_visual_quality_result(metadata, "metrics-005", "good", true, warnings);
    assert(result.meets_baseline == true);
    assert(result.quality_tier == "good");
    assert(result.warnings.size() == 2);
    assert(result.warnings[0] == "Low bitrate");
    assert(result.warnings[1] == "High compression");
    std::cout << "  PASS: custom_visual_quality\n";
}

static void test_visual_quality_baseline_check() {
    ContractMetadata metadata;
    metadata.schema_version = "1.1";
    metadata.trace_id = "test-trace";
    
    auto excellent = build_excellent_visual_quality_result(metadata, "metrics-006");
    assert(excellent.meets_baseline == true);
    
    auto poor = build_poor_visual_quality_result(metadata, "metrics-007");
    assert(poor.meets_baseline == false);
    
    std::cout << "  PASS: visual_quality_baseline_check\n";
}

int main() {
    std::cout << "sdr_upload_visual_quality_test:\n";
    test_default_sdr_upload_config();
    test_successful_sdr_upload_result();
    test_failed_sdr_upload_result();
    test_custom_sdr_upload_result();
    test_excellent_visual_quality();
    test_good_visual_quality();
    test_acceptable_visual_quality();
    test_poor_visual_quality();
    test_custom_visual_quality();
    test_visual_quality_baseline_check();
    std::cout << "ALL 10 TESTS PASSED\n";
    return 0;
}