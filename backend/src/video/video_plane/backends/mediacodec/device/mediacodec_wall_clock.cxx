#include "video/video_plane/backends/mediacodec/device/mediacodec_wall_clock.hpp"

#include <chrono>

namespace kivo::video::mediacodec {

std::int64_t now_unix_millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace kivo::video::mediacodec
