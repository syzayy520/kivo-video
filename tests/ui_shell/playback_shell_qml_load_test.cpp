#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlApplicationEngine>
#include <QUrl>

#include "playback_shell_qml_registration.hpp"
#include "playback_shell_qt_platform_bootstrap.hpp"

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
    kivo::ui::shell::bootstrap_qt_platform_paths(argv[0]);
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
    QElapsedTimer load_timer;
    load_timer.start();
    while (shell_component.isLoading() && load_timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
    }
    if (shell_component.isError()) {
        return 2;
    }
    if (shell_component.status() != QQmlComponent::Ready) {
        return 2;
    }

    if (!registration.playback_page_url.contains(QStringLiteral("PlaybackPage.qml"))) {
        return 4;
    }

    return 0;
}