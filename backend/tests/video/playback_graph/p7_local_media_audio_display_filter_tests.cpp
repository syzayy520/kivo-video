#include "video/playback_graph/local_media/local_media_audio_display_filter.hpp"

using kivo::video::playback_graph::runtime::local_media::
    copy_displayable_audio_tracks;
using kivo::video::playback_graph::runtime::local_media::
    is_local_pcm_audio_display_supported;
using kivo::video::playback_graph::runtime::local_media::
    preferred_displayable_audio_track_id;
using kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus;

namespace {

[[nodiscard]] bool verify_pcm_decodable_codecs_are_displayable() {
    return is_local_pcm_audio_display_supported("dts") &&
           is_local_pcm_audio_display_supported("dca") &&
           is_local_pcm_audio_display_supported("aac") &&
           is_local_pcm_audio_display_supported("flac") &&
           is_local_pcm_audio_display_supported("eac3") &&
           is_local_pcm_audio_display_supported("PCM_S16LE");
}

[[nodiscard]] bool verify_passthrough_only_codecs_stay_hidden() {
    return !is_local_pcm_audio_display_supported("truehd") &&
           !is_local_pcm_audio_display_supported("mlp") &&
           !is_local_pcm_audio_display_supported("TRUEHD") &&
           !is_local_pcm_audio_display_supported("");
}

[[nodiscard]] bool verify_filter_preserves_supported_track_ids_only() {
    LocalMediaPipelineStatus status{};
    status.audio_stream_count = 4;
    status.audio_stream_indices[0] = 1;
    status.audio_stream_codecs[0] = "truehd";
    status.audio_stream_indices[1] = 2;
    status.audio_stream_codecs[1] = "dts";
    status.audio_stream_indices[2] = 3;
    status.audio_stream_codecs[2] = "mlp";
    status.audio_stream_indices[3] = 4;
    status.audio_stream_codecs[3] = "aac";

    std::uint64_t track_ids[LocalMediaPipelineStatus::kMaxStreamTracks]{};
    std::size_t track_count = 0;
    copy_displayable_audio_tracks(status, track_ids, track_count);

    return track_count == 2 && track_ids[0] == 3 && track_ids[1] == 5 &&
           preferred_displayable_audio_track_id(status) == 3;
}

[[nodiscard]] bool verify_all_passthrough_audio_yields_no_product_track() {
    LocalMediaPipelineStatus status{};
    status.audio_stream_count = 2;
    status.audio_stream_indices[0] = 1;
    status.audio_stream_codecs[0] = "truehd";
    status.audio_stream_indices[1] = 2;
    status.audio_stream_codecs[1] = "mlp";

    std::uint64_t track_ids[LocalMediaPipelineStatus::kMaxStreamTracks]{};
    std::size_t track_count = 9;
    copy_displayable_audio_tracks(status, track_ids, track_count);

    return track_count == 0 && preferred_displayable_audio_track_id(status) == 0;
}

}  // namespace

int main() {
    if (!verify_pcm_decodable_codecs_are_displayable()) {
        return 1;
    }
    if (!verify_passthrough_only_codecs_stay_hidden()) {
        return 2;
    }
    if (!verify_filter_preserves_supported_track_ids_only()) {
        return 3;
    }
    if (!verify_all_passthrough_audio_yields_no_product_track()) {
        return 4;
    }
    return 0;
}
