#include "kivo/cinema_engine/video_render_core/fake_sdr_upload_service.hpp"

namespace kivo::cinema_engine {

SdrUploadConfig build_default_sdr_upload_config(const std::string& upload_id) {
    SdrUploadConfig config;
    config.metadata.schema_version = "1.1";
    config.upload_id = upload_id;
    config.enable_color_correction = true;
    config.enable_sharpening = false;
    config.brightness_adjustment = 0.0f;
    config.contrast_adjustment = 1.0f;
    config.saturation_adjustment = 1.0f;
    return config;
}

SdrUploadResult build_successful_sdr_upload_result(const ContractMetadata& metadata, const std::string& upload_id, int32_t frames_processed) {
    SdrUploadResult result;
    result.metadata = metadata;
    result.upload_id = upload_id;
    result.success = true;
    result.frames_processed = frames_processed;
    result.frames_skipped = 0;
    return result;
}

SdrUploadResult build_failed_sdr_upload_result(const ContractMetadata& metadata, const std::string& upload_id, const std::string& error_message) {
    SdrUploadResult result;
    result.metadata = metadata;
    result.upload_id = upload_id;
    result.success = false;
    result.error_message = error_message;
    result.frames_processed = 0;
    result.frames_skipped = 0;
    return result;
}

SdrUploadResult build_custom_sdr_upload_result(const ContractMetadata& metadata, const std::string& upload_id, bool success, int32_t frames_processed, int32_t frames_skipped) {
    SdrUploadResult result;
    result.metadata = metadata;
    result.upload_id = upload_id;
    result.success = success;
    result.frames_processed = frames_processed;
    result.frames_skipped = frames_skipped;
    return result;
}

}  // namespace kivo::cinema_engine