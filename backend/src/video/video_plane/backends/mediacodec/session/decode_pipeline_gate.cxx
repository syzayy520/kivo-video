#include "video/video_plane/backends/mediacodec/session/decode_pipeline_gate.hpp"

#include <chrono>

namespace kivo::video::mediacodec {

void DecodePipelineGate::request_stop() noexcept {
    stop_.store(true, std::memory_order_release);
    // 不带谓词的 notify_all：等待方自己看停止标志与超时，这里只负责把人叫醒。
    room_cv_.notify_all();
}

bool DecodePipelineGate::wait_for_input_room(std::int64_t timeout_us) noexcept {
    if (stop_.load(std::memory_order_acquire)) {
        return false;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    // 用 wait_for 而不是 wait：通知可能早于等待到达（送包先 TRY_AGAIN、
    // 收帧后 release 这一对在时序上完全可以反过来），丢一次通知就死等是最难查的一类 bug。
    room_cv_.wait_for(lock, std::chrono::microseconds(timeout_us),
                      [this] { return stop_.load(std::memory_order_acquire); });
    return !stop_.load(std::memory_order_acquire);
}

void DecodePipelineGate::signal_input_room() noexcept {
    room_cv_.notify_all();
}

void DecodePipelineGate::note_submit_finished() noexcept {
    submit_finished_.store(true, std::memory_order_release);
}

void DecodePipelineGate::reset_for_start() noexcept {
    stop_.store(false, std::memory_order_release);
    submit_finished_.store(false, std::memory_order_release);
}

}  // namespace kivo::video::mediacodec
