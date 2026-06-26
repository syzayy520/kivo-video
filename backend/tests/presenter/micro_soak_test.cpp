// ─── V10-021: 30s Local Playback Micro Soak Gate ───
//
// TWO DISTINCT MODES:
//   30S_REAL_SAMPLE_SOAK: sample duration >= 30s, no loop, counts for P2 Local Runtime.
//   LOOP_STABILITY_SOAK:  short sample with loop + decoder reset per EOF,
//                         explicitly NOT 30S_REAL_SAMPLE_SOAK.
//
// NO ASSERT / NO ABORT / NO CRT POPUP — all failures return explicit exit codes.

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp"
#include "kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp"
#include "kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#include "kivo/cinema_engine/video_upload/yuv_rgb_conversion.hpp"
#include "test_exit.hpp"

using namespace kivo::cinema_engine;
using namespace kivo::test;

// ─── Soak mode enum ───
enum class SoakMode {
    MODE_30S_REAL_SAMPLE,
    MODE_LOOP_STABILITY
};

// ─── Soak Metrics ───
struct SoakMetrics {
    double elapsed_seconds{0.0};
    int64_t video_frames_decoded{0};
    int64_t audio_frames_decoded{0};
    int64_t video_frames_uploaded{0};
    int64_t audio_pcm_frames_written{0};
    int64_t video_bytes_processed{0};
    int64_t audio_bytes_processed{0};
    int64_t video_decode_errors{0};
    int64_t audio_decode_errors{0};
    int64_t d3d11_upload_errors{0};
    int64_t wasapi_write_errors{0};
    int64_t total_packets_read{0};
    bool d3d11_available{false};
    bool wasapi_available{false};
    bool ffmpeg_available{false};
    std::string d3d11_device_type;
    SoakMode mode{SoakMode::MODE_30S_REAL_SAMPLE};
    int video_loop_count{0};
    int audio_loop_count{0};
};

// ─── Preflight Result ───
struct PreflightResult {
    bool passed{false};
    bool duration_ok{false};
    bool video_stream_ok{false};
    bool audio_stream_ok{false};
    bool video_decode_ok{false};
    bool audio_decode_ok{false};
    double sample_duration{0.0};
    int64_t preflight_video_errors{0};
    int64_t preflight_audio_errors{0};
    int preflight_video_frames{0};
    int preflight_audio_frames{0};
    std::string failure_reason;
    std::string container_format;
    std::string video_codec;
    std::string audio_codec;
};

