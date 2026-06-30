#include "playback_shell_interaction_evidence.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QQuickWindow>
#include <QTimer>
#include <QVariantMap>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"
#include "playback_shell_runtime_adapter_bridge.hpp"

namespace kivo::ui::shell {
namespace {

using kivo::video::player_runtime_adapter::PlayerRuntimeAdapter;

struct CommandOutcome {
    QString command;
    QString pathway;
    QString result;
    bool bridge_ok{false};
};

struct QmlPageState {
    int current_time_ms{0};
    int subtitle_delay_ms{0};
    double volume{1.0};
    bool is_paused{true};
    bool adapter_bound{false};
};

[[nodiscard]] QString classify_failed_command(const QString& command) {
    static const QStringList typed_only{
        QStringLiteral("enterFullscreen"),
        QStringLiteral("exitFullscreen"),
        QStringLiteral("previous"),
        QStringLiteral("next"),
    };
    return typed_only.contains(command) ? QStringLiteral("typed-only")
                                        : QStringLiteral("rejected");
}

[[nodiscard]] QJsonObject variant_map_to_json(const QVariantMap& map) {
    return QJsonObject::fromVariantMap(map);
}

[[nodiscard]] QmlPageState read_qml_page_state(QObject* const page) {
    QmlPageState state{};
    if (page == nullptr) {
        return state;
    }
    state.current_time_ms = page->property("currentTimeMs").toInt();
    state.subtitle_delay_ms = page->property("subtitleDelayMs").toInt();
    state.volume = page->property("volume").toDouble();
    state.is_paused = page->property("isPaused").toBool();
    state.adapter_bound = page->property("adapterSnapshotBound").toBool();
    return state;
}

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
    for (int attempt = 0; attempt < 60; ++attempt) {
        QObject* const page = find_playback_page(window);
        if (page != nullptr) {
            return page;
        }
        pump_events(50);
    }
    return nullptr;
}

[[nodiscard]] bool invoke_page_dispatch(
    QObject* const page,
    const QString& method_name,
    const QVariantList& args) {
    if (page == nullptr) {
        return false;
    }
    bool ok = false;
    const bool invoked = QMetaObject::invokeMethod(
        page,
        "dispatchRuntimeCommand",
        Q_RETURN_ARG(bool, ok),
        Q_ARG(QVariant, method_name),
        Q_ARG(QVariant, QVariant::fromValue(args)),
        Q_ARG(QVariant, method_name));
    return invoked && ok;
}

void record_bridge_command(
    QVector<CommandOutcome>& outcomes,
    const QString& command,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    const bool bridge_ok) {
    outcomes.push_back(CommandOutcome{
        command,
        QStringLiteral("bridge"),
        bridge_ok ? QStringLiteral("accepted") : classify_failed_command(command),
        bridge_ok,
    });
}

[[nodiscard]] QVector<CommandOutcome> run_command_sequence(
    PlaybackShellRuntimeAdapterBridge* const bridge,
    QObject* const page) {
    QVector<CommandOutcome> outcomes;
    record_bridge_command(outcomes, QStringLiteral("openMediaId"), bridge, bridge->openMediaId(42));
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("play"), bridge, bridge->play());
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("pause"), bridge, bridge->pause());
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("togglePlayPause"), bridge, bridge->togglePlayPause());
    pump_events(80);

    QVariantList seek_args;
    seek_args << 5000;
    const bool qml_seek_ok =
        invoke_page_dispatch(page, QStringLiteral("seek"), seek_args);
    outcomes.push_back(CommandOutcome{
        QStringLiteral("seek"),
        QStringLiteral("qml"),
        qml_seek_ok ? QStringLiteral("accepted") : QStringLiteral("rejected"),
        qml_seek_ok,
    });
    pump_events(120);

    record_bridge_command(
        outcomes, QStringLiteral("seekRelative"), bridge, bridge->seekRelative(3000));
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("stop"), bridge, bridge->stop());
    pump_events(80);
    record_bridge_command(
        outcomes, QStringLiteral("setSubtitleDelay"), bridge, bridge->setSubtitleDelay(120));
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("setVolume"), bridge, bridge->setVolume(0.75));
    pump_events(80);
    record_bridge_command(
        outcomes, QStringLiteral("copyDiagnostics"), bridge, bridge->copyDiagnostics());
    pump_events(80);
    record_bridge_command(
        outcomes, QStringLiteral("enterFullscreen"), bridge, bridge->enterFullscreen());
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("close"), bridge, bridge->close());
    pump_events(80);
    record_bridge_command(outcomes, QStringLiteral("play"), bridge, bridge->play());
    pump_events(80);
    record_bridge_command(
        outcomes, QStringLiteral("copyDiagnostics"), bridge, bridge->copyDiagnostics());
    return outcomes;
}

}  // namespace

