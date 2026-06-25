#pragma once

#include <fstream>
#include <string>

#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream.hpp"

namespace kivo::cinema_engine {

class LocalFileByteStreamImpl : public LocalFileByteStream {
public:
    LocalFileByteStreamImpl() = default;
    ~LocalFileByteStreamImpl() override;

    LocalFileByteStreamOpenResult open(const LocalFileByteStreamOpenRequest& request) override;
    std::vector<std::uint8_t> read(std::uint64_t size) override;
    std::vector<std::uint8_t> read_at(std::uint64_t offset, std::uint64_t size) override;
    bool seek(std::uint64_t position) override;
    std::uint64_t tell() const override;
    std::uint64_t size() const override;
    bool is_open() const override;
    void close() override;
    std::string error_code() const override;
    std::string inspector_hint() const override;

private:
    std::ifstream file_stream_;
    std::string path_;
    std::uint64_t file_size_ = 0;
    std::string error_code_;
    std::string inspector_hint_;
    std::string trace_id_;
};

}  // namespace kivo::cinema_engine