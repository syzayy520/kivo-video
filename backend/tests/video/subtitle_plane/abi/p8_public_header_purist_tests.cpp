#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
    const std::filesystem::path root = KIVO_SOURCE_ROOT;
    const auto public_root = root / "include" / "kivo" / "video" / "subtitle_plane";
    const std::vector<std::string> forbidden = {
        "std::", "noexcept", "class ", "QString", "QObject", "AVFrame", "AVPacket",
        "windows.h", "d3d11.h", "dxgi.h", "ComPtr", "KivoInputBytesView"};

    for (const auto& entry : std::filesystem::recursive_directory_iterator(public_root)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".h") {
            continue;
        }
        const std::string text = read_file(entry.path());
        for (const auto& token : forbidden) {
            if (contains(text, token)) {
                return 1;
            }
        }
    }

    return 0;
}
