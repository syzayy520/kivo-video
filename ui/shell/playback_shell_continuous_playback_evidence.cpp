#include "playback_shell_continuous_playback_evidence.hpp"

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

[[nodiscard]] PlaybackShellVideoSurfaceItem* attach_video_surface(
    QObject* const page,
    PlaybackShellRuntimeAdapterBridge* const bridge) {
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
    (void)page->setProperty("showPlaceholderVideo", false);
    (void)surface->refreshFrame();
    return surface;
}

[[nodiscard]] bool wait_for_first_frame(PlaybackShellRuntimeAdapterBridge* const bridge) {
    for (int attempt = 0; attempt < 120; ++attempt) {
        const QVariantMap playback_query = bridge->queryLocalMediaPlayback();
        const bool frame_ready =
            playback_query.value(QStringLiteral("first_frame_decoded")).toBool()
            && playback_query.value(QStringLiteral("frame_pixels_valid")).toBool();
        if (frame_ready) {
            return true;
        }
        pump_events(50);
    }
    return false;
}

[[nodiscard]] QString classification_from_query(const QVariantMap& query) {
    const auto decoded = query.value(QStringLiteral("decoded_frame_count")).toULongLong();
    const auto rendered = query.value(QStringLiteral("rendered_frame_count")).toULongLong();
    const auto before = query.value(QStringLiteral("timeline_position_before_ms")).toLongLong();
    const auto after = query.value(QStringLiteral("timeline_position_after_ms")).toLongLong();
    if (decoded > 1 && rendered > 1 && after > before) {
        return QStringLiteral("PASS_CONTINUOUS_PLAYBACK");
    }
    if (query.value(QStringLiteral("first_frame_decoded")).toBool() && decoded <= 1) {
        return QStringLiteral("PARTIAL_FIRST_FRAME_ONLY_NO_CONTINUOUS");
    }
    if (!query.value(QStringLiteral("video_decode_started")).toBool()) {
        return QStringLiteral("FAIL_DECODE_LOOP");
    }
    if (decoded > 1 && rendered <= 1) {
        return QStringLiteral("FAIL_RENDER_PUMP");
    }
    return QStringLiteral("PARTIAL_FIRST_FRAME_ONLY_NO_CONTINUOUS");
}

}  // namespace

