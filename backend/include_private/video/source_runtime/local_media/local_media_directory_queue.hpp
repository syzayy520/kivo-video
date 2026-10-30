#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video::source_runtime::local_media {

struct LocalMediaDirectoryQueueItem {
    std::string path{};
    std::string label{};
    std::uint64_t index{0};
    bool current{false};
};

struct LocalMediaDirectoryQueue {
    std::vector<LocalMediaDirectoryQueueItem> items{};
    std::uint64_t current_index{0};
    bool valid{false};
};

[[nodiscard]] LocalMediaDirectoryQueue build_local_media_directory_queue(
    const std::string& current_path) noexcept;

}  // namespace kivo::video::source_runtime::local_media
