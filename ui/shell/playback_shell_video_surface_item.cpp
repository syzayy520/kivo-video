#include "playback_shell_video_surface_item.hpp"

#include <QPainter>
#include <QTimer>

#include "playback_shell_runtime_adapter_bridge.hpp"

namespace kivo::ui::shell {

PlaybackShellVideoSurfaceItem::PlaybackShellVideoSurfaceItem(QQuickItem* const parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(false);
    setOpaquePainting(true);
}

PlaybackShellVideoSurfaceItem::~PlaybackShellVideoSurfaceItem() {
    stopContinuousRefresh();
}

void PlaybackShellVideoSurfaceItem::setBridge(
    PlaybackShellRuntimeAdapterBridge* const bridge) noexcept {
    bridge_ = bridge;
}

bool PlaybackShellVideoSurfaceItem::refreshFrame() {
    if (bridge_ == nullptr) {
        return false;
    }
    const QImage image = bridge_->pullVideoFrameImage();
    if (image.isNull()) {
        return false;
    }
    frame_image_ = image;
    update();
    return true;
}

void PlaybackShellVideoSurfaceItem::startContinuousRefresh(const int interval_ms) {
    if (refresh_timer_ == nullptr) {
        refresh_timer_ = new QTimer(this);
        connect(refresh_timer_, &QTimer::timeout, this,
                &PlaybackShellVideoSurfaceItem::onContinuousRefreshTick);
    }
    const int effective_interval = interval_ms > 0 ? interval_ms : 33;
    refresh_timer_->start(effective_interval);
}

void PlaybackShellVideoSurfaceItem::stopContinuousRefresh() {
    if (refresh_timer_ != nullptr) {
        refresh_timer_->stop();
    }
}

void PlaybackShellVideoSurfaceItem::onContinuousRefreshTick() {
    if (bridge_ != nullptr) {
        (void)bridge_->pumpLocalMediaPlayback(32);
    }
    (void)refreshFrame();
}

void PlaybackShellVideoSurfaceItem::paint(QPainter* const painter) {
    if (painter == nullptr || frame_image_.isNull()) {
        return;
    }
    const QRectF target(0, 0, width(), height());
    painter->drawImage(target, frame_image_);
}

}  // namespace kivo::ui::shell