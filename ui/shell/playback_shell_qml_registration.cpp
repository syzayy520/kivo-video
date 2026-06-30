#include "playback_shell_qml_registration.hpp"

#include <QDir>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

namespace kivo::ui::shell {

PlaybackShellQmlRegistrationResult configure_playback_shell_engine(
    QQmlApplicationEngine& engine,
    const QString& source_root) {
    PlaybackShellQmlRegistrationResult result{};
    const QDir playback_root(QDir(source_root).filePath("ui/playback"));
    const QString page_path = playback_root.filePath("page/PlaybackPage.qml");
    if (!QFileInfo::exists(page_path)) {
        result.error_message = QStringLiteral("PlaybackPage.qml not found at ") + page_path;
        return result;
    }

    engine.addImportPath(playback_root.filePath("theme"));
    engine.addImportPath(playback_root.path());
    result.playback_page_url = QUrl::fromLocalFile(page_path).toString();
    result.ok = true;
    return result;
}

}  // namespace kivo::ui::shell