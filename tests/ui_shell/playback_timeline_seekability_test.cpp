#include <QGuiApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QString>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

bool property_bool(QObject* const object, const char* const name) {
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
import "qrc:/kivo/ui/playback/overlay"

PlaybackTimeline {
    width: 720
    duration: 0
    position: 0
    bufferedPosition: 0
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackTimelineSeekability.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> timeline(component.create());
    if (!timeline) {
        return 2;
    }

    QObject* const mouse_area = timeline->findChild<QObject*>(QStringLiteral("timelineMouse"));
    if (mouse_area == nullptr) {
        return 3;
    }
    QObject* const current_time = timeline->findChild<QObject*>(QStringLiteral("currentTime"));
    QObject* const total_time = timeline->findChild<QObject*>(QStringLiteral("totalTime"));
    QObject* const bar_wrap = timeline->findChild<QObject*>(QStringLiteral("barWrap"));
    if (current_time == nullptr || total_time == nullptr || bar_wrap == nullptr) {
        return 4;
    }

    if (property_bool(timeline.get(), "timelineAvailable") || property_bool(timeline.get(), "contentVisible") ||
        property_bool(timeline.get(), "seekable") || property_bool(mouse_area, "enabled") ||
        property_bool(current_time, "visible") || property_bool(total_time, "visible") ||
        property_bool(bar_wrap, "visible")) {
        return 5;
    }
    if (timeline->property("duration").toDouble() != 0.0) {
        return 6;
    }

    timeline->setProperty("duration", 5400.0);
    if (property_bool(timeline.get(), "contentVisible") || property_bool(timeline.get(), "seekable") ||
        property_bool(mouse_area, "enabled") || property_bool(current_time, "visible") ||
        property_bool(total_time, "visible") || property_bool(bar_wrap, "visible")) {
        return 7;
    }

    timeline->setProperty("timelineAvailable", true);
    if (!property_bool(timeline.get(), "contentVisible") || !property_bool(timeline.get(), "seekable") ||
        !property_bool(mouse_area, "enabled") || !property_bool(current_time, "visible") ||
        !property_bool(total_time, "visible") || !property_bool(bar_wrap, "visible")) {
        return 8;
    }

    timeline->setProperty("duration", -1.0);
    if (!property_bool(timeline.get(), "contentVisible") || property_bool(timeline.get(), "seekable") ||
        property_bool(mouse_area, "enabled")) {
        return 9;
    }

    return 0;
}
