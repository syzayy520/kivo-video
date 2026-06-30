#include "playback_shell_runtime_adapter_bridge.hpp"

#include <QDir>
#include <QImage>

#include <cstring>

#include "playback_shell_snapshot_mapper.hpp"

namespace kivo::ui::shell {
namespace {

bool command_ok(
    const kivo::video::player_runtime_adapter::AdapterCommandResult& result) noexcept {
    return result.status == kivo::video::player_runtime_adapter::AdapterCommandStatus::Accepted;
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
        emitSnapshotChanged();
    }
    return ok;
}

QVariantMap PlaybackShellRuntimeAdapterBridge::queryLocalMediaPlayback() {
    if (adapter_ == nullptr) {
        return {};
    }
    const auto query = adapter_->query_local_media_playback();
    return QVariantMap{
        {QStringLiteral("active"), query.active},
        {QStringLiteral("source_opened"), query.source_opened},
        {QStringLiteral("demux_started"), query.demux_started},
        {QStringLiteral("video_decode_started"), query.video_decode_started},
        {QStringLiteral("first_frame_decoded"), query.first_frame_decoded},
        {QStringLiteral("d3d11_uploaded"), query.d3d11_uploaded},
        {QStringLiteral("frame_rendered"), query.frame_rendered},
        {QStringLiteral("frame_pixels_valid"), query.frame_pixels_valid},
        {QStringLiteral("continuous_rendering"), query.continuous_rendering},
        {QStringLiteral("decoded_frame_count"),
         static_cast<qulonglong>(query.decoded_frame_count)},
        {QStringLiteral("rendered_frame_count"),
         static_cast<qulonglong>(query.rendered_frame_count)},
        {QStringLiteral("playback_elapsed_ms"),
         static_cast<qint64>(query.playback_elapsed_ms)},
        {QStringLiteral("last_frame_signature"),
         static_cast<qulonglong>(query.last_frame_signature)},
        {QStringLiteral("container"), QString::fromStdString(query.container)},
        {QStringLiteral("video_codec"), QString::fromStdString(query.video_codec)},
        {QStringLiteral("error"), QString::fromStdString(query.error)},
        {QStringLiteral("duration_ms"), static_cast<qint64>(query.duration_ms)},
        {QStringLiteral("width"), query.width},
        {QStringLiteral("height"), query.height},
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
    const auto query = adapter_->query_local_media_playback();
    if (!query.frame_pixels_valid || query.width <= 0 || query.height <= 0 ||
        query.rgba8888.empty()) {
        return {};
    }
    QImage image(query.width, query.height, QImage::Format_RGBA8888);
    if (image.isNull()) {
        return {};
    }
    const auto row_bytes = static_cast<std::size_t>(query.width) * 4;
    const auto expected = row_bytes * static_cast<std::size_t>(query.height);
    if (query.rgba8888.size() < expected) {
        return {};
    }
    for (int row = 0; row < query.height; ++row) {
        std::memcpy(
            image.scanLine(row),
            query.rgba8888.data() + static_cast<std::size_t>(row) * row_bytes,
            row_bytes);
    }
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
    return command_ok(adapter_->enter_fullscreen());
}

bool PlaybackShellRuntimeAdapterBridge::exitFullscreen() {
    if (adapter_ == nullptr) {
        return false;
    }
    return command_ok(adapter_->exit_fullscreen());
}

bool PlaybackShellRuntimeAdapterBridge::previous() {
    if (adapter_ == nullptr) {
        return false;
    }
    return command_ok(adapter_->previous());
}

bool PlaybackShellRuntimeAdapterBridge::next() {
    if (adapter_ == nullptr) {
        return false;
    }
    return command_ok(adapter_->next());
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

void PlaybackShellRuntimeAdapterBridge::publishSnapshotChange() {
    emitSnapshotChanged();
}

}  // namespace kivo::ui::shell