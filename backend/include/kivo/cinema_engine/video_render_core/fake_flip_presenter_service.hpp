#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_render_core/flip_presenter_first_frame_config.hpp"
#include "kivo/cinema_engine/video_render_core/flip_presenter_first_frame_result.hpp"

namespace kivo::cinema_engine {

FlipPresenterFirstFrameConfig build_default_flip_presenter_first_frame_config(const std::string& config_id);
FlipPresenterFirstFrameResult build_successful_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, int64_t presentation_time_us);
FlipPresenterFirstFrameResult build_failed_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, const std::string& error_message);
FlipPresenterFirstFrameResult build_hdr_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, int64_t presentation_time_us);
FlipPresenterFirstFrameResult build_zero_copy_first_frame_result(const ContractMetadata& metadata, const std::string& config_id, int64_t presentation_time_us);

}  // namespace kivo::cinema_engine