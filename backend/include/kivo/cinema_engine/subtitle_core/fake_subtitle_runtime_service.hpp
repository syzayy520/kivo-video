#pragma once

#include "kivo/cinema_engine/subtitle_core/subtitle_track_info.hpp"
#include "kivo/cinema_engine/subtitle_core/subtitle_entry.hpp"
#include "kivo/cinema_engine/subtitle_core/subtitle_load_result.hpp"

namespace kivo::cinema_engine {

// Subtitle runtime state.
enum class SubtitleRuntimeState {
    Unloaded,
    Loaded,
    Active,
    Hidden,
};

// Fake subtitle runtime service for P2-039 testing.
class FakeSubtitleRuntimeService {
public:
    // Track info builders
    static SubtitleTrackInfo build_srt_track();
    static SubtitleTrackInfo build_ass_track();

    // Entry builders
    static SubtitleEntry build_srt_entry();
    static SubtitleEntry build_ass_entry();

    // Load result builders
    static SubtitleLoadResult build_successful_srt_load();
    static SubtitleLoadResult build_successful_ass_load();
    static SubtitleLoadResult build_failed_load();

    // State builders
    static SubtitleRuntimeState build_unloaded_state();
    static SubtitleRuntimeState build_active_state();
};

}  // namespace kivo::cinema_engine
