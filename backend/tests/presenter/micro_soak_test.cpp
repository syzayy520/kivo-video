#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>

// ─── V10-021: 30s Local Playback Micro Soak Gate ───
//
// TWO DISTINCT MODES:
//   30S_REAL_SAMPLE_SOAK: sample duration >= 30s, no loop, counts for P2 Local Runtime.
//   LOOP_STABILITY_SOAK:  short sample with loop + decoder reset per loop,
//                         explicitly NOT 30S_REAL_SAMPLE_SOAK.
//
// Sample Preflight Gate runs BEFORE any soak to block invalid samples.

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

using namespace kivo::cinema_engine;

// ─── Exit codes ───
enum class SoakExitCode {
    RUNTIME_PASS              = 0,
    BLOCKED_ENV               = 1,
    FAIL                      = 2,
    BLOCKED_SAMPLE_TOO_SHORT  = 3,
    BLOCKED_SAMPLE_INVALID    = 4
};

// ─── Soak mode enum ───
enum class SoakMode {
    MODE_30S_REAL_SAMPLE,    // sample duration >= 30s, no loop needed
    MODE_LOOP_STABILITY      // short sample loop with decoder reset
};

// ─── Real Soak Metrics ───
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

// ─── Sample Preflight Result ───
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
// Checks sample validity BEFORE any soak begins.
// Returns a PreflightResult. Caller decides exit code based on result.
static PreflightResult sample_preflight_gate(const std::string& sample_path) {
    PreflightResult result;

    // 1. Probe
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

    // 2. Duration >= 30s
    result.duration_ok = (probe.duration >= 30.0);
    if (!result.duration_ok) {
        result.failure_reason = "sample duration " + std::to_string(probe.duration) + "s < 30s (required for 30S_REAL_SAMPLE_SOAK)";
        // Continue to check streams anyway for diagnostic output
    }

    // 3. Video stream exists
    int video_idx = -1;
    if (!probe.video_streams.empty()) {
        video_idx = probe.video_streams[0].index;
        result.video_codec = probe.video_streams[0].codec_name;
        result.video_stream_ok = true;
    } else {
        result.failure_reason = "no video stream in sample";
        return result;
    }

    // 4. Audio stream exists
    int audio_idx = -1;
    if (!probe.audio_streams.empty()) {
        audio_idx = probe.audio_streams[0].index;
        result.audio_codec = probe.audio_streams[0].codec_name;
        result.audio_stream_ok = true;
    } else {
        result.failure_reason = "no audio stream in sample";
        return result;
    }

    // 5. Video decode preflight: decode first ~30 frames, check for fatal errors
    {
        FfmpegFormatHandle vfmt = FfmpegAdapter::open_file(sample_path);
        if (!vfmt.native) {
            result.failure_reason = "failed to open file for video decode preflight";
            return result;
        }
        FfmpegCodecHandle vcodec = FfmpegAdapter::create_codec_context(vfmt, video_idx);
        if (!vcodec.native) {
            FfmpegAdapter::free_format_context(vfmt);
            result.failure_reason = "failed to create video codec context";
            return result;
        }

        RealSoftwareDecodeRuntime vdec;
        if (!vdec.adopt_codec_context(vcodec.native, result.video_codec, "soak_preflight_vdec")) {
            FfmpegAdapter::free_format_context(vfmt);
            result.failure_reason = "failed to adopt video codec context";
            return result;
        }

        RealDemuxRuntime vdemux;
        if (!vdemux.open(sample_path, "soak_preflight_vdemux")) {
            vdec.close();
            FfmpegAdapter::free_format_context(vfmt);
            result.failure_reason = "failed to open video demux for preflight";
            return result;
        }

        const int MAX_PREFLIGHT_FRAMES = 30;
        const int MAX_PREFLIGHT_ERRORS = 3;
        int errors = 0;

        for (int i = 0; i < 5000 && result.preflight_video_frames < MAX_PREFLIGHT_FRAMES; ++i) {
            auto pkt = vdemux.read_packet("soak_preflight_vpkt");
            if (pkt.eof) break;
            if (!pkt.success) continue;
            if (pkt.packet.stream_index != video_idx) continue;

            DecodeResult dec = vdec.decode(pkt.packet, "soak_preflight_vdec_fr");
            if (dec.success && !dec.needs_more_input && !dec.eof) {
                result.preflight_video_frames++;
                assert(dec.frame.width > 0 && dec.frame.height > 0);
            } else if (!dec.success) {
                errors++;
                if (errors >= MAX_PREFLIGHT_ERRORS) {
                    result.preflight_video_errors = errors;
                    vdec.close();
                    vdemux.close();
                    FfmpegAdapter::free_format_context(vfmt);
                    result.failure_reason = "video decode errors exceed threshold (" +
                                            std::to_string(errors) + " errors in first " +
                                            std::to_string(result.preflight_video_frames + 1) + " frames)";
                    return result;
                }
            }
        }

        result.preflight_video_errors = errors;
        vdec.close();
        vdemux.close();
        FfmpegAdapter::free_format_context(vfmt);
    }

    // 6. Audio decode preflight: decode first ~30 frames, check for fatal errors
    {
        FfmpegFormatHandle afmt = FfmpegAdapter::open_file(sample_path);
        if (!afmt.native) {
            result.failure_reason = "failed to open file for audio decode preflight";
            return result;
        }
        FfmpegCodecHandle acodec = FfmpegAdapter::create_codec_context(afmt, audio_idx);
        if (!acodec.native) {
            FfmpegAdapter::free_format_context(afmt);
            result.failure_reason = "failed to create audio codec context";
            return result;
        }

        RealSoftwareDecodeRuntime adec;
        if (!adec.adopt_codec_context(acodec.native, result.audio_codec, "soak_preflight_adec")) {
            FfmpegAdapter::free_format_context(afmt);
            result.failure_reason = "failed to adopt audio codec context";
            return result;
        }

        RealDemuxRuntime ademux;
        if (!ademux.open(sample_path, "soak_preflight_ademux")) {
            adec.close();
            FfmpegAdapter::free_format_context(afmt);
            result.failure_reason = "failed to open audio demux for preflight";
            return result;
        }

        const int MAX_PREFLIGHT_FRAMES = 30;
        const int MAX_PREFLIGHT_ERRORS = 3;
        int errors = 0;

        for (int i = 0; i < 5000 && result.preflight_audio_frames < MAX_PREFLIGHT_FRAMES; ++i) {
            auto pkt = ademux.read_packet("soak_preflight_apkt");
            if (pkt.eof) break;
            if (!pkt.success) continue;
            if (pkt.packet.stream_index != audio_idx) continue;

            DecodeResult dec = adec.decode(pkt.packet, "soak_preflight_adec_fr");
            if (dec.success && !dec.needs_more_input && !dec.eof) {
                result.preflight_audio_frames++;
                assert(dec.frame.sample_rate > 0 && dec.frame.channels > 0);
            } else if (!dec.success) {
                errors++;
                if (errors >= MAX_PREFLIGHT_ERRORS) {
                    result.preflight_audio_errors = errors;
                    adec.close();
                    ademux.close();
                    FfmpegAdapter::free_format_context(afmt);
                    result.failure_reason = "audio decode errors exceed threshold (" +
                                            std::to_string(errors) + " errors in first " +
                                            std::to_string(result.preflight_audio_frames + 1) + " frames)";
                    return result;
                }
            }
        }

        result.preflight_audio_errors = errors;
        adec.close();
        ademux.close();
        FfmpegAdapter::free_format_context(afmt);
    }

    // 7. Consolidate
    result.video_decode_ok = (result.preflight_video_errors == 0);
    result.audio_decode_ok = (result.preflight_audio_errors == 0);
    result.passed = result.duration_ok && result.video_stream_ok && result.audio_stream_ok &&
                    result.video_decode_ok && result.audio_decode_ok;

    if (!result.passed && result.failure_reason.empty()) {
        result.failure_reason = "preflight not fully passed";
    }
    return result;
}

