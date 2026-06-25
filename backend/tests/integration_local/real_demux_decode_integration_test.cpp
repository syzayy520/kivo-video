#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

using namespace kivo::cinema_engine;

// ─── Integration Test 1: Probe ───
static void test_probe() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_probe (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: test_probe (FFmpeg not enabled)\n";
        return;
    }

    auto probe = RealProbeRuntime::probe_file(sample_path, "probe_001");
    assert(probe.success);
    assert(probe.duration > 0.0);
    assert(!probe.video_streams.empty() || !probe.audio_streams.empty());

    std::cout << "  PASS: probe container=" << probe.container_format
              << " duration=" << probe.duration << "s"
              << " video_streams=" << probe.video_streams.size()
              << " audio_streams=" << probe.audio_streams.size() << "\n";
}

// ─── Integration Test 2: Demux first video packet ───
struct PacketInfo {
    int stream_index;
    int64_t pts;
    int64_t data_size;
    bool is_key_frame;
};

static PacketInfo test_demux_video_packet(const std::string& sample_path,
                                           int video_stream_index) {
    RealDemuxRuntime demux;
    bool ok = demux.open(sample_path, "demux_video_001");
    assert(ok);
    assert(demux.is_open());

    PacketInfo info{-1, 0, 0, false};
    for (int i = 0; i < 500; ++i) {
        auto result = demux.read_packet("demux_video_pkt_" + std::to_string(i));
        if (result.eof) break;
        assert(result.success);
        if (result.packet.stream_index == video_stream_index) {
            info.stream_index = result.packet.stream_index;
            info.pts = result.packet.pts;
            info.data_size = static_cast<int64_t>(result.packet.data.size());
            info.is_key_frame = result.packet.is_key_frame;
            break;
        }
    }

    demux.close();
    assert(info.stream_index >= 0);
    assert(info.data_size > 0);

    std::cout << "  PASS: demux video packet stream_id=" << info.stream_index
              << " pts=" << info.pts << " size=" << info.data_size
              << " keyframe=" << info.is_key_frame << "\n";
    return info;
}

// ─── Integration Test 3: Decode first video frame ───
static void test_decode_video_frame(const std::string& sample_path,
                                     int video_stream_index,
                                     const std::string& codec_name) {
    // Open demuxer
    RealDemuxRuntime demux;
    bool ok = demux.open(sample_path, "decode_video_001");
    assert(ok);

    // Create properly configured video decoder from stream parameters
    FfmpegFormatHandle fmt_handle;
    // We need the native format context. Open the file directly.
    fmt_handle = FfmpegAdapter::open_file(sample_path);
    assert(fmt_handle.native);

    FfmpegCodecHandle codec_handle = FfmpegAdapter::create_codec_context(
        fmt_handle, video_stream_index);
    assert(codec_handle.native);

    // Adopt into RealSoftwareDecodeRuntime
    RealSoftwareDecodeRuntime decoder;
    ok = decoder.adopt_codec_context(codec_handle.native, codec_name,
                                      "decode_video_002");
    assert(ok);
    assert(decoder.is_open());

    // Read packets and send video packets to decoder until we get a frame
    RealDemuxRuntime demux2;
    ok = demux2.open(sample_path, "decode_video_003");
    assert(ok);

    bool decoded_frame = false;
    int frames_decoded = 0;
    for (int i = 0; i < 500 && !decoded_frame; ++i) {
        auto pkt_result = demux2.read_packet("decode_video_pkt_" + std::to_string(i));
        if (pkt_result.eof) break;
        assert(pkt_result.success);

        if (pkt_result.packet.stream_index != video_stream_index) continue;

        DecodeResult dec_result = decoder.decode(pkt_result.packet,
                                                  "decode_video_frame_" + std::to_string(i));
        if (dec_result.success && !dec_result.needs_more_input && !dec_result.eof) {
            // Verify decoded video frame
            assert(dec_result.frame.width > 0);
            assert(dec_result.frame.height > 0);
            assert(!dec_result.frame.pixel_format.empty());
            assert(dec_result.frame.stream_kind == "video");

            std::cout << "  PASS: decode video frame width=" << dec_result.frame.width
                      << " height=" << dec_result.frame.height
                      << " pixel_format=" << dec_result.frame.pixel_format
                      << " pts=" << dec_result.frame.pts << "\n";
            decoded_frame = true;
            frames_decoded++;
        }
    }

    // Drain remaining frames
    if (!decoded_frame) {
        DecodeResult flush_result = decoder.flush("decode_video_flush");
        if (flush_result.success && !flush_result.needs_more_input && !flush_result.eof) {
            assert(flush_result.frame.width > 0);
            assert(flush_result.frame.height > 0);
            assert(!flush_result.frame.pixel_format.empty());

            std::cout << "  PASS: decode video frame (from flush) width="
                      << flush_result.frame.width
                      << " height=" << flush_result.frame.height
                      << " pixel_format=" << flush_result.frame.pixel_format
                      << " pts=" << flush_result.frame.pts << "\n";
            decoded_frame = true;
            frames_decoded++;
        }
    }

    decoder.close();
    demux2.close();
    FfmpegAdapter::free_format_context(fmt_handle);

    assert(decoded_frame);
    std::cout << "  INFO: total video frames decoded: " << frames_decoded << "\n";
}