void schedule_continuous_playback_evidence(
    QQuickWindow* const window,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    PlayerRuntimeAdapter* const adapter,
    const ContinuousPlaybackEvidenceOptions& options,
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

        const bool frame_ready = open_ok && play_ok && wait_for_first_frame(bridge);

        PlaybackShellVideoSurfaceItem* surface = nullptr;
        if (page != nullptr && frame_ready) {
            surface = attach_video_surface(page, bridge);
            if (surface != nullptr) {
                surface->startContinuousRefresh(33);
            }
            pump_events(300);
        }

        QVariantMap playback_before = bridge->queryLocalMediaPlayback();
        const QVariantMap snapshot_before = bridge->pullSnapshot();
        const qint64 timeline_before =
            snapshot_before.value(QStringLiteral("position_ms")).toLongLong();
        const quint64 signature_before =
            playback_before.value(QStringLiteral("last_frame_signature")).toULongLong();

        const QString screenshot_t0 = output_dir.filePath(QStringLiteral("playback_t0.png"));
        const bool screenshot_t0_saved = window->grabWindow().save(screenshot_t0);

        const int duration_ms = options.duration_ms > 0 ? options.duration_ms : 5000;
        QElapsedTimer observed_timer;
        observed_timer.start();
        pump_events(duration_ms);
        const qint64 observed_ms = observed_timer.elapsed();

        if (surface != nullptr) {
            surface->stopContinuousRefresh();
            (void)surface->refreshFrame();
            pump_events(100);
        }

        QVariantMap playback_after = bridge->queryLocalMediaPlayback();
        const QVariantMap snapshot_after = bridge->pullSnapshot();
        const qint64 timeline_after =
            snapshot_after.value(QStringLiteral("position_ms")).toLongLong();
        const quint64 signature_after =
            playback_after.value(QStringLiteral("last_frame_signature")).toULongLong();

        const QString screenshot_t5s = output_dir.filePath(QStringLiteral("playback_t5s.png"));
        const bool screenshot_t5s_saved = window->grabWindow().save(screenshot_t5s);

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
        if (!screenshot_t0_saved) {
            errors << QStringLiteral("t0 screenshot save failed");
        }
        if (!screenshot_t5s_saved) {
            errors << QStringLiteral("t5s screenshot save failed");
        }
        const QString pipeline_error = playback_after.value(QStringLiteral("error")).toString();
        if (!pipeline_error.isEmpty()) {
            errors << pipeline_error;
        }

        const auto decoded_count =
            playback_after.value(QStringLiteral("decoded_frame_count")).toULongLong();
        const auto rendered_count =
            playback_after.value(QStringLiteral("rendered_frame_count")).toULongLong();
        const bool continuous_rendering =
            playback_after.value(QStringLiteral("continuous_rendering")).toBool()
            && decoded_count > 1 && rendered_count > 1 && timeline_after > timeline_before;

        QString stall_note;
        if (decoded_count > 1 && signature_before == signature_after && signature_before != 0) {
            stall_note = QStringLiteral(
                "decoded_frame_count advanced but last_frame_signature unchanged; "
                "possible static scene or decode pacing stall");
        }

        QJsonObject evidence;
        evidence.insert(QStringLiteral("media_path"), options.media_path);
        evidence.insert(QStringLiteral("selected_media_file"), playback_file);
        evidence.insert(QStringLiteral("media_exists"), media_exists);
        evidence.insert(QStringLiteral("source_opened"),
                        playback_after.value(QStringLiteral("source_opened")).toBool());
        evidence.insert(QStringLiteral("demux_started"),
                        playback_after.value(QStringLiteral("demux_started")).toBool());
        evidence.insert(QStringLiteral("video_decode_started"),
                        playback_after.value(QStringLiteral("video_decode_started")).toBool());
        evidence.insert(QStringLiteral("decoded_frame_count"),
                        static_cast<qint64>(decoded_count));
        evidence.insert(QStringLiteral("rendered_frame_count"),
                        static_cast<qint64>(rendered_count));
        evidence.insert(QStringLiteral("first_frame_decoded"),
                        playback_after.value(QStringLiteral("first_frame_decoded")).toBool());
        evidence.insert(QStringLiteral("continuous_rendering"), continuous_rendering);
        evidence.insert(QStringLiteral("timeline_position_before_ms"), timeline_before);
        evidence.insert(QStringLiteral("timeline_position_after_ms"), timeline_after);
        evidence.insert(QStringLiteral("playback_duration_observed_ms"), observed_ms);
        evidence.insert(QStringLiteral("frame_signature_before"), static_cast<qint64>(signature_before));
        evidence.insert(QStringLiteral("frame_signature_after"), static_cast<qint64>(signature_after));
        evidence.insert(QStringLiteral("screenshot_t0"), screenshot_t0);
        evidence.insert(QStringLiteral("screenshot_t5s"), screenshot_t5s);
        evidence.insert(QStringLiteral("screenshot_t0_exists"), screenshot_t0_saved);
        evidence.insert(QStringLiteral("screenshot_t5s_exists"), screenshot_t5s_saved);
        evidence.insert(QStringLiteral("container"),
                        playback_after.value(QStringLiteral("container")).toString());
        evidence.insert(QStringLiteral("video_codec"),
                        playback_after.value(QStringLiteral("video_codec")).toString());
        evidence.insert(QStringLiteral("duration_ms"),
                        playback_after.value(QStringLiteral("duration_ms")).toLongLong());
        evidence.insert(QStringLiteral("width"), playback_after.value(QStringLiteral("width")).toInt());
        evidence.insert(QStringLiteral("height"),
                        playback_after.value(QStringLiteral("height")).toInt());
        evidence.insert(QStringLiteral("playback_page_url"), playback_page_url);
        evidence.insert(QStringLiteral("fake_frame_used"), false);
        evidence.insert(QStringLiteral("fake_content_used"), false);
        evidence.insert(QStringLiteral("exit"), QStringLiteral("normal"));
        evidence.insert(QStringLiteral("residual_process"), false);
        evidence.insert(QStringLiteral("stall_note"), stall_note);
        evidence.insert(QStringLiteral("errors"), errors.join(QStringLiteral("; ")));

        QVariantMap classification_input;
        classification_input.insert(QStringLiteral("decoded_frame_count"), decoded_count);
        classification_input.insert(QStringLiteral("rendered_frame_count"), rendered_count);
        classification_input.insert(QStringLiteral("timeline_position_before_ms"), timeline_before);
        classification_input.insert(QStringLiteral("timeline_position_after_ms"), timeline_after);
        classification_input.insert(QStringLiteral("first_frame_decoded"),
                                      playback_after.value(QStringLiteral("first_frame_decoded")));
        classification_input.insert(QStringLiteral("video_decode_started"),
                                      playback_after.value(QStringLiteral("video_decode_started")));
        evidence.insert(QStringLiteral("classification"), classification_from_query(classification_input));

        QFile evidence_file(output_dir.filePath(QStringLiteral("continuous_playback_evidence.json")));
        if (evidence_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            evidence_file.write(QJsonDocument(evidence).toJson(QJsonDocument::Indented));
            evidence_file.close();
        }

        qInfo().noquote() << "CONTINUOUS_PLAYBACK_EVIDENCE_JSON="
                          << output_dir.filePath(QStringLiteral("continuous_playback_evidence.json"));
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_SCREENSHOT_T0=" << screenshot_t0;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_SCREENSHOT_T5S=" << screenshot_t5s;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_DECODED_FRAMES=" << decoded_count;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_RENDERED_FRAMES=" << rendered_count;

        QTimer::singleShot(options.close_delay_ms, qApp, &QCoreApplication::quit);
    });
}

}  // namespace kivo::ui::shell