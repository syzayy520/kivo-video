#include "playback_shell_runtime_adapter_bridge.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQuickWindow>
#include <QScreen>

#include <cstdint>
#include <memory>
#include <new>
#include <vector>

#include "playback_shell_snapshot_mapper.hpp"

namespace kivo::ui::shell {
namespace {

using kivo::video::player_runtime_adapter::AdapterAspectMode;
using kivo::video::player_runtime_adapter::AdapterToneMappingMode;
using SharedRgbaFrame = std::shared_ptr<const std::vector<std::uint8_t>>;

void release_shared_rgba_frame(void* const info) noexcept {
    delete static_cast<SharedRgbaFrame*>(info);
}

bool command_ok(
    const kivo::video::player_runtime_adapter::AdapterCommandResult& result) noexcept {
    return result.status == kivo::video::player_runtime_adapter::AdapterCommandStatus::Accepted;
}

[[nodiscard]] AdapterAspectMode aspect_mode_from_qml(const QString& mode) noexcept {
    if (mode == QStringLiteral("fill")) {
        return AdapterAspectMode::Fill;
    }
    if (mode == QStringLiteral("stretch")) {
        return AdapterAspectMode::Stretch;
    }
    if (mode == QStringLiteral("original")) {
        return AdapterAspectMode::Original;
    }
    return AdapterAspectMode::Fit;
}

[[nodiscard]] AdapterToneMappingMode tone_mode_from_qml(const QString& mode) noexcept {
    if (mode == QStringLiteral("off")) {
        return AdapterToneMappingMode::Off;
    }
    if (mode == QStringLiteral("sdr")) {
        return AdapterToneMappingMode::Sdr;
    }
    if (mode == QStringLiteral("hdr")) {
        return AdapterToneMappingMode::HdrPassthrough;
    }
    return AdapterToneMappingMode::Auto;
}

}  // namespace

PlaybackShellRuntimeAdapterBridge::PlaybackShellRuntimeAdapterBridge(
    kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* const adapter,
    QObject* const parent)
    : QObject(parent), adapter_(adapter) {}

void PlaybackShellRuntimeAdapterBridge::emitSnapshotChanged() {
    emit snapshotChanged();
}

bool PlaybackShellRuntimeAdapterBridge::openMediaId(const quint64 media_id) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->open_media_id(media_id));
    if (ok) {
        clearCachedVideoFrame();
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::openLocalMediaFile(const QString& path) {
    if (adapter_ == nullptr || path.isEmpty()) {
        return false;
    }
    const QByteArray utf8 = QDir::toNativeSeparators(path).toUtf8();
    const bool ok = command_ok(adapter_->open_local_media_file(utf8.constData()));
    if (ok) {
        clearCachedVideoFrame();
        emitSnapshotChanged();
    }
    return ok;
}

QVariantMap PlaybackShellRuntimeAdapterBridge::queryLocalMediaPlayback() {
    if (adapter_ == nullptr) {
        return {};
    }
    const auto query = adapter_->query_local_media_playback(false);
    const auto snapshot = adapter_->snapshot();
    return QVariantMap{
        {QStringLiteral("active"), query.active},
        {QStringLiteral("source_session_connected"), query.source_session_connected},
        {QStringLiteral("source_opened"), query.source_opened},
        {QStringLiteral("demux_started"), query.demux_started},
        {QStringLiteral("video_decode_started"), query.video_decode_started},
        {QStringLiteral("audio_decode_started"), query.audio_decode_started},
        {QStringLiteral("audio_output_started"), query.audio_output_started},
        {QStringLiteral("first_frame_decoded"), query.first_frame_decoded},
        {QStringLiteral("d3d11_uploaded"), query.d3d11_uploaded},
        {QStringLiteral("d3d11_upload_available"), query.d3d11_upload_available},
        {QStringLiteral("d3d11_upload_initialized"), query.d3d11_upload_initialized},
        {QStringLiteral("frame_rendered"), query.frame_rendered},
        {QStringLiteral("frame_pixels_valid"), query.frame_pixels_valid},
        {QStringLiteral("continuous_rendering"), query.continuous_rendering},
        {QStringLiteral("decoded_frame_count"),
         static_cast<qulonglong>(query.decoded_frame_count)},
        {QStringLiteral("rendered_frame_count"),
         static_cast<qulonglong>(query.rendered_frame_count)},
        {QStringLiteral("d3d11_frames_uploaded"),
         static_cast<qulonglong>(query.d3d11_frames_uploaded)},
        {QStringLiteral("d3d11_upload_failures"),
         static_cast<qulonglong>(query.d3d11_upload_failures)},
        {QStringLiteral("d3d11_upload_last_elapsed_ms"),
         static_cast<qulonglong>(query.d3d11_upload_last_elapsed_ms)},
        {QStringLiteral("d3d11_upload_max_elapsed_ms"),
         static_cast<qulonglong>(query.d3d11_upload_max_elapsed_ms)},
        {QStringLiteral("audio_packets_submitted"),
         static_cast<qulonglong>(query.audio_packets_submitted)},
        {QStringLiteral("audio_frames_decoded"),
         static_cast<qulonglong>(query.audio_frames_decoded)},
        {QStringLiteral("audio_frames_written"),
         static_cast<qulonglong>(query.audio_frames_written)},
        {QStringLiteral("audio_flush_count"),
         static_cast<qulonglong>(query.audio_flush_count)},
        {QStringLiteral("audio_writer_reset_count"),
         static_cast<qulonglong>(query.audio_writer_reset_count)},
        {QStringLiteral("audio_stale_packets_discarded"),
         static_cast<qulonglong>(query.audio_stale_packets_discarded)},
        {QStringLiteral("p7_background_pump_count"),
         static_cast<qulonglong>(query.p7_background_pump_count)},
        {QStringLiteral("p7_background_pump_last_elapsed_ms"),
         static_cast<qulonglong>(query.p7_background_pump_last_elapsed_ms)},
        {QStringLiteral("p7_background_pump_max_elapsed_ms"),
         static_cast<qulonglong>(query.p7_background_pump_max_elapsed_ms)},
        {QStringLiteral("p7_background_pump_total_elapsed_ms"),
         static_cast<qulonglong>(query.p7_background_pump_total_elapsed_ms)},
        {QStringLiteral("p7_background_pump_last_new_frames"),
         static_cast<qulonglong>(query.p7_background_pump_last_new_frames)},
        {QStringLiteral("p7_background_pump_packet_budget"),
         static_cast<qulonglong>(query.p7_background_pump_packet_budget)},
        {QStringLiteral("p7_background_pump_max_packet_budget"),
         static_cast<qulonglong>(query.p7_background_pump_max_packet_budget)},
        {QStringLiteral("p7_seek_preroll_active"), query.p7_seek_preroll_active},
        {QStringLiteral("p7_seek_preroll_completed_count"),
         static_cast<qulonglong>(query.p7_seek_preroll_completed_count)},
        {QStringLiteral("p7_seek_preroll_last_elapsed_ms"),
         static_cast<qulonglong>(query.p7_seek_preroll_last_elapsed_ms)},
        {QStringLiteral("p7_seek_preroll_max_elapsed_ms"),
         static_cast<qulonglong>(query.p7_seek_preroll_max_elapsed_ms)},
        {QStringLiteral("p5_local_video_runtime_connected"),
         query.p5_local_video_runtime_connected},
        {QStringLiteral("p5_audio_packet_boundary_connected"),
         query.p5_audio_packet_boundary_connected},
        {QStringLiteral("p5_subtitle_packet_boundary_connected"),
         query.p5_subtitle_packet_boundary_connected},
        {QStringLiteral("p8_embedded_packet_ingest_connected"),
         query.p8_embedded_packet_ingest_connected},
        {QStringLiteral("p8_pgs_bitmap_render_connected"),
         query.p8_pgs_bitmap_render_connected},
        {QStringLiteral("p5_audio_packets_dispatched"),
         static_cast<qulonglong>(query.p5_audio_packets_dispatched)},
        {QStringLiteral("p5_subtitle_packets_dispatched"),
         static_cast<qulonglong>(query.p5_subtitle_packets_dispatched)},
        {QStringLiteral("p8_subtitle_packets_submitted"),
         static_cast<qulonglong>(query.p8_subtitle_packets_submitted)},
        {QStringLiteral("p8_unsupported_feature_count"),
         static_cast<qulonglong>(query.p8_unsupported_feature_count)},
        {QStringLiteral("p3_candidate_set_created"), query.p3_candidate_set_created},
        {QStringLiteral("p3_handoff_created"), query.p3_handoff_created},
        {QStringLiteral("p3_handoff_consumed_by_p7"), query.p3_handoff_consumed_by_p7},
        {QStringLiteral("p4_open_envelope_from_p3_created"),
         query.p4_open_envelope_from_p3_created},
        {QStringLiteral("p4_timeline_contract_connected"),
         query.p4_timeline_contract_connected},
        {QStringLiteral("p4_timeline_uses_public_contract"),
         query.p4_timeline_uses_public_contract},
        {QStringLiteral("p4_runtime_state_machine_owner"),
         query.p4_runtime_state_machine_owner},
        {QStringLiteral("p4_timeline_revision"),
         static_cast<qulonglong>(query.p4_timeline_revision)},
        {QStringLiteral("p4_timeline_generation"),
         static_cast<qulonglong>(query.p4_timeline_generation)},
        {QStringLiteral("p4_timeline_commanded_position_ms"),
         static_cast<qint64>(query.p4_timeline_commanded_position_ms)},
        {QStringLiteral("p4_timeline_observed_position_ms"),
         static_cast<qint64>(query.p4_timeline_observed_position_ms)},
        {QStringLiteral("p4_timeline_duration_ms"),
         static_cast<qint64>(query.p4_timeline_duration_ms)},
        {QStringLiteral("p4_timeline_buffered_start_ms"),
         static_cast<qint64>(query.p4_timeline_buffered_start_ms)},
        {QStringLiteral("p4_timeline_buffered_end_ms"),
         static_cast<qint64>(query.p4_timeline_buffered_end_ms)},
        {QStringLiteral("p3_candidate_kind"), QString::fromStdString(query.p3_candidate_kind)},
        {QStringLiteral("p3_handoff_id"), QString::fromStdString(query.p3_handoff_id)},
        {QStringLiteral("p3_candidate_set_id"),
         QString::fromStdString(query.p3_candidate_set_id)},
        {QStringLiteral("playback_elapsed_ms"),
         static_cast<qint64>(query.playback_elapsed_ms)},
        {QStringLiteral("last_frame_signature"),
         static_cast<qulonglong>(query.last_frame_signature)},
        {QStringLiteral("last_frame_time_us"),
         static_cast<qint64>(query.last_frame_time_us)},
        {QStringLiteral("last_frame_duration_us"),
         static_cast<qint64>(query.last_frame_duration_us)},
        {QStringLiteral("container"), QString::fromStdString(query.container)},
        {QStringLiteral("source_provider_kind"),
         QString::fromStdString(query.source_provider_kind)},
        {QStringLiteral("d3d11_device_type"),
         QString::fromStdString(query.d3d11_device_type)},
        {QStringLiteral("d3d11_upload_error"),
         QString::fromStdString(query.d3d11_upload_error)},
        {QStringLiteral("video_codec"), QString::fromStdString(query.video_codec)},
        {QStringLiteral("audio_codec"), QString::fromStdString(query.audio_codec)},
        {QStringLiteral("audio_error"), QString::fromStdString(query.audio_error)},
        {QStringLiteral("error"), QString::fromStdString(query.error)},
        {QStringLiteral("duration_ms"), static_cast<qint64>(query.duration_ms)},
        {QStringLiteral("width"), query.width},
        {QStringLiteral("height"), query.height},
        {QStringLiteral("video_stream_count"),
         static_cast<qulonglong>(query.video_stream_count)},
        {QStringLiteral("audio_stream_count"),
         static_cast<qulonglong>(query.audio_stream_count)},
        {QStringLiteral("subtitle_stream_count"),
         static_cast<qulonglong>(query.subtitle_stream_count)},
        {QStringLiteral("subtitle_frame_runtime_connected"),
         snapshot.connections.subtitle_frame ==
             kivo::video::player_runtime_adapter::AdapterConnectionStatus::ConnectedToP7},
        {QStringLiteral("subtitle_frame_available"), snapshot.subtitle_frame_available},
        {QStringLiteral("subtitle_primary"), QString::fromUtf8(snapshot.subtitle_primary)},
        {QStringLiteral("subtitle_secondary"), QString::fromUtf8(snapshot.subtitle_secondary)},
        {QStringLiteral("p4_playback_revision"),
         static_cast<qulonglong>(snapshot.p7_snapshot.playback_revision.value)},
        {QStringLiteral("p7_retained_command_count"),
         static_cast<qulonglong>(snapshot.p7_snapshot.retained_command_count)},
        {QStringLiteral("p7_late_event_discard_count"),
         static_cast<qulonglong>(snapshot.p7_snapshot.late_event_discard_count)},
    };
}

int PlaybackShellRuntimeAdapterBridge::pumpLocalMediaPlayback(const int max_packets) {
    if (adapter_ == nullptr) {
        return 0;
    }
    const int new_frames = adapter_->pump_local_media_playback(max_packets);
    if (new_frames > 0) {
        emitSnapshotChanged();
    }
    return new_frames;
}

QImage PlaybackShellRuntimeAdapterBridge::pullVideoFrameImage() {
    if (adapter_ == nullptr) {
        return {};
    }
    const auto metadata = adapter_->query_local_media_playback(false);
    if (!metadata.frame_pixels_valid || metadata.width <= 0 || metadata.height <= 0) {
        return {};
    }
    if (!cached_video_frame_.isNull() &&
        metadata.last_frame_signature == cached_video_frame_signature_) {
        return {};
    }

    const auto query = adapter_->query_local_media_playback(true);
    const auto frame_pixels = query.rgba8888;
    if (!query.frame_pixels_valid || query.width <= 0 || query.height <= 0 ||
        frame_pixels == nullptr || frame_pixels->empty()) {
        return {};
    }
    const auto row_bytes = static_cast<std::size_t>(query.width) * 4;
    const auto expected = row_bytes * static_cast<std::size_t>(query.height);
    if (frame_pixels->size() < expected) {
        return {};
    }
    auto* const holder = new (std::nothrow) SharedRgbaFrame(frame_pixels);
    if (holder == nullptr) {
        return {};
    }
    QImage image(reinterpret_cast<const uchar*>(frame_pixels->data()),
                 query.width,
                 query.height,
                 static_cast<qsizetype>(row_bytes),
                 QImage::Format_RGBA8888,
                 release_shared_rgba_frame,
                 holder);
    if (image.isNull()) {
        delete holder;
        return {};
    }
    cached_video_frame_ = image;
    cached_video_frame_signature_ = query.last_frame_signature;
    return image;
}

bool PlaybackShellRuntimeAdapterBridge::play() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->play());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::pause() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->pause());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::togglePlayPause() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->toggle_play_pause());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::stop() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->stop());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::seek(const qint64 position_ms) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->seek(position_ms));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::seekRelative(const qint64 delta_ms) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->seek_relative(delta_ms));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::stepForward(const qint64 step_ms) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->step_forward(step_ms));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::stepBackward(const qint64 step_ms) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->step_backward(step_ms));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setVolume(const double value) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_volume(value));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setMuted(const bool muted) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_muted(muted));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::selectSubtitleTrack(const quint64 track_id) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->select_subtitle_track(track_id));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::disableSubtitle() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->disable_subtitle());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setSubtitleDelay(const qint64 delay_ms) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_subtitle_delay(delay_ms));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setSubtitleSize(const double size) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_subtitle_size(size));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setAudioDelay(const qint64 delay_ms) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_audio_delay(delay_ms));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setAspectMode(const QString& mode) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_aspect_mode(aspect_mode_from_qml(mode)));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setToneMappingMode(const QString& mode) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok =
        command_ok(adapter_->set_tone_mapping_mode(tone_mode_from_qml(mode)));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setPlaybackSpeed(const double speed) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_playback_speed(speed));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::selectAudioTrack(const quint64 track_id) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->select_audio_track(track_id));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::handleShortcutName(const QString& name) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok =
        command_ok(adapter_->handle_shortcut_name(name.toUtf8().constData()));
    if (ok) {
        applyWindowPolicyFromSnapshot();
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::handleShortcut(const QString& name) {
    return handleShortcutName(name);
}

bool PlaybackShellRuntimeAdapterBridge::enterFullscreen() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->enter_fullscreen());
    if (ok) {
        applyWindowPolicyFromSnapshot();
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::exitFullscreen() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->exit_fullscreen());
    if (ok) {
        applyWindowPolicyFromSnapshot();
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::moveToScreen(const int screen_index) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->move_to_screen(screen_index));
    if (ok) {
        applyWindowPolicyFromSnapshot();
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::previous() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->previous());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::next() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->next());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::playNextEpisode() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->play_next_episode());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::playPlaylistItem(const quint64 index) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->play_playlist_item(index));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::setAutoPlayNext(const bool enabled) {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->set_auto_play_next(enabled));
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::retry() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->retry());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::close() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->close());
    if (ok) {
        clearCachedVideoFrame();
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::releaseSurface() {
    if (adapter_ == nullptr) {
        return false;
    }
    const bool ok = command_ok(adapter_->release_surface());
    if (ok) {
        emitSnapshotChanged();
    }
    return ok;
}

bool PlaybackShellRuntimeAdapterBridge::copyDiagnostics() {
    if (adapter_ == nullptr) {
        return false;
    }
    return command_ok(adapter_->copy_diagnostics());
}

QVariantMap PlaybackShellRuntimeAdapterBridge::pullSnapshot() {
    if (adapter_ == nullptr) {
        return {};
    }
    return map_adapter_snapshot(adapter_->snapshot());
}

void PlaybackShellRuntimeAdapterBridge::bindWindow(QQuickWindow* const window) noexcept {
    window_ = window;
}

void PlaybackShellRuntimeAdapterBridge::publishSnapshotChange() {
    emitSnapshotChanged();
}

void PlaybackShellRuntimeAdapterBridge::applyWindowPolicyFromSnapshot() {
    if (adapter_ == nullptr || window_.isNull()) {
        return;
    }

    const auto snapshot = adapter_->snapshot();
    if (snapshot.screen_index_valid) {
        const QList<QScreen*> screens = QGuiApplication::screens();
        if (snapshot.screen_index >= 0 && snapshot.screen_index < screens.size()) {
            window_->setScreen(screens.at(snapshot.screen_index));
        }
    }

    if (snapshot.fullscreen_like) {
        window_->showFullScreen();
    } else {
        window_->showNormal();
    }
}

void PlaybackShellRuntimeAdapterBridge::clearCachedVideoFrame() noexcept {
    cached_video_frame_ = {};
    cached_video_frame_signature_ = 0;
}

}  // namespace kivo::ui::shell
