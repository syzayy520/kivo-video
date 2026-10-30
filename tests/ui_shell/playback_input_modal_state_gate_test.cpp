#include <QGuiApplication>
#include <QMetaObject>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QString>
#include <QVariant>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

bool invoke_bool(QObject* const object, const char* const method) {
    QVariant result;
    const bool invoked =
        QMetaObject::invokeMethod(object, method, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

int int_property(QObject* const object, const char* const name) {
    return object == nullptr ? -1 : object->property(name).toInt();
}

QString string_property(QObject* const object, const char* const name) {
    return object == nullptr ? QString{} : object->property(name).toString();
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
import "qrc:/kivo/ui/playback/states"

Item {
    id: root

    property int playRequests: 0
    property int skipRequests: 0
    property int volumeRequests: 0
    property int muteRequests: 0
    property int fullscreenRequests: 0
    property int openRequests: 0
    property int backRequests: 0
    property alias activeSheet: overlayState.activeSheet
    property alias statusKind: overlayState.statusKind
    property alias playbackAvailable: controller.playbackAvailable

    PlaybackOverlayState {
        id: overlayState
    }

    PlaybackInputController {
        id: controller
        overlayState: overlayState
        playbackAvailable: false
        seekAvailable: true
        subtitleAvailable: true
        onRequestPlayPause: root.playRequests += 1
        onRequestSkipForward: root.skipRequests += 1
        onRequestVolumeStep: root.volumeRequests += 1
        onRequestMuteToggle: root.muteRequests += 1
        onRequestFullscreenToggle: root.fullscreenRequests += 1
        onRequestOpenFile: root.openRequests += 1
        onRequestBack: root.backRequests += 1
    }

    function press(key) {
        var event = { key: key, accepted: false }
        return controller.handle(event)
    }

    function pressSpace() { return press(Qt.Key_Space) }
    function pressRight() { return press(Qt.Key_Right) }
    function pressUp() { return press(Qt.Key_Up) }
    function pressMute() { return press(Qt.Key_M) }
    function pressFullscreen() { return press(Qt.Key_F) }
    function pressOpen() { return press(Qt.Key_O) }
    function pressEscape() { return press(Qt.Key_Escape) }
    function pressSubtitle() { return press(Qt.Key_S) }
    function openSettingsSheet() { overlayState.openSheet("settings") }
    function setBuffering() { overlayState.setBuffering(4) }
    function clearStatus() { overlayState.clearStatus() }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackInputModalStateGate.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }

    if (invoke_bool(root.get(), "pressSpace") || invoke_bool(root.get(), "pressUp") ||
        invoke_bool(root.get(), "pressMute")) {
        return 3;
    }
    if (int_property(root.get(), "playRequests") != 0 ||
        int_property(root.get(), "volumeRequests") != 0 ||
        int_property(root.get(), "muteRequests") != 0) {
        return 4;
    }
    if (!invoke_bool(root.get(), "pressOpen") ||
        int_property(root.get(), "openRequests") != 1) {
        return 5;
    }
    if (!invoke_bool(root.get(), "pressFullscreen") ||
        int_property(root.get(), "fullscreenRequests") != 1) {
        return 6;
    }

    root->setProperty("playbackAvailable", true);
    if (!invoke_bool(root.get(), "pressSpace") ||
        int_property(root.get(), "playRequests") != 1) {
        return 7;
    }

    if (!QMetaObject::invokeMethod(root.get(), "openSettingsSheet")) {
        return 8;
    }
    if (string_property(root.get(), "activeSheet") != QStringLiteral("settings")) {
        return 9;
    }
    if (invoke_bool(root.get(), "pressSpace") || invoke_bool(root.get(), "pressRight") ||
        invoke_bool(root.get(), "pressUp") || invoke_bool(root.get(), "pressSubtitle")) {
        return 10;
    }
    if (int_property(root.get(), "playRequests") != 1 ||
        int_property(root.get(), "skipRequests") != 0 ||
        int_property(root.get(), "volumeRequests") != 0) {
        return 11;
    }
    if (!invoke_bool(root.get(), "pressEscape") ||
        !string_property(root.get(), "activeSheet").isEmpty()) {
        return 12;
    }

    if (!QMetaObject::invokeMethod(root.get(), "setBuffering") ||
        string_property(root.get(), "statusKind") != QStringLiteral("buffering")) {
        return 13;
    }
    if (invoke_bool(root.get(), "pressSpace") || invoke_bool(root.get(), "pressRight") ||
        invoke_bool(root.get(), "pressUp") || invoke_bool(root.get(), "pressSubtitle")) {
        return 14;
    }
    if (int_property(root.get(), "playRequests") != 1 ||
        int_property(root.get(), "skipRequests") != 0 ||
        int_property(root.get(), "volumeRequests") != 0) {
        return 15;
    }
    if (!invoke_bool(root.get(), "pressOpen") ||
        int_property(root.get(), "openRequests") != 2) {
        return 16;
    }
    if (!invoke_bool(root.get(), "pressEscape") ||
        int_property(root.get(), "backRequests") != 1) {
        return 17;
    }

    if (!QMetaObject::invokeMethod(root.get(), "clearStatus")) {
        return 18;
    }
    if (!invoke_bool(root.get(), "pressRight") ||
        int_property(root.get(), "skipRequests") != 1) {
        return 19;
    }

    return 0;
}
