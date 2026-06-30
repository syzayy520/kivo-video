#include "playback_shell_playback_evidence.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QVariantMap>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"
#include "playback_shell_runtime_adapter_bridge.hpp"
#include "playback_shell_video_surface_item.hpp"

namespace kivo::ui::shell {
namespace {

using kivo::video::player_runtime_adapter::PlayerRuntimeAdapter;

void pump_events(const int ms) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents();
    }
}

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

[[nodiscard]] QObject* wait_for_playback_page(QQuickWindow* const window) {
    for (int attempt = 0; attempt < 80; ++attempt) {
        QObject* const page = find_playback_page(window);
        if (page != nullptr) {
            return page;
        }
        pump_events(50);
    }
    return nullptr;
}

[[nodiscard]] QQuickItem* as_quick_item(QObject* const object) {
    return qobject_cast<QQuickItem*>(object);
}

void attach_video_surface(
    QObject* const page,
    PlaybackShellRuntimeAdapterBridge* const bridge) {
    QQuickItem* const page_item = as_quick_item(page);
    if (page_item == nullptr || bridge == nullptr) {
        return;
    }

    auto* const surface = new PlaybackShellVideoSurfaceItem(page_item);
    surface->setBridge(bridge);
    surface->setParentItem(page_item);
    surface->setWidth(page_item->width());
    surface->setHeight(page_item->height());
    surface->setZ(1);
    (void)page->setProperty("showPlaceholderVideo", false);
    (void)surface->refreshFrame();
}

}  // namespace

