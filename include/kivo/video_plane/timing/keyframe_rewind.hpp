// keyframe_rewind.hpp -- P5E E7: fallback rewind to keyframe
#pragma once
#include <cstdint>
#include <vector>
#include <optional>

namespace kivo::video_plane::timing {

struct KeyframeEntry {
    uint64_t frame_index{0};
    int64_t pts_ms{0};
};

class FakeKeyframeIndex {
public:
    void add_keyframe(uint64_t frame_index, int64_t pts_ms) {
        keyframes_.push_back({frame_index, pts_ms});
    }

    // Find the nearest keyframe at or before target_pts (rewind)
    std::optional<KeyframeEntry> rewind_to_keyframe(int64_t target_pts) const {
        if (keyframes_.empty()) return std::nullopt;
        const KeyframeEntry* best = nullptr;
        for (auto const& kf : keyframes_) {
            if (kf.pts_ms <= target_pts) {
                best = &kf;
            } else {
                break;
            }
        }
        if (best) return *best;
        // No keyframe before target -- rewind to first keyframe
        return keyframes_.front();
    }

    // Find the nearest keyframe at or after target_pts (fast-forward)
    std::optional<KeyframeEntry> forward_to_keyframe(int64_t target_pts) const {
        for (auto const& kf : keyframes_) {
            if (kf.pts_ms >= target_pts) return kf;
        }
        return std::nullopt;
    }

    size_t keyframe_count() const { return keyframes_.size(); }
    std::vector<KeyframeEntry> const& keyframes() const { return keyframes_; }

private:
    std::vector<KeyframeEntry> keyframes_;
};

}  // namespace kivo::video_plane::timing
