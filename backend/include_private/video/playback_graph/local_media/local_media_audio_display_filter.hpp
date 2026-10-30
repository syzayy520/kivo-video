#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "video/video_plane/runtime/local_media/local_video_first_frame_pipeline.hpp"

namespace kivo::video::playback_graph::runtime::local_media {

[[nodiscard]] bool is_local_pcm_audio_display_supported(
    std::string_view codec_name) noexcept;

void copy_displayable_audio_tracks(
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus& status,
    std::uint64_t* track_ids,
    std::size_t& track_count) noexcept;

[[nodiscard]] std::uint64_t preferred_displayable_audio_track_id(
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus& status)
    noexcept;

}  // namespace kivo::video::playback_graph::runtime::local_media
