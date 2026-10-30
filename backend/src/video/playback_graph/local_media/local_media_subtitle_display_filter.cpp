#include "video/playback_graph/local_media/local_media_subtitle_display_filter.hpp"

#include <string>

#include "kivo/video/playback_graph/track/track_inventory_snapshot.hpp"

namespace kivo::video::playback_graph::runtime::local_media {
namespace {

[[nodiscard]] std::string lower_ascii(std::string_view value) {
    std::string lowered(value);
    for (char& ch : lowered) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return lowered;
}

[[nodiscard]] std::uint64_t track_id_from_stream_index(
    const std::int32_t stream_index) noexcept {
    return stream_index >= 0 ? static_cast<std::uint64_t>(stream_index) + 1u : 0u;
}

}  // namespace

bool is_embedded_subtitle_display_supported(const std::string_view codec_name) noexcept {
    const std::string codec = lower_ascii(codec_name);
    return codec == "subrip" || codec == "srt" || codec == "ass" || codec == "ssa" ||
           codec == "webvtt" || codec == "mov_text" || codec == "text";
}

void copy_displayable_subtitle_tracks(
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus& status,
    std::uint64_t* const track_ids,
    std::size_t& track_count) noexcept {
    track_count = 0;
    if (track_ids == nullptr) {
        return;
    }

    const std::size_t capped =
        status.subtitle_stream_count < TrackInventorySnapshot::kMaxTracks
            ? status.subtitle_stream_count
            : TrackInventorySnapshot::kMaxTracks;
    for (std::size_t index = 0; index < capped; ++index) {
        if (!is_embedded_subtitle_display_supported(status.subtitle_stream_codecs[index])) {
            continue;
        }

        const std::uint64_t track_id =
            track_id_from_stream_index(status.subtitle_stream_indices[index]);
        if (track_id != 0) {
            track_ids[track_count++] = track_id;
        }
    }
}

}  // namespace kivo::video::playback_graph::runtime::local_media
