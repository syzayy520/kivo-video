#include "kivo/cinema_engine/video_render_core/fake_flip_presenter_service.hpp"

namespace kivo::cinema_engine {

FlipPresenterFirstFrameConfig build_default_flip_presenter_first_frame_config(const std::string& config_id) {
    FlipPresenterFirstFrameConfig config;
    config.metadata.schema_version = "1.1";
    config.config_id = config_id;
    config.enable_hdr = false;
    config.enable_zero_copy = true;
    config.width = 1920;
    config.height = 1080;
    config.pixel_format = "rgba8";
    config.vsync_enabled = true;
    config.max_latency = 2;
    return config;
}

FlipPresenterFirstFrameResult build_successful_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, int64_t presentation_time_us) {
    FlipPresenterFirstFrameResult result;
    result.metadata = metadata;
    result.config_id = config_id;
    result.success = true;
    result.first_frame_presented = true;
    result.presentation_time_us = presentation_time_us;
    result.hdr_active = false;
    result.zero_copy_active = true;
    return result;
}

FlipPresenterFirstFrameResult build_failed_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, const std::string& error_message) {
    FlipPresenterFirstFrameResult result;
    result.metadata = metadata;
    result.config_id = config_id;
    result.success = false;
    result.first_frame_presented = false;
    result.presentation_time_us = 0;
    result.error_message = error_message;
    result.hdr_active = false;
    result.zero_copy_active = false;
    return result;
}

FlipPresenterFirstFrameResult build_hdr_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, int64_t presentation_time_us) {
    FlipPresenterFirstFrameResult result;
    result.metadata = metadata;
    result.config_id = config_id;
    result.success = true;
    result.first_frame_presented = true;
    result.presentation_time_us = presentation_time_us;
    result.hdr_active = true;
    result.zero_copy_active = true;
    return result;
}

FlipPresenterFirstFrameResult build_zero_copy_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, int64_t presentation_time_us) {
    FlipPresenterFirstFrameResult result;
    result.metadata = metadata;
    result.config_id = config_id;
    result.success = true;
    result.first_frame_presented = true;
    result.presentation_time_us = presentation_time_us;
    result.hdr_active = false;
    result.zero_copy_active = true;
    return result;
}

}  // namespace kivo::cinema_engine