#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/video/playback_graph/subtitle/subtitle_frame_snapshot.hpp"

namespace kivo::video::playback_graph::runtime {

struct EmbeddedSubtitlePacketSubmission {
    std::int64_t start_time_us{0};
    std::int64_t end_time_us{0};
    bool key_packet{false};
    std::string codec_name;
    std::vector<std::uint8_t> data;
};

struct SubtitleFrameP8BridgeStats {
    bool embedded_track_attached{false};
    bool embedded_packet_ingest_connected{false};
    bool pgs_bitmap_render_connected{false};
    std::uint64_t submitted_packet_count{0};
    std::uint64_t rejected_packet_count{0};
    std::uint64_t unsupported_feature_count{0};
};

class SubtitleFrameP8Bridge {
public:
    void attach(std::uint64_t track_id, bool allow_contract_fixture_source) noexcept;
    void detach() noexcept;
    void reset() noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] SubtitleFrameP8BridgeStats stats() const noexcept;
    [[nodiscard]] bool submit_embedded_packet(
        const EmbeddedSubtitlePacketSubmission& packet) noexcept;

    [[nodiscard]] SubtitleFrameSnapshot query(std::int64_t media_time_ms,
                                              std::int64_t delay_ms,
                                              std::uint64_t track_id,
                                              bool subtitle_enabled) const noexcept;

private:
    [[nodiscard]] bool ensure_api() noexcept;

    mutable bool api_ready_{false};
    mutable std::uint64_t plane_value_{0};
    std::uint64_t track_id_{0};
    SubtitleFrameP8BridgeStats stats_{};
};

}  // namespace kivo::video::playback_graph::runtime
