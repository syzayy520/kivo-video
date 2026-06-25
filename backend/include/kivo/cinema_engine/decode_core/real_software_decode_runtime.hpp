#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "kivo/cinema_engine/decode_core/decode_result.hpp"
#include "kivo/cinema_engine/decode_core/decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"

namespace kivo::cinema_engine {

// RealSoftwareDecodeRuntime wraps FFmpeg software decoding behind the adapter boundary.
// No AVCodecContext leaks into business code.
class RealSoftwareDecodeRuntime {
public:
    RealSoftwareDecodeRuntime();
    ~RealSoftwareDecodeRuntime();

    // Non-copyable, movable
    RealSoftwareDecodeRuntime(const RealSoftwareDecodeRuntime&) = delete;
    RealSoftwareDecodeRuntime& operator=(const RealSoftwareDecodeRuntime&) = delete;
    RealSoftwareDecodeRuntime(RealSoftwareDecodeRuntime&& other) noexcept;
    RealSoftwareDecodeRuntime& operator=(RealSoftwareDecodeRuntime&& other) noexcept;

    // Open a decoder for a specific codec (by name, e.g. "h264", "aac")
    bool open(const std::string& codec_name, const std::string& trace_id);

    // Adopt an already-opened native codec context (takes ownership via void*)
    bool adopt_codec_context(void* native_context, const std::string& codec_name,
                             const std::string& trace_id);

    // Decode a packet, producing zero or more frames
    DecodeResult decode(const KivoPacket& packet, const std::string& trace_id);

    // Flush the decoder (signal EOF)
    DecodeResult flush(const std::string& trace_id);

    // Close the decoder
    void close();

    // Get current state
    bool is_open() const;
    bool is_flushed() const;
    std::string codec_name() const;
    int32_t width() const;
    int32_t height() const;
    std::string pixel_format() const;
    std::string error_code() const;
    std::string inspector_hint() const;

private:
    void* codec_context_;   // AVCodecContext* — opaque to business code
    std::string codec_name_;
    bool is_open_;
    bool is_flushed_;
    int32_t width_;
    int32_t height_;
    std::string pixel_format_;
    std::string error_code_;
    std::string inspector_hint_;
    std::string trace_id_;
};

}  // namespace kivo::cinema_engine
