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

QObject* overlay_state(QObject* const page) {
    return page == nullptr ? nullptr : page->property("overlayState").value<QObject*>();
}

bool invoke_bool(QObject* const object, const char* const method) {
    QVariant result;
    const bool invoked =
        QMetaObject::invokeMethod(object, method, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

bool invoke_no_arg(QObject* const object, const char* const method) {
    return object != nullptr && QMetaObject::invokeMethod(object, method, Qt::DirectConnection);
}

bool invoke_sheet(QObject* const state, const QString& sheet) {
    return state != nullptr &&
           QMetaObject::invokeMethod(state,
                                     "openSheet",
                                     Qt::DirectConnection,
                                     Q_ARG(QVariant, sheet));
}

bool invoke_buffering(QObject* const state, const int seconds) {
    return state != nullptr &&
           QMetaObject::invokeMethod(state,
                                     "setBuffering",
                                     Qt::DirectConnection,
                                     Q_ARG(QVariant, seconds));
}

int int_property(QObject* const object, const char* const name) {
    return object == nullptr ? -1 : object->property(name).toInt();
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
import "qrc:/kivo/ui/playback/page"

PlaybackPage {
    id: page
    width: 960
    height: 540

    property int fullscreenDispatches: 0

    onPlaybackActionRequested: function(actionName) {
        if (actionName === "adapterMissing:enterFullscreen"
                || actionName === "adapterMissing:exitFullscreen")
            fullscreenDispatches += 1
    }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackFullscreenGestureStateGate.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> page(component.create());
    if (!page) {
        return 2;
    }

    QObject* const state = overlay_state(page.get());
    if (state == nullptr) {
        return 3;
    }

    if (invoke_bool(page.get(), "requestFullscreenGesture") ||
        int_property(page.get(), "fullscreenDispatches") != 0) {
        return 4;
    }

    page->setProperty("durationMs", 42000);
    if (!invoke_bool(page.get(), "requestFullscreenGesture") ||
        int_property(page.get(), "fullscreenDispatches") != 1) {
        return 12;
    }

    if (!invoke_sheet(state, QStringLiteral("settings"))) {
        return 5;
    }
    if (invoke_bool(page.get(), "requestFullscreenGesture") ||
        int_property(page.get(), "fullscreenDispatches") != 1) {
        return 6;
    }
    if (!invoke_no_arg(state, "closeSheet")) {
        return 7;
    }

    if (!invoke_buffering(state, 4)) {
        return 8;
    }
    if (invoke_bool(page.get(), "requestFullscreenGesture") ||
        int_property(page.get(), "fullscreenDispatches") != 1) {
        return 9;
    }
    if (!invoke_no_arg(state, "clearStatus")) {
        return 10;
    }

    page->setProperty("fullscreenLike", true);
    if (!invoke_bool(page.get(), "requestFullscreenGesture") ||
        int_property(page.get(), "fullscreenDispatches") != 2) {
        return 11;
    }

    return 0;
}
