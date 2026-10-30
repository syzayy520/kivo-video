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

QObject* overlay_state(QObject* const page) {
    return page == nullptr ? nullptr : page->property("overlayState").value<QObject*>();
}

QString active_sheet(QObject* const page) {
    QObject* const state = overlay_state(page);
    return state == nullptr ? QString{} : state->property("activeSheet").toString();
}

QString status_kind(QObject* const page) {
    QObject* const state = overlay_state(page);
    return state == nullptr ? QString{} : state->property("statusKind").toString();
}

bool context_menu_open(QObject* const page) {
    return page != nullptr && page->property("contextMenuOpen").toBool();
}

bool bool_property(QObject* const object, const char* const name) {
    return object != nullptr && object->property(name).toBool();
}

bool request_context_menu(QObject* const page, const int x, const int y) {
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(page,
                                                   "requestContextMenu",
                                                   Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, x),
                                                   Q_ARG(QVariant, y));
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
import QtQuick.Controls 6.5
import "qrc:/kivo/ui/playback/page"

ApplicationWindow {
    width: 960
    height: 540
    visible: true

    PlaybackPage {
        objectName: "page"
        anchors.fill: parent
    }
}
)qml",
                      QUrl(QStringLiteral("qrc:/kivo/tests/PlaybackContextMenuStateGate.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> root(component.create());
    if (!root) {
        return 2;
    }

    QObject* const page = root->findChild<QObject*>(QStringLiteral("page"));
    QObject* const state = overlay_state(page);
    QObject* const menu =
        page == nullptr ? nullptr : page->findChild<QObject*>(QStringLiteral("playbackContextMenu"));
    if (page == nullptr || state == nullptr || menu == nullptr) {
        return 3;
    }

    if (!request_context_menu(page, 120, 120)) {
        return 4;
    }
    app.processEvents();
    if (!context_menu_open(page)) {
        return 5;
    }
    QObject* const play_pause = menu->findChild<QObject*>(QStringLiteral("playPauseMenuItem"));
    QObject* const open_video = menu->findChild<QObject*>(QStringLiteral("openVideoMenuItem"));
    QObject* const open_folder = menu->findChild<QObject*>(QStringLiteral("openFolderMenuItem"));
    QObject* const playback_separator =
        menu->findChild<QObject*>(QStringLiteral("playbackMenuSeparator"));
    QObject* const more_item = menu->findChild<QObject*>(QStringLiteral("moreMenuItem"));
    QObject* const skip_backward =
        menu->findChild<QObject*>(QStringLiteral("skipBackwardMenuItem"));
    QObject* const skip_forward = menu->findChild<QObject*>(QStringLiteral("skipForwardMenuItem"));
    QObject* const input =
        page == nullptr ? nullptr : page->findChild<QObject*>(QStringLiteral("playbackInputController"));
    if (play_pause == nullptr || open_video == nullptr || open_folder == nullptr ||
        playback_separator == nullptr || more_item == nullptr || skip_backward == nullptr ||
        skip_forward == nullptr || input == nullptr) {
        return 17;
    }
    if (!bool_property(open_video, "visible") || !bool_property(open_folder, "visible") ||
        bool_property(play_pause, "visible") || bool_property(playback_separator, "visible") ||
        bool_property(more_item, "visible")) {
        return 18;
    }
    if (!invoke_no_arg(page, "closeContextMenu")) {
        return 6;
    }
    app.processEvents();
    if (context_menu_open(page)) {
        return 7;
    }

    page->setProperty("subtitleOptions",
                      QVariantList{option_item(QStringLiteral("trackId"),
                                               QStringLiteral("stale-sub"),
                                               QStringLiteral("Stale subtitle"))});
    if (!request_context_menu(page, 120, 120)) {
        return 27;
    }
    app.processEvents();
    if (!context_menu_open(page) || !bool_property(open_video, "visible") ||
        !bool_property(open_folder, "visible") || bool_property(play_pause, "visible") ||
        bool_property(more_item, "visible") || bool_property(playback_separator, "visible")) {
        return 28;
    }
    if (!invoke_no_arg(page, "closeContextMenu")) {
        return 29;
    }
    page->setProperty("subtitleOptions", QVariantList{});
    app.processEvents();

    page->setProperty("durationMs", 42000);
    if (!request_context_menu(page, 120, 120)) {
        return 19;
    }
    app.processEvents();
    if (!context_menu_open(page) || !bool_property(play_pause, "visible") ||
        bool_property(open_video, "visible") || bool_property(open_folder, "visible") ||
        bool_property(playback_separator, "visible") ||
        !bool_property(skip_backward, "visible") || !bool_property(skip_forward, "visible") ||
        !bool_property(input, "seekAvailable")) {
        return 20;
    }
    if (!invoke_no_arg(page, "closeContextMenu")) {
        return 21;
    }
    app.processEvents();

    page->setProperty("playbackState", QStringLiteral("Closed"));
    if (!request_context_menu(page, 120, 120)) {
        return 30;
    }
    app.processEvents();
    if (!context_menu_open(page) || !bool_property(open_video, "visible") ||
        !bool_property(open_folder, "visible") || bool_property(play_pause, "visible") ||
        bool_property(skip_backward, "visible") || bool_property(skip_forward, "visible") ||
        bool_property(more_item, "visible") || bool_property(playback_separator, "visible") ||
        bool_property(input, "seekAvailable")) {
        return 31;
    }
    if (!invoke_no_arg(page, "closeContextMenu")) {
        return 32;
    }
    page->setProperty("playbackState", QString{});
    app.processEvents();

    page->setProperty("subtitleOptions",
                      QVariantList{option_item(QStringLiteral("trackId"),
                                               QStringLiteral("1"),
                                               QStringLiteral("English"))});
    if (!request_context_menu(page, 120, 120)) {
        return 22;
    }
    app.processEvents();
    if (!context_menu_open(page) || bool_property(open_video, "visible") ||
        bool_property(open_folder, "visible") || !bool_property(more_item, "visible") ||
        bool_property(playback_separator, "visible")) {
        return 23;
    }
    if (!invoke_no_arg(more_item, "click")) {
        return 24;
    }
    app.processEvents();
    if (active_sheet(page) != QStringLiteral("more")) {
        return 25;
    }
    if (!invoke_no_arg(page, "closeContextMenu")) {
        return 26;
    }
    app.processEvents();

    if (!invoke_sheet(state, QStringLiteral("settings")) ||
        active_sheet(page) != QStringLiteral("settings")) {
        return 8;
    }
    if (request_context_menu(page, 120, 120)) {
        return 9;
    }
    app.processEvents();
    if (context_menu_open(page) || !active_sheet(page).isEmpty()) {
        return 10;
    }

    if (!invoke_buffering(state, 3) || status_kind(page) != QStringLiteral("buffering")) {
        return 11;
    }
    if (request_context_menu(page, 120, 120)) {
        return 12;
    }
    app.processEvents();
    if (context_menu_open(page) || status_kind(page) != QStringLiteral("buffering")) {
        return 13;
    }

    if (!invoke_no_arg(state, "clearStatus")) {
        return 14;
    }
    if (!request_context_menu(page, 120, 120)) {
        return 15;
    }
    app.processEvents();
    if (!context_menu_open(page)) {
        return 16;
    }

    return 0;
}
