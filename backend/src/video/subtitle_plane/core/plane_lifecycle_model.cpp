#include "video/subtitle_plane/core/plane_lifecycle_model.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitleTimelineAvailability timeline_for_plane_state(
    const KivoSubtitlePlaneState state) noexcept {
    switch (state) {
        case KivoSubtitlePlaneState_Created:
        case KivoSubtitlePlaneState_Discovering:
        case KivoSubtitlePlaneState_OpeningSource:
            return KivoSubtitleTimelineAvailability_NotYetStarted;
        case KivoSubtitlePlaneState_Reading:
        case KivoSubtitlePlaneState_Parsing:
        case KivoSubtitlePlaneState_Indexing:
        case KivoSubtitlePlaneState_Priming:
            return KivoSubtitleTimelineAvailability_Buffering;
        case KivoSubtitlePlaneState_Ready:
        case KivoSubtitlePlaneState_Active:
        case KivoSubtitlePlaneState_Paused:
        case KivoSubtitlePlaneState_EosReached:
            return KivoSubtitleTimelineAvailability_Available;
        case KivoSubtitlePlaneState_Seeking:
        case KivoSubtitlePlaneState_Flushing:
        case KivoSubtitlePlaneState_TrackSwitching:
            return KivoSubtitleTimelineAvailability_Seeking;
        case KivoSubtitlePlaneState_TimelineUnavailable:
            return KivoSubtitleTimelineAvailability_TemporarilyUnavailable;
        case KivoSubtitlePlaneState_TimelineNotStarted:
            return KivoSubtitleTimelineAvailability_NotYetStarted;
        case KivoSubtitlePlaneState_Recovering:
            return KivoSubtitleTimelineAvailability_Recovering;
        case KivoSubtitlePlaneState_Closed:
        case KivoSubtitlePlaneState_Closing:
            return KivoSubtitleTimelineAvailability_Closed;
        case KivoSubtitlePlaneState_Failed:
            return KivoSubtitleTimelineAvailability_Failed;
        default:
            return KivoSubtitleTimelineAvailability_Failed;
    }
}

bool plane_state_allows_frame_build(const KivoSubtitlePlaneState state) noexcept {
    return state == KivoSubtitlePlaneState_Ready ||
           state == KivoSubtitlePlaneState_Active ||
           state == KivoSubtitlePlaneState_Paused ||
           state == KivoSubtitlePlaneState_EosReached;
}

}  // namespace kivo::video::subtitle_plane::runtime
