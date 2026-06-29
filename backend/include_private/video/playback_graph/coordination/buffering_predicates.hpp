#pragma once

#include <cstdint>

namespace kivo::video::playback_graph::runtime {

struct BufferingInputs {
    bool audio_selected{true};
    bool video_selected{true};
    bool audio_ready{false};
    bool video_ready{false};
    bool live_source{false};
    bool source_explicitly_terminated{false};
    std::uint32_t underflow_duration_ms{0};
    std::uint32_t stalled_timeout_ms{3000};
    std::uint32_t debounce_ms{100};
};

[[nodiscard]] bool buffering_ready(const BufferingInputs& inputs) noexcept;
[[nodiscard]] bool rebuffering_needed(const BufferingInputs& inputs) noexcept;
[[nodiscard]] bool stalled_predicate(const BufferingInputs& inputs) noexcept;

}  // namespace kivo::video::playback_graph::runtime
