#include "playback_shell_qml_registration.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QUrl>

namespace kivo::ui::shell {

namespace {

constexpr auto kPlaybackPageQrcUrl = "qrc:/kivo/ui/playback/page/PlaybackPage.qml";
constexpr auto kPlaybackPageQrcPath = ":/kivo/ui/playback/page/PlaybackPage.qml";

[[nodiscard]] bool playback_page_qrc_available() {
    return QFile::exists(QString::fromLatin1(kPlaybackPageQrcPath));
}

[[nodiscard]] PlaybackShellQmlRegistrationResult configure_file_fallback(
    QQmlApplicationEngine& engine,
    const QString& source_root) {
    PlaybackShellQmlRegistrationResult result{};
    const QDir playback_root(QDir(source_root).filePath(QStringLiteral("ui/playback")));
    const QString page_path = playback_root.filePath(QStringLiteral("page/PlaybackPage.qml"));
    if (!QFileInfo::exists(page_path)) {
        result.error_message =
            QStringLiteral("PlaybackPage.qml not found at ") + page_path;
        return result;
    }

    engine.addImportPath(playback_root.filePath(QStringLiteral("theme")));
    engine.addImportPath(playback_root.path());
    result.playback_page_url = QUrl::fromLocalFile(page_path).toString();
    result.ok = true;
    return result;
}

}  // namespace

PlaybackShellQmlRegistrationResult configure_playback_shell_engine(
    QQmlApplicationEngine& engine,
    const QString& source_root) {
    if (qEnvironmentVariableIsSet("KIVO_PLAYBACK_QML_FILE_FALLBACK")) {
        const QString root =
            source_root.isEmpty() ? QStringLiteral("C:/kivo video") : source_root;
        return configure_file_fallback(engine, root);
    }

    PlaybackShellQmlRegistrationResult result{};
    if (!playback_page_qrc_available()) {
        result.error_message =
            QStringLiteral("PlaybackPage.qml qrc resource missing at ")
            + QString::fromLatin1(kPlaybackPageQrcPath);
        return result;
    }

    result.playback_page_url = QString::fromLatin1(kPlaybackPageQrcUrl);
    result.ok = true;
    return result;
}

}  // namespace kivo::ui::shell