#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/subtitle/subtitle_frame_snapshot.hpp"

namespace kivo::video::playback_graph::runtime {

class SubtitleFrameP8Bridge {
public:
    void attach(std::uint64_t track_id) noexcept;
    void detach() noexcept;
    void reset() noexcept;

    [[nodiscard]] bool active() const noexcept;

    [[nodiscard]] SubtitleFrameSnapshot query(std::int64_t media_time_ms,
                                              std::int64_t delay_ms,
                                              std::uint64_t track_id,
                                              bool subtitle_enabled) const noexcept;

private:
    [[nodiscard]] bool ensure_api() noexcept;

    mutable bool api_ready_{false};
    mutable std::uint64_t plane_value_{0};
    std::uint64_t track_id_{0};
};

}  // namespace kivo::video::playback_graph::runtime