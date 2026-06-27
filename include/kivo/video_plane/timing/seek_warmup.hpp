// seek_warmup.hpp -- P5E E6: seek warmup discard
#pragma once
#include <cstdint>
#include <vector>

namespace kivo::video_plane::timing {

enum class WarmupState {
    Idle,
    WarmupActive,
    WarmupComplete
};

struct SeekWarmupConfig {
    uint32_t max_discard_frames{8};     // max frames to discard after seek
    int64_t warmup_timeout_ms{500};     // timeout for warmup phase
    bool discard_until_keyframe{true};  // discard until first keyframe after seek
};

class FakeSeekWarmup {
public:
    explicit FakeSeekWarmup(SeekWarmupConfig cfg = {}) : config_(cfg) {}

    void start_seek() {
        state_ = WarmupState::WarmupActive;
        discarded_count_ = 0;
        keyframe_seen_ = false;
    }

    // Returns true if frame should be discarded during warmup
    bool should_discard(uint64_t frame_index, bool is_keyframe) {
        if (state_ != WarmupState::WarmupActive) return false;
        if (config_.discard_until_keyframe && !keyframe_seen_) {
            if (!is_keyframe) {
                ++discarded_count_;
                if (discarded_count_ >= config_.max_discard_frames) {
                    state_ = WarmupState::WarmupComplete;
                }
                return true; // discard non-keyframe before first keyframe
            }
            keyframe_seen_ = true;
        }
        if (discarded_count_ >= config_.max_discard_frames) {
            state_ = WarmupState::WarmupComplete;
            return false;
        }
        ++discarded_count_;
        if (discarded_count_ >= config_.max_discard_frames) {
            state_ = WarmupState::WarmupComplete;
        }
        return true;
    }

    void complete() { state_ = WarmupState::WarmupComplete; }

    WarmupState state() const { return state_; }
    uint32_t discarded_count() const { return discarded_count_; }
    bool keyframe_seen() const { return keyframe_seen_; }

private:
    SeekWarmupConfig config_;
    WarmupState state_{WarmupState::Idle};
    uint32_t discarded_count_{0};
    bool keyframe_seen_{false};
};

}  // namespace kivo::video_plane::timing
