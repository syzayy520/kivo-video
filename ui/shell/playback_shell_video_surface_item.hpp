#pragma once

#include <QImage>
#include <QQuickPaintedItem>

class QTimer;

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

class PlaybackShellVideoSurfaceItem final : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit PlaybackShellVideoSurfaceItem(QQuickItem* parent = nullptr);
    ~PlaybackShellVideoSurfaceItem() override;

    void setBridge(PlaybackShellRuntimeAdapterBridge* bridge) noexcept;
    [[nodiscard]] bool refreshFrame();
    void startContinuousRefresh(int interval_ms);
    void stopContinuousRefresh();

    void paint(QPainter* painter) override;

private:
    void onContinuousRefreshTick();

    PlaybackShellRuntimeAdapterBridge* bridge_{nullptr};
    QImage frame_image_;
    QTimer* refresh_timer_{nullptr};
};

}  // namespace kivo::ui::shell