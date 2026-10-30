#include "playback_shell_video_surface_item.hpp"

#include <QElapsedTimer>
#include <QQuickWindow>
#include <QRectF>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QTimer>
#include <QVariantMap>

#include "playback_shell_runtime_adapter_bridge.hpp"
#include "playback_shell_video_surface_geometry.hpp"

namespace kivo::ui::shell {
namespace {

void update_elapsed_stats(
    const qint64 elapsed_ms,
    qint64& last_elapsed_ms,
    qint64& max_elapsed_ms,
    qint64& total_elapsed_ms) noexcept {
    last_elapsed_ms = elapsed_ms;
    if (elapsed_ms > max_elapsed_ms) {
        max_elapsed_ms = elapsed_ms;
    }
    total_elapsed_ms += elapsed_ms;
}

class OwnedTextureNode final : public QSGSimpleTextureNode {
public:
    ~OwnedTextureNode() override { delete texture(); }

    void replaceTexture(QSGTexture* const texture) {
        QSGTexture* const previous = this->texture();
        setTexture(texture);
        delete previous;
    }
};

}  // namespace

PlaybackShellVideoSurfaceItem::PlaybackShellVideoSurfaceItem(QQuickItem* const parent)
    : QQuickItem(parent) {
    setFlag(QQuickItem::ItemHasContents, true);
    setClip(true);
}

PlaybackShellVideoSurfaceItem::~PlaybackShellVideoSurfaceItem() {
    stopContinuousRefresh();
}

void PlaybackShellVideoSurfaceItem::setBridge(
    PlaybackShellRuntimeAdapterBridge* const bridge) noexcept {
    if (bridge_ != nullptr) {
        disconnect(bridge_, nullptr, this, nullptr);
    }
    bridge_ = bridge;
    if (bridge_ != nullptr) {
        connect(bridge_, &PlaybackShellRuntimeAdapterBridge::snapshotChanged, this, [this]() {
            refreshSnapshotPolicy();
            update();
        });
    }
    refreshSnapshotPolicy();
}

bool PlaybackShellVideoSurfaceItem::refreshFrame() {
    QElapsedTimer timer;
    timer.start();
    ++stats_.refresh_attempt_count;
    if (bridge_ == nullptr) {
        ++stats_.refresh_miss_count;
        update_elapsed_stats(timer.elapsed(),
                             stats_.refresh_last_elapsed_ms,
                             stats_.refresh_max_elapsed_ms,
                             stats_.refresh_total_elapsed_ms);
        return false;
    }
    refreshSnapshotPolicy();
    const QImage image = bridge_->pullVideoFrameImage();
    if (image.isNull()) {
        ++stats_.refresh_miss_count;
        update_elapsed_stats(timer.elapsed(),
                             stats_.refresh_last_elapsed_ms,
                             stats_.refresh_max_elapsed_ms,
                             stats_.refresh_total_elapsed_ms);
        return false;
    }
    frame_image_ = image;
    ++scene_graph_frame_revision_;
    update();
    ++stats_.refresh_success_count;
    update_elapsed_stats(timer.elapsed(),
                         stats_.refresh_last_elapsed_ms,
                         stats_.refresh_max_elapsed_ms,
                         stats_.refresh_total_elapsed_ms);
    return true;
}

PlaybackShellVideoSurfaceStats PlaybackShellVideoSurfaceItem::stats() const noexcept {
    return stats_;
}

void PlaybackShellVideoSurfaceItem::startContinuousRefresh(const int interval_ms) {
    if (refresh_timer_ == nullptr) {
        refresh_timer_ = new QTimer(this);
        connect(refresh_timer_, &QTimer::timeout, this,
                &PlaybackShellVideoSurfaceItem::onContinuousRefreshTick);
    }
    const int effective_interval = interval_ms > 0 ? interval_ms : 16;
    refresh_timer_->start(effective_interval);
}

void PlaybackShellVideoSurfaceItem::stopContinuousRefresh() {
    if (refresh_timer_ != nullptr) {
        refresh_timer_->stop();
    }
}

void PlaybackShellVideoSurfaceItem::onContinuousRefreshTick() {
    (void)refreshFrame();
}

void PlaybackShellVideoSurfaceItem::refreshSnapshotPolicy() {
    if (bridge_ == nullptr) {
        aspect_mode_ = QStringLiteral("fit");
        return;
    }
    const QString aspect_mode =
        bridge_->pullSnapshot().value(QStringLiteral("aspectMode")).toString();
    aspect_mode_ = aspect_mode.isEmpty() ? QStringLiteral("fit") : aspect_mode;
}

QSGNode* PlaybackShellVideoSurfaceItem::updatePaintNode(
    QSGNode* const old_node,
    UpdatePaintNodeData* const data) {
    (void)data;
    QElapsedTimer timer;
    timer.start();
    ++stats_.paint_count;

    auto* node = static_cast<OwnedTextureNode*>(old_node);
    if (node == nullptr) {
        node = new OwnedTextureNode();
        node->setFiltering(QSGTexture::Linear);
        node->setOwnsTexture(false);
    }

    const QRectF target =
        frame_image_.isNull()
            ? QRectF(0, 0, width(), height())
            : playback_video_surface_target_rect(
                  frame_image_.size(), QSizeF(width(), height()), aspect_mode_);
    node->setRect(target);

    if (!frame_image_.isNull() && window() != nullptr &&
        scene_graph_uploaded_revision_ != scene_graph_frame_revision_) {
        QSGTexture* const texture = window()->createTextureFromImage(frame_image_);
        if (texture != nullptr) {
            node->replaceTexture(texture);
            scene_graph_uploaded_revision_ = scene_graph_frame_revision_;
        }
    }

    update_elapsed_stats(timer.elapsed(),
                         stats_.paint_last_elapsed_ms,
                         stats_.paint_max_elapsed_ms,
                         stats_.paint_total_elapsed_ms);
    return node;
}

}  // namespace kivo::ui::shell
