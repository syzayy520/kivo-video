#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

class QUrl;

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;
class PlaybackShellVideoSurfaceItem;

class PlaybackShellManualMediaOpenController final : public QObject {
    Q_OBJECT

public:
    explicit PlaybackShellManualMediaOpenController(
        PlaybackShellRuntimeAdapterBridge* bridge,
        QObject* parent = nullptr);

    Q_INVOKABLE bool openLocalMediaUrl(QObject* playback_page, const QUrl& media_url);
    Q_INVOKABLE bool openLocalMediaPath(QObject* playback_page, const QString& media_path);

private:
    PlaybackShellRuntimeAdapterBridge* bridge_{nullptr};
    QPointer<PlaybackShellVideoSurfaceItem> surface_;
};

}  // namespace kivo::ui::shell
