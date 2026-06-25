#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_open_request.hpp"
#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_open_result.hpp"

namespace kivo::cinema_engine {

class LocalFileByteStream {
public:
    virtual ~LocalFileByteStream() = default;

    // Open a local file for reading
    virtual LocalFileByteStreamOpenResult open(const LocalFileByteStreamOpenRequest& request) = 0;

    // Read data from current position
    virtual std::vector<std::uint8_t> read(std::uint64_t size) = 0;

    // Read data from specific offset
    virtual std::vector<std::uint8_t> read_at(std::uint64_t offset, std::uint64_t size) = 0;

    // Seek to absolute position
    virtual bool seek(std::uint64_t position) = 0;

    // Get current position
    virtual std::uint64_t tell() const = 0;

    // Get total file size
    virtual std::uint64_t size() const = 0;

    // Check if stream is open and ready
    virtual bool is_open() const = 0;

    // Close the stream
    virtual void close() = 0;

    // Get error code if any
    virtual std::string error_code() const = 0;

    // Get inspector hint
    virtual std::string inspector_hint() const = 0;
};

}  // namespace kivo::cinema_engine