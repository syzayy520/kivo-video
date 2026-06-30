#pragma once

#include <QObject>
#include <QVariantMap>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge final : public QObject {
    Q_OBJECT

public:
    explicit PlaybackShellRuntimeAdapterBridge(
        kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter,
        QObject* parent = nullptr);

    Q_INVOKABLE bool openMediaId(quint64 media_id);
    Q_INVOKABLE bool togglePlayPause();
    Q_INVOKABLE bool seek(qint64 position_ms);
    Q_INVOKABLE bool stepForward(qint64 step_ms);
    Q_INVOKABLE bool stepBackward(qint64 step_ms);
    Q_INVOKABLE bool setVolume(double value);
    Q_INVOKABLE bool setMuted(bool muted);
    Q_INVOKABLE bool selectSubtitleTrack(quint64 track_id);
    Q_INVOKABLE bool setSubtitleDelay(qint64 delay_ms);
    Q_INVOKABLE bool setSubtitleSize(double size);
    Q_INVOKABLE bool selectAudioTrack(quint64 track_id);
    Q_INVOKABLE bool handleShortcutName(const QString& name);
    Q_INVOKABLE bool handleShortcut(const QString& name);
    Q_INVOKABLE bool enterFullscreen();
    Q_INVOKABLE bool exitFullscreen();
    Q_INVOKABLE bool previous();
    Q_INVOKABLE bool next();
    Q_INVOKABLE bool retry();
    Q_INVOKABLE bool close();
    Q_INVOKABLE bool releaseSurface();
    Q_INVOKABLE QVariantMap pullSnapshot();

signals:
    void snapshotChanged();

private:
    void emitSnapshotChanged();

    kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter_{nullptr};
};

}  // namespace kivo::ui::shell