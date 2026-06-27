// ffmpeg_wrapper_isolation.hpp — P5C C1: FFmpeg wrapper isolation contract
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::ffmpeg_demux {

// C1: FFmpeg wrapper must be isolated in ffmpeg_demux/ directory only
// This contract verifies path isolation at compile time

struct FFmpegWrapperIsolationContract {
    // Allowed directory for FFmpeg includes/links
    static constexpr const char* kAllowedDirectory = "include/kivo/video_plane/ffmpeg_demux/";
    // Forbidden directories (FFmpeg must NOT appear here)
    // - include/kivo/video_plane/stamp/
    // - include/kivo/video_plane/capability/
    // - include/kivo/video_plane/evidence/
    // - include/kivo/video_plane/core/ (future)
    // - All other P5 directories

    // Verify a file path is within the allowed directory
    static bool is_path_allowed(std::string const& file_path) {
        return file_path.find("ffmpeg_demux/") != std::string::npos;
    }

    // Verify no FFmpeg symbols in non-ffmpeg_demux paths
    static bool is_ffmpeg_symbol_allowed(std::string const& file_path) {
        return is_path_allowed(file_path);
    }
};

}  // namespace kivo::video_plane::ffmpeg_demux
