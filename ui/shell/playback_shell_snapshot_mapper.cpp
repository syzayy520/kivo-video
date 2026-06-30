#include "playback_shell_snapshot_mapper.hpp"

#include <QVariantList>

namespace kivo::ui::shell {
namespace {

QVariantList map_buffered_ranges(
    const std::vector<kivo::video::player_runtime_adapter::AdapterTimeRange>& ranges) {
    QVariantList mapped;
    mapped.reserve(static_cast<int>(ranges.size()));
    for (const auto& range : ranges) {
        mapped.push_back(QVariantMap{
            {"startMs", range.start_ms},
            {"endMs", range.end_ms},
            {"start_ms", range.start_ms},
            {"end_ms", range.end_ms},
        });
    }
    return mapped;
}

}  // namespace

QVariantMap map_adapter_snapshot(
    const kivo::video::player_runtime_adapter::AdapterSnapshot& snapshot) {
    return QVariantMap{
        {"positionMs", snapshot.position_ms},
        {"durationMs", snapshot.duration_ms},
        {"position_ms", snapshot.position_ms},
        {"duration_ms", snapshot.duration_ms},
        {"isPlaying", snapshot.is_playing},
        {"isPaused", snapshot.is_paused},
        {"is_playing", snapshot.is_playing},
        {"is_paused", snapshot.is_paused},
        {"isBuffering", snapshot.is_buffering},
        {"is_buffering", snapshot.is_buffering},
        {"isRecovering", false},
        {"is_recovering", false},
        {"hasError", snapshot.playback_state ==
                         kivo::video::player_runtime_adapter::RuntimePlaybackState::Failed},
        {"closed", snapshot.closed},
        {"volume", snapshot.volume},
        {"muted", snapshot.muted},
        {"subtitlePrimary", QString::fromUtf8(snapshot.subtitle_primary)},
        {"subtitle_primary", QString::fromUtf8(snapshot.subtitle_primary)},
        {"subtitleSecondary", QString::fromUtf8(snapshot.subtitle_secondary)},
        {"subtitle_secondary", QString::fromUtf8(snapshot.subtitle_secondary)},
        {"subtitleEnabled", snapshot.subtitle_enabled},
        {"subtitle_enabled", snapshot.subtitle_enabled},
        {"subtitleDelayMs", snapshot.subtitle_delay_ms},
        {"subtitle_delay_ms", snapshot.subtitle_delay_ms},
        {"audioDelayMs", snapshot.audio_delay_ms},
        {"audio_delay_ms", snapshot.audio_delay_ms},
        {"bufferedRanges", map_buffered_ranges(snapshot.buffered_ranges)},
        {"videoReady", snapshot.video_ready},
        {"video_ready", snapshot.video_ready},
        {"videoWidth", snapshot.video_size.width},
        {"videoHeight", snapshot.video_size.height},
        {"video_width", snapshot.video_size.width},
        {"video_height", snapshot.video_size.height},
    };
}

}  // namespace kivo::ui::shell