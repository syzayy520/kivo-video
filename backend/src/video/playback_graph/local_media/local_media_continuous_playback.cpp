#include "video/playback_graph/local_media/local_media_continuous_playback.hpp"

#include "video/playback_graph/local_media/local_media_audio_dispatch.hpp"

namespace kivo::video::playback_graph::local_media {
namespace {

constexpr int kBackgroundPumpPacketBudget = 8;
constexpr int kBackgroundSeekPrerollPacketBudget = 256;
constexpr auto kBackgroundPumpFrameSleep = std::chrono::milliseconds(1);
constexpr auto kBackgroundPumpIdleSleep = std::chrono::milliseconds(2);

[[nodiscard]] std::int64_t elapsed_ms_since(
    const std::chrono::steady_clock::time_point anchor) noexcept {
    const auto elapsed = std::chrono::steady_clock::now() - anchor;
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

[[nodiscard]] std::int64_t frame_position_ms(
    const LocalMediaFrameBuffer& frame_buffer) noexcept {
    return frame_buffer.media_time_us > 0 ? frame_buffer.media_time_us / 1000 : 0;
}

}  // namespace

LocalMediaContinuousPlayback::LocalMediaContinuousPlayback() = default;

LocalMediaContinuousPlayback::~LocalMediaContinuousPlayback() {
    close();
}

bool LocalMediaContinuousPlayback::is_active() const noexcept {
    std::lock_guard lock(mutex_);
    return stats_.active;
}

LocalMediaContinuousStats LocalMediaContinuousPlayback::stats() const noexcept {
    std::lock_guard lock(mutex_);
    return stats_;
}

LocalMediaFrameBuffer LocalMediaContinuousPlayback::frame_buffer() const noexcept {
    std::lock_guard lock(mutex_);
    return frame_buffer_;
}

void LocalMediaContinuousPlayback::copy_video_runtime_stats() noexcept {
    const auto& video = video_runtime_.stats();
    stats_.active = video.active;
    stats_.source_session_connected = video.source_session_connected;
    stats_.demux_started = video.demux_started;
    stats_.video_decode_started = video.video_decode_started;
    stats_.first_frame_decoded = video.first_frame_decoded;
    stats_.frame_rendered = video.frame_rendered;
    stats_.continuous_rendering = video.continuous_rendering;
    stats_.d3d11_upload_available = video.d3d11_upload_available;
    stats_.d3d11_upload_initialized = video.d3d11_upload_initialized;
    stats_.d3d11_uploaded = video.d3d11_uploaded;
    stats_.p5_local_video_runtime_connected =
        video.active && video.demux_started && video.video_decode_started;
    stats_.p5_audio_packet_boundary_connected =
        video.audio_decoder_open_dispatched || video.audio_packets_dispatched > 0;
    stats_.p5_subtitle_packet_boundary_connected = video.subtitle_packets_dispatched > 0;
    stats_.decoded_frame_count = video.decoded_frame_count;
    stats_.rendered_frame_count = video.rendered_frame_count;
    stats_.d3d11_frames_uploaded = video.d3d11_frames_uploaded;
    stats_.d3d11_upload_failures = video.d3d11_upload_failures;
    stats_.d3d11_upload_last_elapsed_ms = video.d3d11_upload_last_elapsed_ms;
    stats_.d3d11_upload_max_elapsed_ms = video.d3d11_upload_max_elapsed_ms;
    stats_.p5_audio_packets_dispatched = video.audio_packets_dispatched;
    stats_.p5_subtitle_packets_dispatched = video.subtitle_packets_dispatched;
    stats_.last_frame_signature = video.last_frame_signature;
    stats_.last_frame_time_us = video.last_frame_time_us;
    stats_.last_frame_duration_us = video.last_frame_duration_us;
    stats_.source_provider_kind = video.source_provider_kind;
    stats_.d3d11_device_type = video.d3d11_device_type;
    stats_.d3d11_upload_error = video.d3d11_upload_error;
    const auto subtitle_stats = subtitle_frame_bridge_.stats();
    stats_.p8_embedded_packet_ingest_connected =
        subtitle_stats.embedded_packet_ingest_connected;
    stats_.p8_pgs_bitmap_render_connected =
        subtitle_stats.pgs_bitmap_render_connected;
    stats_.p8_subtitle_packets_submitted =
        subtitle_stats.submitted_packet_count;
    stats_.p8_unsupported_feature_count =
        subtitle_stats.unsupported_feature_count;
    if (!video.audio_error.empty()) {
        stats_.audio_error = video.audio_error;
    }
    if (!video.error.empty()) {
        stats_.error = video.error;
    }
}

void LocalMediaContinuousPlayback::copy_video_frame_buffer() noexcept {
    const auto& video_frame = video_runtime_.frame_buffer();
    frame_buffer_.valid = video_frame.valid;
    frame_buffer_.width = video_frame.width;
    frame_buffer_.height = video_frame.height;
    frame_buffer_.media_time_us = video_frame.media_time_us;
    frame_buffer_.duration_us = video_frame.duration_us;
    frame_buffer_.rgba8888 = video_frame.rgba8888;
}

int LocalMediaContinuousPlayback::pump_packets_runtime(const int max_packets) noexcept {
    {
        std::lock_guard state_lock(mutex_);
        if (!stats_.active) {
            return 0;
        }
        if (paused_) {
            stats_.position_ms = paused_position_ms_;
            stats_.playback_elapsed_ms = paused_position_ms_;
            return 0;
        }
    }

    int new_frames = 0;
    {
        std::lock_guard runtime_lock(runtime_mutex_);
        if (!video_runtime_.is_active()) {
            return 0;
        }

        new_frames = video_runtime_.pump_packets(max_packets);

        std::lock_guard state_lock(mutex_);
        copy_video_runtime_stats();
        if (new_frames > 0 || (!frame_buffer_.valid && video_runtime_.frame_buffer().valid)) {
            copy_video_frame_buffer();
        }
        copy_audio_runtime_stats(audio_runtime_, stats_);

        if (frame_buffer_.valid && new_frames > 0) {
            if (waiting_for_seek_frame_ && seek_preroll_anchor_valid_) {
                const auto elapsed =
                    static_cast<std::uint64_t>(elapsed_ms_since(seek_preroll_anchor_));
                ++stats_.p7_seek_preroll_completed_count;
                stats_.p7_seek_preroll_last_elapsed_ms = elapsed;
                if (elapsed > stats_.p7_seek_preroll_max_elapsed_ms) {
                    stats_.p7_seek_preroll_max_elapsed_ms = elapsed;
                }
            }
            timeline_base_ms_ = frame_position_ms(frame_buffer_);
            timeline_anchor_ = std::chrono::steady_clock::now();
            waiting_for_seek_frame_ = false;
            seek_preroll_anchor_valid_ = false;
        }
        stats_.p7_seek_preroll_active = waiting_for_seek_frame_;
        stats_.position_ms = waiting_for_seek_frame_
                                 ? timeline_base_ms_
                                 : timeline_base_ms_ + elapsed_ms_since(timeline_anchor_);
        stats_.playback_elapsed_ms = stats_.position_ms;
    }
    return new_frames;
}

int LocalMediaContinuousPlayback::pump_packets(const int max_packets) noexcept {
    return pump_packets_runtime(max_packets);
}

void LocalMediaContinuousPlayback::select_subtitle_track(
    const std::uint64_t track_id,
    const int stream_index,
    const std::string& codec_name) noexcept {
    if (track_id == 0 || stream_index < 0 || codec_name.empty()) {
        return;
    }
    {
        std::lock_guard state_lock(mutex_);
        subtitle_frame_bridge_.attach(track_id, false);
    }
    {
        std::lock_guard runtime_lock(runtime_mutex_);
        video_runtime_.select_subtitle_stream(stream_index, codec_name);
        std::lock_guard state_lock(mutex_);
        copy_video_runtime_stats();
    }
}

void LocalMediaContinuousPlayback::disable_subtitle() noexcept {
    {
        std::lock_guard state_lock(mutex_);
        subtitle_frame_bridge_.detach();
    }
    {
        std::lock_guard runtime_lock(runtime_mutex_);
        video_runtime_.select_subtitle_stream(-1, {});
        std::lock_guard state_lock(mutex_);
        copy_video_runtime_stats();
    }
}

kivo::video::playback_graph::SubtitleFrameSnapshot
LocalMediaContinuousPlayback::query_subtitle_frame(
    const std::int64_t media_time_ms,
    const std::int64_t delay_ms,
    const std::uint64_t track_id,
    const bool subtitle_enabled) const noexcept {
    std::lock_guard lock(mutex_);
    return subtitle_frame_bridge_.query(media_time_ms, delay_ms, track_id, subtitle_enabled);
}

void LocalMediaContinuousPlayback::start_background_pump() noexcept {
    stop_background_pump();
    pump_stop_requested_.store(false, std::memory_order_release);
    pump_thread_ = std::thread([this]() { background_pump_loop(); });
}

void LocalMediaContinuousPlayback::stop_background_pump() noexcept {
    pump_stop_requested_.store(true, std::memory_order_release);
    if (pump_thread_.joinable()) {
        pump_thread_.join();
    }
}

void LocalMediaContinuousPlayback::background_pump_loop() noexcept {
    while (!pump_stop_requested_.load(std::memory_order_acquire)) {
        int new_frames = 0;
        std::uint64_t elapsed_ms = 0;
        int packet_budget = kBackgroundPumpPacketBudget;
        bool seek_preroll = false;
        {
            std::lock_guard lock(mutex_);
            seek_preroll = waiting_for_seek_frame_;
            if (seek_preroll) {
                packet_budget = kBackgroundSeekPrerollPacketBudget;
            }
        }
        {
            const auto pump_start = std::chrono::steady_clock::now();
            new_frames = pump_packets_runtime(packet_budget);
            elapsed_ms = static_cast<std::uint64_t>(elapsed_ms_since(pump_start));
        }
        {
            std::lock_guard lock(mutex_);
            ++stats_.p7_background_pump_count;
            stats_.p7_background_pump_last_elapsed_ms = elapsed_ms;
            if (elapsed_ms > stats_.p7_background_pump_max_elapsed_ms) {
                stats_.p7_background_pump_max_elapsed_ms = elapsed_ms;
            }
            stats_.p7_background_pump_total_elapsed_ms += elapsed_ms;
            stats_.p7_background_pump_last_new_frames =
                new_frames > 0 ? static_cast<std::uint64_t>(new_frames) : 0;
            stats_.p7_background_pump_packet_budget = static_cast<std::uint64_t>(packet_budget);
            if (static_cast<std::uint64_t>(packet_budget) >
                stats_.p7_background_pump_max_packet_budget) {
                stats_.p7_background_pump_max_packet_budget =
                    static_cast<std::uint64_t>(packet_budget);
            }
        }
        if (seek_preroll && new_frames <= 0) {
            std::this_thread::yield();
            continue;
        }
        std::this_thread::sleep_for(
            new_frames > 0 ? kBackgroundPumpFrameSleep : kBackgroundPumpIdleSleep);
    }
}

void LocalMediaContinuousPlayback::pause() noexcept {
    std::lock_guard lock(mutex_);
    if (!stats_.active || paused_) {
        return;
    }
    paused_position_ms_ = timeline_base_ms_ + elapsed_ms_since(timeline_anchor_);
    paused_ = true;
    audio_runtime_.pause();
    stats_.position_ms = paused_position_ms_;
    stats_.playback_elapsed_ms = paused_position_ms_;
}

void LocalMediaContinuousPlayback::resume() noexcept {
    std::lock_guard lock(mutex_);
    if (!stats_.active || !paused_) {
        return;
    }
    timeline_base_ms_ = paused_position_ms_;
    timeline_anchor_ = std::chrono::steady_clock::now();
    paused_ = false;
    audio_runtime_.resume();
}

bool LocalMediaContinuousPlayback::seek_to(const std::int64_t target_ms) noexcept {
    {
        std::lock_guard lock(mutex_);
        if (!stats_.active || target_ms < 0) {
            return false;
        }
    }

    {
        std::lock_guard runtime_lock(runtime_mutex_);
        if (!video_runtime_.seek_to(target_ms)) {
            std::lock_guard state_lock(mutex_);
            copy_video_runtime_stats();
            return false;
        }
    }

    {
        std::lock_guard lock(mutex_);
        frame_buffer_ = {};
        audio_runtime_.flush();
        timeline_base_ms_ = target_ms;
        paused_position_ms_ = target_ms;
        timeline_anchor_ = std::chrono::steady_clock::now();
        waiting_for_seek_frame_ = true;
        seek_preroll_anchor_ = timeline_anchor_;
        seek_preroll_anchor_valid_ = true;
        stats_.p7_seek_preroll_active = true;
        stats_.position_ms = target_ms;
        stats_.playback_elapsed_ms = target_ms;
        copy_audio_runtime_stats(audio_runtime_, stats_);
    }
    {
        std::lock_guard runtime_lock(runtime_mutex_);
        std::lock_guard state_lock(mutex_);
        copy_video_runtime_stats();
    }
    return true;
}

void LocalMediaContinuousPlayback::close() noexcept {
    stop_background_pump();
    std::lock_guard runtime_lock(runtime_mutex_);
    std::lock_guard lock(mutex_);
    video_runtime_.close();
    audio_runtime_.close();
    subtitle_frame_bridge_.reset();
    stats_ = {};
    frame_buffer_ = {};
    timeline_base_ms_ = 0;
    paused_position_ms_ = 0;
    paused_ = false;
    waiting_for_seek_frame_ = false;
    seek_preroll_anchor_valid_ = false;
}

}  // namespace kivo::video::playback_graph::local_media