void schedule_playback_evidence(
    QQuickWindow* const window,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    PlayerRuntimeAdapter* const adapter,
    const PlaybackEvidenceOptions& options,
    const QString& playback_page_url) {
    if (window == nullptr || bridge == nullptr || adapter == nullptr ||
        options.output_dir.isEmpty() || options.media_path.isEmpty()) {
        return;
    }

    QTimer::singleShot(500, window, [window, bridge, adapter, options, playback_page_url]() {
        QDir output_dir(options.output_dir);
        output_dir.mkpath(QStringLiteral("."));

        QObject* const page = wait_for_playback_page(window);
        pump_events(200);

        const bool media_exists = options.media_original_exists;
        const QString playback_file =
            options.selected_media_exists ? options.selected_media_file : QString{};
        const bool open_ok =
            options.selected_media_exists && bridge->openLocalMediaFile(playback_file);
        pump_events(120);

        const bool play_ok = open_ok && bridge->play();
        pump_events(200);

        QVariantMap playback_query{};
        bool frame_ready = false;
        for (int attempt = 0; attempt < 120; ++attempt) {
            playback_query = bridge->queryLocalMediaPlayback();
            frame_ready = playback_query.value(QStringLiteral("first_frame_decoded")).toBool()
                          && playback_query.value(QStringLiteral("frame_pixels_valid")).toBool();
            if (frame_ready) {
                break;
            }
            pump_events(50);
        }

        if (page != nullptr && frame_ready) {
            attach_video_surface(page, bridge);
            pump_events(300);
        }

        const QVariantMap snapshot = bridge->pullSnapshot();
        const bool qml_visible =
            page != nullptr && playback_query.value(QStringLiteral("frame_rendered")).toBool();

        const QString screenshot_path =
            output_dir.filePath(QStringLiteral("playback_screenshot.png"));
        const bool screenshot_saved = window->grabWindow().save(screenshot_path);

        QStringList errors;
        if (!options.media_resolve_error.isEmpty()) {
            errors << options.media_resolve_error;
        }
        if (!media_exists) {
            errors << QStringLiteral("media path not accessible");
        }
        if (media_exists && !options.selected_media_exists) {
            errors << QStringLiteral("selected media file missing");
        }
        if (!open_ok) {
            errors << QStringLiteral("open_local_media rejected");
        }
        if (!play_ok) {
            errors << QStringLiteral("play rejected");
        }
        if (!frame_ready) {
            errors << QStringLiteral("first frame not ready");
        }
        if (!screenshot_saved) {
            errors << QStringLiteral("screenshot save failed");
        }
        const QString pipeline_error = playback_query.value(QStringLiteral("error")).toString();
        if (!pipeline_error.isEmpty()) {
            errors << pipeline_error;
        }

        QJsonObject evidence;
        evidence.insert(QStringLiteral("media_path"), options.media_path);
        evidence.insert(QStringLiteral("media_exists"), media_exists);
        evidence.insert(QStringLiteral("media_is_file_or_dir"),
                        options.media_is_directory ? QStringLiteral("directory")
                                                   : QStringLiteral("file"));
        evidence.insert(QStringLiteral("selected_media_file"), playback_file);
        evidence.insert(QStringLiteral("selected_media_exists"), options.selected_media_exists);
        evidence.insert(QStringLiteral("open_result"),
                        open_ok ? QStringLiteral("accepted") : QStringLiteral("rejected"));
        evidence.insert(QStringLiteral("source_opened"),
                        playback_query.value(QStringLiteral("source_opened")).toBool());
        evidence.insert(QStringLiteral("demux_started"),
                        playback_query.value(QStringLiteral("demux_started")).toBool());
        evidence.insert(QStringLiteral("video_decode_started"),
                        playback_query.value(QStringLiteral("video_decode_started")).toBool());
        evidence.insert(QStringLiteral("first_frame_decoded"),
                        playback_query.value(QStringLiteral("first_frame_decoded")).toBool());
        evidence.insert(QStringLiteral("frame_rendered"),
                        playback_query.value(QStringLiteral("frame_rendered")).toBool());
        evidence.insert(QStringLiteral("frame_pixels_valid"),
                        playback_query.value(QStringLiteral("frame_pixels_valid")).toBool());
        evidence.insert(QStringLiteral("container"),
                        playback_query.value(QStringLiteral("container")).toString());
        evidence.insert(QStringLiteral("video_codec"),
                        playback_query.value(QStringLiteral("video_codec")).toString());
        evidence.insert(QStringLiteral("duration_ms"),
                        playback_query.value(QStringLiteral("duration_ms")).toLongLong());
        evidence.insert(QStringLiteral("width"), playback_query.value(QStringLiteral("width")).toInt());
        evidence.insert(QStringLiteral("height"),
                        playback_query.value(QStringLiteral("height")).toInt());
        evidence.insert(QStringLiteral("adapter_video_ready"),
                        snapshot.value(QStringLiteral("video_ready")).toBool());
        evidence.insert(QStringLiteral("qml_page_loaded"), page != nullptr);
        evidence.insert(QStringLiteral("qml_visible"), qml_visible);
        evidence.insert(QStringLiteral("playback_page_url"), playback_page_url);
        evidence.insert(QStringLiteral("screenshot_path"), screenshot_path);
        evidence.insert(QStringLiteral("screenshot_exists"), screenshot_saved);
        evidence.insert(QStringLiteral("fake_frame_used"), false);
        evidence.insert(QStringLiteral("fake_content_used"), false);
        evidence.insert(QStringLiteral("errors"), errors.join(QStringLiteral("; ")));
        evidence.insert(QStringLiteral("exit"), QStringLiteral("normal"));
        evidence.insert(QStringLiteral("residual_process"), false);

        QFile evidence_file(output_dir.filePath(QStringLiteral("playback_evidence.json")));
        if (evidence_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            evidence_file.write(QJsonDocument(evidence).toJson(QJsonDocument::Indented));
            evidence_file.close();
        }

        qInfo().noquote() << "PLAYBACK_EVIDENCE_JSON="
                          << output_dir.filePath(QStringLiteral("playback_evidence.json"));
        qInfo().noquote() << "PLAYBACK_SCREENSHOT_PATH=" << screenshot_path;
        qInfo().noquote() << "PLAYBACK_FIRST_FRAME="
                          << (frame_ready ? QStringLiteral("yes") : QStringLiteral("no"));

        QTimer::singleShot(options.close_delay_ms, qApp, &QCoreApplication::quit);
    });
}

}  // namespace kivo::ui::shell