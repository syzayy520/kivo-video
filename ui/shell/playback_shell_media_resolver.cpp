#include "playback_shell_media_resolver.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <filesystem>

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

[[nodiscard]] QString find_first_video_file_with_std_fs(const QString& directory_path) {
    std::error_code error;
    std::filesystem::recursive_directory_iterator iterator(
        std::filesystem::path(directory_path.toStdWString()),
        std::filesystem::directory_options::skip_permission_denied,
        error);
    const std::filesystem::recursive_directory_iterator end;
    while (!error && iterator != end) {
        const auto entry_path = iterator->path();
        if (iterator->is_regular_file(error)) {
            QString suffix = QString::fromStdWString(entry_path.extension().wstring());
            if (suffix.startsWith(QLatin1Char('.'))) {
                suffix.remove(0, 1);
            }
            if (is_video_extension(suffix)) {
                return QDir::toNativeSeparators(QString::fromStdWString(entry_path.wstring()));
            }
        }
        iterator.increment(error);
    }
    return {};
}

[[nodiscard]] QString find_first_video_file_resilient(const QString& directory_path) {
    const QString selected = find_first_video_file(directory_path);
    if (!selected.isEmpty()) {
        return selected;
    }
    return find_first_video_file_with_std_fs(directory_path);
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
    const QString selected =
        find_first_video_file_resilient(QDir::fromNativeSeparators(media_path));
    resolved.selected_file = selected.isEmpty() ? QString{} : QDir::toNativeSeparators(selected);
    resolved.selected_exists = !resolved.selected_file.isEmpty();
    if (!resolved.selected_exists) {
        resolved.error = QStringLiteral("no video file found in directory");
    }
    return resolved;
}

}  // namespace kivo::ui::shell
