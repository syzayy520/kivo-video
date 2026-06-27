// avio_bridge.hpp — P5C C2: AVIO bridge copy-to-pool contract
#pragma once
#include <cstdint>
#include <string>
#include "../input_bridge/ffmpeg_copy_to_pool_contract.hpp"
#include "../input_bridge/demux_buffer_pool.hpp"

namespace kivo::video_plane::ffmpeg_demux {

// AVIO bridge: copies FFmpeg AVIO data into DemuxBufferPool
// This is a contract/fake — no real FFmpeg AVIO dependency
struct AvioBridgeConfig {
    input_bridge::FFmpegCopyToPoolContract copy_contract;
    uint64_t default_buffer_size{64 * 1024};  // 64 KiB default
    bool operator==(AvioBridgeConfig const&) const = default;
};

class FakeAvioBridge {
public:
    explicit FakeAvioBridge(AvioBridgeConfig const& config)
        : config_(config) {}

    // Copy data from AVIO into pool
    bool copy_to_pool(input_bridge::DemuxBufferPool& pool, uint64_t data_size) {
        if (!input_bridge::validate_packet_against_contract(
                config_.copy_contract, data_size)) {
            return false;  // oversized or invalid
        }
        auto handle = pool.allocate(data_size);
        return handle.has_value();
    }

    AvioBridgeConfig const& config() const { return config_; }

private:
    AvioBridgeConfig config_;
};

}  // namespace kivo::video_plane::ffmpeg_demux