// ─── Integration Test 4: Demux first audio packet ───
static PacketInfo test_demux_audio_packet(const std::string& sample_path,
                                           int audio_stream_index) {
    RealDemuxRuntime demux;
    bool ok = demux.open(sample_path, "demux_audio_001");
    assert(ok);

    PacketInfo info{-1, 0, 0, false};
    for (int i = 0; i < 500; ++i) {
        auto result = demux.read_packet("demux_audio_pkt_" + std::to_string(i));
        if (result.eof) break;
        assert(result.success);
        if (result.packet.stream_index == audio_stream_index) {
            info.stream_index = result.packet.stream_index;
            info.pts = result.packet.pts;
            info.data_size = static_cast<int64_t>(result.packet.data.size());
            info.is_key_frame = result.packet.is_key_frame;
            break;
        }
    }

    demux.close();
    assert(info.stream_index >= 0);
    assert(info.data_size > 0);

    std::cout << "  PASS: demux audio packet stream_id=" << info.stream_index
              << " pts=" << info.pts << " size=" << info.data_size << "\n";
    return info;
}

// ─── Integration Test 5: Decode first audio frame ───
static void test_decode_audio_frame(const std::string& sample_path,
                                     int audio_stream_index,
                                     const std::string& codec_name) {
    // Create properly configured audio decoder from stream parameters
    FfmpegFormatHandle fmt_handle = FfmpegAdapter::open_file(sample_path);
    assert(fmt_handle.native);

    FfmpegCodecHandle codec_handle = FfmpegAdapter::create_codec_context(
        fmt_handle, audio_stream_index);
    assert(codec_handle.native);

    RealSoftwareDecodeRuntime decoder;
    bool ok = decoder.adopt_codec_context(codec_handle.native, codec_name,
                                           "decode_audio_001");
    assert(ok);
    assert(decoder.is_open());

    // Read packets and send audio packets to decoder until we get a frame
    RealDemuxRuntime demux;
    ok = demux.open(sample_path, "decode_audio_002");
    assert(ok);

    bool decoded_frame = false;
    int frames_decoded = 0;
    for (int i = 0; i < 500 && !decoded_frame; ++i) {
        auto pkt_result = demux.read_packet("decode_audio_pkt_" + std::to_string(i));
        if (pkt_result.eof) break;
        assert(pkt_result.success);

        if (pkt_result.packet.stream_index != audio_stream_index) continue;

        DecodeResult dec_result = decoder.decode(pkt_result.packet,
                                                  "decode_audio_frame_" + std::to_string(i));
        if (dec_result.success && !dec_result.needs_more_input && !dec_result.eof) {
            assert(dec_result.frame.sample_rate > 0);
            assert(dec_result.frame.channels > 0);
            assert(!dec_result.frame.sample_format.empty());
            assert(dec_result.frame.stream_kind == "audio");

            std::cout << "  PASS: decode audio frame sample_rate=" << dec_result.frame.sample_rate
                      << " channels=" << dec_result.frame.channels
                      << " sample_format=" << dec_result.frame.sample_format
                      << " pts=" << dec_result.frame.pts << "\n";
            decoded_frame = true;
            frames_decoded++;
        }
    }

    // Drain remaining frames
    if (!decoded_frame) {
        DecodeResult flush_result = decoder.flush("decode_audio_flush");
        if (flush_result.success && !flush_result.needs_more_input && !flush_result.eof) {
            assert(flush_result.frame.sample_rate > 0);
            assert(flush_result.frame.channels > 0);
            assert(!flush_result.frame.sample_format.empty());

            std::cout << "  PASS: decode audio frame (from flush) sample_rate="
                      << flush_result.frame.sample_rate
                      << " channels=" << flush_result.frame.channels
                      << " sample_format=" << flush_result.frame.sample_format
                      << " pts=" << flush_result.frame.pts << "\n";
            decoded_frame = true;
            frames_decoded++;
        }
    }

    decoder.close();
    demux.close();
    FfmpegAdapter::free_format_context(fmt_handle);

    assert(decoded_frame);
    std::cout << "  INFO: total audio frames decoded: " << frames_decoded << "\n";
}

