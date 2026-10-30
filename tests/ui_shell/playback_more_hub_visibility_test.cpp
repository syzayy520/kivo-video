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
#include <QVariantList>
#include <QVariantMap>

#include "playback_shell_qt_platform_bootstrap.hpp"

namespace {

bool bool_property(QObject* const object, const char* const name) {
    return object != nullptr && object->property(name).toBool();
}

QString active_sheet(QObject* const overlay) {
    QObject* const state =
        overlay == nullptr ? nullptr : overlay->property("stateModel").value<QObject*>();
    return state == nullptr ? QString{} : state->property("activeSheet").toString();
}

bool invoke_open_sheet(QObject* const overlay, const QString& sheet_name) {
    QVariant result;
    const bool invoked =
        QMetaObject::invokeMethod(overlay,
                                  "openSheetIfAvailable",
                                  Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariant, sheet_name));
    return invoked && result.toBool();
}

bool invoke_more_action(QObject* const more_sheet, const QString& action_name) {
    return more_sheet != nullptr &&
           QMetaObject::invokeMethod(more_sheet,
                                     "actionRequested",
                                     Qt::DirectConnection,
                                     Q_ARG(QString, action_name));
}

int int_property(QObject* const object, const char* const name) {
    return object == nullptr ? -1 : object->property(name).toInt();
}

QString string_property(QObject* const object, const char* const name) {
    return object == nullptr ? QString{} : object->property(name).toString();
}

int list_count(QObject* const object, const char* const name) {
    return object == nullptr ? -1 : object->property(name).toList().size();
}

bool close_sheet(QObject* const overlay) {
    QObject* const state =
        overlay == nullptr ? nullptr : overlay->property("stateModel").value<QObject*>();
    return state != nullptr &&
           QMetaObject::invokeMethod(state, "closeSheet", Qt::DirectConnection);
}

bool show_overlay(QObject* const overlay) {
    QObject* const state =
        overlay == nullptr ? nullptr : overlay->property("stateModel").value<QObject*>();
    return state != nullptr &&
           QMetaObject::invokeMethod(state, "showVisible", Qt::DirectConnection);
}

void pump_events(const int milliseconds = 180) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < milliseconds) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }
}

QVariantMap option_item(const QString& id_key, const QString& id, const QString& label) {
    QVariantMap item;
    item.insert(id_key, id);
    item.insert(QStringLiteral("label"), label);
    item.insert(QStringLiteral("name"), label);
    return item;
}

QVariantMap action_item(const QString& action_name, const QString& label) {
    QVariantMap item;
    item.insert(QStringLiteral("actionName"), action_name);
    item.insert(QStringLiteral("label"), label);
    item.insert(QStringLiteral("iconName"), QStringLiteral("more"));
    return item;
}

QVariantMap action_name_only_item(const QString& action_name) {
    QVariantMap item;
    item.insert(QStringLiteral("actionName"), action_name);
    return item;
}

QVariantMap label_only_item(const QString& label) {
    QVariantMap item;
    item.insert(QStringLiteral("label"), label);
    return item;
}

QVariantMap blank_action_item() {
    QVariantMap item;
    item.insert(QStringLiteral("actionName"), QString{});
    item.insert(QStringLiteral("label"), QString{});
    return item;
}

}  // namespace

