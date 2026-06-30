#pragma once

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QString>

namespace kivo::ui::shell {

inline void bootstrap_qt_platform_paths(const char* const argv0) noexcept {
    const QString app_dir = QFileInfo(QString::fromLocal8Bit(argv0)).absolutePath();
    if (app_dir.isEmpty()) {
        return;
    }

    QCoreApplication::addLibraryPath(app_dir);
    QCoreApplication::addLibraryPath(QDir(app_dir).filePath(QStringLiteral("plugins")));

    const QByteArray existing_path = qgetenv("PATH");
    const QByteArray app_dir_path = QDir::toNativeSeparators(app_dir).toUtf8();
    if (existing_path.isEmpty()) {
        qputenv("PATH", app_dir_path.constData());
        return;
    }
    if (!existing_path.startsWith(app_dir_path)) {
        qputenv("PATH", app_dir_path + ';' + existing_path);
    }
}

}  // namespace kivo::ui::shell