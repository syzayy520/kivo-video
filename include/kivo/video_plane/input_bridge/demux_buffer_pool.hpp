// demux_buffer_pool.hpp — P5B B3: Demux Buffer Pool with allocation cap
#pragma once
#include <cstdint>
#include <vector>
#include <optional>
#include <unordered_map>
#include "../allocation/allocation_limit_policy.hpp"

namespace kivo::video_plane::input_bridge {

struct BufferPoolConfig {
    uint64_t max_total_bytes{allocation::AllocationDefaults::kMaxSingleMediaAllocation};
    uint64_t max_single_buffer_bytes{16 * 1024 * 1024};  // 16 MiB
    bool enforce_cap{true};
    bool operator==(BufferPoolConfig const&) const = default;
};

struct BufferPoolStats {
    uint64_t total_allocated{0};
    uint64_t total_freed{0};
    uint64_t current_usage{0};
    uint32_t buffer_count{0};
    bool cap_exceeded{false};
};

// DemuxBufferPool — manages demux buffer allocations with cap enforcement
class DemuxBufferPool {
public:
    explicit DemuxBufferPool(BufferPoolConfig const& config)
        : config_(config) {}

    // Allocate buffer, returns nullopt if cap exceeded
    std::optional<uint64_t> allocate(uint64_t bytes) {
        if (config_.enforce_cap) {
            if (bytes > config_.max_single_buffer_bytes) return std::nullopt;
            if (stats_.current_usage + bytes > config_.max_total_bytes) {
                stats_.cap_exceeded = true;
                return std::nullopt;
            }
        }
        uint64_t handle = ++next_handle_;
        stats_.total_allocated += bytes;
        stats_.current_usage += bytes;
        stats_.buffer_count++;
        buffer_sizes_[handle] = bytes;
        return handle;
    }

    // Free buffer
    bool free(uint64_t handle) {
        auto it = buffer_sizes_.find(handle);
        if (it == buffer_sizes_.end()) return false;
        stats_.total_freed += it->second;
        stats_.current_usage -= it->second;
        stats_.buffer_count--;
        buffer_sizes_.erase(it);
        return true;
    }

    BufferPoolStats const& stats() const { return stats_; }
    BufferPoolConfig const& config() const { return config_; }

private:
    BufferPoolConfig config_;
    BufferPoolStats stats_;
    uint64_t next_handle_{0};
    std::unordered_map<uint64_t, uint64_t> buffer_sizes_;
};

}  // namespace kivo::video_plane::input_bridge
