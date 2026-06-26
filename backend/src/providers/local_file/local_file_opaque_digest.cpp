#include "kivo/video/providers/local_file/local_file_opaque_digest.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>

namespace kivo::video::providers::local_file {

std::string compute_opaque_digest(
    const std::string& internal_path,
    std::uint64_t file_size,
    std::uint64_t last_write_time) {
    // Simple hash combining path, size, mtime (not cryptographically secure)
    std::size_t h = 0;
    auto combine = [&h](const void* data, std::size_t len) {
        const auto* p = static_cast<const unsigned char*>(data);
        for (std::size_t i = 0; i < len; ++i) {
            h = h * 31 + p[i];
        }
    };
    combine(internal_path.data(), internal_path.size());
    combine(&file_size, sizeof(file_size));
    combine(&last_write_time, sizeof(last_write_time));
    // Salt/version
    std::uint32_t version = 1;
    combine(&version, sizeof(version));

    // Output as hex
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << h;
    return oss.str();
}

std::string digest_prefix(const std::string& digest, std::size_t len) {
    return digest.substr(0, std::min(len, digest.size()));
}

std::string safe_extension(const std::string& path) {
    auto pos = path.rfind('.');
    if (pos == std::string::npos) return "";
    auto sep = path.rfind('\\');
    auto fsep = path.rfind('/');
    if (sep != std::string::npos && sep > pos) return "";
    if (fsep != std::string::npos && fsep > pos) return "";
    return path.substr(pos);
}

}  // namespace kivo::video::providers::local_file
