#include "playback_shell_media_resolver.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

namespace kivo::ui::shell {
namespace {

[[nodiscard]] bool is_video_extension(const QString& suffix) {
    const QString lower = suffix.toLower();
    return lower == QStringLiteral("mkv") || lower == QStringLiteral("mp4")
           || lower == QStringLiteral("m2ts") || lower == QStringLiteral("ts")
           || lower == QStringLiteral("mov") || lower == QStringLiteral("avi");
}

[[nodiscard]] QString find_first_video_file(const QString& directory_path) {
    QDirIterator iterator(directory_path, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString file_path = iterator.next();
        if (is_video_extension(QFileInfo(file_path).suffix())) {
            return file_path;
        }
    }
    return {};
}

}  // namespace

ResolvedMediaPath resolve_playback_media_path(const QString& media_path) {
    ResolvedMediaPath resolved{};
    resolved.original_path = media_path;
    if (media_path.isEmpty()) {
        resolved.error = QStringLiteral("media path empty");
        return resolved;
    }

    const QFileInfo info(media_path);
    resolved.original_exists = info.exists();
    if (!resolved.original_exists) {
        resolved.error = QStringLiteral("media path not found");
        return resolved;
    }

    if (info.isFile()) {
        resolved.selected_file = QDir::toNativeSeparators(info.absoluteFilePath());
        resolved.selected_exists = true;
        return resolved;
    }

    if (!info.isDir()) {
        resolved.error = QStringLiteral("media path is not a file or directory");
        return resolved;
    }

    resolved.is_directory = true;
    const QString selected = find_first_video_file(info.absoluteFilePath());
    resolved.selected_file = selected.isEmpty() ? QString{} : QDir::toNativeSeparators(selected);
    resolved.selected_exists = !resolved.selected_file.isEmpty();
    if (!resolved.selected_exists) {
        resolved.error = QStringLiteral("no video file found in directory");
    }
    return resolved;
}

}  // namespace kivo::ui::shell