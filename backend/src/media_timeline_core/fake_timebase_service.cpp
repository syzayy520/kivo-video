#include "kivo/cinema_engine/media_timeline_core/fake_timebase_service.hpp"

namespace kivo::cinema_engine {

TimebaseNormalization normalize_from_container_time(
    int32_t tb_num, int32_t tb_den, int64_t container_time) {
  TimebaseNormalization result;
  result.metadata.schema_version = "1.1";
  result.container_timebase_num = tb_num;
  result.container_timebase_den = tb_den;
  result.stream_timebase_num = tb_num;
  result.stream_timebase_den = tb_den;

  if (tb_den == 0) {
    result.is_valid = false;
    return result;
  }

  // Convert to microseconds: container_time * (num/den) * 1000000
  result.kivo_media_time_us = container_time * tb_num * 1000000LL / tb_den;
  result.is_valid = true;
  return result;
}

TimebaseNormalization normalize_from_stream_time(
    int32_t tb_num, int32_t tb_den, int64_t stream_time) {
  TimebaseNormalization result;
  result.metadata.schema_version = "1.1";
  result.container_timebase_num = tb_num;
  result.container_timebase_den = tb_den;
  result.stream_timebase_num = tb_num;
  result.stream_timebase_den = tb_den;

  if (tb_den == 0) {
    result.is_valid = false;
    return result;
  }

  result.kivo_media_time_us = stream_time * tb_num * 1000000LL / tb_den;
  result.is_valid = true;
  return result;
}

int64_t kivo_media_time_to_container_time(
    int64_t kivo_media_time_us, int32_t tb_num, int32_t tb_den) {
  if (tb_num == 0) return 0;
  return kivo_media_time_us * tb_den / (tb_num * 1000000LL);
}

int64_t kivo_media_time_to_stream_time(
    int64_t kivo_media_time_us, int32_t tb_num, int32_t tb_den) {
  if (tb_num == 0) return 0;
  return kivo_media_time_us * tb_den / (tb_num * 1000000LL);
}

}  // namespace kivo::cinema_engine
