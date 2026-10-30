#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "video/video_plane/runtime/local_media/local_video_d3d11_upload.hpp"

namespace kivo::video::video_plane::runtime::local_media {

struct LocalVideoFrameBuffer {
    bool valid{false};
    std::int32_t width{0};
    std::int32_t height{0};
    std::int64_t media_time_us{0};
    std::int64_t duration_us{0};
    std::shared_ptr<const std::vector<std::uint8_t>> rgba8888;
};

struct LocalVideoAudioDecoderOpen {
    void* codec_context{nullptr};
    std::string codec_name;
    std::int32_t stream_index{-1};
};

struct LocalVideoAudioPacket {
    std::int32_t stream_index{-1};
    std::int64_t pts{0};
    std::int64_t dts{0};
    std::int64_t duration{0};
    bool is_key_frame{false};
    std::vector<std::uint8_t> data;
    std::vector<kivo::cinema_engine::KivoPacketSideData> side_data;
};

struct LocalVideoSubtitlePacket {
    std::int32_t stream_index{-1};
    std::int64_t start_time_us{0};
    std::int64_t end_time_us{0};
    bool is_key_frame{false};
    std::string codec_name;
    std::vector<std::uint8_t> data;
};

using LocalVideoAudioDecoderOpenCallback =
    bool (*)(void* user_data, LocalVideoAudioDecoderOpen&& request) noexcept;
using LocalVideoAudioPacketCallback =
    void (*)(void* user_data, LocalVideoAudioPacket&& packet) noexcept;
using LocalVideoSubtitlePacketCallback =
    void (*)(void* user_data, LocalVideoSubtitlePacket&& packet) noexcept;

struct LocalVideoRuntimeOpenRequest {
    std::string media_path;
    std::int32_t video_stream_index{-1};
    std::string video_codec;
    std::int32_t audio_stream_index{-1};
    std::string audio_codec;
    std::int32_t subtitle_stream_index{-1};
    std::string subtitle_codec;
    std::int64_t start_position_ms{0};
    void* callback_user_data{nullptr};
    LocalVideoAudioDecoderOpenCallback audio_decoder_open{nullptr};
    LocalVideoAudioPacketCallback audio_packet_ready{nullptr};
    LocalVideoSubtitlePacketCallback subtitle_packet_ready{nullptr};
};

struct LocalVideoRuntimeStats {
    bool active{false};
    bool source_session_connected{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool audio_decoder_open_dispatched{false};
    bool first_frame_decoded{false};
    bool frame_rendered{false};
    bool continuous_rendering{false};
    bool d3d11_upload_available{false};
    bool d3d11_upload_initialized{false};
    bool d3d11_uploaded{false};
    std::uint64_t decoded_frame_count{0};
    std::uint64_t rendered_frame_count{0};
    std::uint64_t d3d11_frames_uploaded{0};
    std::uint64_t d3d11_upload_failures{0};
    std::uint64_t d3d11_upload_last_elapsed_ms{0};
    std::uint64_t d3d11_upload_max_elapsed_ms{0};
    std::uint64_t audio_packets_dispatched{0};
    std::uint64_t subtitle_packets_dispatched{0};
    std::uint64_t last_frame_signature{0};
    std::int64_t last_frame_time_us{0};
    std::int64_t last_frame_duration_us{0};
    std::string source_provider_kind;
    std::string d3d11_device_type;
    std::string d3d11_upload_error;
    std::string audio_error;
    std::string error;
};

class LocalVideoRuntime {
public:
    LocalVideoRuntime();
    ~LocalVideoRuntime();

    LocalVideoRuntime(const LocalVideoRuntime&) = delete;
    LocalVideoRuntime& operator=(const LocalVideoRuntime&) = delete;

    [[nodiscard]] bool start(const LocalVideoRuntimeOpenRequest& request) noexcept;
    [[nodiscard]] int pump_packets(int max_packets) noexcept;
    void select_subtitle_stream(std::int32_t stream_index,
                                const std::string& codec_name) noexcept;
    [[nodiscard]] bool seek_to(std::int64_t target_ms) noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept { return stats_.active; }
    [[nodiscard]] const LocalVideoRuntimeStats& stats() const noexcept { return stats_; }
    [[nodiscard]] const LocalVideoFrameBuffer& frame_buffer() const noexcept {
        return frame_buffer_;
    }

private:
    struct FfmpegSession;

    void dispatch_audio_packet(LocalVideoAudioPacket&& packet) noexcept;
    void dispatch_subtitle_packet(LocalVideoSubtitlePacket&& packet) noexcept;

    LocalVideoRuntimeStats stats_{};
    LocalVideoFrameBuffer frame_buffer_{};
    LocalVideoD3d11Upload d3d11_upload_{};
    std::string media_path_{};
    std::int32_t video_stream_index_{-1};
    std::string video_codec_{};
    std::int32_t audio_stream_index_{-1};
    std::string audio_codec_{};
    std::int32_t subtitle_stream_index_{-1};
    std::string subtitle_codec_{};
    std::int64_t discard_before_media_time_us_{-1};
    void* callback_user_data_{nullptr};
    LocalVideoAudioPacketCallback audio_packet_ready_{nullptr};
    LocalVideoSubtitlePacketCallback subtitle_packet_ready_{nullptr};
    int next_packet_index_{0};
    std::unique_ptr<FfmpegSession> ffmpeg_session_;
};

}  // namespace kivo::video::video_plane::runtime::local_media
