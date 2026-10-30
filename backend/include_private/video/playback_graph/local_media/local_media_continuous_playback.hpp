#pragma once

#include <chrono>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <memory>
#include <string>
#include <thread>

#include "video/audio_plane/runtime/local_pcm/local_pcm_audio_runtime.hpp"
#include "video/playback_graph/p8_bridge/subtitle_frame_p8_bridge.hpp"
#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

namespace kivo::video::playback_graph::local_media {

using LocalMediaFrameBuffer =
    kivo::video::video_plane::runtime::local_media::LocalVideoFrameBuffer;

struct LocalMediaContinuousStats {
    bool active{false};
    bool source_session_connected{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool audio_decode_started{false};
    bool audio_output_started{false};
    bool first_frame_decoded{false};
    bool frame_rendered{false};
    bool continuous_rendering{false};
    bool d3d11_upload_available{false};
    bool d3d11_upload_initialized{false};
    bool d3d11_uploaded{false};
    bool p5_local_video_runtime_connected{false};
    bool p5_audio_packet_boundary_connected{false};
    bool p5_subtitle_packet_boundary_connected{false};
    bool p8_embedded_packet_ingest_connected{false};
    bool p8_pgs_bitmap_render_connected{false};
    std::uint64_t decoded_frame_count{0};
    std::uint64_t rendered_frame_count{0};
    std::uint64_t d3d11_frames_uploaded{0};
    std::uint64_t d3d11_upload_failures{0};
    std::uint64_t d3d11_upload_last_elapsed_ms{0};
    std::uint64_t d3d11_upload_max_elapsed_ms{0};
    std::uint64_t p5_audio_packets_dispatched{0};
    std::uint64_t p5_subtitle_packets_dispatched{0};
    std::uint64_t p8_subtitle_packets_submitted{0};
    std::uint64_t p8_unsupported_feature_count{0};
    std::uint64_t audio_packets_submitted{0};
    std::uint64_t audio_frames_decoded{0};
    std::uint64_t audio_frames_written{0};
    std::uint64_t audio_flush_count{0};
    std::uint64_t audio_writer_reset_count{0};
    std::uint64_t audio_stale_packets_discarded{0};
    std::uint64_t p7_background_pump_count{0};
    std::uint64_t p7_background_pump_last_elapsed_ms{0};
    std::uint64_t p7_background_pump_max_elapsed_ms{0};
    std::uint64_t p7_background_pump_total_elapsed_ms{0};
    std::uint64_t p7_background_pump_last_new_frames{0};
    std::uint64_t p7_background_pump_packet_budget{0};
    std::uint64_t p7_background_pump_max_packet_budget{0};
    bool p7_seek_preroll_active{false};
    std::uint64_t p7_seek_preroll_completed_count{0};
    std::uint64_t p7_seek_preroll_last_elapsed_ms{0};
    std::uint64_t p7_seek_preroll_max_elapsed_ms{0};
    std::int64_t position_ms{0};
    std::int64_t playback_elapsed_ms{0};
    std::uint64_t last_frame_signature{0};
    std::int64_t last_frame_time_us{0};
    std::int64_t last_frame_duration_us{0};
    std::string source_provider_kind;
    std::string d3d11_device_type;
    std::string d3d11_upload_error;
    std::string audio_codec;
    std::string audio_error;
    std::string error;
};

class LocalMediaContinuousPlayback {
public:
    LocalMediaContinuousPlayback();
    ~LocalMediaContinuousPlayback();

    LocalMediaContinuousPlayback(const LocalMediaContinuousPlayback&) = delete;
    LocalMediaContinuousPlayback& operator=(const LocalMediaContinuousPlayback&) = delete;

    [[nodiscard]] bool start(
        const std::string& media_path,
        int video_stream_index,
        const std::string& video_codec,
        int audio_stream_index,
        const std::string& audio_codec,
        int subtitle_stream_index,
        const std::string& subtitle_codec,
        std::uint64_t subtitle_track_id,
        std::int64_t start_position_ms = 0) noexcept;
    [[nodiscard]] int pump_packets(int max_packets) noexcept;
    void select_subtitle_track(std::uint64_t track_id,
                               int stream_index,
                               const std::string& codec_name) noexcept;
    void disable_subtitle() noexcept;
    [[nodiscard]] kivo::video::playback_graph::SubtitleFrameSnapshot query_subtitle_frame(
        std::int64_t media_time_ms,
        std::int64_t delay_ms,
        std::uint64_t track_id,
        bool subtitle_enabled) const noexcept;
    void pause() noexcept;
    void resume() noexcept;
    [[nodiscard]] bool seek_to(std::int64_t target_ms) noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept;
    [[nodiscard]] LocalMediaContinuousStats stats() const noexcept;
    [[nodiscard]] LocalMediaFrameBuffer frame_buffer() const noexcept;

private:
    using LocalVideoAudioDecoderOpen =
        kivo::video::video_plane::runtime::local_media::LocalVideoAudioDecoderOpen;
    using LocalVideoAudioPacket =
        kivo::video::video_plane::runtime::local_media::LocalVideoAudioPacket;
    using LocalVideoSubtitlePacket =
        kivo::video::video_plane::runtime::local_media::LocalVideoSubtitlePacket;

    static bool open_audio_decoder_callback(
        void* user_data,
        LocalVideoAudioDecoderOpen&& request) noexcept;
    static void audio_packet_callback(
        void* user_data,
        LocalVideoAudioPacket&& packet) noexcept;
    static void subtitle_packet_callback(
        void* user_data,
        LocalVideoSubtitlePacket&& packet) noexcept;

    [[nodiscard]] bool open_audio_decoder(LocalVideoAudioDecoderOpen&& request) noexcept;
    void submit_audio_packet(LocalVideoAudioPacket&& packet) noexcept;
    void submit_subtitle_packet(LocalVideoSubtitlePacket&& packet) noexcept;
    void copy_video_runtime_stats() noexcept;
    void copy_video_frame_buffer() noexcept;
    [[nodiscard]] int pump_packets_runtime(int max_packets) noexcept;
    void start_background_pump() noexcept;
    void stop_background_pump() noexcept;
    void background_pump_loop() noexcept;

    mutable std::recursive_mutex mutex_{};
    mutable std::mutex runtime_mutex_{};
    std::atomic_bool pump_stop_requested_{false};
    std::thread pump_thread_{};
    LocalMediaContinuousStats stats_{};
    LocalMediaFrameBuffer frame_buffer_{};
    std::int64_t timeline_base_ms_{0};
    std::int64_t paused_position_ms_{0};
    bool paused_{false};
    bool waiting_for_seek_frame_{false};
    bool seek_preroll_anchor_valid_{false};
    std::chrono::steady_clock::time_point timeline_anchor_{};
    std::chrono::steady_clock::time_point seek_preroll_anchor_{};
    kivo::video::video_plane::runtime::local_media::LocalVideoRuntime video_runtime_{};
    kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioRuntime audio_runtime_{};
    kivo::video::playback_graph::runtime::SubtitleFrameP8Bridge subtitle_frame_bridge_{};
};

}  // namespace kivo::video::playback_graph::local_media
