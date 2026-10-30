#include "video/video_plane/backends/mediacodec/session/surface_rebuild_tracker.hpp"

namespace kivo::video::mediacodec {

void SurfaceRebuildTracker::note_detach(std::int64_t mono_ns) noexcept {
    detach_at_mono_ns_.store(mono_ns, std::memory_order_relaxed);
    first_frame_mono_ns_.store(-1, std::memory_order_relaxed);
    pending_frames_.store(0, std::memory_order_relaxed);
    // release 语义：收帧线程看到 kDetached 之后就不再计数（此时没有面，交出去的帧不算恢复帧）。
    phase_.store(kDetached, std::memory_order_release);
}

void SurfaceRebuildTracker::note_rebind(std::int64_t mono_ns) noexcept {
    const std::int64_t detach_ns = detach_at_mono_ns_.load(std::memory_order_relaxed);
    if (detach_ns >= 0) {
        last_gap_ms_.store((mono_ns - detach_ns) / 1'000'000, std::memory_order_relaxed);
    }
    rebind_at_mono_ns_.store(mono_ns, std::memory_order_relaxed);
    first_frame_mono_ns_.store(-1, std::memory_order_relaxed);
    pending_frames_.store(0, std::memory_order_relaxed);
    rebinds_.fetch_add(1, std::memory_order_relaxed);
    phase_.store(kAwaitingFirstFrame, std::memory_order_release);
}

void SurfaceRebuildTracker::note_release(std::int64_t mono_ns, bool rendered) noexcept {
    // 常态是 kIdle：一次 acquire 读就返回，不进任何分支。
    if (phase_.load(std::memory_order_acquire) != kAwaitingFirstFrame) {
        return;
    }
    if (rendered) {
        std::int64_t expected = -1;
        if (first_frame_mono_ns_.compare_exchange_strong(expected, mono_ns,
                                                        std::memory_order_acq_rel)) {
            const std::int64_t rebind_ns = rebind_at_mono_ns_.load(std::memory_order_relaxed);
            if (rebind_ns >= 0) {
                last_resume_ms_.store((mono_ns - rebind_ns) / 1'000'000,
                                      std::memory_order_relaxed);
            }
            last_frames_lead_.store(pending_frames_.load(std::memory_order_relaxed),
                                    std::memory_order_relaxed);
            phase_.store(kIdle, std::memory_order_release);
        }
        return;
    }
    // 没有面可上（或被按节拍丢掉）：这一帧没让画面回来，计入"领先帧数"。
    pending_frames_.fetch_add(1, std::memory_order_relaxed);
}

SurfaceRebuildStats SurfaceRebuildTracker::snapshot() const noexcept {
    SurfaceRebuildStats out;
    out.rebinds = rebinds_.load(std::memory_order_relaxed);
    out.last_gap_ms = last_gap_ms_.load(std::memory_order_relaxed);
    out.last_resume_ms = last_resume_ms_.load(std::memory_order_relaxed);
    out.last_frames_lead = last_frames_lead_.load(std::memory_order_relaxed);
    return out;
}

}  // namespace kivo::video::mediacodec
