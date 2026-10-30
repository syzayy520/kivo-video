#pragma once

#include <QtGlobal>
#include <QImage>
#include <QQuickItem>
#include <QString>

class QTimer;

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

struct PlaybackShellVideoSurfaceStats {
    quint64 refresh_attempt_count{0};
    quint64 refresh_success_count{0};
    quint64 refresh_miss_count{0};
    qint64 refresh_last_elapsed_ms{0};
    qint64 refresh_max_elapsed_ms{0};
    qint64 refresh_total_elapsed_ms{0};
    quint64 paint_count{0};
    qint64 paint_last_elapsed_ms{0};
    qint64 paint_max_elapsed_ms{0};
    qint64 paint_total_elapsed_ms{0};
};

class PlaybackShellVideoSurfaceItem final : public QQuickItem {
    Q_OBJECT

public:
    explicit PlaybackShellVideoSurfaceItem(QQuickItem* parent = nullptr);
    ~PlaybackShellVideoSurfaceItem() override;

    void setBridge(PlaybackShellRuntimeAdapterBridge* bridge) noexcept;
    [[nodiscard]] bool refreshFrame();
    [[nodiscard]] PlaybackShellVideoSurfaceStats stats() const noexcept;
    void startContinuousRefresh(int interval_ms);
    void stopContinuousRefresh();

    QSGNode* updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* data) override;

private:
    void onContinuousRefreshTick();
    void refreshSnapshotPolicy();

    PlaybackShellRuntimeAdapterBridge* bridge_{nullptr};
    QImage frame_image_;
    QString aspect_mode_{QStringLiteral("fit")};
    quint64 scene_graph_frame_revision_{0};
    quint64 scene_graph_uploaded_revision_{0};
    QTimer* refresh_timer_{nullptr};
    PlaybackShellVideoSurfaceStats stats_{};
};

}  // namespace kivo::ui::shell
