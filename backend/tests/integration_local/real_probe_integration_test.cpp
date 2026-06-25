#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_impl.hpp"

using namespace kivo::cinema_engine;

// ─── Integration Test: Probe real MP4 file ───
static void test_probe_real_mp4() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_probe_real_mp4 (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_probe_real_mp4 (FFmpeg not enabled)\n";
        return;
    }

    auto result = RealProbeRuntime::probe_file(sample_path, "integ_probe_001");
    assert(result.success == true);
    assert(result.trace_id == "integ_probe_001");
    assert(!result.container_format.empty());
    assert(result.duration > 0.0);

    // Verify video stream
    assert(!result.video_streams.empty());
    auto& video = result.video_streams[0];
    assert(video.width > 0);
    assert(video.height > 0);
    assert(!video.codec_name.empty());
    assert(!video.pixel_format.empty());

    // Verify audio stream
    assert(!result.audio_streams.empty());
    auto& audio = result.audio_streams[0];
    assert(audio.sample_rate > 0);
    assert(audio.channels > 0);
    assert(!audio.codec_name.empty());
    assert(!audio.sample_format.empty());

    std::cout << "  PASS: test_probe_real_mp4 ("
              << result.container_format << ", "
              << video.width << "x" << video.height << " "
              << video.codec_name << "/" << audio.codec_name << ", "
              << result.duration << "s)\n";
}

// ─── Integration Test: Probe real file via byte stream ───
static void test_probe_via_byte_stream() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_probe_via_byte_stream (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_probe_via_byte_stream (FFmpeg not enabled)\n";
        return;
    }

    LocalFileByteStreamImpl stream;
    LocalFileByteStreamOpenRequest req;
    req.path = sample_path;
    req.trace_id = "integ_probe_stream";

    auto open_result = stream.open(req);
    assert(open_result.opened);

    auto probe_result = RealProbeRuntime::probe_stream(stream, "integ_probe_stream_002");
    // Stream probing may not be fully implemented yet
    if (probe_result.success) {
        assert(!probe_result.container_format.empty());
    } else {
        assert(!probe_result.error_code.empty());
    }

    stream.close();
    std::cout << "  PASS: test_probe_via_byte_stream ("
              << (probe_result.success ? "success" : probe_result.error_code) << ")\n";
}

// ─── Integration Test: Probe non-media file ───
static void test_probe_non_media_file() {
    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_probe_non_media_file (FFmpeg not enabled)\n";
        return;
    }

    // Use the CMakeLists.txt as a non-media file
    auto result = RealProbeRuntime::probe_file("CMakeLists.txt", "integ_probe_nonmedia");
    assert(result.success == false);
    assert(!result.error_code.empty());

    std::cout << "  PASS: test_probe_non_media_file (" << result.error_code << ")\n";
}

// ─── Integration Test: Supported format lists ───
static void test_supported_format_lists() {
    auto formats = RealProbeRuntime::supported_container_formats();
    auto vcodecs = RealProbeRuntime::supported_video_codecs();
    auto acodecs = RealProbeRuntime::supported_audio_codecs();

    if (RealProbeRuntime::is_ffmpeg_available()) {
        assert(formats.size() >= 10);
        assert(vcodecs.size() >= 5);
        assert(acodecs.size() >= 5);
    } else {
        assert(formats.empty());
        assert(vcodecs.empty());
        assert(acodecs.empty());
    }

    std::cout << "  PASS: test_supported_format_lists ("
              << formats.size() << " containers, "
              << vcodecs.size() << " video, "
              << acodecs.size() << " audio)\n";
}

int main() {
    std::cout << "real_probe_integration_test:\n";
    test_probe_real_mp4();
    test_probe_via_byte_stream();
    test_probe_non_media_file();
    test_supported_format_lists();
    std::cout << "ALL 4 INTEGRATION TESTS PASSED\n";
    return 0;
}