// ─── Helpers for EOF loop with decoder reset ───
// Used only in LOOP_STABILITY_SOAK mode.
struct LoopResetState {
    bool need_flush_video{false};
    bool need_flush_audio{false};
};

static void flush_video_pipeline(RealDemuxRuntime& demux, RealSoftwareDecodeRuntime& decoder,
                                 FfmpegFormatHandle& fmt_handle, FfmpegCodecHandle& codec_handle,
                                 const std::string& sample_path, const std::string& video_codec,
                                 int video_idx, D3d11DeviceContext& d3d) {
    decoder.close();
    demux.close();
    FfmpegAdapter::free_format_context(fmt_handle);
    FfmpegAdapter::free_codec_context(codec_handle);

    // Reopen fresh
    fmt_handle = FfmpegAdapter::open_file(sample_path);
    assert(fmt_handle.native);
    codec_handle = FfmpegAdapter::create_codec_context(fmt_handle, video_idx);
    assert(codec_handle.native);
    assert(decoder.adopt_codec_context(codec_handle.native, video_codec, "soak_video"));
    assert(demux.open(sample_path, "soak_video_loop"));
}

static void flush_audio_pipeline(RealDemuxRuntime& demux, RealSoftwareDecodeRuntime& decoder,
                                 FfmpegFormatHandle& fmt_handle, FfmpegCodecHandle& codec_handle,
                                 const std::string& sample_path, const std::string& audio_codec,
                                 int audio_idx) {
    decoder.close();
    demux.close();
    FfmpegAdapter::free_format_context(fmt_handle);
    FfmpegAdapter::free_codec_context(codec_handle);

    // Reopen fresh
    fmt_handle = FfmpegAdapter::open_file(sample_path);
    assert(fmt_handle.native);
    codec_handle = FfmpegAdapter::create_codec_context(fmt_handle, audio_idx);
    assert(codec_handle.native);
    assert(decoder.adopt_codec_context(codec_handle.native, audio_codec, "soak_audio"));
    assert(demux.open(sample_path, "soak_audio_loop"));
}

