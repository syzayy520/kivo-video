// timestamp_wrap.hpp -- P5E E3: timestamp wrap detection
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::timing {

struct TimestampWrapResult {
    bool wrapped{false};
    int64_t unwrapped_value{0};
    uint32_t wrap_count{0};
    std::string wrap_direction; // "forward" or "backward"
};

// Detect timestamp wraparound and produce unwrapped monotonic value
class TimestampWrapDetector {
public:
    TimestampWrapDetector() = default;

    TimestampWrapResult process(int64_t raw_ts) {
        TimestampWrapResult r;
        r.unwrapped_value = raw_ts;
        if (!initialized_) {
            last_raw_ = raw_ts;
            initialized_ = true;
            return r;
        }
        int64_t diff = raw_ts - last_raw_;
        // Detect forward wrap (e.g. 32-bit overflow)
        if (diff < -k_wrap_threshold_) {
            r.wrapped = true;
            r.wrap_direction = "forward";
            ++wrap_count_;
            r.wrap_count = wrap_count_;
            r.unwrapped_value = raw_ts + static_cast<int64_t>(wrap_count_) * k_wrap_cycle_;
        } else if (diff > k_wrap_threshold_) {
            r.wrapped = true;
            r.wrap_direction = "backward";
            r.unwrapped_value = raw_ts - static_cast<int64_t>(wrap_count_) * k_wrap_cycle_;
        } else {
            r.wrap_count = wrap_count_;
        }
        last_raw_ = raw_ts;
        return r;
    }

    uint32_t total_wraps() const { return wrap_count_; }
    bool is_initialized() const { return initialized_; }

private:
    static constexpr int64_t k_wrap_threshold_ = (static_cast<int64_t>(1) << 31); // 2^31
    static constexpr int64_t k_wrap_cycle_ = (static_cast<int64_t>(1) << 32);     // 2^32
    int64_t last_raw_{0};
    uint32_t wrap_count_{0};
    bool initialized_{false};
};

}  // namespace kivo::video_plane::timing
