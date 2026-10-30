#include <QGuiApplication>
#include <QMetaObject>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
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

bool bool_property(QObject* const object, const char* const name) {
    return object != nullptr && object->property(name).toBool();
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
import QtQuick.Controls 6.5
import "qrc:/kivo/ui/playback/input"
import "qrc:/kivo/ui/playback/overlay"
import "qrc:/kivo/ui/playback/menu"

ApplicationWindow {
    id: root
    width: 960
    height: 540
    visible: true

    property int backwardRequests: 0
    property int forwardRequests: 0
    property alias inputSeekAvailable: input.seekAvailable
    property alias controlsPlaybackAvailable: controls.playbackAvailable
    property alias controlsSeekAvailable: controls.seekAvailable
    property alias menuSeekAvailable: contextMenu.seekAvailable

    PlaybackInputController {
        id: input
        onRequestSkipBackward: root.backwardRequests += 1
        onRequestSkipForward: root.forwardRequests += 1
    }

    TransportControls {
        id: controls
        onRewindRequested: root.backwardRequests += 10
        onForwardRequested: root.forwardRequests += 10
    }

    PlaybackContextMenu {
        id: contextMenu
    }

    function pressLeft() {
        var event = { key: Qt.Key_Left, accepted: false }
        return input.handle(event)
    }

    function pressRight() {
        var event = { key: Qt.Key_Right, accepted: false }
        return input.handle(event)
    }

    function openContextMenu() {
        contextMenu.popup(120, 120)
    }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackSeekAvailability.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }

    QObject* const rewind_button = root->findChild<QObject*>(QStringLiteral("rewindButton"));
    QObject* const play_pause_button =
        root->findChild<QObject*>(QStringLiteral("playPauseButton"));
    QObject* const forward_button = root->findChild<QObject*>(QStringLiteral("forwardButton"));
    QObject* const skip_backward_menu =
        root->findChild<QObject*>(QStringLiteral("skipBackwardMenuItem"));
    QObject* const skip_forward_menu =
        root->findChild<QObject*>(QStringLiteral("skipForwardMenuItem"));
    if (rewind_button == nullptr || play_pause_button == nullptr || forward_button == nullptr ||
        skip_backward_menu == nullptr || skip_forward_menu == nullptr) {
        return 3;
    }

    if (invoke_bool(root.get(), "pressLeft") || invoke_bool(root.get(), "pressRight")) {
        return 4;
    }
    if (int_property(root.get(), "backwardRequests") != 0 ||
        int_property(root.get(), "forwardRequests") != 0) {
        return 5;
    }
    if (bool_property(rewind_button, "visible") || bool_property(play_pause_button, "visible") ||
        bool_property(forward_button, "visible") ||
        bool_property(skip_backward_menu, "visible") ||
        bool_property(skip_forward_menu, "visible")) {
        return 6;
    }

    root->setProperty("controlsPlaybackAvailable", true);
    app.processEvents();
    if (!bool_property(play_pause_button, "visible")) {
        return 10;
    }

    root->setProperty("inputSeekAvailable", true);
    root->setProperty("controlsSeekAvailable", true);
    root->setProperty("menuSeekAvailable", true);
    QMetaObject::invokeMethod(root.get(), "openContextMenu");
    app.processEvents();
    if (!invoke_bool(root.get(), "pressLeft") || !invoke_bool(root.get(), "pressRight")) {
        return 7;
    }
    if (int_property(root.get(), "backwardRequests") != 1 ||
        int_property(root.get(), "forwardRequests") != 1) {
        return 8;
    }
    if (!bool_property(rewind_button, "visible") || !bool_property(forward_button, "visible") ||
        !bool_property(skip_backward_menu, "visible") ||
        !bool_property(skip_forward_menu, "visible")) {
        return 9;
    }

    return 0;
}
