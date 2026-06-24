#pragma once

#include <cstdint>

#include "kivo/cinema_engine/media_timeline_core/timebase_normalization.hpp"

namespace kivo::cinema_engine {

// Build a TimebaseNormalization from container time.
TimebaseNormalization normalize_from_container_time(
    int32_t tb_num, int32_t tb_den, int64_t container_time);

// Build a TimebaseNormalization from stream time.
TimebaseNormalization normalize_from_stream_time(
    int32_t tb_num, int32_t tb_den, int64_t stream_time);

// Convert KivoMediaTime back to container time.
int64_t kivo_media_time_to_container_time(
    int64_t kivo_media_time_us, int32_t tb_num, int32_t tb_den);

// Convert KivoMediaTime back to stream time.
int64_t kivo_media_time_to_stream_time(
    int64_t kivo_media_time_us, int32_t tb_num, int32_t tb_den);

}  // namespace kivo::cinema_engine
