#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "kivo/cinema_engine/demux_core/demux_result.hpp"
#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream.hpp"

namespace kivo::cinema_engine {

// RealDemuxRuntime wraps FFmpeg demuxing behind the adapter boundary.
// No AVFormatContext leaks into business code.
class RealDemuxRuntime {
public:
    RealDemuxRuntime();
    ~RealDemuxRuntime();

    // Non-copyable, movable
    RealDemuxRuntime(const RealDemuxRuntime&) = delete;
    RealDemuxRuntime& operator=(const RealDemuxRuntime&) = delete;
    RealDemuxRuntime(RealDemuxRuntime&& other) noexcept;
    RealDemuxRuntime& operator=(RealDemuxRuntime&& other) noexcept;

    // Open a local file for demuxing
    bool open(const std::string& path, const std::string& trace_id);

    // Close the demuxer
    void close();

    // Read the next packet. Returns eof=true when no more packets.
    DemuxResult read_packet(const std::string& trace_id);

    // Seek to a specific time (seconds)
    bool seek_to_time(double seconds);

    // Get current state
    bool is_open() const;
    bool is_eof() const;
    std::string container_format() const;
    double duration() const;
    std::string error_code() const;
    std::string inspector_hint() const;

    // Get stream count
    int video_stream_count() const;
    int audio_stream_count() const;
    int subtitle_stream_count() const;

private:
    void* format_context_;   // AVFormatContext* — opaque to business code
    std::string container_format_;
    double duration_;
    bool is_open_;
    bool is_eof_;
    std::string error_code_;
    std::string inspector_hint_;
    std::string trace_id_;
    int video_stream_count_;
    int audio_stream_count_;
    int subtitle_stream_count_;
};

}  // namespace kivo::cinema_engine
