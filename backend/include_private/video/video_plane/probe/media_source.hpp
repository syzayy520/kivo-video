#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// FFmpeg types stay opaque here on purpose: the probe header is included by
// translation units that must not see third-party types.
struct AVFormatContext;
struct AVPacket;

namespace kivo::video::video_plane::probe {

// One demuxed video packet, borrowed from the source. Valid until the next call
// to read_next().
struct SourcePacketView {
    const std::uint8_t* data{nullptr};
    std::size_t size{0};
    std::int64_t pts{0};
    std::int64_t dts{0};
    bool key_frame{false};
};

// What the container says about the video stream.
struct VideoStreamInfo {
    std::string codec_name;
    std::int32_t width{0};
    std::int32_t height{0};
    std::int32_t time_base_num{0};
    std::int32_t time_base_den{0};
    std::int32_t bit_depth{8};
    std::string container_pixel_format;
    std::int64_t duration_us{0};
    const std::uint8_t* extradata{nullptr};
    std::size_t extradata_size{0};
};

enum class ReadStatus : std::uint8_t {
    Packet = 0,   // a video packet is available in current_packet()
    EndOfStream,
    Failed,
};

// Single responsibility: turn a file into video packets.
// Knows nothing about decoding, devices or presentation.
class MediaSource {
public:
    MediaSource() = default;
    ~MediaSource();

    MediaSource(const MediaSource&) = delete;
    auto operator=(const MediaSource&) -> MediaSource& = delete;

    [[nodiscard]] auto open(const std::string& path, std::string& error) -> bool;
    void close() noexcept;

    [[nodiscard]] auto video_stream() const noexcept -> const VideoStreamInfo& {
        return video_info_;
    }
    [[nodiscard]] auto current_packet() const noexcept -> const SourcePacketView& {
        return packet_view_;
    }

    // Skips non-video packets internally; callers only ever see video or EOS.
    [[nodiscard]] auto read_next(std::string& error) -> ReadStatus;

private:
    AVFormatContext* format_ctx_{nullptr};
    AVPacket* packet_{nullptr};
    std::int32_t video_stream_index_{-1};
    VideoStreamInfo video_info_{};
    SourcePacketView packet_view_{};
    std::string last_error_;
};

}  // namespace kivo::video::video_plane::probe
