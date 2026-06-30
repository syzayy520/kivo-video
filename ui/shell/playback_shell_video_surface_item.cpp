#include "playback_shell_video_surface_item.hpp"

#include <QPainter>

#include "playback_shell_runtime_adapter_bridge.hpp"

namespace kivo::ui::shell {

PlaybackShellVideoSurfaceItem::PlaybackShellVideoSurfaceItem(QQuickItem* const parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(false);
    setOpaquePainting(true);
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

void PlaybackShellVideoSurfaceItem::paint(QPainter* const painter) {
    if (painter == nullptr || frame_image_.isNull()) {
        return;
    }
    const QRectF target(0, 0, width(), height());
    painter->drawImage(target, frame_image_);
}

}  // namespace kivo::ui::shell