int main(int argc, char* argv[]) {
    kivo::ui::shell::bootstrap_qt_platform_paths(argv[0]);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    QQmlEngine engine;

    QQmlComponent default_more_component(&engine);
    default_more_component.setData(R"qml(
import QtQuick 6.5
import "qrc:/kivo/ui/playback/sheets"

MorePlaybackSheet {
    width: 360
    open: true
}
)qml",
                                   QUrl(QStringLiteral("qrc:/kivo/tests/DefaultMorePlaybackSheet.qml")));
    if (default_more_component.isError()) {
        return 21;
    }
    QScopedPointer<QObject> default_more(default_more_component.create());
    if (!default_more) {
        return 22;
    }
    if (list_count(default_more.get(), "hubActions") != 0) {
        return 23;
    }
    default_more->setProperty("settingsAvailable", true);
    pump_events();
    if (list_count(default_more.get(), "hubActions") != 1) {
        return 24;
    }
    default_more->setProperty("settingsAvailable", false);
    default_more->setProperty(
        "actions",
        QVariantList{action_name_only_item(QStringLiteral("inspect")),
                     label_only_item(QStringLiteral("Inspect"))});
    pump_events();
    if (list_count(default_more.get(), "hubActions") != 0) {
        return 28;
    }

    QQmlComponent component(&engine);
    component.setData(R"qml(
import QtQuick 6.5
import "qrc:/kivo/ui/playback/overlay"

Item {
    id: root

    property alias playbackOverlay: overlay
    property int customActionDispatches: 0
    property string lastCustomAction: ""

    PlaybackOverlay {
        id: overlay
        width: 960
        height: 540

        onSheetActionRequested: function(actionName) {
            root.customActionDispatches += 1
            root.lastCustomAction = actionName
        }
    }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackMoreHubVisibility.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }
    QObject* const overlay = root->property("playbackOverlay").value<QObject*>();
    if (overlay == nullptr) {
        return 16;
    }

    QObject* const more_button = overlay->findChild<QObject*>(QStringLiteral("moreToolButton"));
    QObject* const more_sheet = overlay->findChild<QObject*>(QStringLiteral("morePlaybackSheet"));
    if (more_button == nullptr) {
        return 3;
    }
    if (more_sheet == nullptr) {
        return 17;
    }
    if (!show_overlay(overlay)) {
        return 12;
    }
    pump_events();

    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        !active_sheet(overlay).isEmpty()) {
        return 4;
    }

    overlay->setProperty("subtitleOptions",
                         QVariantList{option_item(QStringLiteral("trackId"),
                                                  QStringLiteral("sub-en"),
                                                  QStringLiteral("English"))});
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        !active_sheet(overlay).isEmpty()) {
        return 25;
    }

    overlay->setProperty("playbackAvailable", true);
    pump_events();
    if (!bool_property(more_button, "visible") ||
        !invoke_open_sheet(overlay, QStringLiteral("more")) ||
        active_sheet(overlay) != QStringLiteral("more")) {
        return 5;
    }
    if (!close_sheet(overlay)) {
        return 6;
    }

    overlay->setProperty("playbackAvailable", false);
    overlay->setProperty("subtitleOptions", QVariantList{});
    overlay->setProperty("moreActions", QVariantList{blank_action_item()});
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        !active_sheet(overlay).isEmpty()) {
        return 18;
    }

    overlay->setProperty("moreActions",
                         QVariantList{blank_action_item(),
                                      action_item(QStringLiteral("inspect"), QStringLiteral("Inspect"))});
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        !active_sheet(overlay).isEmpty()) {
        return 26;
    }

    overlay->setProperty("playbackAvailable", true);
    pump_events();
    if (!bool_property(more_button, "visible") ||
        !invoke_open_sheet(overlay, QStringLiteral("more")) ||
        active_sheet(overlay) != QStringLiteral("more") ||
        list_count(more_sheet, "hubActions") != 1) {
        return 7;
    }
    if (!invoke_more_action(more_sheet, QStringLiteral("inspect")) ||
        int_property(root.get(), "customActionDispatches") != 1 ||
        string_property(root.get(), "lastCustomAction") != QStringLiteral("inspect")) {
        return 19;
    }
    if (!close_sheet(overlay)) {
        return 8;
    }

    overlay->setProperty(
        "moreActions",
        QVariantList{action_name_only_item(QStringLiteral("inspect")),
                     label_only_item(QStringLiteral("Inspect"))});
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        !active_sheet(overlay).isEmpty()) {
        return 29;
    }

    overlay->setProperty("moreActions", QVariantList{});
    overlay->setProperty("playbackAvailable", false);
    overlay->setProperty("playbackSpeed", 1.25);
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        !active_sheet(overlay).isEmpty()) {
        return 20;
    }

    overlay->setProperty("playbackAvailable", true);
    pump_events();
    if (!bool_property(more_button, "visible") ||
        !invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        active_sheet(overlay) != QStringLiteral("settings")) {
        return 9;
    }
    if (!close_sheet(overlay)) {
        return 10;
    }

    overlay->setProperty("playbackSpeed", 1.0);
    overlay->setProperty("playbackAvailable", false);
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        !active_sheet(overlay).isEmpty()) {
        return 11;
    }

    overlay->setProperty("audioTrackLabel", QStringLiteral("DTS 5.1"));
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        !active_sheet(overlay).isEmpty()) {
        return 27;
    }

    overlay->setProperty("playbackAvailable", true);
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("audio")) ||
        invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        !active_sheet(overlay).isEmpty()) {
        return 13;
    }

    overlay->setProperty("audioTrackOptions",
                         QVariantList{option_item(QStringLiteral("trackId"),
                                                  QStringLiteral("dts-main"),
                                                  QStringLiteral("DTS 5.1"))});
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("audio")) ||
        invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        !active_sheet(overlay).isEmpty()) {
        return 14;
    }

    overlay->setProperty(
        "audioTrackOptions",
        QVariantList{option_item(QStringLiteral("trackId"),
                                 QStringLiteral("dts-main"),
                                 QStringLiteral("DTS 5.1")),
                     option_item(QStringLiteral("trackId"),
                                 QStringLiteral("aac-commentary"),
                                 QStringLiteral("Commentary"))});
    pump_events();
    if (!bool_property(more_button, "visible") ||
        !invoke_open_sheet(overlay, QStringLiteral("audio")) ||
        active_sheet(overlay) != QStringLiteral("audio")) {
        return 30;
    }
    if (!close_sheet(overlay)) {
        return 31;
    }
    if (!invoke_open_sheet(overlay, QStringLiteral("more")) ||
        active_sheet(overlay) != QStringLiteral("more") ||
        list_count(more_sheet, "hubActions") != 2) {
        return 32;
    }
    if (!close_sheet(overlay)) {
        return 33;
    }

    overlay->setProperty("audioTrackLabel", QString{});
    overlay->setProperty("audioTrackOptions", QVariantList{});
    pump_events();
    if (bool_property(more_button, "visible") ||
        invoke_open_sheet(overlay, QStringLiteral("more")) ||
        invoke_open_sheet(overlay, QStringLiteral("settings")) ||
        !active_sheet(overlay).isEmpty()) {
        return 15;
    }

    return 0;
}
