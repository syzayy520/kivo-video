// encoded_input_io_adapter.hpp — P5B B2: Encoded Input IO Adapter (fake runtime)
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "../allocation/allocation_limit_policy.hpp"

namespace kivo::video_plane::input_bridge {

enum class IOAdapterKind {
    LocalFile,
    HttpRange,
    WebDav,
    AList,
    Emby,
    Unknown
};

struct EncodedInputIOConfig {
    IOAdapterKind kind{IOAdapterKind::LocalFile};
    uint64_t max_probe_bytes{allocation::AllocationDefaults::kMaxProbeBytes};
    uint64_t max_read_chunk_bytes{4 * 1024 * 1024};  // 4 MiB
    bool operator==(EncodedInputIOConfig const&) const = default;
};

struct ReadResult {
    bool success{false};
    bool eof{false};
    uint64_t bytes_read{0};
    std::vector<uint8_t> data;
};

// Fake EncodedInputIOAdapter — simulates encoded input IO without real IO
class FakeEncodedInputIOAdapter {
public:
    explicit FakeEncodedInputIOAdapter(EncodedInputIOConfig const& config)
        : config_(config) {}

    // Simulate open
    bool open(std::string const& source_uri) {
        source_uri_ = source_uri;
        is_open_ = true;
        position_ = 0;
        return true;
    }

    // Simulate read
    ReadResult read(uint64_t bytes) {
        ReadResult result;
        if (!is_open_) return result;
        if (bytes > config_.max_read_chunk_bytes) bytes = config_.max_read_chunk_bytes;
        // Generate fake data
        result.data.resize(static_cast<size_t>(bytes), 0xAA);
        result.bytes_read = bytes;
        result.success = true;
        position_ += bytes;
        return result;
    }

    // Simulate seek
    bool seek(uint64_t offset) {
        if (!is_open_) return false;
        position_ = offset;
        return true;
    }

    // Simulate probe
    std::optional<uint64_t> probe() {
        if (!is_open_) return std::nullopt;
        return config_.max_probe_bytes;
    }

    void close() { is_open_ = false; }
    bool is_open() const { return is_open_; }
    uint64_t position() const { return position_; }
    IOAdapterKind kind() const { return config_.kind; }

private:
    EncodedInputIOConfig config_;
    std::string source_uri_;
    bool is_open_{false};
    uint64_t position_{0};
};

}  // namespace kivo::video_plane::input_bridge
