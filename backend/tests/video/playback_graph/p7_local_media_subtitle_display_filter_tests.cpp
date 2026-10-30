#include "video/playback_graph/local_media/local_media_subtitle_display_filter.hpp"

using kivo::video::playback_graph::runtime::local_media::
    copy_displayable_subtitle_tracks;
using kivo::video::playback_graph::runtime::local_media::
    is_embedded_subtitle_display_supported;
using kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus;

namespace {

[[nodiscard]] bool verify_supported_text_codecs_are_displayable() {
    return is_embedded_subtitle_display_supported("subrip") &&
           is_embedded_subtitle_display_supported("srt") &&
           is_embedded_subtitle_display_supported("ass") &&
           is_embedded_subtitle_display_supported("ssa") &&
           is_embedded_subtitle_display_supported("webvtt") &&
           is_embedded_subtitle_display_supported("mov_text") &&
           is_embedded_subtitle_display_supported("TEXT");
}

[[nodiscard]] bool verify_bitmap_codecs_stay_hidden_from_product_inventory() {
    return !is_embedded_subtitle_display_supported("hdmv_pgs_subtitle") &&
           !is_embedded_subtitle_display_supported("pgs") &&
           !is_embedded_subtitle_display_supported("dvd_subtitle") &&
           !is_embedded_subtitle_display_supported("vobsub") &&
           !is_embedded_subtitle_display_supported("dvb_subtitle") &&
           !is_embedded_subtitle_display_supported("unknown");
}

[[nodiscard]] bool verify_filter_preserves_text_track_ids_only() {
    LocalMediaPipelineStatus status{};
    status.subtitle_stream_count = 5;
    status.subtitle_stream_indices[0] = 4;
    status.subtitle_stream_codecs[0] = "hdmv_pgs_subtitle";
    status.subtitle_stream_indices[1] = 5;
    status.subtitle_stream_codecs[1] = "subrip";
    status.subtitle_stream_indices[2] = 6;
    status.subtitle_stream_codecs[2] = "dvd_subtitle";
    status.subtitle_stream_indices[3] = 7;
    status.subtitle_stream_codecs[3] = "ass";
    status.subtitle_stream_indices[4] = 8;
    status.subtitle_stream_codecs[4] = "dvb_subtitle";

    std::uint64_t track_ids[LocalMediaPipelineStatus::kMaxStreamTracks]{};
    std::size_t track_count = 0;
    copy_displayable_subtitle_tracks(status, track_ids, track_count);

    return track_count == 2 && track_ids[0] == 6 && track_ids[1] == 8;
}

}  // namespace

int main() {
    if (!verify_supported_text_codecs_are_displayable()) {
        return 1;
    }
    if (!verify_bitmap_codecs_stay_hidden_from_product_inventory()) {
        return 2;
    }
    if (!verify_filter_preserves_text_track_ids_only()) {
        return 3;
    }
    return 0;
}
