// bframe_reorder.hpp -- P5E E5: B-frame reorder
#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>

namespace kivo::video_plane::timing {

struct ReorderFrame {
    uint64_t decode_order{0};
    uint64_t presentation_order{0};
    int64_t pts_ms{0};
    bool is_keyframe{false};
};

class FakeBFrameReorderer {
public:
    void set_max_depth(uint32_t depth) { max_depth_ = depth; }

    // Push a frame in decode order; internally reorders to presentation order
    void push(ReorderFrame f) {
        buffer_.push_back(f);
        if (buffer_.size() > max_depth_) {
            flush_one();
        }
    }

    // Flush all remaining frames in presentation order
    std::vector<ReorderFrame> flush_all() {
        std::sort(buffer_.begin(), buffer_.end(),
            [](ReorderFrame const& a, ReorderFrame const& b) {
                return a.presentation_order < b.presentation_order;
            });
        std::vector<ReorderFrame> result = std::move(buffer_);
        buffer_.clear();
        return result;
    }

    // Get output count so far
    size_t output_count() const { return output_.size(); }

    // Get all output frames
    std::vector<ReorderFrame> const& output() const { return output_; }

private:
    void flush_one() {
        if (buffer_.empty()) return;
        auto it = std::min_element(buffer_.begin(), buffer_.end(),
            [](ReorderFrame const& a, ReorderFrame const& b) {
                return a.presentation_order < b.presentation_order;
            });
        output_.push_back(*it);
        buffer_.erase(it);
    }

    uint32_t max_depth_{16};
    std::vector<ReorderFrame> buffer_;
    std::vector<ReorderFrame> output_;
};

}  // namespace kivo::video_plane::timing
