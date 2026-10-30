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
import "qrc:/kivo/ui/playback/input"
import "qrc:/kivo/ui/playback/states"

Item {
    id: root

    property int subtitleRequests: 0
    property alias overlayVisible: overlayState.overlayVisible
    property alias playbackAvailable: controller.playbackAvailable
    property alias subtitleAvailable: controller.subtitleAvailable

    PlaybackOverlayState {
        id: overlayState
    }

    PlaybackInputController {
        id: controller
        overlayState: overlayState
        onRequestSubtitleSheet: root.subtitleRequests += 1
    }

    function pressSubtitle() {
        var event = { key: Qt.Key_S, accepted: false }
        return controller.handle(event)
    }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackInputHiddenShortcut.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }

    if (invoke_bool(root.get(), "pressSubtitle")) {
        return 3;
    }
    if (bool_property(root.get(), "overlayVisible") ||
        int_property(root.get(), "subtitleRequests") != 0) {
        return 4;
    }

    root->setProperty("subtitleAvailable", true);
    if (invoke_bool(root.get(), "pressSubtitle")) {
        return 5;
    }
    if (bool_property(root.get(), "overlayVisible") ||
        int_property(root.get(), "subtitleRequests") != 0) {
        return 6;
    }

    root->setProperty("playbackAvailable", true);
    if (!invoke_bool(root.get(), "pressSubtitle")) {
        return 7;
    }
    if (!bool_property(root.get(), "overlayVisible") ||
        int_property(root.get(), "subtitleRequests") != 1) {
        return 8;
    }

    return 0;
}
