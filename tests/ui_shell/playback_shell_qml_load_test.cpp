#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QUrl>

#include "playback_shell_qml_registration.hpp"

namespace {

[[nodiscard]] QString resolve_source_root() {
    const QString app_dir = QCoreApplication::applicationDirPath();
    QDir dir(app_dir);
    for (int depth = 0; depth < 8; ++depth) {
        if (dir.exists("ui/playback/page/PlaybackPage.qml")) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QStringLiteral("C:/kivo video");
}

}  // namespace

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const auto registration =
        kivo::ui::shell::configure_playback_shell_engine(engine, resolve_source_root());
    if (!registration.ok) {
        return 1;
    }

    QQmlComponent shell_component(
        &engine, QUrl(QStringLiteral("qrc:/kivo/ui/shell/PlaybackShellRoot.qml")));
    if (shell_component.isError()) {
        return 2;
    }

    QObject* const root = shell_component.create();
    if (root == nullptr) {
        return 2;
    }
    engine.setObjectOwnership(root, QQmlEngine::CppOwnership);

    auto* const window = qobject_cast<QQuickWindow*>(root);
    if (window == nullptr) {
        return 3;
    }

    if (!registration.playback_page_url.contains(QStringLiteral("PlaybackPage.qml"))) {
        return 4;
    }

    return 0;
}