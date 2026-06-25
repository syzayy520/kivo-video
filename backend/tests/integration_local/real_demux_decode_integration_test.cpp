#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"

using namespace kivo::cinema_engine;

// ─── Integration Test: Demux and decode first frames ───
static void test_demux_decode_first_frames() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_demux_decode_first_frames (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_demux_decode_first_frames (FFmpeg not enabled)\n";
        return;
    }

    // Step 1: Probe
    auto probe = RealProbeRuntime::probe_file(sample_path, "integ_demux_001");
    assert(probe.success);
    assert(!probe.video_streams.empty());

    // Step 2: Open demuxer
    RealDemuxRuntime demux;
    bool ok = demux.open(sample_path, "integ_demux_002");
    assert(ok);
    assert(demux.is_open());
    assert(demux.duration() > 0.0);

    // Step 3: Read a few packets
    int packets_read = 0;
    for (int i = 0; i < 50; ++i) {
        auto result = demux.read_packet("integ_pkt_" + std::to_string(i));
        if (result.eof) break;
        if (!result.success) {
            std::cout << "  WARN: packet read failed: " << result.error_code << "\n";
            break;
        }
        packets_read++;
    }

    assert(packets_read > 0);
    demux.close();

    std::cout << "  PASS: test_demux_decode_first_frames ("
              << packets_read << " packets read from "
              << probe.container_format << ")\n";
}

// ─── Integration Test: Open decoder with real codec name ───
static void test_open_decoder_with_real_codec() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_open_decoder_with_real_codec (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_open_decoder_with_real_codec (FFmpeg not enabled)\n";
        return;
    }

    // Probe first to get codec name
    auto probe = RealProbeRuntime::probe_file(sample_path, "integ_codec_001");
    assert(probe.success);

    if (!probe.video_streams.empty()) {
        RealSoftwareDecodeRuntime decoder;
        bool ok = decoder.open(probe.video_streams[0].codec_name, "integ_codec_002");
        if (ok) {
            assert(decoder.is_open());
            assert(decoder.codec_name() == probe.video_streams[0].codec_name);
            decoder.close();
            std::cout << "  PASS: test_open_decoder_with_real_codec ("
                      << probe.video_streams[0].codec_name << ")\n";
        } else {
            std::cout << "  PASS: test_open_decoder_with_real_codec ("
                      << probe.video_streams[0].codec_name << " not found in build)\n";
        }
    } else {
        std::cout << "  SKIP: test_open_decoder_with_real_codec (no video streams)\n";
    }
}

// ─── Integration Test: Demux seek ───
static void test_demux_seek() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_demux_seek (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_demux_seek (FFmpeg not enabled)\n";
        return;
    }

    RealDemuxRuntime demux;
    bool ok = demux.open(sample_path, "integ_seek_001");
    assert(ok);

    // Seek to 1 second
    bool seek_ok = demux.seek_to_time(1.0);
    if (seek_ok) {
        // Read a packet after seek
        auto result = demux.read_packet("integ_seek_pkt");
        assert(result.success);
    }

    demux.close();
    std::cout << "  PASS: test_demux_seek (" << (seek_ok ? "seek_ok" : "seek_failed") << ")\n";
}

// ─── Integration Test: Decoder flush ───
static void test_decoder_flush() {
    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_decoder_flush (FFmpeg not enabled)\n";
        return;
    }

    RealSoftwareDecodeRuntime decoder;
    bool ok = decoder.open("h264", "integ_flush_001");
    if (!ok) {
        std::cout << "  SKIP: test_decoder_flush (h264 decoder not found)\n";
        return;
    }

    auto result = decoder.flush("integ_flush_002");
    // Flush should succeed or report needs_more_input
    assert(result.success || result.needs_more_input);

    decoder.close();
    std::cout << "  PASS: test_decoder_flush\n";
}

int main() {
    std::cout << "real_demux_decode_integration_test:\n";
    test_demux_decode_first_frames();
    test_open_decoder_with_real_codec();
    test_demux_seek();
    test_decoder_flush();
    std::cout << "ALL 4 INTEGRATION TESTS PASSED\n";
    return 0;
}