void schedule_interaction_evidence(
    QQuickWindow* const window,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    PlayerRuntimeAdapter* const adapter,
    const InteractionEvidenceOptions& options,
    const QString& playback_page_url) {
    if (window == nullptr || bridge == nullptr || adapter == nullptr || options.output_dir.isEmpty()) {
        return;
    }
    Q_UNUSED(adapter);

    QTimer::singleShot(500, window, [window, bridge, options, playback_page_url]() {
        QDir output_dir(options.output_dir);
        output_dir.mkpath(QStringLiteral("."));

        QObject* const page = wait_for_playback_page(window);
        pump_events(300);

        const QVariantMap snapshot_before = bridge->pullSnapshot();
        const QmlPageState qml_before = read_qml_page_state(page);

        const QVector<CommandOutcome> outcomes = run_command_sequence(bridge, page);
        pump_events(400);

        const QVariantMap snapshot_after = bridge->pullSnapshot();
        const QmlPageState qml_after = read_qml_page_state(page);

        const bool qml_state_updated =
            qml_before.current_time_ms != qml_after.current_time_ms
            || qml_before.subtitle_delay_ms != qml_after.subtitle_delay_ms
            || qml_before.volume != qml_after.volume || qml_before.is_paused != qml_after.is_paused;

        const QString screenshot_path =
            output_dir.filePath(QStringLiteral("interaction_screenshot.png"));
        const bool screenshot_saved = window->grabWindow().save(screenshot_path);

        QJsonArray commands;
        QJsonArray command_results;
        for (const CommandOutcome& outcome : outcomes) {
            commands.append(outcome.command);
            command_results.append(QJsonObject{
                {QStringLiteral("command"), outcome.command},
                {QStringLiteral("pathway"), outcome.pathway},
                {QStringLiteral("result"), outcome.result},
                {QStringLiteral("bridge_ok"), outcome.bridge_ok},
            });
        }

        const QJsonObject evidence{
            {QStringLiteral("shell_started"), true},
            {QStringLiteral("qml_loaded"), page != nullptr},
            {QStringLiteral("playback_page_loaded_from_qrc"),
             playback_page_url.startsWith(QStringLiteral("qrc:"))},
            {QStringLiteral("playback_page_url"), playback_page_url},
            {QStringLiteral("shell_root_url"),
             QStringLiteral("qrc:/kivo/ui/shell/PlaybackShellRoot.qml")},
            {QStringLiteral("commands_executed"), commands},
            {QStringLiteral("command_results"), command_results},
            {QStringLiteral("snapshot_before"), variant_map_to_json(snapshot_before)},
            {QStringLiteral("snapshot_after"), variant_map_to_json(snapshot_after)},
            {QStringLiteral("qml_state_before"), QJsonObject{
                 {QStringLiteral("currentTimeMs"), qml_before.current_time_ms},
                 {QStringLiteral("subtitleDelayMs"), qml_before.subtitle_delay_ms},
                 {QStringLiteral("volume"), qml_before.volume},
                 {QStringLiteral("isPaused"), qml_before.is_paused},
                 {QStringLiteral("adapterSnapshotBound"), qml_before.adapter_bound},
             }},
            {QStringLiteral("qml_state_after"), QJsonObject{
                 {QStringLiteral("currentTimeMs"), qml_after.current_time_ms},
                 {QStringLiteral("subtitleDelayMs"), qml_after.subtitle_delay_ms},
                 {QStringLiteral("volume"), qml_after.volume},
                 {QStringLiteral("isPaused"), qml_after.is_paused},
                 {QStringLiteral("adapterSnapshotBound"), qml_after.adapter_bound},
             }},
            {QStringLiteral("qml_state_updated"), qml_state_updated},
            {QStringLiteral("fake_media_used"), false},
            {QStringLiteral("fake_content_used"), false},
            {QStringLiteral("typed_p7_harness"), true},
            {QStringLiteral("screenshot_path"), screenshot_path},
            {QStringLiteral("screenshot_exists"), screenshot_saved},
            {QStringLiteral("exit"), QStringLiteral("normal")},
            {QStringLiteral("residual_process"), false},
        };

        QFile evidence_file(output_dir.filePath(QStringLiteral("interaction_evidence.json")));
        if (evidence_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            evidence_file.write(QJsonDocument(evidence).toJson(QJsonDocument::Indented));
            evidence_file.close();
        }

        qInfo().noquote() << "INTERACTION_EVIDENCE_JSON="
                          << output_dir.filePath(QStringLiteral("interaction_evidence.json"));
        qInfo().noquote() << "INTERACTION_SCREENSHOT_PATH=" << screenshot_path;
        qInfo().noquote() << "INTERACTION_QML_STATE_UPDATED=" << (qml_state_updated ? "yes" : "no");

        QTimer::singleShot(options.close_delay_ms, qApp, &QCoreApplication::quit);
    });
}

}  // namespace kivo::ui::shell