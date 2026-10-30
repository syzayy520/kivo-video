#include "video/source_runtime/local_media/local_media_directory_queue.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace local_media = kivo::video::source_runtime::local_media;

namespace {

[[nodiscard]] std::string path_to_utf8_string(const fs::path& path) {
    const auto utf8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

[[nodiscard]] bool require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << "\n";
    }
    return value;
}

void write_marker_file(const fs::path& path) {
    std::ofstream out(path, std::ios::binary);
    out << "kivo";
}

}  // namespace

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() /
                          ("kivo_local_media_directory_queue_" + std::to_string(stamp));

    std::error_code error;
    fs::create_directories(root, error);
    if (error) {
        std::cerr << "FAIL: could not create temp directory\n";
        return 1;
    }

    const fs::path first = root / "01 - opening.mkv";
    const fs::path current = root / "02 - current.MP4";
    const fs::path third = root / "03 - next.m2ts";
    const fs::path ignored_text = root / "notes.txt";
    const fs::path ignored_dir = root / "04 - folder.mkv";

    write_marker_file(first);
    write_marker_file(current);
    write_marker_file(third);
    write_marker_file(ignored_text);
    fs::create_directories(ignored_dir, error);

    const auto queue =
        local_media::build_local_media_directory_queue(path_to_utf8_string(current));

    bool ok = true;
    ok = require(queue.valid, "directory queue should be valid") && ok;
    ok = require(queue.items.size() == 3, "queue should include only real video files") && ok;
    ok = require(queue.current_index == 1, "current index should match current media") && ok;

    if (queue.items.size() == 3) {
        ok = require(queue.items[0].label == "01 - opening.mkv", "first label sorted") && ok;
        ok = require(queue.items[1].label == "02 - current.MP4", "current label sorted") && ok;
        ok = require(queue.items[2].label == "03 - next.m2ts", "third label sorted") && ok;
        ok = require(!queue.items[0].current, "first item is not current") && ok;
        ok = require(queue.items[1].current, "second item is current") && ok;
        ok = require(!queue.items[2].current, "third item is not current") && ok;
    }

    fs::remove(first, error);
    fs::remove(current, error);
    fs::remove(third, error);
    fs::remove(ignored_text, error);
    fs::remove(ignored_dir, error);
    fs::remove(root, error);

    return ok ? 0 : 1;
}
