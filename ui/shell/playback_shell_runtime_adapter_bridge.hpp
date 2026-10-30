#pragma once

#include <QImage>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

#include <cstdint>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

class QQuickWindow;

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge final : public QObject {
    Q_OBJECT

public:
    explicit PlaybackShellRuntimeAdapterBridge(
        kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter,
        QObject* parent = nullptr);

    Q_INVOKABLE bool openMediaId(quint64 media_id);
    Q_INVOKABLE bool openLocalMediaFile(const QString& path);
    Q_INVOKABLE QVariantMap queryLocalMediaPlayback();
    Q_INVOKABLE int pumpLocalMediaPlayback(int max_packets);
    [[nodiscard]] QImage pullVideoFrameImage();
    Q_INVOKABLE bool play();
    Q_INVOKABLE bool pause();
    Q_INVOKABLE bool togglePlayPause();
    Q_INVOKABLE bool stop();
    Q_INVOKABLE bool seek(qint64 position_ms);
    Q_INVOKABLE bool seekRelative(qint64 delta_ms);
    Q_INVOKABLE bool stepForward(qint64 step_ms);
    Q_INVOKABLE bool stepBackward(qint64 step_ms);
    Q_INVOKABLE bool setVolume(double value);
    Q_INVOKABLE bool setMuted(bool muted);
    Q_INVOKABLE bool selectSubtitleTrack(quint64 track_id);
    Q_INVOKABLE bool disableSubtitle();
    Q_INVOKABLE bool setSubtitleDelay(qint64 delay_ms);
    Q_INVOKABLE bool setSubtitleSize(double size);
    Q_INVOKABLE bool setAudioDelay(qint64 delay_ms);
    Q_INVOKABLE bool setAspectMode(const QString& mode);
    Q_INVOKABLE bool setToneMappingMode(const QString& mode);
    Q_INVOKABLE bool setPlaybackSpeed(double speed);
    Q_INVOKABLE bool selectAudioTrack(quint64 track_id);
    Q_INVOKABLE bool handleShortcutName(const QString& name);
    Q_INVOKABLE bool handleShortcut(const QString& name);
    Q_INVOKABLE bool enterFullscreen();
    Q_INVOKABLE bool exitFullscreen();
    Q_INVOKABLE bool moveToScreen(int screen_index);
    Q_INVOKABLE bool previous();
    Q_INVOKABLE bool next();
    Q_INVOKABLE bool playNextEpisode();
    Q_INVOKABLE bool playPlaylistItem(quint64 index);
    Q_INVOKABLE bool setAutoPlayNext(bool enabled);
    Q_INVOKABLE bool retry();
    Q_INVOKABLE bool close();
    Q_INVOKABLE bool releaseSurface();
    Q_INVOKABLE bool copyDiagnostics();
    Q_INVOKABLE QVariantMap pullSnapshot();

    void bindWindow(QQuickWindow* window) noexcept;
    void publishSnapshotChange();

signals:
    void snapshotChanged();

private:
    void emitSnapshotChanged();
    void applyWindowPolicyFromSnapshot();
    void clearCachedVideoFrame() noexcept;

    kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter_{nullptr};
    QPointer<QQuickWindow> window_{};
    QImage cached_video_frame_{};
    std::uint64_t cached_video_frame_signature_{0};
};

}  // namespace kivo::ui::shell
