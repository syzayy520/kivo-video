#include "playback_shell_video_launch_support.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVariantMap>

#include "playback_shell_runtime_adapter_bridge.hpp"
#include "playback_shell_video_surface_item.hpp"

namespace kivo::ui::shell {
namespace {

[[nodiscard]] QObject* find_playback_page(QQuickWindow* const window) {
    if (window == nullptr) {
        return nullptr;
    }
    const auto children = window->findChildren<QObject*>();
    for (QObject* const child : children) {
        const QString class_name = QString::fromLatin1(child->metaObject()->className());
        if (!class_name.contains(QStringLiteral("Loader"))) {
            continue;
        }
        QObject* const item = child->property("item").value<QObject*>();
        if (item != nullptr) {
            return item;
        }
    }
    return nullptr;
}

[[nodiscard]] QQuickItem* as_quick_item(QObject* const object) {
    return qobject_cast<QQuickItem*>(object);
}

}  // namespace

void pump_events(const int ms) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents();
    }
}

QObject* wait_for_playback_page(QQuickWindow* const window) {
    for (int attempt = 0; attempt < 80; ++attempt) {
        QObject* const page = find_playback_page(window);
        if (page != nullptr) {
            return page;
        }
        pump_events(50);
    }
    return nullptr;
}

PlaybackShellVideoSurfaceItem* attach_video_surface(
    QObject* const page,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    const bool start_refresh) {
    QQuickItem* const page_item = as_quick_item(page);
    if (page_item == nullptr || bridge == nullptr) {
        return nullptr;
    }

    auto* const surface = new PlaybackShellVideoSurfaceItem(page_item);
    surface->setBridge(bridge);
    surface->setParentItem(page_item);
    surface->setWidth(page_item->width());
    surface->setHeight(page_item->height());
    surface->setZ(1);
    QObject::connect(page_item, &QQuickItem::widthChanged, surface,
                     [surface, page_item]() { surface->setWidth(page_item->width()); });
    QObject::connect(page_item, &QQuickItem::heightChanged, surface,
                     [surface, page_item]() { surface->setHeight(page_item->height()); });
    (void)page->setProperty("showPlaceholderVideo", false);
    (void)surface->refreshFrame();
    if (start_refresh) {
        surface->startContinuousRefresh(33);
    }
    return surface;
}

bool wait_for_first_frame(PlaybackShellRuntimeAdapterBridge* const bridge) {
    if (bridge == nullptr) {
        return false;
    }
    for (int attempt = 0; attempt < 120; ++attempt) {
        const QVariantMap query = bridge->queryLocalMediaPlayback();
        if (query.value(QStringLiteral("first_frame_decoded")).toBool() &&
            query.value(QStringLiteral("frame_pixels_valid")).toBool()) {
            return true;
        }
        pump_events(50);
    }
    return false;
}

qint64 snapshot_position_ms(PlaybackShellRuntimeAdapterBridge* const bridge) {
    if (bridge == nullptr) {
        return 0;
    }
    return bridge->pullSnapshot().value(QStringLiteral("position_ms")).toLongLong();
}

bool snapshot_ready_after_stop(PlaybackShellRuntimeAdapterBridge* const bridge) {
    if (bridge == nullptr) {
        return false;
    }
    const QVariantMap snapshot = bridge->pullSnapshot();
    return !snapshot.value(QStringLiteral("is_playing")).toBool() &&
           !snapshot.value(QStringLiteral("closed")).toBool();
}

}  // namespace kivo::ui::shell
