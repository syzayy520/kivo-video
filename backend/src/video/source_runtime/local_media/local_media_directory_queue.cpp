#include "video/source_runtime/local_media/local_media_directory_queue.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

#include "kivo/video/providers/local_file/local_file_path_validation.hpp"

namespace kivo::video::source_runtime::local_media {
namespace {

[[nodiscard]] std::string path_to_utf8_string(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

[[nodiscard]] std::string extension_lower(std::filesystem::path path) {
    std::string extension = path_to_utf8_string(path.extension());
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension;
}

[[nodiscard]] bool is_video_extension(const std::filesystem::path& path) {
    const std::string extension = extension_lower(path);
    return extension == ".mkv" || extension == ".mp4" || extension == ".m2ts" ||
           extension == ".ts" || extension == ".mov" || extension == ".avi";
}

[[nodiscard]] bool same_path(const std::filesystem::path& left,
                             const std::filesystem::path& right) {
    std::error_code error;
    if (std::filesystem::equivalent(left, right, error)) {
        return true;
    }
    return path_to_utf8_string(left) == path_to_utf8_string(right);
}

}  // namespace

LocalMediaDirectoryQueue build_local_media_directory_queue(
    const std::string& current_path) noexcept {
    LocalMediaDirectoryQueue queue{};
    if (current_path.empty()) {
        return queue;
    }

    std::error_code error;
    const std::filesystem::path current = std::filesystem::u8path(current_path);
    const std::filesystem::path directory = current.parent_path();
    if (directory.empty() || !std::filesystem::is_directory(directory, error) || error) {
        return queue;
    }

    std::vector<std::filesystem::path> paths;
    for (std::filesystem::directory_iterator it(
             directory, std::filesystem::directory_options::skip_permission_denied, error);
         !error && it != std::filesystem::directory_iterator{};
         it.increment(error)) {
        std::error_code status_error;
        if (!it->is_regular_file(status_error) || status_error ||
            !is_video_extension(it->path())) {
            continue;
        }
        std::uint64_t ignored_size = 0;
        std::uint64_t ignored_mtime = 0;
        const auto probe =
            providers::local_file::probe_file(path_to_utf8_string(it->path()),
                                              ignored_size,
                                              ignored_mtime);
        if (probe.is_ok()) {
            paths.push_back(it->path());
        }
    }
    if (paths.empty()) {
        return queue;
    }

    std::sort(paths.begin(), paths.end(), [](const auto& left, const auto& right) {
        return path_to_utf8_string(left.filename()) < path_to_utf8_string(right.filename());
    });

    queue.items.reserve(paths.size());
    for (std::size_t i = 0; i < paths.size(); ++i) {
        LocalMediaDirectoryQueueItem item{};
        item.index = static_cast<std::uint64_t>(i);
        item.path = path_to_utf8_string(paths[i]);
        item.label = path_to_utf8_string(paths[i].filename());
        item.current = same_path(paths[i], current);
        if (item.current) {
            queue.current_index = item.index;
        }
        queue.items.push_back(std::move(item));
    }

    queue.valid = true;
    return queue;
}

}  // namespace kivo::video::source_runtime::local_media
