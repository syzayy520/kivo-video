#include "playback_shell_runtime_adapter_bridge.hpp"

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