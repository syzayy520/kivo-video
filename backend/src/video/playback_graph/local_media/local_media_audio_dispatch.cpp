#include "video/playback_graph/local_media/local_media_audio_dispatch.hpp"

#include <utility>

namespace kivo::video::playback_graph::local_media {

void copy_audio_runtime_stats(
    const kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioRuntime& runtime,
    LocalMediaContinuousStats& stats) noexcept {
    const auto& audio = runtime.stats();
    stats.audio_decode_started = audio.decode_started;
    stats.audio_output_started = audio.output_started;
    stats.audio_packets_submitted = audio.packets_submitted;
    stats.audio_frames_decoded = audio.frames_decoded;
    stats.audio_frames_written = audio.frames_written;
    stats.audio_flush_count = audio.flush_count;
    stats.audio_writer_reset_count = audio.writer_reset_count;
    stats.audio_stale_packets_discarded = audio.stale_packets_discarded;
    stats.audio_codec = audio.codec_name;
    stats.audio_error = audio.error;
}

void submit_audio_packet(
    kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioRuntime& runtime,
    kivo::video::video_plane::runtime::local_media::LocalVideoAudioPacket packet,
    LocalMediaContinuousStats& stats) noexcept {
    if (!runtime.is_active()) {
        return;
    }

    kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioPacket audio_packet;
    audio_packet.stream_index = packet.stream_index;
    audio_packet.pts = packet.pts;
    audio_packet.dts = packet.dts;
    audio_packet.duration = packet.duration;
    audio_packet.is_key_frame = packet.is_key_frame;
    audio_packet.data = std::move(packet.data);
    audio_packet.side_data = std::move(packet.side_data);
    (void)runtime.submit_packet(std::move(audio_packet));
    copy_audio_runtime_stats(runtime, stats);
}

}  // namespace kivo::video::playback_graph::local_media
