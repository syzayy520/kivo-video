#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_render_core/sdr_upload_config.hpp"
#include "kivo/cinema_engine/video_render_core/sdr_upload_result.hpp"

namespace kivo::cinema_engine {

SdrUploadConfig build_default_sdr_upload_config(const std::string& upload_id);
SdrUploadResult build_successful_sdr_upload_result(const ContractMetadata& metadata, const std::string& upload_id, int32_t frames_processed);
SdrUploadResult build_failed_sdr_upload_result(const ContractMetadata& metadata, const std::string& upload_id, const std::string& error_message);
SdrUploadResult build_custom_sdr_upload_result(const ContractMetadata& metadata, const std::string& upload_id, bool success, int32_t frames_processed, int32_t frames_skipped);

}  // namespace kivo::cinema_engine