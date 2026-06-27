// vfr_timing.hpp -- P5E E4: VFR (variable frame rate) timing
#pragma once
#include <cstdint>
#include <vector>
#include <optional>

namespace kivo::video_plane::timing {

struct VfrFrameTiming {
    uint64_t frame_index{0};
    int64_t pts_ms{0};
    int64_t duration_ms{0};       // 0 = unknown (use next pts - this pts)
    bool is_vfr{false};           // this frame has non-uniform timing
};

class FakeVfrTimingEngine {
public:
    void add_frame(uint64_t index, int64_t pts_ms, int64_t duration_ms = 0) {
        frames_.push_back({index, pts_ms, duration_ms, false});
    }

    // Compute inter-frame durations and flag VFR frames
    void compute_durations() {
        for (size_t i = 0; i < frames_.size(); ++i) {
            if (i + 1 < frames_.size()) {
                int64_t interval = frames_[i + 1].pts_ms - frames_[i].pts_ms;
                if (frames_[i].duration_ms == 0) {
                    frames_[i].duration_ms = interval;
                }
            }
        }
        // Detect VFR: if durations differ, mark as VFR
        if (frames_.size() < 2) return;
        int64_t base_duration = frames_[0].duration_ms;
        for (auto& f : frames_) {
            if (f.duration_ms != base_duration && f.duration_ms > 0) {
                f.is_vfr = true;
            }
        }
    }

    std::optional<VfrFrameTiming> get_frame(uint64_t index) const {
        for (auto const& f : frames_) {
            if (f.frame_index == index) return f;
        }
        return std::nullopt;
    }

    bool has_vfr() const {
        for (auto const& f : frames_) {
            if (f.is_vfr) return true;
        }
        return false;
    }

    size_t frame_count() const { return frames_.size(); }

private:
    std::vector<VfrFrameTiming> frames_;
};

}  // namespace kivo::video_plane::timing