// ─── Main ───
int main() {
    std::cout << "real_demux_decode_integration_test:\n";
    std::cout << "====================================\n";

    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: KIVO_SAMPLE_H264_AAC_MP4 not set\n";
        std::cout << "  Set this env var to a real H.264+AAC MP4 file path.\n";
        return 0;
    }

    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: FFmpeg not enabled\n";
        return 0;
    }

    std::cout << "  Sample: " << sample_path << "\n\n";

    // Step 1: Probe
    std::cout << "[Step 1] Probe:\n";
    auto probe = RealProbeRuntime::probe_file(sample_path, "main_probe");
    assert(probe.success);
    assert(!probe.video_streams.empty() || !probe.audio_streams.empty());
    std::cout << "  PASS: probe container=" << probe.container_format
              << " duration=" << probe.duration << "s"
              << " video=" << probe.video_streams.size()
              << " audio=" << probe.audio_streams.size() << "\n\n";

    int video_idx = -1;
    std::string video_codec;
    int audio_idx = -1;
    std::string audio_codec;

    if (!probe.video_streams.empty()) {
        video_idx = probe.video_streams[0].index;
        video_codec = probe.video_streams[0].codec_name;
        std::cout << "  Video stream: index=" << video_idx
                  << " codec=" << video_codec
                  << " " << probe.video_streams[0].width << "x"
                  << probe.video_streams[0].height << "\n";
    }
    if (!probe.audio_streams.empty()) {
        audio_idx = probe.audio_streams[0].index;
        audio_codec = probe.audio_streams[0].codec_name;
        std::cout << "  Audio stream: index=" << audio_idx
                  << " codec=" << audio_codec
                  << " " << probe.audio_streams[0].sample_rate << "Hz"
                  << " " << probe.audio_streams[0].channels << "ch\n";
    }
    std::cout << "\n";

    // Step 2: Demux + decode video
    if (video_idx >= 0) {
        std::cout << "[Step 2] Demux + decode VIDEO:\n";
        auto vinfo = test_demux_video_packet(sample_path, video_idx);
        test_decode_video_frame(sample_path, video_idx, video_codec);
        std::cout << "\n";
    } else {
        std::cout << "[Step 2] SKIP: no video stream\n\n";
    }

    // Step 3: Demux + decode audio
    if (audio_idx >= 0) {
        std::cout << "[Step 3] Demux + decode AUDIO:\n";
        auto ainfo = test_demux_audio_packet(sample_path, audio_idx);
        test_decode_audio_frame(sample_path, audio_idx, audio_codec);
        std::cout << "\n";
    } else {
        std::cout << "[Step 3] SKIP: no audio stream\n\n";
    }

    std::cout << "====================================\n";
    std::cout << "ALL REAL DEMUX+DECODE INTEGRATION TESTS PASSED\n";
    return 0;
}
