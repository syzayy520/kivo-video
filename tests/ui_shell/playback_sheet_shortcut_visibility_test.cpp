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

bool invoke_shortcut(QObject* const page, const QString& shortcut, const QString& sheet) {
    QVariant result;
    const bool invoked =
        QMetaObject::invokeMethod(page,
                                  "requestSheetShortcut",
                                  Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariant, shortcut),
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

bool close_sheet(QObject* const page) {
    QObject* const state = overlay_state(page);
    return state != nullptr &&
           QMetaObject::invokeMethod(state, "closeSheet", Qt::DirectConnection);
}

}  // namespace

int main(int argc, char* argv[]) {
    kivo::ui::shell::bootstrap_qt_platform_paths(argv[0]);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    QQmlEngine engine;
    QQmlComponent component(&engine,
                            QUrl(QStringLiteral("qrc:/kivo/ui/playback/page/PlaybackPage.qml")));

    if (component.isError()) {
        return 1;
    }

    QScopedPointer<QObject> page(component.create());
    if (!page) {
        return 2;
    }

    if (overlay_state(page.get()) == nullptr) {
        return 3;
    }

    if (invoke_shortcut(page.get(), QStringLiteral("subtitle"), QStringLiteral("subtitle"))) {
        return 4;
    }
    if (!active_sheet(page.get()).isEmpty()) {
        return 5;
    }

    if (invoke_shortcut(page.get(), QStringLiteral("audioTrack"), QStringLiteral("audio"))) {
        return 6;
    }
    if (!active_sheet(page.get()).isEmpty()) {
        return 7;
    }

    if (invoke_shortcut(page.get(), QStringLiteral("episode"), QStringLiteral("episodes"))) {
        return 8;
    }
    if (!active_sheet(page.get()).isEmpty()) {
        return 9;
    }

    if (invoke_shortcut(page.get(), QStringLiteral("version"), QStringLiteral("version"))) {
        return 10;
    }
    if (!active_sheet(page.get()).isEmpty()) {
        return 11;
    }

    page->setProperty("subtitleOptions",
                      QVariantList{option_item(QStringLiteral("trackId"),
                                               QStringLiteral("1"),
                                               QStringLiteral("English"))});
    if (invoke_shortcut(page.get(), QStringLiteral("subtitle"), QStringLiteral("subtitle")) ||
        !active_sheet(page.get()).isEmpty()) {
        return 19;
    }
    page->setProperty("durationMs", 42000);
    if (!invoke_shortcut(page.get(), QStringLiteral("subtitle"), QStringLiteral("subtitle")) ||
        active_sheet(page.get()) != QStringLiteral("subtitle")) {
        return 12;
    }
    if (!close_sheet(page.get())) {
        return 13;
    }

    page->setProperty("audioTrackOptions",
                      QVariantList{option_item(QStringLiteral("trackId"),
                                               QStringLiteral("2"),
                                               QStringLiteral("DTS 5.1"))});
    if (invoke_shortcut(page.get(), QStringLiteral("audioTrack"), QStringLiteral("audio")) ||
        !active_sheet(page.get()).isEmpty()) {
        return 20;
    }

    page->setProperty(
        "audioTrackOptions",
        QVariantList{option_item(QStringLiteral("trackId"),
                                 QStringLiteral("2"),
                                 QStringLiteral("DTS 5.1")),
                     option_item(QStringLiteral("trackId"),
                                 QStringLiteral("3"),
                                 QStringLiteral("Commentary"))});
    if (!invoke_shortcut(page.get(), QStringLiteral("audioTrack"), QStringLiteral("audio")) ||
        active_sheet(page.get()) != QStringLiteral("audio")) {
        return 14;
    }
    if (!close_sheet(page.get())) {
        return 15;
    }

    page->setProperty("episodeOptions",
                      QVariantList{option_item(QStringLiteral("episodeId"),
                                               QStringLiteral("episode-2"),
                                               QStringLiteral("Episode 2"))});
    if (!invoke_shortcut(page.get(), QStringLiteral("episode"), QStringLiteral("episodes")) ||
        active_sheet(page.get()) != QStringLiteral("episodes")) {
        return 16;
    }
    if (!close_sheet(page.get())) {
        return 17;
    }

    page->setProperty("versionOptions",
                      QVariantList{option_item(QStringLiteral("versionId"),
                                               QStringLiteral("uhd"),
                                               QStringLiteral("UHD"))});
    if (!invoke_shortcut(page.get(), QStringLiteral("version"), QStringLiteral("version")) ||
        active_sheet(page.get()) != QStringLiteral("version")) {
        return 18;
    }

    return 0;
}
