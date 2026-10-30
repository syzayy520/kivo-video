#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

namespace {

[[nodiscard]] std::string resolve_sample_path() {
    if (const char* env = std::getenv("KIVO_P10_MEDIA")) {
        if (env[0] != '\0') {
            return std::string(env);
        }
    }

    const std::filesystem::path candidates[] = {
        std::filesystem::path("tests/real/test_h264_aac_35s.mp4"),
        std::filesystem::path("C:/kivo video/tests/real/test_h264_aac_35s.mp4"),
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
    }
    return {};
}

[[nodiscard]] std::uint64_t alternate_audio_track(
    const kivo::video::player_runtime_adapter::AdapterSnapshot& snapshot) {
    for (const auto track_id : snapshot.audio_track_ids) {
        if (track_id != snapshot.selected_audio_track) {
            return track_id;
        }
    }
    return 0;
}

}  // namespace

int main() {
#ifndef KIVO_ENABLE_FFMPEG
    std::cout << "SKIP: ffmpeg disabled\n";
    return 0;
#else
    const std::string sample_path = resolve_sample_path();
    if (sample_path.empty()) {
        std::cout << "SKIP: no local media sample\n";
        return 0;
    }

    using kivo::video::player_runtime_adapter::AdapterCommandStatus;
    using kivo::video::player_runtime_adapter::PlayerRuntimeAdapter;

    PlayerRuntimeAdapter adapter{};
    const auto open = adapter.open_local_media_file(sample_path);
    if (open.status != AdapterCommandStatus::Accepted) {
        const auto probe_query = adapter.query_local_media_playback();
        std::cerr << "FAIL: open_local_media_file rejected"
                  << " p7_error=" << static_cast<int>(open.p7_error)
                  << " pipeline_error=" << probe_query.error << "\n";
        return 1;
    }

    const auto query_after_open = adapter.query_local_media_playback();
    if (!query_after_open.source_opened) {
        std::cerr << "FAIL: source not opened"
                  << " error=" << query_after_open.error << "\n";
        return 2;
    }

    const auto play = adapter.play();
    if (play.status != AdapterCommandStatus::Accepted) {
        std::cerr << "FAIL: play rejected\n";
        return 3;
    }

    const auto query = adapter.query_local_media_playback();
    if (!query.demux_started || !query.video_decode_started) {
        std::cerr << "FAIL: demux/decode not started\n";
        return 4;
    }
    if (!query.first_frame_decoded || !query.frame_pixels_valid) {
        std::cerr << "FAIL: first frame not decoded\n";
        return 5;
    }
    if (!query.frame_rendered || query.width <= 0 || query.height <= 0) {
        std::cerr << "FAIL: frame not rendered\n";
        return 6;
    }

    const auto snapshot = adapter.snapshot();
    if (!snapshot.video_ready) {
        std::cerr << "FAIL: adapter snapshot video_ready false\n";
        return 7;
    }
    if (snapshot.selected_video_track == 0 || snapshot.video_track_ids.empty()) {
        std::cerr << "FAIL: adapter snapshot missing selected video track\n";
        return 8;
    }

    const std::uint64_t target_audio_track = alternate_audio_track(snapshot);
    if (target_audio_track != 0) {
        const auto track_switch = adapter.select_audio_track(target_audio_track);
        if (track_switch.status != AdapterCommandStatus::Accepted) {
            std::cerr << "FAIL: select_audio_track rejected"
                      << " target=" << target_audio_track
                      << " p7_error=" << static_cast<int>(track_switch.p7_error) << "\n";
            return 9;
        }
        for (int attempt = 0; attempt < 20; ++attempt) {
            (void)adapter.pump_local_media_playback(8);
        }
        const auto after_track_switch = adapter.snapshot();
        const auto query_after_track_switch = adapter.query_local_media_playback();
        if (after_track_switch.selected_audio_track != target_audio_track ||
            !query_after_track_switch.active ||
            !query_after_track_switch.p5_audio_packet_boundary_connected) {
            std::cerr << "FAIL: audio track switch not reflected"
                      << " selected=" << after_track_switch.selected_audio_track
                      << " target=" << target_audio_track
                      << " active=" << query_after_track_switch.active
                      << " p5_audio_boundary="
                      << query_after_track_switch.p5_audio_packet_boundary_connected << "\n";
            return 10;
        }
    }

    std::cout << "PASS local media first frame "
              << query.width << "x" << query.height << " codec=" << query.video_codec << "\n";
    return 0;
#endif
}
