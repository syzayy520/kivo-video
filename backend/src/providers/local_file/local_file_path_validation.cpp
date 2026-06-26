#include "kivo/video/providers/local_file/local_file_path_validation.hpp"
#include <filesystem>
#include <system_error>

namespace kivo::video::providers::local_file {

source_core::SourceError validate_path(const std::string& path) {
    if (path.empty()) return {source_core::SourceErrorCode::invalid_request, "empty path"};
    if (path.find('\0') != std::string::npos)
        return {source_core::SourceErrorCode::invalid_request, "path contains NUL"};

    std::error_code ec;
    auto p = std::filesystem::path(path);
    if (!p.is_absolute())
        return {source_core::SourceErrorCode::invalid_request, "relative path rejected"};

    // symlink_status first: reliably detect symlinks without following
    auto lst = std::filesystem::symlink_status(p, ec);
    if (!ec && std::filesystem::is_symlink(lst))
        return {source_core::SourceErrorCode::invalid_request, "symlink rejected"};

    auto status = std::filesystem::status(p, ec);
    if (ec) return {source_core::SourceErrorCode::not_found, "path not accessible"};
    if (std::filesystem::is_directory(status))
        return {source_core::SourceErrorCode::invalid_request, "directory rejected"};

    if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
        return {source_core::SourceErrorCode::unsupported_provider, "UNC path deferred"};

    return source_core::SourceError::ok();
}

source_core::SourceError probe_file(const std::string& path, std::uint64_t& out_size, std::uint64_t& out_mtime) {
    std::error_code ec;
    auto p = std::filesystem::path(path);
    if (!std::filesystem::exists(p, ec))
        return {source_core::SourceErrorCode::not_found, "file does not exist"};
    if (ec) return {source_core::SourceErrorCode::internal_error, "filesystem error"};
    if (!std::filesystem::is_regular_file(p, ec) || ec)
        return {source_core::SourceErrorCode::invalid_request, "not a regular file"};
    out_size = std::filesystem::file_size(p, ec);
    if (ec) out_size = 0;
    auto ftime = std::filesystem::last_write_time(p, ec);
    if (!ec) out_mtime = static_cast<std::uint64_t>(ftime.time_since_epoch().count());
    else out_mtime = 0;
    return source_core::SourceError::ok();
}

}  // namespace kivo::video::providers::local_file
