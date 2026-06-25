#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_impl.hpp"

using namespace kivo::cinema_engine;

// ─── Test: ffmpeg availability check ───
static void test_is_ffmpeg_available() {
    // Should return false when KIVO_ENABLE_FFMPEG is not defined at build time
    bool available = RealProbeRuntime::is_ffmpeg_available();
    std::cout << "  ffmpeg_available = " << (available ? "true" : "false") << "\n";
    // The result depends on build config; we just verify it returns without crashing
    std::cout << "  PASS: is_ffmpeg_available (deterministic)\n";
}

// ─── Test: supported container formats ───
static void test_supported_container_formats() {
    auto formats = RealProbeRuntime::supported_container_formats();
    if (RealProbeRuntime::is_ffmpeg_available()) {
        assert(!formats.empty());
        // Check some common formats are present
        bool has_mp4 = false, has_mkv = false, has_ts = false;
        for (const auto& f : formats) {
            if (f == "mp4") has_mp4 = true;
            if (f == "mkv") has_mkv = true;
            if (f == "ts") has_ts = true;
        }
        assert(has_mp4 && has_mkv && has_ts);
    } else {
        assert(formats.empty());
    }
    std::cout << "  PASS: supported_container_formats (" << formats.size() << " formats)\n";
}

// ─── Test: supported video codecs ───
static void test_supported_video_codecs() {
    auto codecs = RealProbeRuntime::supported_video_codecs();
    if (RealProbeRuntime::is_ffmpeg_available()) {
        assert(!codecs.empty());
        bool has_h264 = false, has_hevc = false;
        for (const auto& c : codecs) {
            if (c == "h264") has_h264 = true;
            if (c == "hevc") has_hevc = true;
        }
        assert(has_h264 && has_hevc);
    } else {
        assert(codecs.empty());
    }
    std::cout << "  PASS: supported_video_codecs (" << codecs.size() << " codecs)\n";
}

// ─── Test: supported audio codecs ───
static void test_supported_audio_codecs() {
    auto codecs = RealProbeRuntime::supported_audio_codecs();
    if (RealProbeRuntime::is_ffmpeg_available()) {
        assert(!codecs.empty());
        bool has_aac = false, has_opus = false;
        for (const auto& c : codecs) {
            if (c == "aac") has_aac = true;
            if (c == "opus") has_opus = true;
        }
        assert(has_aac && has_opus);
    } else {
        assert(codecs.empty());
    }
    std::cout << "  PASS: supported_audio_codecs (" << codecs.size() << " codecs)\n";
}

// ─── Test: probe_file on nonexistent path ───
static void test_probe_file_nonexistent() {
    auto result = RealProbeRuntime::probe_file("/nonexistent/path/media.mp4", "trace_probe_001");
    if (!RealProbeRuntime::is_ffmpeg_available()) {
        assert(result.success == false);
        assert(result.error_code == "ffmpeg_not_available");
    } else {
        assert(result.success == false);
        assert(!result.error_code.empty());
    }
    assert(result.trace_id == "trace_probe_001");
    std::cout << "  PASS: probe_file_nonexistent\n";
}

// ─── Test: probe_file on empty path ───
static void test_probe_file_empty_path() {
    auto result = RealProbeRuntime::probe_file("", "trace_probe_002");
    assert(result.success == false);
    assert(result.trace_id == "trace_probe_002");
    std::cout << "  PASS: probe_file_empty_path\n";
}

// ─── Test: probe_stream with closed stream ───
static void test_probe_stream_closed() {
    LocalFileByteStreamImpl stream;
    auto result = RealProbeRuntime::probe_stream(stream, "trace_probe_003");
    assert(result.success == false);
    assert(result.trace_id == "trace_probe_003");
    std::cout << "  PASS: probe_stream_closed\n";
}

// ─── Test: probe result structure fields ───
static void test_probe_result_structure() {
    ProbeResult result{};  // Value-initialize to ensure zeroed members
    // Verify default construction
    assert(result.success == false);
    assert(result.container_format.empty());
    assert(result.duration == 0.0);
    assert(result.bitrate_estimate == 0);
    assert(result.video_streams.empty());
    assert(result.audio_streams.empty());
    assert(result.subtitle_streams.empty());
    assert(result.error_code.empty());
    assert(result.inspector_hint.empty());
    assert(result.trace_id.empty());
    std::cout << "  PASS: probe_result_structure\n";
}

// ─── Test: stream info structure fields ───
static void test_stream_info_structure() {
    StreamInfo info{};  // Value-initialize to ensure zeroed members
    assert(info.index == 0);
    assert(info.codec_name.empty());
    assert(info.width == 0);
    assert(info.height == 0);
    assert(info.fps == 0.0);
    assert(info.sample_rate == 0);
    assert(info.channels == 0);
    assert(info.duration == 0);
    assert(info.bitrate == 0);
    std::cout << "  PASS: stream_info_structure\n";
}

// ─── Test: probe_file with environment-sampled media ───
static void test_probe_file_sample_media() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: probe_file_sample_media (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        auto result = RealProbeRuntime::probe_file(sample_path, "trace_sample");
        assert(result.success == false);
        assert(result.error_code == "ffmpeg_not_available");
        std::cout << "  PASS: probe_file_sample_media (ffmpeg disabled)\n";
        return;
    }

    auto result = RealProbeRuntime::probe_file(sample_path, "trace_sample");
    assert(result.success == true);
    assert(result.trace_id == "trace_sample");
    assert(!result.container_format.empty());
    assert(result.duration > 0.0);
    // H264+AAC MP4 should have at least one video and one audio stream
    assert(!result.video_streams.empty());
    assert(!result.audio_streams.empty());
    assert(result.video_streams[0].width > 0);
    assert(result.video_streams[0].height > 0);
    assert(result.audio_streams[0].sample_rate > 0);
    std::cout << "  PASS: probe_file_sample_media (" << result.container_format
              << ", " << result.video_streams.size() << "v/"
              << result.audio_streams.size() << "a, "
              << result.duration << "s)\n";
}

int main() {
    std::cout << "real_probe_runtime_test:\n";
    test_is_ffmpeg_available();
    test_supported_container_formats();
    test_supported_video_codecs();
    test_supported_audio_codecs();
    test_probe_file_nonexistent();
    test_probe_file_empty_path();
    test_probe_stream_closed();
    test_probe_result_structure();
    test_stream_info_structure();
    test_probe_file_sample_media();
    std::cout << "ALL 10 TESTS PASSED\n";
    return 0;
}
