#include <QGuiApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaObject>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QString>
#include <QThread>
#include <QVariant>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

QObject* state_model(QObject* const overlay) {
    return overlay == nullptr ? nullptr : overlay->property("stateModel").value<QObject*>();
}

bool bool_property(QObject* const object, const char* const name) {
    return object != nullptr && object->property(name).toBool();
}

QString string_property(QObject* const object, const char* const name) {
    return object == nullptr ? QString{} : object->property(name).toString();
}

bool invoke(QObject* const object, const char* const method) {
    return object != nullptr && QMetaObject::invokeMethod(object, method, Qt::DirectConnection);
}

bool invoke_set_buffering(QObject* const object) {
    return object != nullptr &&
           QMetaObject::invokeMethod(object,
                                     "setBuffering",
                                     Qt::DirectConnection,
                                     Q_ARG(QVariant, QVariant(3)));
}

bool invoke_set_error(QObject* const object) {
    return object != nullptr && QMetaObject::invokeMethod(object, "setError", Qt::DirectConnection);
}

void pump_events(const int milliseconds = 180) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < milliseconds) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    kivo::ui::shell::bootstrap_qt_platform_paths(argv[0]);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    QQmlEngine engine;

    QQmlComponent default_volume_component(&engine);
    default_volume_component.setData(R"qml(
import QtQuick 6.5
import "qrc:/kivo/ui/playback/overlay"

VolumeInlineControl {
    width: 132
    height: 42
}
)qml",
                                     QUrl(QStringLiteral("qrc:/kivo/tests/DefaultVolumeInlineControl.qml")));
    if (default_volume_component.isError()) {
        return 30;
    }
    QScopedPointer<QObject> default_volume(default_volume_component.create());
    if (!default_volume) {
        return 31;
    }
    QObject* const default_volume_button =
        default_volume->findChild<QObject*>(QStringLiteral("volumeButton"));
    if (default_volume_button == nullptr) {
        return 32;
    }
    if (bool_property(default_volume_button, "visible")) {
        return 33;
    }
    default_volume->setProperty("volumeAvailable", true);
    app.processEvents();
    if (!bool_property(default_volume_button, "visible")) {
        return 34;
    }

    QQmlComponent component(&engine);
    component.setData(R"qml(
import QtQuick 6.5
import "qrc:/kivo/ui/playback/overlay"

PlaybackOverlay {
    width: 960
    height: 540
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackStatusChromeVisibility.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> overlay(component.create());
    if (!overlay) {
        return 2;
    }

    QObject* const state = state_model(overlay.get());
    QObject* const bottom =
        overlay->findChild<QObject*>(QStringLiteral("bottomControlOverlay"));
    QObject* const top = overlay->findChild<QObject*>(QStringLiteral("topInfoOverlay"));
    QObject* const bottom_gradient =
        overlay->findChild<QObject*>(QStringLiteral("bottomGradientOverlay"));
    QObject* const top_gradient =
        overlay->findChild<QObject*>(QStringLiteral("topGradientOverlay"));
    QObject* const retry_button = overlay->findChild<QObject*>(QStringLiteral("retryButton"));
    QObject* const open_video_button =
        overlay->findChild<QObject*>(QStringLiteral("openVideoButton"));
    QObject* const error_detail =
        overlay->findChild<QObject*>(QStringLiteral("statusErrorDetailText"));
    QObject* const subtitle_overlay =
        overlay->findChild<QObject*>(QStringLiteral("subtitleOverlay"));
    QObject* const volume_control =
        overlay->findChild<QObject*>(QStringLiteral("volumeInlineControl"));
    QObject* const volume_button =
        overlay->findChild<QObject*>(QStringLiteral("volumeButton"));
    QObject* const tool_controls =
        overlay->findChild<QObject*>(QStringLiteral("playbackToolControls"));
    QObject* const fullscreen_button =
        overlay->findChild<QObject*>(QStringLiteral("fullscreenToolButton"));
    if (state == nullptr || bottom == nullptr || top == nullptr ||
        bottom_gradient == nullptr || top_gradient == nullptr || retry_button == nullptr ||
        open_video_button == nullptr || error_detail == nullptr || subtitle_overlay == nullptr ||
        volume_control == nullptr || volume_button == nullptr || tool_controls == nullptr ||
        fullscreen_button == nullptr) {
        return 3;
    }

    if (!invoke(state, "showVisible")) {
        return 4;
    }
    pump_events();
    if (!bool_property(overlay.get(), "chromeVisible")) {
        return 5;
    }
    if (bool_property(overlay.get(), "bottomChromeAvailable") ||
        bool_property(overlay.get(), "bottomChromeVisible") ||
        bool_property(bottom, "overlayVisible") ||
        bool_property(bottom_gradient, "overlayVisible")) {
        return 43;
    }
    if (bool_property(top, "overlayVisible") ||
        bool_property(top_gradient, "overlayVisible")) {
        return 28;
    }
    overlay->setProperty("title", QStringLiteral("Shelter"));
    app.processEvents();
    if (!bool_property(top, "overlayVisible") ||
        !bool_property(top_gradient, "overlayVisible")) {
        return 29;
    }
    if (bool_property(bottom, "timelineVisible") ||
        bool_property(bottom, "mediaControlsVisible")) {
        return 10;
    }
    if (bool_property(volume_button, "visible") ||
        bool_property(volume_control, "volumeAvailable")) {
        return 35;
    }
    if (bool_property(fullscreen_button, "visible")) {
        return 37;
    }
    if (bool_property(tool_controls, "fullscreenAvailable")) {
        return 41;
    }

    overlay->setProperty("playbackAvailable", true);
    pump_events();
    if (!bool_property(overlay.get(), "playbackAvailable")) {
        return 16;
    }
    if (!bool_property(overlay.get(), "bottomChromeAvailable") ||
        !bool_property(overlay.get(), "bottomChromeVisible") ||
        !bool_property(bottom, "overlayVisible") ||
        !bool_property(bottom_gradient, "overlayVisible")) {
        return 44;
    }
    if (!bool_property(bottom, "playbackAvailable")) {
        return 17;
    }
    if (bool_property(bottom, "timelineVisible")) {
        return 11;
    }
    if (!bool_property(bottom, "mediaControlsVisible")) {
        return 13;
    }
    if (bool_property(volume_button, "visible") ||
        bool_property(volume_control, "volumeAvailable")) {
        return 36;
    }
    overlay->setProperty("muted", true);
    pump_events();
    if (!bool_property(volume_control, "volumeAvailable") ||
        !bool_property(volume_button, "visible")) {
        return 48;
    }
    overlay->setProperty("muted", false);
    overlay->setProperty("volume", 0.75);
    pump_events();
    if (!bool_property(volume_control, "volumeAvailable") ||
        !bool_property(volume_button, "visible")) {
        return 49;
    }
    overlay->setProperty("volume", 1.0);
    pump_events();
    if (bool_property(volume_button, "visible") ||
        bool_property(volume_control, "volumeAvailable")) {
        return 50;
    }
    if (!bool_property(tool_controls, "fullscreenAvailable")) {
        return 42;
    }
    if (!bool_property(fullscreen_button, "visible")) {
        return 38;
    }

    overlay->setProperty("duration", 60.0);
    app.processEvents();
    if (!bool_property(bottom, "timelineVisible")) {
        return 12;
    }
    overlay->setProperty("primarySubtitle", QStringLiteral("Visible line"));
    app.processEvents();
    if (!bool_property(subtitle_overlay, "subtitleVisible")) {
        return 24;
    }

    overlay->setProperty("playbackAvailable", false);
    pump_events();
    if (bool_property(overlay.get(), "seekAvailable") ||
        bool_property(overlay.get(), "bottomChromeAvailable") ||
        bool_property(overlay.get(), "bottomChromeVisible") ||
        bool_property(bottom, "timelineVisible") ||
        bool_property(bottom, "overlayVisible") ||
        bool_property(bottom_gradient, "overlayVisible")) {
        return 45;
    }
    if (bool_property(fullscreen_button, "visible")) {
        return 39;
    }
    overlay->setProperty("fullscreen", true);
    pump_events();
    if (!bool_property(overlay.get(), "bottomChromeAvailable") ||
        !bool_property(overlay.get(), "bottomChromeVisible") ||
        !bool_property(bottom, "overlayVisible") ||
        !bool_property(bottom_gradient, "overlayVisible")) {
        return 46;
    }
    if (!bool_property(fullscreen_button, "visible")) {
        return 40;
    }
    overlay->setProperty("fullscreen", false);
    pump_events();
    if (bool_property(overlay.get(), "bottomChromeAvailable") ||
        bool_property(overlay.get(), "bottomChromeVisible") ||
        bool_property(bottom, "overlayVisible") ||
        bool_property(bottom_gradient, "overlayVisible")) {
        return 47;
    }
    if (!invoke_set_error(state)) {
        return 18;
    }
    app.processEvents();
    if (bool_property(retry_button, "visible") ||
        !bool_property(open_video_button, "visible")) {
        return 19;
    }
    if (bool_property(subtitle_overlay, "subtitleVisible")) {
        return 25;
    }
    overlay->setProperty("errorMessage", QStringLiteral("Playback unavailable"));
    app.processEvents();
    if (bool_property(error_detail, "visible")) {
        return 22;
    }

    overlay->setProperty("errorMessage", QStringLiteral("Codec unavailable"));
    app.processEvents();
    if (!bool_property(error_detail, "visible") ||
        string_property(error_detail, "text") != QStringLiteral("Codec unavailable")) {
        return 23;
    }

    overlay->setProperty("playbackAvailable", true);
    app.processEvents();
    if (!bool_property(retry_button, "visible") ||
        bool_property(open_video_button, "visible")) {
        return 20;
    }

    if (!invoke(state, "clearStatus")) {
        return 21;
    }
    app.processEvents();
    if (!bool_property(subtitle_overlay, "subtitleVisible")) {
        return 26;
    }

    if (!invoke_set_buffering(state)) {
        return 6;
    }
    if (bool_property(overlay.get(), "chromeVisible") ||
        bool_property(bottom, "overlayVisible") ||
        bool_property(bottom_gradient, "overlayVisible") ||
        bool_property(top, "overlayVisible") ||
        bool_property(top_gradient, "overlayVisible")) {
        return 7;
    }
    if (bool_property(subtitle_overlay, "subtitleVisible")) {
        return 27;
    }

    if (!invoke(state, "clearStatus")) {
        return 8;
    }
    if (!bool_property(overlay.get(), "chromeVisible") ||
        !bool_property(bottom, "overlayVisible") ||
        !bool_property(bottom_gradient, "overlayVisible") ||
        !bool_property(top, "overlayVisible") ||
        !bool_property(top_gradient, "overlayVisible")) {
        return 9;
    }

    return 0;
}