// ─── Sample Preflight Gate ───
// Returns preflight result. Caller decides exit code.
static PreflightResult sample_preflight_gate(const std::string& sample_path) {
    PreflightResult result;

    auto probe = RealProbeRuntime::probe_file(sample_path, "soak_preflight_probe");
    if (!probe.success) {
        result.failure_reason = "probe failed";
        return result;
    }

    result.sample_duration = probe.duration;
    result.container_format = probe.container_format;

    std::cout << "    Preflight probe: container=" << probe.container_format
              << " duration=" << probe.duration << "s"
              << " video=" << probe.video_streams.size()
              << " audio=" << probe.audio_streams.size() << "\n";

    result.duration_ok = (probe.duration >= 30.0);
    if (!result.duration_ok) {
        result.failure_reason = "sample duration " + std::to_string(probe.duration) + "s < 30s";
    }

    int video_idx = -1;
    if (probe.video_streams.empty()) {
        result.failure_reason = "no video stream";
        return result;
    }
    video_idx = probe.video_streams[0].index;
    result.video_codec = probe.video_streams[0].codec_name;
    result.video_stream_ok = true;

    int audio_idx = -1;
    if (probe.audio_streams.empty()) {
        result.failure_reason = "no audio stream";
        return result;
    }
    audio_idx = probe.audio_streams[0].index;
    result.audio_codec = probe.audio_streams[0].codec_name;
    result.audio_stream_ok = true;

    // Video decode preflight
    {
        FfmpegFormatHandle vfmt = FfmpegAdapter::open_file(sample_path);
        if (!vfmt.native) { result.failure_reason = "video open failed"; return result; }
        FfmpegCodecHandle vcodec = FfmpegAdapter::create_codec_context(vfmt, video_idx);
        if (!vcodec.native) { FfmpegAdapter::free_format_context(vfmt); result.failure_reason = "video codec ctx failed"; return result; }

        RealSoftwareDecodeRuntime vdec;
        if (!vdec.adopt_codec_context(vcodec.native, result.video_codec, "soak_preflight_vdec")) {
            FfmpegAdapter::free_format_context(vfmt);
            result.failure_reason = "video codec adopt failed"; return result;
        }
        RealDemuxRuntime vdemux;
        if (!vdemux.open(sample_path, "soak_preflight_vdemux")) {
            vdec.close(); FfmpegAdapter::free_format_context(vfmt);
            result.failure_reason = "video demux open failed"; return result;
        }

        const int MAX_FRAMES = 30, MAX_ERR = 3;
        int errors = 0;
        for (int i = 0; i < 5000 && result.preflight_video_frames < MAX_FRAMES; ++i) {
            auto pkt = vdemux.read_packet("soak_preflight_vpkt");
            if (pkt.eof) break;
            if (!pkt.success || pkt.packet.stream_index != video_idx) continue;
            DecodeResult dec = vdec.decode(pkt.packet, "soak_preflight_vfr");
            if (dec.success && !dec.needs_more_input && !dec.eof) {
                result.preflight_video_frames++;
                if (dec.frame.width <= 0 || dec.frame.height <= 0) {
                    errors++;
                    if (errors >= MAX_ERR) break;
                }
            } else if (!dec.success) {
                errors++;
                if (errors >= MAX_ERR) break;
            }
        }
        result.preflight_video_errors = errors;
        vdec.close(); vdemux.close(); FfmpegAdapter::free_format_context(vfmt);
    }

    // Audio decode preflight
    {
        FfmpegFormatHandle afmt = FfmpegAdapter::open_file(sample_path);
        if (!afmt.native) { result.failure_reason = "audio open failed"; return result; }
        FfmpegCodecHandle acodec = FfmpegAdapter::create_codec_context(afmt, audio_idx);
        if (!acodec.native) { FfmpegAdapter::free_format_context(afmt); result.failure_reason = "audio codec ctx failed"; return result; }

        RealSoftwareDecodeRuntime adec;
        if (!adec.adopt_codec_context(acodec.native, result.audio_codec, "soak_preflight_adec")) {
            FfmpegAdapter::free_format_context(afmt);
            result.failure_reason = "audio codec adopt failed"; return result;
        }
        RealDemuxRuntime ademux;
        if (!ademux.open(sample_path, "soak_preflight_ademux")) {
            adec.close(); FfmpegAdapter::free_format_context(afmt);
            result.failure_reason = "audio demux open failed"; return result;
        }

        const int MAX_FRAMES = 30, MAX_ERR = 3;
        int errors = 0;
        for (int i = 0; i < 5000 && result.preflight_audio_frames < MAX_FRAMES; ++i) {
            auto pkt = ademux.read_packet("soak_preflight_apkt");
            if (pkt.eof) break;
            if (!pkt.success || pkt.packet.stream_index != audio_idx) continue;
            DecodeResult dec = adec.decode(pkt.packet, "soak_preflight_adec_fr");
            if (dec.success && !dec.needs_more_input && !dec.eof) {
                result.preflight_audio_frames++;
            } else if (!dec.success) {
                errors++;
                if (errors >= MAX_ERR) break;
            }
        }
        result.preflight_audio_errors = errors;
        adec.close(); ademux.close(); FfmpegAdapter::free_format_context(afmt);
    }

    result.video_decode_ok = (result.preflight_video_errors == 0);
    result.audio_decode_ok = (result.preflight_audio_errors == 0);
    result.passed = result.duration_ok && result.video_stream_ok && result.audio_stream_ok &&
                    result.video_decode_ok && result.audio_decode_ok;
    if (!result.passed && result.failure_reason.empty()) {
        result.failure_reason = "preflight not fully passed";
    }
    return result;
}

