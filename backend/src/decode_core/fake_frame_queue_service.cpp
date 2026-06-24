#include "kivo/cinema_engine/decode_core/fake_frame_queue_service.hpp"

namespace kivo::cinema_engine {

FrameQueueConfig build_default_frame_queue_config(const std::string& queue_id) {
    FrameQueueConfig config;
    config.metadata.schema_version = "1.1";
    config.queue_id = queue_id;
    config.max_frames = 10;
    config.max_bytes = 100 * 1024 * 1024; // 100MB
    config.drop_on_overflow = false;
    return config;
}

FrameQueueConfig build_frame_queue_config(const std::string& queue_id, size_t max_frames, size_t max_bytes) {
    FrameQueueConfig config;
    config.metadata.schema_version = "1.1";
    config.queue_id = queue_id;
    config.max_frames = max_frames;
    config.max_bytes = max_bytes;
    config.drop_on_overflow = false;
    return config;
}

FrameQueueStatus build_empty_frame_queue_status(const std::string& queue_id) {
    FrameQueueStatus status;
    status.metadata.schema_version = "1.1";
    status.queue_id = queue_id;
    status.current_frames = 0;
    status.current_bytes = 0;
    status.is_full = false;
    status.is_empty = true;
    return status;
}

FrameQueueStatus build_full_frame_queue_status(const std::string& queue_id) {
    FrameQueueStatus status;
    status.metadata.schema_version = "1.1";
    status.queue_id = queue_id;
    status.current_frames = 10;
    status.current_bytes = 100 * 1024 * 1024; // 100MB
    status.is_full = true;
    status.is_empty = false;
    return status;
}

FrameQueueStatus build_frame_queue_status(const std::string& queue_id, size_t current_frames, size_t current_bytes) {
    FrameQueueStatus status;
    status.metadata.schema_version = "1.1";
    status.queue_id = queue_id;
    status.current_frames = current_frames;
    status.current_bytes = current_bytes;
    status.is_full = (current_frames >= 10 || current_bytes >= 100 * 1024 * 1024);
    status.is_empty = (current_frames == 0);
    return status;
}

}  // namespace kivo::cinema_engine