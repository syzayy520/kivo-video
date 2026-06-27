// fake_render_path.hpp -- P5F F2: fake render path
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::gpu_render {

struct RenderFrame {
    uint64_t frame_id{0};
    int64_t pts_ms{0};
    uint32_t width{0};
    uint32_t height{0};
    bool presented{false};
};

class FakeRenderPath {
public:
    bool initialize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        initialized_ = true;
        return true;
    }

    bool submit_frame(uint64_t frame_id, int64_t pts_ms) {
        if (!initialized_) return false;
        RenderFrame f;
        f.frame_id = frame_id;
        f.pts_ms = pts_ms;
        f.width = width_;
        f.height = height_;
        f.presented = true;
        frames_.push_back(f);
        return true;
    }

    bool present() {
        if (frames_.empty()) return false;
        presented_count_++;
        return true;
    }

    uint32_t presented_count() const { return presented_count_; }
    size_t submitted_count() const { return frames_.size(); }
    bool is_initialized() const { return initialized_; }

    std::vector<RenderFrame> const& frames() const { return frames_; }

private:
    bool initialized_{false};
    uint32_t width_{0};
    uint32_t height_{0};
    uint32_t presented_count_{0};
    std::vector<RenderFrame> frames_;
};

}  // namespace kivo::video_plane::gpu_render
