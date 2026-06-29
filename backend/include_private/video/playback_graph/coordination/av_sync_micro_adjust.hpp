#pragma once

#include <cstdint>

namespace kivo::video::playback_graph::runtime {

enum class ClockMaster {
    Audio,
    Video,
    External
};

struct AvSyncInputs {
    bool audio_selected{true};
    bool video_selected{true};
    bool live_source{false};
    bool p6_micro_adjust_capability_known{true};
    bool p6_micro_adjust_supported{true};
    std::int32_t drift_ms{0};
    std::uint32_t drift_threshold_ms{40};
};

struct AvSyncDecision {
    ClockMaster master{ClockMaster::Audio};
    bool micro_adjust_allowed{false};
    bool capability_refreshed{false};
    bool capability_changed{false};
    bool rollback_required{false};
};

class AvSyncMicroAdjust {
public:
    [[nodiscard]] AvSyncDecision decide(const AvSyncInputs& inputs,
                                        bool previous_capability_supported) noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
