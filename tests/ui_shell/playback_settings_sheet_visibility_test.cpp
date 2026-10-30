#include <QGuiApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

bool visible(QObject* const object) {
    return object != nullptr && object->property("visible").toBool();
}

int height(QObject* const object) {
    return object == nullptr ? -1 : object->property("height").toInt();
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
import "qrc:/kivo/ui/playback/sheets"

SettingsSheet {
    width: 360
    open: true
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/SettingsSheetVisibility.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }

    QObject* const audio_sync = root->findChild<QObject*>(QStringLiteral("audioSyncRow"));
    QObject* const subtitle_sync = root->findChild<QObject*>(QStringLiteral("subtitleSyncRow"));
    QObject* const speed = root->findChild<QObject*>(QStringLiteral("speedRow"));
    QObject* const continue_playing =
        root->findChild<QObject*>(QStringLiteral("continuePlayingRow"));
    if (audio_sync == nullptr || subtitle_sync == nullptr || speed == nullptr ||
        continue_playing == nullptr) {
        return 3;
    }

    if (visible(speed) || height(speed) != 0) {
        return 4;
    }
    if (visible(audio_sync) || height(audio_sync) != 0) {
        return 5;
    }
    if (visible(subtitle_sync) || height(subtitle_sync) != 0) {
        return 6;
    }
    if (visible(continue_playing) || height(continue_playing) != 0) {
        return 10;
    }

    root->setProperty("playbackSpeedControlsAvailable", true);
    if (!visible(speed) || height(speed) <= 0) {
        return 12;
    }

    root->setProperty("audioControlsAvailable", true);
    if (!visible(audio_sync) || height(audio_sync) <= 0) {
        return 7;
    }

    root->setProperty("subtitleControlsAvailable", true);
    if (!visible(subtitle_sync) || height(subtitle_sync) <= 0) {
        return 8;
    }

    root->setProperty("autoPlayNextControlsAvailable", true);
    if (!visible(continue_playing) || height(continue_playing) <= 0) {
        return 11;
    }

    root->setProperty("audioControlsAvailable", false);
    root->setProperty("subtitleControlsAvailable", false);
    root->setProperty("playbackSpeedControlsAvailable", false);
    root->setProperty("autoPlayNextControlsAvailable", false);
    if (visible(speed) || height(speed) != 0 ||
        visible(audio_sync) || height(audio_sync) != 0 ||
        visible(subtitle_sync) || height(subtitle_sync) != 0 ||
        visible(continue_playing) || height(continue_playing) != 0) {
        return 9;
    }

    return 0;
}
