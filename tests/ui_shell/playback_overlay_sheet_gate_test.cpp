#include <QGuiApplication>
#include <QMetaObject>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

QObject* overlay_state(QObject* const overlay) {
    return overlay == nullptr ? nullptr : overlay->property("stateModel").value<QObject*>();
}

QString active_sheet(QObject* const overlay) {
    QObject* const state = overlay_state(overlay);
    return state == nullptr ? QString{} : state->property("activeSheet").toString();
}

bool invoke_sheet(QObject* const overlay, const QString& method, const QString& sheet) {
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(overlay,
                                                   method.toUtf8().constData(),
                                                   Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, sheet));
    return invoked && result.toBool();
}

QVariantMap option_item(const QString& id_key, const QString& id, const QString& label) {
    QVariantMap item;
    item.insert(id_key, id);
    item.insert(QStringLiteral("label"), label);
    item.insert(QStringLiteral("name"), label);
    return item;
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

PlaybackOverlay {
    width: 960
    height: 540
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackOverlaySheetGate.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> overlay(component.create());
    if (!overlay) {
        return 2;
    }
    if (overlay_state(overlay.get()) == nullptr) {
        return 3;
    }

    if (invoke_sheet(overlay.get(), QStringLiteral("openSheetIfAvailable"),
                     QStringLiteral("subtitle"))) {
        return 4;
    }
    if (!active_sheet(overlay.get()).isEmpty()) {
        return 5;
    }

    if (invoke_sheet(overlay.get(), QStringLiteral("openSheetIfAvailable"),
                     QStringLiteral("settings"))) {
        return 6;
    }

    overlay->setProperty("playbackAvailable", true);

    if (invoke_sheet(overlay.get(), QStringLiteral("openSheetIfAvailable"),
                     QStringLiteral("audio"))) {
        return 7;
    }
    if (!active_sheet(overlay.get()).isEmpty()) {
        return 8;
    }

    overlay->setProperty("audioTrackOptions",
                         QVariantList{option_item(QStringLiteral("trackId"),
                                                  QStringLiteral("dts-main"),
                                                  QStringLiteral("DTS 5.1"))});
    if (invoke_sheet(overlay.get(), QStringLiteral("openSheetIfAvailable"),
                     QStringLiteral("audio")) ||
        !active_sheet(overlay.get()).isEmpty()) {
        return 12;
    }

    overlay->setProperty(
        "audioTrackOptions",
        QVariantList{option_item(QStringLiteral("trackId"),
                                 QStringLiteral("dts-main"),
                                 QStringLiteral("DTS 5.1")),
                     option_item(QStringLiteral("trackId"),
                                 QStringLiteral("aac-commentary"),
                                 QStringLiteral("Commentary"))});
    if (!invoke_sheet(overlay.get(), QStringLiteral("openSheetIfAvailable"),
                      QStringLiteral("audio")) ||
        active_sheet(overlay.get()) != QStringLiteral("audio")) {
        return 13;
    }

    overlay->setProperty("subtitleOptions",
                         QVariantList{option_item(QStringLiteral("trackId"),
                                                  QStringLiteral("1"),
                                                  QStringLiteral("English"))});
    if (!invoke_sheet(overlay.get(), QStringLiteral("openSheetIfAvailable"),
                      QStringLiteral("subtitle")) ||
        active_sheet(overlay.get()) != QStringLiteral("subtitle")) {
        return 9;
    }

    overlay->setProperty("versionOptions",
                         QVariantList{option_item(QStringLiteral("versionId"),
                                                  QStringLiteral("uhd"),
                                                  QStringLiteral("UHD"))});
    if (!invoke_sheet(overlay.get(), QStringLiteral("toggleSheetIfAvailable"),
                      QStringLiteral("version")) ||
        active_sheet(overlay.get()) != QStringLiteral("version")) {
        return 10;
    }

    if (!invoke_sheet(overlay.get(), QStringLiteral("toggleSheetIfAvailable"),
                      QStringLiteral("version")) ||
        !active_sheet(overlay.get()).isEmpty()) {
        return 11;
    }

    return 0;
}
