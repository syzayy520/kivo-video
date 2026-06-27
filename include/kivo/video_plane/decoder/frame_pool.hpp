// frame_pool.hpp — P5D D4: FramePool recycle
#pragma once
#include <cstdint>
#include <vector>

namespace kivo::video_plane::decoder {

struct FramePoolConfig {
    uint32_t max_frames{16};
    uint64_t frame_bytes{0};  // 0 = variable
    bool recycle_on_release{true};
    bool operator==(FramePoolConfig const&) const = default;
};

class FakeFramePool {
public:
    explicit FakeFramePool(FramePoolConfig const& config)
        : config_(config) {}

    uint64_t acquire() {
        // Try recycled first
        if (!recycled_.empty() && config_.recycle_on_release) {
            uint64_t h = recycled_.back();
            recycled_.pop_back();
            in_use_.push_back(h);
            return h;
        }
        if (in_use_.size() >= config_.max_frames) return 0;  // pool exhausted
        uint64_t h = ++next_handle_;
        in_use_.push_back(h);
        return h;
    }

    bool release(uint64_t handle) {
        for (auto it = in_use_.begin(); it != in_use_.end(); ++it) {
            if (*it == handle) {
                in_use_.erase(it);
                if (config_.recycle_on_release) {
                    recycled_.push_back(handle);
                }
                return true;
            }
        }
        return false;
    }

    uint32_t in_use_count() const { return static_cast<uint32_t>(in_use_.size()); }
    uint32_t recycled_count() const { return static_cast<uint32_t>(recycled_.size()); }
    bool is_exhausted() const { return in_use_.size() >= config_.max_frames; }

private:
    FramePoolConfig config_;
    std::vector<uint64_t> in_use_;
    std::vector<uint64_t> recycled_;
    uint64_t next_handle_{0};
};

}  // namespace kivo::video_plane::decoder