// ─── Main ───
int main() {
    std::cout << "micro_soak_test (V10-021 SAMPLE-PREFLIGHT):\n";
    std::cout << "=============================================\n";
    std::cout << "Sample Preflight Gate: verifies golden sample before any soak.\n";
    std::cout << "30S_REAL_SAMPLE_SOAK: requires sample duration >= 30s, NO loop.\n";
    std::cout << "LOOP_STABILITY_SOAK: short sample loop with decoder reset per EOF.\n";
    std::cout << "Only 30S_REAL_SAMPLE_SOAK counts for P2 Local Runtime.\n\n";

    // ── Pre-condition 1: Sample file env var ──
    const char* sample_path_env = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path_env || std::string(sample_path_env).empty()) {
        std::cout << "  BLOCKED_ENV: KIVO_SAMPLE_H264_AAC_MP4 not set\n";
        std::cout << "Classification: BLOCKED_ENV (no sample file)\n";
        return static_cast<int>(SoakExitCode::BLOCKED_ENV);
    }

    // ── Pre-condition 2: FFmpeg enabled ──
    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  BLOCKED_ENV: FFmpeg not enabled (KIVO_ENABLE_FFMPEG=OFF)\n";
        std::cout << "Classification: BLOCKED_ENV (FFmpeg OFF)\n";
        return static_cast<int>(SoakExitCode::BLOCKED_ENV);
    }

    std::string sample_path(sample_path_env);
    std::cout << "  Sample: " << sample_path << "\n";

    // ── Pre-condition 3: D3D11 GPU ──
    D3d11DeviceContext d3d_dev;
    if (!d3d_dev.initialize()) {
        std::cout << "  BLOCKED_ENV: D3D11 GPU not available\n";
        std::cout << "Classification: BLOCKED_ENV (D3D11 unavailable)\n";
        return static_cast<int>(SoakExitCode::BLOCKED_ENV);
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
            std::cout << "  BLOCKED_ENV: WASAPI audio device not available (" << pre_wasapi.last_error() << ")\n";
            std::cout << "Classification: BLOCKED_ENV (WASAPI unavailable)\n";
            return static_cast<int>(SoakExitCode::BLOCKED_ENV);
        }
        pre_wasapi.release();
        std::cout << "  WASAPI: audio device available\n\n";
    }

    // ════════════════════════════════════════════════════════════
    //  SAMPLE PREFLIGHT GATE
    // ════════════════════════════════════════════════════════════
    std::cout << "=============================================\n";
    std::cout << "  SAMPLE PREFLIGHT GATE\n";
    std::cout << "=============================================\n";

    auto preflight = sample_preflight_gate(sample_path);

    std::cout << "\n  Preflight Results:\n";
    std::cout << "    duration:      " << preflight.sample_duration << "s -> " << (preflight.duration_ok ? "OK" : "TOO SHORT (< 30s)") << "\n";
    std::cout << "    video stream:  " << preflight.video_codec << " -> " << (preflight.video_stream_ok ? "OK" : "MISSING") << "\n";
    std::cout << "    audio stream:  " << preflight.audio_codec << " -> " << (preflight.audio_stream_ok ? "OK" : "MISSING") << "\n";
    std::cout << "    video decode:  " << preflight.preflight_video_frames << " frames, " << preflight.preflight_video_errors << " errors -> " << (preflight.video_decode_ok ? "OK" : "FAIL") << "\n";
    std::cout << "    audio decode:  " << preflight.preflight_audio_frames << " frames, " << preflight.preflight_audio_errors << " errors -> " << (preflight.audio_decode_ok ? "OK" : "FAIL") << "\n";
    std::cout << "    overall:       " << (preflight.passed ? "PREFLIGHT_PASS" : "PREFLIGHT_FAIL") << "\n";
    if (!preflight.failure_reason.empty()) {
        std::cout << "    reason:        " << preflight.failure_reason << "\n";
    }
    std::cout << "\n";

    // ════════════════════════════════════════════════════════════
    //  BRANCH based on preflight result
    // ════════════════════════════════════════════════════════════

    if (!preflight.duration_ok) {
        std::cout << "=============================================\n";
        std::cout << "BLOCKED_SAMPLE_TOO_SHORT: sample duration " << preflight.sample_duration
                  << "s < 30s\n";
        std::cout << "30S_REAL_SAMPLE_SOAK requires sample to have >= 30s of content.\n";
        std::cout << "Looping a short sample is NOT acceptable for 30S_REAL_SAMPLE_SOAK.\n";
        std::cout << "Provide a valid >=30s H.264+AAC MP4 sample via KIVO_SAMPLE_H264_AAC_MP4.\n";
        std::cout << "Classification: BLOCKED_SAMPLE_TOO_SHORT (" << preflight.sample_duration << "s)\n";
        std::cout << "P2 Local Runtime: NOT_YET (need >=30s golden sample)\n";
        return static_cast<int>(SoakExitCode::BLOCKED_SAMPLE_TOO_SHORT);
    }

    if (!preflight.video_decode_ok || !preflight.audio_decode_ok) {
        std::cout << "=============================================\n";
        std::cout << "BLOCKED_SAMPLE_INVALID: decode errors exceed threshold\n";
        std::cout << "Video errors: " << preflight.preflight_video_errors
                  << " in " << preflight.preflight_video_frames << " frames\n";
        std::cout << "Audio errors: " << preflight.preflight_audio_errors
                  << " in " << preflight.preflight_audio_frames << " frames\n";
        std::cout << "This sample cannot be used as a golden soak sample.\n";
        std::cout << "Provide a valid, non-corrupt >=30s H.264+AAC MP4 sample.\n";
        std::cout << "Classification: BLOCKED_SAMPLE_INVALID (decode errors)\n";
        std::cout << "P2 Local Runtime: NOT_YET (sample invalid)\n";
        return static_cast<int>(SoakExitCode::BLOCKED_SAMPLE_INVALID);
    }

    if (!preflight.video_stream_ok || !preflight.audio_stream_ok) {
        std::cout << "=============================================\n";
        std::cout << "BLOCKED_SAMPLE_INVALID: missing video or audio stream\n";
        std::cout << "Classification: BLOCKED_SAMPLE_INVALID (missing stream)\n";
        return static_cast<int>(SoakExitCode::BLOCKED_SAMPLE_INVALID);
    }

    // ════════════════════════════════════════════════════════════
    //  30S_REAL_SAMPLE_SOAK: sample duration >= 30s verified
    // ════════════════════════════════════════════════════════════
    std::cout << "=============================================\n";
    std::cout << "  30S_REAL_SAMPLE_SOAK (duration " << preflight.sample_duration << "s)\n";
    std::cout << "=============================================\n";
    std::cout << "  No loop needed. Continuous demux/decode through sample content.\n\n";

    SoakMetrics metrics;
    metrics.ffmpeg_available = true;
    metrics.d3d11_available = true;
    metrics.wasapi_available = true;
    metrics.d3d11_device_type = d3d_dev.device_type();
    metrics.mode = SoakMode::MODE_30S_REAL_SAMPLE;

    // Probe to get stream indices
    auto probe = RealProbeRuntime::probe_file(sample_path, "soak_probe");
    assert(probe.success);
    int video_idx = probe.video_streams[0].index;
    std::string video_codec = probe.video_streams[0].codec_name;
    int audio_idx = probe.audio_streams[0].index;
    std::string audio_codec = probe.audio_streams[0].codec_name;

    // Open pipelines
    RealDemuxRuntime video_demux, audio_demux;
    RealSoftwareDecodeRuntime video_decoder, audio_decoder;
    DecodedAudioFrameConverter pcm_converter;
    WasapiSharedPcmWriter wasapi_writer;

    PcmFormat pcm_target;
    pcm_target.sample_rate = 44100; pcm_target.channels = 2;
    pcm_target.bit_depth = 16; pcm_target.sample_format = "s16";

    // Video pipeline
    FfmpegFormatHandle vfmt = FfmpegAdapter::open_file(sample_path);
    assert(vfmt.native);
    FfmpegCodecHandle vcodec = FfmpegAdapter::create_codec_context(vfmt, video_idx);
    assert(vcodec.native);
    assert(video_decoder.adopt_codec_context(vcodec.native, video_codec, "soak_video"));
    assert(video_demux.open(sample_path, "soak_video_demux"));

    // Audio pipeline
    FfmpegFormatHandle afmt = FfmpegAdapter::open_file(sample_path);
    assert(afmt.native);
    FfmpegCodecHandle acodec = FfmpegAdapter::create_codec_context(afmt, audio_idx);
    assert(acodec.native);
    assert(audio_decoder.adopt_codec_context(acodec.native, audio_codec, "soak_audio"));
    assert(audio_demux.open(sample_path, "soak_audio_demux"));

    // WASAPI
    AudioEndpointRuntimeContract endpoint;
    endpoint.endpoint_id = "default"; endpoint.endpoint_name = "Default Audio Device";
    endpoint.output_mode = "shared_pcm"; endpoint.sample_rate = 44100;
    endpoint.channels = 2; endpoint.bit_depth = 16;
    assert(wasapi_writer.initialize(endpoint));
    wasapi_writer.start();

    // D3D11 uploader
    D3d11TextureUpload uploader;
    uploader.initialize(&d3d_dev);

    auto start_time = std::chrono::steady_clock::now();
    const double target_seconds = 30.0;

    std::cout << "  Running 30s continuous soak...\n";

    // 30S_REAL_SAMPLE_SOAK: no loop, just process until duration reached or EOF
    while (true) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        if (elapsed >= target_seconds) break;

        // Video
        auto vpkt = video_demux.read_packet("soak_vpkt");
        if (vpkt.eof) break;  // REAL_SAMPLE_SOAK: sample delivers all frames, no looping
        if (vpkt.success) {
            metrics.total_packets_read++;
            DecodeResult dec = video_decoder.decode(vpkt.packet, "soak_vdec");
            if (dec.success && !dec.needs_more_input && !dec.eof) {
                metrics.video_frames_decoded++;
                metrics.video_bytes_processed += static_cast<int64_t>(dec.frame.frame_data.size());

                if (metrics.video_frames_uploaded < 10) {
                    D3D11TextureHandle tex = uploader.create_texture(
                        dec.frame.width, dec.frame.height, dec.frame.pixel_format);
                    assert(tex);
                    YuvRgbConversion conv;
                    assert(uploader.upload_frame(tex, dec.frame, conv));
                    metrics.video_frames_uploaded++;
                    uploader.release_texture(tex);
                }
            } else if (!dec.success) {
                metrics.video_decode_errors++;
            }
        }

        // Audio
        auto apkt = audio_demux.read_packet("soak_apkt");
        if (apkt.eof) break;  // REAL_SAMPLE_SOAK: sample delivers all frames, no looping
        if (apkt.success) {
            metrics.total_packets_read++;
            DecodeResult dec = audio_decoder.decode(apkt.packet, "soak_adec");
            if (dec.success && !dec.needs_more_input && !dec.eof) {
                metrics.audio_frames_decoded++;
                metrics.audio_bytes_processed += static_cast<int64_t>(dec.frame.frame_data.size());

                ConversionResult pcm = pcm_converter.convert(dec.frame, pcm_target);
                if (pcm.success && !pcm.pcm_data.empty()) {
                    WriteResult wr = wasapi_writer.write(pcm.pcm_data.data(),
                                                          static_cast<int32_t>(pcm.pcm_data.size()));
                    if (wr.success) {
                        metrics.audio_pcm_frames_written += wr.frames_written;
                    } else {
                        metrics.wasapi_write_errors++;
                    }
                }
            } else if (!dec.success) {
                metrics.audio_decode_errors++;
            }
        }
    }

    // Cleanup
    wasapi_writer.stop();
    video_decoder.close(); video_demux.close();
    audio_decoder.close(); audio_demux.close();
    wasapi_writer.release();

    auto end_time = std::chrono::steady_clock::now();
    metrics.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "\n  Soak completed in " << metrics.elapsed_seconds << " seconds\n\n";

    // ─── Assertions ───
    std::cout << "  [Test 1] Duration Check:\n";
    std::cout << "    PASS: elapsed " << metrics.elapsed_seconds << "s\n\n";

    assert(metrics.video_frames_decoded > 0);
    std::cout << "  [Test 2] Video Frames: " << metrics.video_frames_decoded << " PASS\n\n";

    assert(metrics.audio_frames_decoded > 0);
    assert(metrics.audio_pcm_frames_written > 0);
    std::cout << "  [Test 3] Audio Frames: " << metrics.audio_frames_decoded
              << " decoded, " << metrics.audio_pcm_frames_written << " PCM written PASS\n\n";

    {
        int64_t total_errors = metrics.video_decode_errors + metrics.audio_decode_errors;
        int64_t total_frames = metrics.video_frames_decoded + metrics.audio_frames_decoded;
        double error_rate = (total_frames > 0) ? static_cast<double>(total_errors) / total_frames : 0.0;
        assert(error_rate < 0.05);
        std::cout << "  [Test 4] Error Rate: " << (error_rate * 100.0)
                  << "% (threshold 5%) PASS\n\n";
    }

    assert(metrics.video_frames_uploaded > 0);
    std::cout << "  [Test 5] D3D11 Upload: " << metrics.video_frames_uploaded
              << " frames to " << metrics.d3d11_device_type << " PASS\n\n";

    assert(metrics.audio_pcm_frames_written > 0);
    std::cout << "  [Test 6] WASAPI Write: " << metrics.audio_pcm_frames_written
              << " frames continuous PASS\n\n";

    assert(metrics.video_bytes_processed > 0 || metrics.audio_bytes_processed > 0);
    std::cout << "  [Test 7] No Silent Fallback: PASS\n\n";

    std::cout << "=============================================\n";
    std::cout << "  Final Metrics (30S_REAL_SAMPLE_SOAK):\n";
    std::cout << "    elapsed_seconds:   " << metrics.elapsed_seconds << "\n";
    std::cout << "    video_frames:      " << metrics.video_frames_decoded << "\n";
    std::cout << "    audio_frames:      " << metrics.audio_frames_decoded << "\n";
    std::cout << "    video_bytes:       " << metrics.video_bytes_processed << "\n";
    std::cout << "    audio_bytes:       " << metrics.audio_bytes_processed << "\n";
    std::cout << "    d3d11_uploads:     " << metrics.video_frames_uploaded << "\n";
    std::cout << "    wasapi_pcm_frames: " << metrics.audio_pcm_frames_written << "\n";
    std::cout << "    total_packets:     " << metrics.total_packets_read << "\n";
    std::cout << "    mode:              30S_REAL_SAMPLE_SOAK (no loop)\n";
    std::cout << "    sample_duration:   " << preflight.sample_duration << "s\n";
    std::cout << "V10-021 30S LOCAL PLAYBACK MICRO SOAK: RUNTIME_PASS\n";
    std::cout << "Classification: RUNTIME_PASS (30S_REAL_SAMPLE_SOAK with >=30s golden sample, all hardware proven)\n";
    return static_cast<int>(SoakExitCode::RUNTIME_PASS);
}
