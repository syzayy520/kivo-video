#pragma once

#include "video/audio_plane/runtime/local_pcm/local_pcm_audio_runtime.hpp"
#include "video/playback_graph/local_media/local_media_continuous_playback.hpp"
#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

namespace kivo::video::playback_graph::local_media {

void copy_audio_runtime_stats(
    const kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioRuntime& runtime,
    LocalMediaContinuousStats& stats) noexcept;

void submit_audio_packet(
    kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioRuntime& runtime,
    kivo::video::video_plane::runtime::local_media::LocalVideoAudioPacket packet,
    LocalMediaContinuousStats& stats) noexcept;

}  // namespace kivo::video::playback_graph::local_media
