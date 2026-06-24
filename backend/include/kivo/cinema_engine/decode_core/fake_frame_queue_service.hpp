#pragma once

#include "kivo/cinema_engine/decode_core/frame_queue_config.hpp"
#include "kivo/cinema_engine/decode_core/frame_queue_status.hpp"

namespace kivo::cinema_engine {

FrameQueueConfig build_default_frame_queue_config(const std::string& queue_id);
FrameQueueConfig build_frame_queue_config(const std::string& queue_id, size_t max_frames, size_t max_bytes);
FrameQueueStatus build_empty_frame_queue_status(const std::string& queue_id);
FrameQueueStatus build_full_frame_queue_status(const std::string& queue_id);
FrameQueueStatus build_frame_queue_status(const std::string& queue_id, size_t current_frames, size_t current_bytes);

}  // namespace kivo::cinema_engine