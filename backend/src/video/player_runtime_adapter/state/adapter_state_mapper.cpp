#include "video/player_runtime_adapter/state/adapter_state_mapper.hpp"

namespace kivo::video::player_runtime_adapter::runtime {
namespace {

using playback_graph::PlaybackAspectMode;
using playback_graph::PlaybackDeinterlaceMode;
using playback_graph::PlaybackGraphState;
using playback_graph::PlaybackScaleMode;
using playback_graph::PlaybackToneMappingMode;
using playback_graph::VideoRenderAttachmentState;

[[nodiscard]] AdapterAspectMode map_aspect_mode(PlaybackAspectMode mode) noexcept {
    switch (mode) {
        case PlaybackAspectMode::Fit:
            return AdapterAspectMode::Fit;
        case PlaybackAspectMode::Fill:
            return AdapterAspectMode::Fill;
        case PlaybackAspectMode::Stretch:
            return AdapterAspectMode::Stretch;
        case PlaybackAspectMode::Original:
            return AdapterAspectMode::Original;
    }
    return AdapterAspectMode::Fit;
}

[[nodiscard]] AdapterScaleMode map_scale_mode(PlaybackScaleMode mode) noexcept {
    switch (mode) {
        case PlaybackScaleMode::Auto:
            return AdapterScaleMode::Auto;
        case PlaybackScaleMode::Fit:
            return AdapterScaleMode::Fit;
        case PlaybackScaleMode::Fill:
            return AdapterScaleMode::Fill;
    }
    return AdapterScaleMode::Auto;
}

[[nodiscard]] AdapterToneMappingMode map_tone_mapping_mode(
    PlaybackToneMappingMode mode) noexcept {
    switch (mode) {
        case PlaybackToneMappingMode::Auto:
            return AdapterToneMappingMode::Auto;
        case PlaybackToneMappingMode::Off:
            return AdapterToneMappingMode::Off;
        case PlaybackToneMappingMode::Sdr:
            return AdapterToneMappingMode::Sdr;
        case PlaybackToneMappingMode::HdrPassthrough:
            return AdapterToneMappingMode::HdrPassthrough;
    }
    return AdapterToneMappingMode::Auto;
}

[[nodiscard]] AdapterDeinterlaceMode map_deinterlace_mode(
    PlaybackDeinterlaceMode mode) noexcept {
    switch (mode) {
        case PlaybackDeinterlaceMode::Auto:
            return AdapterDeinterlaceMode::Auto;
        case PlaybackDeinterlaceMode::Off:
            return AdapterDeinterlaceMode::Off;
        case PlaybackDeinterlaceMode::On:
            return AdapterDeinterlaceMode::On;
    }
    return AdapterDeinterlaceMode::Auto;
}

[[nodiscard]] RenderState map_render_state(VideoRenderAttachmentState state) noexcept {
    switch (state) {
        case VideoRenderAttachmentState::SurfaceAttached:
            return RenderState::SurfaceReady;
        case VideoRenderAttachmentState::SurfaceDetached:
        case VideoRenderAttachmentState::SurfaceReleased:
            return RenderState::SurfaceReleased;
        case VideoRenderAttachmentState::NoSurface:
            return RenderState::NoSurface;
    }
    return RenderState::NoSurface;
}

}  // namespace

RuntimePlaybackState map_playback_state(PlaybackGraphState state) noexcept {
    switch (state) {
        case PlaybackGraphState::NotCreated:
            return RuntimePlaybackState::NotCreated;
        case PlaybackGraphState::Created:
            return RuntimePlaybackState::Created;
        case PlaybackGraphState::Building:
            return RuntimePlaybackState::Opening;
        case PlaybackGraphState::Ready:
            return RuntimePlaybackState::Ready;
        case PlaybackGraphState::Starting:
            return RuntimePlaybackState::Starting;
        case PlaybackGraphState::Buffering:
            return RuntimePlaybackState::Buffering;
        case PlaybackGraphState::Playing:
            return RuntimePlaybackState::Playing;
        case PlaybackGraphState::Rebuffering:
            return RuntimePlaybackState::Rebuffering;
        case PlaybackGraphState::Stalled:
            return RuntimePlaybackState::Stalled;
        case PlaybackGraphState::Pausing:
            return RuntimePlaybackState::Pausing;
        case PlaybackGraphState::Paused:
            return RuntimePlaybackState::Paused;
        case PlaybackGraphState::Resuming:
            return RuntimePlaybackState::Resuming;
        case PlaybackGraphState::Seeking:
            return RuntimePlaybackState::Seeking;
        case PlaybackGraphState::TrackSwitching:
            return RuntimePlaybackState::TrackSwitching;
        case PlaybackGraphState::EosReached:
            return RuntimePlaybackState::EosReached;
        case PlaybackGraphState::Recovering:
            return RuntimePlaybackState::Recovering;
        case PlaybackGraphState::BuildFailed:
        case PlaybackGraphState::Failed:
            return RuntimePlaybackState::Failed;
        case PlaybackGraphState::Closing:
            return RuntimePlaybackState::Closing;
        case PlaybackGraphState::Closed:
            return RuntimePlaybackState::Closed;
        case PlaybackGraphState::Flushing:
        case PlaybackGraphState::Draining:
        case PlaybackGraphState::SystemSuspending:
        case PlaybackGraphState::SystemSuspended:
        case PlaybackGraphState::SystemResuming:
            return RuntimePlaybackState::Buffering;
    }

    return RuntimePlaybackState::Failed;
}

BufferReason map_buffer_reason(PlaybackGraphState state) noexcept {
    switch (state) {
        case PlaybackGraphState::Building:
            return BufferReason::Opening;
        case PlaybackGraphState::Buffering:
        case PlaybackGraphState::Starting:
            return BufferReason::InitialBuffer;
        case PlaybackGraphState::Rebuffering:
            return BufferReason::Rebuffer;
        case PlaybackGraphState::Stalled:
            return BufferReason::Stalled;
        case PlaybackGraphState::Seeking:
        case PlaybackGraphState::Flushing:
            return BufferReason::Seeking;
        default:
            return BufferReason::None;
    }
}

AdapterSnapshot map_snapshot(const playback_graph::PlaybackSession& session,
                             std::uint32_t rebuffer_count) noexcept {
    const auto p7_snapshot = session.snapshot();
    const auto timeline = session.query_timeline();
    const auto video_surface = session.query_video_surface();
    const auto diagnostics = session.query_diagnostics_summary();
    const auto subtitle = session.query_subtitle();
    const auto audio_policy = session.query_audio_output_policy();
    const auto settings_policy = session.query_playback_settings_policy();

    AdapterSnapshot snapshot{};
    snapshot.p7_snapshot = p7_snapshot;
    snapshot.playback_state = map_playback_state(p7_snapshot.state);
    snapshot.last_error = p7_snapshot.last_error;
    snapshot.closed = p7_snapshot.closed;
    snapshot.buffer_reason = map_buffer_reason(p7_snapshot.state);
    snapshot.rebuffer_count = rebuffer_count;

    snapshot.is_opening = p7_snapshot.state == PlaybackGraphState::Building;
    snapshot.is_playing = p7_snapshot.state == PlaybackGraphState::Playing;
    snapshot.is_paused = p7_snapshot.state == PlaybackGraphState::Paused;
    snapshot.is_buffering = snapshot.buffer_reason != BufferReason::None &&
                            snapshot.buffer_reason != BufferReason::Opening;

    if (timeline.valid) {
        snapshot.position_ms = timeline.position_ms;
        snapshot.duration_ms = timeline.duration_ms;
        snapshot.connections.timeline = AdapterConnectionStatus::ConnectedToP7;
        if (timeline.buffered_range.end_ms > timeline.buffered_range.start_ms) {
            snapshot.buffered_ranges.push_back(
                AdapterTimeRange{timeline.buffered_range.start_ms, timeline.buffered_range.end_ms});
            snapshot.connections.buffered_ranges = AdapterConnectionStatus::ConnectedToP7;
        }
    }

    if (video_surface.render_state != VideoRenderAttachmentState::NoSurface ||
        video_surface.ready) {
        snapshot.video_ready = video_surface.ready;
        snapshot.video_size.width = video_surface.width;
        snapshot.video_size.height = video_surface.height;
        snapshot.aspect_ratio = video_surface.aspect_ratio;
        snapshot.dropped_frames = video_surface.dropped_frames;
        snapshot.render_state = map_render_state(video_surface.render_state);
        snapshot.connections.video_surface = AdapterConnectionStatus::ConnectedToP7;
        snapshot.connections.video_telemetry = AdapterConnectionStatus::ConnectedToP7;
    }

    if (diagnostics.valid) {
        snapshot.connections.recovery_actions = AdapterConnectionStatus::ConnectedToP7;
    }

    if (subtitle.valid) {
        snapshot.selected_subtitle_track = subtitle.selected_track_id;
        snapshot.subtitle_enabled = subtitle.enabled;
        snapshot.subtitle_delay_ms = subtitle.delay_ms;
        snapshot.connections.subtitle_tracks = AdapterConnectionStatus::ConnectedToP7;
        if (subtitle.frame_available) {
            snapshot.connections.subtitle_frame = AdapterConnectionStatus::ConnectedToP7;
        }
    }

    if (audio_policy.valid) {
        snapshot.volume = audio_policy.volume;
        snapshot.muted = audio_policy.muted;
        snapshot.audio_delay_ms = audio_policy.delay_ms;
        snapshot.connections.audio_volume = AdapterConnectionStatus::ConnectedToP7;
        if (audio_policy.device_id[0] != '\0') {
            snapshot.connections.audio_device = AdapterConnectionStatus::ConnectedToP7;
        }
    }

    if (settings_policy.valid) {
        snapshot.aspect_mode = map_aspect_mode(settings_policy.aspect);
        snapshot.scale_mode = map_scale_mode(settings_policy.scale);
        snapshot.tone_mapping_mode = map_tone_mapping_mode(settings_policy.tone_mapping);
        snapshot.deinterlace_mode = map_deinterlace_mode(settings_policy.deinterlace);
        snapshot.playback_speed = settings_policy.playback_speed;
        snapshot.subtitle_size = settings_policy.subtitle_size;
        snapshot.connections.user_settings_policy = AdapterConnectionStatus::ConnectedToP7;
    }

    return snapshot;
}

}  // namespace kivo::video::player_runtime_adapter::runtime