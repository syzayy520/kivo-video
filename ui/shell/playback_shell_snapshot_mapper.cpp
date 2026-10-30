#include "playback_shell_snapshot_mapper.hpp"

#include <QString>
#include <QVariant>
#include <QVariantList>

#include <cstdint>

namespace kivo::ui::shell {
namespace {

using kivo::video::player_runtime_adapter::AdapterAspectMode;
using kivo::video::player_runtime_adapter::BufferReason;
using kivo::video::player_runtime_adapter::RuntimePlaybackState;
using kivo::video::player_runtime_adapter::AdapterToneMappingMode;

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

QVariantList map_track_options(const std::vector<std::uint64_t>& track_ids,
                               const QString& label_prefix) {
    QVariantList mapped;
    mapped.reserve(static_cast<int>(track_ids.size()));
    for (std::size_t index = 0; index < track_ids.size(); ++index) {
        const auto track_id = track_ids[index];
        const QString label =
            QStringLiteral("%1 %2").arg(label_prefix).arg(static_cast<qulonglong>(index + 1));
        mapped.push_back(QVariantMap{
            {"trackId", QVariant::fromValue<qulonglong>(track_id)},
            {"id", QVariant::fromValue<qulonglong>(track_id)},
            {"label", label},
            {"name", label},
        });
    }
    return mapped;
}

QVariantList map_playlist_items(
    const std::vector<kivo::video::player_runtime_adapter::AdapterPlaylistItem>& items) {
    QVariantList mapped;
    mapped.reserve(static_cast<int>(items.size()));
    for (const auto& item : items) {
        mapped.push_back(QVariantMap{
            {"episodeId", QString::number(item.index + 1)},
            {"itemIndex", QVariant::fromValue<qulonglong>(item.index)},
            {"name", QString::fromStdString(item.label)},
            {"durationText", QString{}},
            {"progress", 0.0},
            {"current", item.current},
        });
    }
    return mapped;
}

QString map_aspect_mode(const AdapterAspectMode mode) {
    switch (mode) {
        case AdapterAspectMode::Fill:
            return QStringLiteral("fill");
        case AdapterAspectMode::Stretch:
            return QStringLiteral("stretch");
        case AdapterAspectMode::Original:
            return QStringLiteral("original");
        case AdapterAspectMode::Fit:
            return QStringLiteral("fit");
    }
    return QStringLiteral("fit");
}

QString map_tone_mapping_mode(const AdapterToneMappingMode mode) {
    switch (mode) {
        case AdapterToneMappingMode::Off:
            return QStringLiteral("off");
        case AdapterToneMappingMode::Sdr:
            return QStringLiteral("sdr");
        case AdapterToneMappingMode::HdrPassthrough:
            return QStringLiteral("hdr");
        case AdapterToneMappingMode::Auto:
            return QStringLiteral("auto");
    }
    return QStringLiteral("auto");
}

QString map_buffer_reason(const BufferReason reason) {
    switch (reason) {
        case BufferReason::Opening:
            return QStringLiteral("opening");
        case BufferReason::InitialBuffer:
            return QStringLiteral("initial");
        case BufferReason::Rebuffer:
            return QStringLiteral("rebuffer");
        case BufferReason::Stalled:
            return QStringLiteral("stalled");
        case BufferReason::Seeking:
            return QStringLiteral("seeking");
        case BufferReason::None:
            return QStringLiteral("none");
    }
    return QStringLiteral("none");
}

QString map_playback_state(const RuntimePlaybackState state) {
    switch (state) {
        case RuntimePlaybackState::NotCreated:
            return QStringLiteral("NotCreated");
        case RuntimePlaybackState::Created:
            return QStringLiteral("Created");
        case RuntimePlaybackState::Opening:
            return QStringLiteral("Opening");
        case RuntimePlaybackState::Ready:
            return QStringLiteral("Ready");
        case RuntimePlaybackState::Starting:
            return QStringLiteral("Starting");
        case RuntimePlaybackState::Buffering:
            return QStringLiteral("Buffering");
        case RuntimePlaybackState::Playing:
            return QStringLiteral("Playing");
        case RuntimePlaybackState::Rebuffering:
            return QStringLiteral("Rebuffering");
        case RuntimePlaybackState::Stalled:
            return QStringLiteral("Stalled");
        case RuntimePlaybackState::Pausing:
            return QStringLiteral("Pausing");
        case RuntimePlaybackState::Paused:
            return QStringLiteral("Paused");
        case RuntimePlaybackState::Resuming:
            return QStringLiteral("Resuming");
        case RuntimePlaybackState::Seeking:
            return QStringLiteral("Seeking");
        case RuntimePlaybackState::TrackSwitching:
            return QStringLiteral("TrackSwitching");
        case RuntimePlaybackState::EosReached:
            return QStringLiteral("EosReached");
        case RuntimePlaybackState::Recovering:
            return QStringLiteral("Recovering");
        case RuntimePlaybackState::Failed:
            return QStringLiteral("Failed");
        case RuntimePlaybackState::Closing:
            return QStringLiteral("Closing");
        case RuntimePlaybackState::Closed:
            return QStringLiteral("Closed");
    }
    return QStringLiteral("Failed");
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
        {"isOpening", snapshot.is_opening},
        {"is_opening", snapshot.is_opening},
        {"bufferReason", map_buffer_reason(snapshot.buffer_reason)},
        {"buffer_reason", map_buffer_reason(snapshot.buffer_reason)},
        {"rebufferCount", snapshot.rebuffer_count},
        {"rebuffer_count", snapshot.rebuffer_count},
        {"playbackState", map_playback_state(snapshot.playback_state)},
        {"playback_state", map_playback_state(snapshot.playback_state)},
        {"isRecovering", false},
        {"is_recovering", false},
        {"hasError", snapshot.playback_state ==
                         kivo::video::player_runtime_adapter::RuntimePlaybackState::Failed},
        {"closed", snapshot.closed},
        {"volume", snapshot.volume},
        {"muted", snapshot.muted},
        {"fullscreenLike", snapshot.fullscreen_like},
        {"fullscreen_like", snapshot.fullscreen_like},
        {"screenIndex", snapshot.screen_index},
        {"screen_index", snapshot.screen_index},
        {"screenIndexValid", snapshot.screen_index_valid},
        {"screen_index_valid", snapshot.screen_index_valid},
        {"autoPlayNext", snapshot.auto_play_next},
        {"auto_play_next", snapshot.auto_play_next},
        {"playlistQueueConnected", snapshot.playlist_queue_connected},
        {"playlist_queue_connected", snapshot.playlist_queue_connected},
        {"playlistHasPrevious", snapshot.playlist_has_previous},
        {"playlist_has_previous", snapshot.playlist_has_previous},
        {"playlistHasNext", snapshot.playlist_has_next},
        {"playlist_has_next", snapshot.playlist_has_next},
        {"playlistHasNextEpisode", snapshot.playlist_has_next_episode},
        {"playlist_has_next_episode", snapshot.playlist_has_next_episode},
        {"playlistCurrentIndex", QVariant::fromValue<qulonglong>(snapshot.playlist_current_index)},
        {"playlist_current_index", QVariant::fromValue<qulonglong>(snapshot.playlist_current_index)},
        {"playlistItemCount", QVariant::fromValue<qulonglong>(snapshot.playlist_item_count)},
        {"playlist_item_count", QVariant::fromValue<qulonglong>(snapshot.playlist_item_count)},
        {"episodeOptions", map_playlist_items(snapshot.playlist_items)},
        {"episode_options", map_playlist_items(snapshot.playlist_items)},
        {"currentEpisode", QString::number(snapshot.playlist_current_index + 1)},
        {"current_episode", QString::number(snapshot.playlist_current_index + 1)},
        {"subtitlePrimary", QString::fromUtf8(snapshot.subtitle_primary)},
        {"subtitle_primary", QString::fromUtf8(snapshot.subtitle_primary)},
        {"subtitleSecondary", QString::fromUtf8(snapshot.subtitle_secondary)},
        {"subtitle_secondary", QString::fromUtf8(snapshot.subtitle_secondary)},
        {"subtitleEnabled", snapshot.subtitle_enabled},
        {"subtitle_enabled", snapshot.subtitle_enabled},
        {"subtitleFrameAvailable", snapshot.subtitle_frame_available},
        {"subtitle_frame_available", snapshot.subtitle_frame_available},
        {"subtitleFrameRuntimeConnected",
         snapshot.connections.subtitle_frame ==
             kivo::video::player_runtime_adapter::AdapterConnectionStatus::ConnectedToP7},
        {"subtitle_frame_runtime_connected",
         snapshot.connections.subtitle_frame ==
             kivo::video::player_runtime_adapter::AdapterConnectionStatus::ConnectedToP7},
        {"selectedSubtitleTrack", QVariant::fromValue<qulonglong>(snapshot.selected_subtitle_track)},
        {"selected_subtitle_track", QVariant::fromValue<qulonglong>(snapshot.selected_subtitle_track)},
        {"subtitleOptions", map_track_options(snapshot.subtitle_track_ids,
                                              QStringLiteral("Subtitle"))},
        {"subtitle_options", map_track_options(snapshot.subtitle_track_ids,
                                               QStringLiteral("Subtitle"))},
        {"subtitleDelayMs", snapshot.subtitle_delay_ms},
        {"subtitle_delay_ms", snapshot.subtitle_delay_ms},
        {"selectedAudioTrack", QVariant::fromValue<qulonglong>(snapshot.selected_audio_track)},
        {"selected_audio_track", QVariant::fromValue<qulonglong>(snapshot.selected_audio_track)},
        {"audioTrackOptions", map_track_options(snapshot.audio_track_ids,
                                                QStringLiteral("Audio"))},
        {"audio_track_options", map_track_options(snapshot.audio_track_ids,
                                                  QStringLiteral("Audio"))},
        {"selectedVideoTrack", QVariant::fromValue<qulonglong>(snapshot.selected_video_track)},
        {"selected_video_track", QVariant::fromValue<qulonglong>(snapshot.selected_video_track)},
        {"videoTrackOptions", map_track_options(snapshot.video_track_ids,
                                                QStringLiteral("Video"))},
        {"video_track_options", map_track_options(snapshot.video_track_ids,
                                                  QStringLiteral("Video"))},
        {"audioDelayMs", snapshot.audio_delay_ms},
        {"audio_delay_ms", snapshot.audio_delay_ms},
        {"subtitleSize", snapshot.subtitle_size},
        {"subtitle_size", snapshot.subtitle_size},
        {"playbackSpeed", snapshot.playback_speed},
        {"playback_speed", snapshot.playback_speed},
        {"aspectMode", map_aspect_mode(snapshot.aspect_mode)},
        {"aspect_mode", map_aspect_mode(snapshot.aspect_mode)},
        {"toneMappingMode", map_tone_mapping_mode(snapshot.tone_mapping_mode)},
        {"tone_mapping_mode", map_tone_mapping_mode(snapshot.tone_mapping_mode)},
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
