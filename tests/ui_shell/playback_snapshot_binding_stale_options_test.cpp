#include <QGuiApplication>
#include <QMetaObject>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QVariantList>
#include <QVariantMap>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

QVariantMap base_snapshot() {
    QVariantMap snapshot;
    snapshot.insert(QStringLiteral("positionMs"), 0);
    snapshot.insert(QStringLiteral("durationMs"), 90000);
    snapshot.insert(QStringLiteral("isPaused"), true);
    snapshot.insert(QStringLiteral("playbackState"), QStringLiteral("Ready"));
    snapshot.insert(QStringLiteral("volume"), 1.0);
    snapshot.insert(QStringLiteral("muted"), false);
    snapshot.insert(QStringLiteral("subtitleOptions"), QVariantList{});
    snapshot.insert(QStringLiteral("audioTrackOptions"), QVariantList{});
    snapshot.insert(QStringLiteral("versionOptions"), QVariantList{});
    snapshot.insert(QStringLiteral("versionTechnicalRows"), QVariantList{});
    snapshot.insert(QStringLiteral("subtitleSize"), 1.0);
    snapshot.insert(QStringLiteral("playbackSpeed"), 1.0);
    snapshot.insert(QStringLiteral("aspectMode"), QStringLiteral("fit"));
    snapshot.insert(QStringLiteral("toneMappingMode"), QStringLiteral("auto"));
    return snapshot;
}

QVariantMap episode(const QString& id, const QString& name, const bool current) {
    QVariantMap item;
    item.insert(QStringLiteral("episodeId"), id);
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("current"), current);
    return item;
}

QVariantMap version_item(const QString& id, const QString& name) {
    QVariantMap item;
    item.insert(QStringLiteral("versionId"), id);
    item.insert(QStringLiteral("name"), name);
    return item;
}

QVariantMap technical_row(const QString& label, const QString& value) {
    QVariantMap item;
    item.insert(QStringLiteral("label"), label);
    item.insert(QStringLiteral("value"), value);
    return item;
}

bool invoke_apply(QObject* const root, const QVariantMap& snapshot) {
    QVariant ok;
    const bool invoked = QMetaObject::invokeMethod(
        root,
        "apply",
        Q_RETURN_ARG(QVariant, ok),
        Q_ARG(QVariant, QVariant::fromValue(snapshot)));
    return invoked && ok.toBool();
}

int episode_count(QObject* const target) {
    return target->property("episodeOptions").toList().size();
}

int version_count(QObject* const target) {
    return target->property("versionOptions").toList().size();
}

int version_technical_row_count(QObject* const target) {
    return target->property("versionTechnicalRows").toList().size();
}

}  // namespace

int main(int argc, char* argv[]) {
    kivo::ui::shell::bootstrap_qt_platform_paths(argv[0]);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"qml(
import QtQuick 6.5
import "qrc:/kivo/ui/playback/input"

Item {
    id: root

    property alias targetObject: target

    QtObject {
        id: fakeAdapter
    }

    QtObject {
        id: fakeDispatch
        property var snapshot: ({})
        function pullSnapshot(adapter) {
            return snapshot
        }
    }

    QtObject {
        id: target
        property int currentTimeMs: 0
        property int durationMs: 0
        property real buffered: 0
        property bool isPaused: false
        property bool isBuffering: false
        property bool isOpening: false
        property string bufferReason: "none"
        property int rebufferCount: 0
        property string playbackState: ""
        property bool isRecovering: false
        property bool hasError: false
        property bool fullscreenLike: false
        property bool autoPlayNext: false
        property bool playlistQueueConnected: false
        property bool playlistHasPrevious: false
        property bool playlistHasNext: false
        property bool playlistHasNextEpisode: false
        property var episodeOptions: [{ "episodeId": "stale", "name": "Stale" }]
        property string currentEpisode: "stale"
        property var versionOptions: [{ "versionId": "stale", "name": "Stale" }]
        property var versionTechnicalRows: [{ "label": "Codec", "value": "stale" }]
        property real volume: 1.0
        property bool isMuted: false
        property string subtitlePrimary: ""
        property string subtitleSecondary: ""
        property bool subtitleEnabled: false
        property var selectedSubtitle: ""
        property var subtitleOptions: []
        property int subtitleDelayMs: 0
        property var audioTrackLabel: ""
        property var audioTrackOptions: []
        property int audioDelayMs: 0
        property real subtitleSize: 1.0
        property real playbackSpeed: 1.0
        property string aspectMode: "fit"
        property string toneMappingMode: "auto"
    }

    PlaybackSnapshotBinding {
        id: binding
        runtimeAdapter: fakeAdapter
        target: target
        dispatch: fakeDispatch
    }

    function apply(snapshot) {
        fakeDispatch.snapshot = snapshot
        binding.refresh()
        return true
    }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackSnapshotBindingHarness.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }

    QObject* const target = root->property("targetObject").value<QObject*>();
    if (target == nullptr) {
        return 3;
    }

    QVariantMap with_episode = base_snapshot();
    with_episode.insert(QStringLiteral("episodeOptions"),
                        QVariantList{episode(QStringLiteral("1"), QStringLiteral("Episode 1"), true)});
    with_episode.insert(QStringLiteral("currentEpisode"), QStringLiteral("1"));
    with_episode.insert(QStringLiteral("versionOptions"),
                        QVariantList{version_item(QStringLiteral("uhd"), QStringLiteral("UHD"))});
    with_episode.insert(QStringLiteral("versionTechnicalRows"),
                        QVariantList{technical_row(QStringLiteral("Codec"), QStringLiteral("HEVC"))});
    if (!invoke_apply(root.get(), with_episode)) {
        return 4;
    }
    if (episode_count(target) != 1 ||
        target->property("currentEpisode").toString() != QStringLiteral("1") ||
        version_count(target) != 1 || version_technical_row_count(target) != 1) {
        return 5;
    }

    QVariantMap without_episode = base_snapshot();
    without_episode.insert(QStringLiteral("episodeOptions"), QVariantList{});
    without_episode.insert(QStringLiteral("currentEpisode"), QStringLiteral("7"));
    without_episode.insert(QStringLiteral("versionOptions"), QVariantList{});
    without_episode.insert(QStringLiteral("versionTechnicalRows"),
                           QVariantList{technical_row(QStringLiteral("Codec"),
                                                      QStringLiteral("ShouldClear"))});
    if (!invoke_apply(root.get(), without_episode)) {
        return 6;
    }
    if (episode_count(target) != 0 || !target->property("currentEpisode").toString().isEmpty() ||
        version_count(target) != 0 || version_technical_row_count(target) != 0) {
        return 7;
    }

    return 0;
}
