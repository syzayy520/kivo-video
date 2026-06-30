#pragma once

#include <QImage>
#include <QQuickPaintedItem>

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

class PlaybackShellVideoSurfaceItem final : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit PlaybackShellVideoSurfaceItem(QQuickItem* parent = nullptr);

    void setBridge(PlaybackShellRuntimeAdapterBridge* bridge) noexcept;
    [[nodiscard]] bool refreshFrame();

    void paint(QPainter* painter) override;

private:
    PlaybackShellRuntimeAdapterBridge* bridge_{nullptr};
    QImage frame_image_;
};

}  // namespace kivo::ui::shell