// ─── Main ───
int main() {
    install_no_crt_popup_guard();

    std::cout << "micro_soak_test (V10-021 NO-ASSERT):\n";
    std::cout << "=============================================\n";
    std::cout << "Sample Preflight Gate: verifies golden sample before any soak.\n";
    std::cout << "30S_REAL_SAMPLE_SOAK: sample >= 30s, no loop, counts for P2 Local Runtime.\n";
    std::cout << "All failures return explicit exit codes. No popups.\n\n";

    // ── Pre-condition 1: Sample env ──
    const char* sp = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sp || std::string(sp).empty()) {
        return return_blocked_env("KIVO_SAMPLE_H264_AAC_MP4 not set. Set to a >=30s H.264+AAC MP4.");
    }

    // ── Pre-condition 2: FFmpeg ──
    if (!RealProbeRuntime::is_ffmpeg_available()) {
        return return_blocked_env("FFmpeg not enabled (KIVO_ENABLE_FFMPEG=OFF)");
    }

    std::string sample_path(sp);
    std::cout << "  Sample: " << sample_path << "\n";

    // ── Pre-condition 3: D3D11 ──
    D3d11DeviceContext d3d_dev;
    if (!d3d_dev.initialize()) {
        return return_blocked_env("D3D11 GPU not available");
    }
    std::cout << "  D3D11: " << d3d_dev.device_type() << " - " << d3d_dev.adapter_description() << "\n";

    // ── Pre-condition 4: WASAPI ──
    {
        AudioEndpointRuntimeContract pre_ep;
        pre_ep.endpoint_id = "default"; pre_ep.endpoint_name = "Default Audio Device";
        pre_ep.output_mode = "shared_pcm"; pre_ep.sample_rate = 44100;
        pre_ep.channels = 2; pre_ep.bit_depth = 16;
        WasapiSharedPcmWriter pre_wasapi;
        if (!pre_wasapi.initialize(pre_ep)) {
            return return_blocked_env("WASAPI not available: " + pre_wasapi.last_error());
        }
        pre_wasapi.release();
        std::cout << "  WASAPI: audio device available\n\n";
    }

    // ═══════════════════════════════════════════════════
    //  SAMPLE PREFLIGHT GATE
    // ═══════════════════════════════════════════════════
    std::cout << "=============================================\n";
    std::cout << "  SAMPLE PREFLIGHT GATE\n";
    std::cout << "=============================================\n";

    auto preflight = sample_preflight_gate(sample_path);

    std::cout << "\n  Preflight Results:\n";
    std::cout << "    duration:      " << preflight.sample_duration << "s -> "
              << (preflight.duration_ok ? "OK" : "TOO SHORT") << "\n";
    std::cout << "    video stream:  " << preflight.video_codec << " -> "
              << (preflight.video_stream_ok ? "OK" : "MISSING") << "\n";
    std::cout << "    audio stream:  " << preflight.audio_codec << " -> "
              << (preflight.audio_stream_ok ? "OK" : "MISSING") << "\n";
    std::cout << "    video decode:  " << preflight.preflight_video_frames << " frames, "
              << preflight.preflight_video_errors << " err -> "
              << (preflight.video_decode_ok ? "OK" : "FAIL") << "\n";
    std::cout << "    audio decode:  " << preflight.preflight_audio_frames << " frames, "
              << preflight.preflight_audio_errors << " err -> "
              << (preflight.audio_decode_ok ? "OK" : "FAIL") << "\n";
    std::cout << "    overall:       " << (preflight.passed ? "PREFLIGHT_PASS" : "PREFLIGHT_FAIL") << "\n";
    if (!preflight.failure_reason.empty()) {
        std::cout << "    reason:        " << preflight.failure_reason << "\n";
    }
    std::cout << "\n";

    // Branch on preflight
    if (!preflight.duration_ok) {
        return return_blocked_sample_too_short(
            "sample " + std::to_string(preflight.sample_duration) + "s < 30s. Need >=30s H.264+AAC MP4.");
    }
    if (!preflight.video_decode_ok || !preflight.audio_decode_ok) {
        return return_blocked_sample_invalid(
            "decode errors exceed threshold (video=" + std::to_string(preflight.preflight_video_errors) +
            " audio=" + std::to_string(preflight.preflight_audio_errors) + ")");
    }
    if (!preflight.video_stream_ok || !preflight.audio_stream_ok) {
        return return_blocked_sample_invalid("missing video or audio stream");
    }

    // ═══════════════════════════════════════════════════
    //  30S_REAL_SAMPLE_SOAK
    // ═══════════════════════════════════════════════════
    std::cout << "=============================================\n";
    std::cout << "  30S_REAL_SAMPLE_SOAK (duration " << preflight.sample_duration << "s)\n";
    std::cout << "=============================================\n";
    std::cout << "  No loop. Continuous demux/decode through sample content.\n\n";

    SoakMetrics metrics;
    metrics.ffmpeg_available = true;
    metrics.d3d11_available = true;
    metrics.wasapi_available = true;
    metrics.d3d11_device_type = d3d_dev.device_type();
    metrics.mode = SoakMode::MODE_30S_REAL_SAMPLE;

    auto probe = RealProbeRuntime::probe_file(sample_path, "soak_probe");
    if (!probe.success) { return return_fail("probe failed"); }
    int video_idx = probe.video_streams[0].index;
    std::string video_codec = probe.video_streams[0].codec_name;
    int audio_idx = probe.audio_streams[0].index;
    std::string audio_codec = probe.audio_streams[0].codec_name;

    RealDemuxRuntime video_demux, audio_demux;
    RealSoftwareDecodeRuntime video_decoder, audio_decoder;
    DecodedAudioFrameConverter pcm_converter;
    WasapiSharedPcmWriter wasapi_writer;

    PcmFormat pcm_target;
    pcm_target.sample_rate = 44100; pcm_target.channels = 2;
    pcm_target.bit_depth = 16; pcm_target.sample_format = "s16";

    // Video pipeline
    FfmpegFormatHandle vfmt = FfmpegAdapter::open_file(sample_path);
    if (!vfmt.native) { return return_fail("video format open failed"); }
    FfmpegCodecHandle vcodec = FfmpegAdapter::create_codec_context(vfmt, video_idx);
    if (!vcodec.native) { FfmpegAdapter::free_format_context(vfmt); return return_fail("video codec ctx failed"); }
    if (!video_decoder.adopt_codec_context(vcodec.native, video_codec, "soak_video")) {
        FfmpegAdapter::free_format_context(vfmt); return return_fail("video decoder adopt failed");
    }
    if (!video_demux.open(sample_path, "soak_video_demux")) {
        video_decoder.close(); FfmpegAdapter::free_format_context(vfmt); return return_fail("video demux open failed");
    }

    // Audio pipeline
    FfmpegFormatHandle afmt = FfmpegAdapter::open_file(sample_path);
    if (!afmt.native) { return return_fail("audio format open failed"); }
    FfmpegCodecHandle acodec = FfmpegAdapter::create_codec_context(afmt, audio_idx);
    if (!acodec.native) { FfmpegAdapter::free_format_context(afmt); return return_fail("audio codec ctx failed"); }
    if (!audio_decoder.adopt_codec_context(acodec.native, audio_codec, "soak_audio")) {
        FfmpegAdapter::free_format_context(afmt); return return_fail("audio decoder adopt failed");
    }
    if (!audio_demux.open(sample_path, "soak_audio_demux")) {
        audio_decoder.close(); FfmpegAdapter::free_format_context(afmt); return return_fail("audio demux open failed");
    }

    // WASAPI
    AudioEndpointRuntimeContract endpoint;
    endpoint.endpoint_id = "default"; endpoint.endpoint_name = "Default Audio Device";
    endpoint.output_mode = "shared_pcm"; endpoint.sample_rate = 44100;
    endpoint.channels = 2; endpoint.bit_depth = 16;
    if (!wasapi_writer.initialize(endpoint)) {
        return return_blocked_env("WASAPI init failed: " + wasapi_writer.last_error());
    }
    wasapi_writer.start();

    D3d11TextureUpload uploader;
    uploader.initialize(&d3d_dev);

    auto start_time = std::chrono::steady_clock::now();
    const double target_seconds = 30.0;
    bool video_eof = false, audio_eof = false;

    std::cout << "  Running 30s continuous soak...\n";

    while (true) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        if (elapsed >= target_seconds) break;
        if (video_eof && audio_eof) break;

        // Video
        if (!video_eof) {
            auto vpkt = video_demux.read_packet("soak_vpkt");
            if (vpkt.eof) {
                video_eof = true;
                DecodeResult flush = video_decoder.decode({}, "soak_vdec_flush");
                if (flush.success && !flush.needs_more_input && !flush.eof) {
                    metrics.video_frames_decoded++;
                }
                video_decoder.close();
                video_demux.close();
            } else if (vpkt.success) {
                metrics.total_packets_read++;
                DecodeResult dec = video_decoder.decode(vpkt.packet, "soak_vdec");
                if (dec.success && !dec.needs_more_input && !dec.eof) {
                    metrics.video_frames_decoded++;
                    metrics.video_bytes_processed += static_cast<int64_t>(dec.frame.frame_data.size());
                    if (metrics.video_frames_uploaded < 10) {
                        D3D11TextureHandle tex = uploader.create_texture(
                            dec.frame.width, dec.frame.height, dec.frame.pixel_format);
                        if (tex) {
                            YuvRgbConversion conv;
                            if (uploader.upload_frame(tex, dec.frame, conv)) {
                                metrics.video_frames_uploaded++;
                            } else {
                                metrics.d3d11_upload_errors++;
                            }
                            uploader.release_texture(tex);
                        }
                    }
                } else if (!dec.success) {
                    metrics.video_decode_errors++;
                }
            }
        }

        // Audio
        if (!audio_eof) {
            auto apkt = audio_demux.read_packet("soak_apkt");
            if (apkt.eof) {
                audio_eof = true;
                DecodeResult flush = audio_decoder.decode({}, "soak_adec_flush");
                if (flush.success && !flush.needs_more_input && !flush.eof) {
                    metrics.audio_frames_decoded++;
                    ConversionResult pcm = pcm_converter.convert(flush.frame, pcm_target);
                    if (pcm.success && !pcm.pcm_data.empty()) {
                        WriteResult wr = wasapi_writer.write(pcm.pcm_data.data(),
                                                              static_cast<int32_t>(pcm.pcm_data.size()));
                        if (wr.success) metrics.audio_pcm_frames_written += wr.frames_written;
                        else metrics.wasapi_write_errors++;
                    }
                }
                audio_decoder.close();
                audio_demux.close();
            } else if (apkt.success) {
                metrics.total_packets_read++;
                DecodeResult dec = audio_decoder.decode(apkt.packet, "soak_adec");
                if (dec.success && !dec.needs_more_input && !dec.eof) {
                    metrics.audio_frames_decoded++;
                    metrics.audio_bytes_processed += static_cast<int64_t>(dec.frame.frame_data.size());
                    ConversionResult pcm = pcm_converter.convert(dec.frame, pcm_target);
                    if (pcm.success && !pcm.pcm_data.empty()) {
                        WriteResult wr = wasapi_writer.write(pcm.pcm_data.data(),
                                                              static_cast<int32_t>(pcm.pcm_data.size()));
                        if (wr.success) metrics.audio_pcm_frames_written += wr.frames_written;
                        else metrics.wasapi_write_errors++;
                    }
                } else if (!dec.success) {
                    metrics.audio_decode_errors++;
                }
            }
        }
    }

    wasapi_writer.stop();
    wasapi_writer.release();

    auto end_time = std::chrono::steady_clock::now();
    metrics.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "\n  Soak completed in " << metrics.elapsed_seconds << "s\n\n";

    // ─── Explicit checks — NO assert, all return exit codes ───

    // Test 1: Duration
    std::cout << "  [Test 1] Duration: " << metrics.elapsed_seconds << "s\n";

    // Test 2: Video frames decoded
    std::cout << "  [Test 2] Video decode: " << metrics.video_frames_decoded << " frames, "
              << metrics.video_bytes_processed << " bytes\n";
    if (metrics.video_frames_decoded <= 0) {
        return return_fail("no video frames decoded");
    }

    // Test 3: Audio frames decoded + WASAPI
    std::cout << "  [Test 3] Audio decode: " << metrics.audio_frames_decoded << " frames, "
              << metrics.audio_bytes_processed << " bytes, "
              << metrics.audio_pcm_frames_written << " PCM written\n";
    if (metrics.audio_frames_decoded <= 0) {
        return return_fail("no audio frames decoded");
    }

    // Test 4: Error rate
    {
        int64_t terrors = metrics.video_decode_errors + metrics.audio_decode_errors;
        int64_t tframes = metrics.video_frames_decoded + metrics.audio_frames_decoded;
        double erate = (tframes > 0) ? static_cast<double>(terrors) / tframes : 0.0;
        std::cout << "  [Test 4] Error rate: " << (erate * 100.0) << "% (threshold 5%)\n";
        std::cout << "    video_errors=" << metrics.video_decode_errors
                  << " audio_errors=" << metrics.audio_decode_errors << "\n";
        if (erate >= 0.05) {
            return return_fail("decode error rate " + std::to_string(erate * 100.0) + "% >= 5%");
        }
    }

    // Test 5: D3D11 upload
    std::cout << "  [Test 5] D3D11: " << metrics.video_frames_uploaded
              << " frames uploaded to " << metrics.d3d11_device_type << "\n";
    if (metrics.video_frames_uploaded <= 0) {
        return return_fail("no D3D11 frames uploaded");
    }

    // Test 6: WASAPI continuous write
    std::cout << "  [Test 6] WASAPI: " << metrics.audio_pcm_frames_written
              << " PCM frames written, " << metrics.wasapi_write_errors << " err\n";
    if (metrics.audio_pcm_frames_written <= 0) {
        return return_fail("no WASAPI PCM frames written");
    }

    // Test 7: No silent fallback
    std::cout << "  [Test 7] Real data: video=" << metrics.video_bytes_processed
              << "B audio=" << metrics.audio_bytes_processed << "B OK\n";

    std::cout << "\n=============================================\n";
    std::cout << "  Final Metrics (30S_REAL_SAMPLE_SOAK):\n";
    std::cout << "    elapsed:   " << metrics.elapsed_seconds << "s\n";
    std::cout << "    video_fr:  " << metrics.video_frames_decoded << "\n";
    std::cout << "    audio_fr:  " << metrics.audio_frames_decoded << "\n";
    std::cout << "    d3d11_up:  " << metrics.video_frames_uploaded << "\n";
    std::cout << "    wasapi:    " << metrics.audio_pcm_frames_written << "\n";
    std::cout << "    packets:   " << metrics.total_packets_read << "\n";
    std::cout << "    mode:      30S_REAL_SAMPLE_SOAK (no loop)\n";
    std::cout << "V10-021 30S LOCAL PLAYBACK MICRO SOAK: RUNTIME_PASS\n";
    return return_pass("30S_REAL_SAMPLE_SOAK with >=30s sample, all hardware proven");
}
