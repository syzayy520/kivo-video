#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_impl.hpp"

#include <filesystem>
#include <stdexcept>

namespace kivo::cinema_engine {

LocalFileByteStreamImpl::~LocalFileByteStreamImpl() {
    close();
}

LocalFileByteStreamOpenResult LocalFileByteStreamImpl::open(const LocalFileByteStreamOpenRequest& request) {
    LocalFileByteStreamOpenResult result;
    result.trace_id = request.trace_id;
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(request.path)) {
            result.opened = false;
            result.error_code = "file_not_found";
            result.inspector_hint = "File does not exist: " + request.path;
            error_code_ = "file_not_found";
            inspector_hint_ = result.inspector_hint;
            return result;
        }
        
        // Check file size
        file_size_ = std::filesystem::file_size(request.path);
        
        // Open file
        file_stream_.open(request.path, std::ios::binary);
        if (!file_stream_.is_open()) {
            result.opened = false;
            result.error_code = "permission_denied";
            result.inspector_hint = "Failed to open file: " + request.path;
            error_code_ = "permission_denied";
            inspector_hint_ = result.inspector_hint;
            return result;
        }
        
        // Store path and trace
        path_ = request.path;
        trace_id_ = request.trace_id;
        
        // Check if seekable
        bool seekable = file_stream_.seekg(0).good();
        
        result.opened = true;
        result.size_bytes = file_size_;
        result.seekable = seekable;
        result.error_code.clear();
        result.inspector_hint = "Successfully opened file: " + request.path;
        error_code_.clear();
        inspector_hint_ = result.inspector_hint;
        
        return result;
    } catch (const std::exception& e) {
        result.opened = false;
        result.error_code = "open_failed";
        result.inspector_hint = "Exception during open: " + std::string(e.what());
        error_code_ = "open_failed";
        inspector_hint_ = result.inspector_hint;
        return result;
    }
}

std::vector<std::uint8_t> LocalFileByteStreamImpl::read(std::uint64_t size) {
    if (!file_stream_.is_open()) {
        return {};
    }
    
    std::vector<std::uint8_t> buffer(size);
    file_stream_.read(reinterpret_cast<char*>(buffer.data()), size);
    auto bytes_read = file_stream_.gcount();
    buffer.resize(bytes_read);
    return buffer;
}

std::vector<std::uint8_t> LocalFileByteStreamImpl::read_at(std::uint64_t offset, std::uint64_t size) {
    if (!file_stream_.is_open()) {
        return {};
    }
    
    // Save current position
    auto current_pos = file_stream_.tellg();
    
    // Seek to offset
    file_stream_.seekg(offset);
    if (!file_stream_.good()) {
        return {};
    }
    
    // Read data
    std::vector<std::uint8_t> buffer(size);
    file_stream_.read(reinterpret_cast<char*>(buffer.data()), size);
    auto bytes_read = file_stream_.gcount();
    buffer.resize(bytes_read);
    
    // Restore position
    file_stream_.seekg(current_pos);
    
    return buffer;
}

bool LocalFileByteStreamImpl::seek(std::uint64_t position) {
    if (!file_stream_.is_open()) {
        return false;
    }
    
    file_stream_.seekg(position);
    return file_stream_.good();
}

std::uint64_t LocalFileByteStreamImpl::tell() const {
    if (!file_stream_.is_open()) {
        return 0;
    }
    
    // tellg() is non-const; cast away const for position query (safe: no mutation)
    return const_cast<std::ifstream&>(file_stream_).tellg();
}

std::uint64_t LocalFileByteStreamImpl::size() const {
    return file_size_;
}

bool LocalFileByteStreamImpl::is_open() const {
    return file_stream_.is_open();
}

void LocalFileByteStreamImpl::close() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    error_code_.clear();
    inspector_hint_.clear();
}

std::string LocalFileByteStreamImpl::error_code() const {
    return error_code_;
}

std::string LocalFileByteStreamImpl::inspector_hint() const {
    return inspector_hint_;
}

}  // namespace kivo::cinema_engine