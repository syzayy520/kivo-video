// ffmpeg_frame_converter_tests.cpp — P6C tests: FFmpegFrameConverter (C9-C12)
// Tests AVFrame → DecodedAudioFrame conversion + format change detection.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
}

#include "video/audio_plane/decode/ffmpeg/ffmpeg_frame_converter.hpp"
#include "ffmpeg_decode_config.hpp"
#include "kivo/video/audio_plane/decode/decoded_audio_frame.hpp"

namespace rt = kivo::video::audio_plane::backend::decode::ffmpeg;
using kivo::video::audio_plane::decode::DecodedAudioFrame;

static int g_failed = 0;

#define P6C_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// C9: convert fake AVFrame → DecodedAudioFrame with correct fields
static void test_c09_convert_frame() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) throw std::runtime_error("av_frame_alloc failed");
    frame->sample_rate = 48000;
    frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&frame->ch_layout, 2);
    frame->pts = 1000;

    rt::FFmpegFrameConverter converter;
    auto result = converter.convert(frame, 42);
    av_frame_free(&frame);

    if (result.sample_rate_hz != 48000) throw std::runtime_error("sample_rate must be 48000");
    if (result.channel_count != 2) throw std::runtime_error("channel_count must be 2");
    if (result.pts != 1000) throw std::runtime_error("pts must be 1000");
    if (result.pool_slot_id != 42) throw std::runtime_error("pool_slot_id must be 42");
    // frame_id is set by receive_frame(), not by convert(); skip frame_id check here.
    std::printf("[C09] convert AVFrame PASS\n");
}

// C10: detect_format_changed — same format → false
static void test_c10_no_format_change() {
    AVFrame* frame = av_frame_alloc();
    frame->sample_rate = 48000;
    frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&frame->ch_layout, 2);

    rt::FFmpegDecodeConfig prev;
    prev.sample_rate_hz = 48000;
    prev.channel_count = 2;
    prev.bit_depth = 16;

    rt::FFmpegFrameConverter converter;
    bool changed = converter.detect_format_changed(frame, prev);
    av_frame_free(&frame);

    if (changed) throw std::runtime_error("format change must be false for same format");
    std::printf("[C10] no format change PASS\n");
}

// C11: detect_format_changed — different sample_rate → true
static void test_c11_format_change_sample_rate() {
    AVFrame* frame = av_frame_alloc();
    frame->sample_rate = 44100;
    frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&frame->ch_layout, 2);

    rt::FFmpegDecodeConfig prev;
    prev.sample_rate_hz = 48000;
    prev.channel_count = 2;
    prev.bit_depth = 16;

    rt::FFmpegFrameConverter converter;
    bool changed = converter.detect_format_changed(frame, prev);
    av_frame_free(&frame);

    if (!changed) throw std::runtime_error("format change must be true for different sample_rate");
    std::printf("[C11] format change sample_rate PASS\n");
}

// C12: detect_format_changed — different channel count → true
static void test_c12_format_change_channels() {
    AVFrame* frame = av_frame_alloc();
    frame->sample_rate = 48000;
    frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&frame->ch_layout, 6);  // 5.1

    rt::FFmpegDecodeConfig prev;
    prev.sample_rate_hz = 48000;
    prev.channel_count = 2;
    prev.bit_depth = 16;

    rt::FFmpegFrameConverter converter;
    bool changed = converter.detect_format_changed(frame, prev);
    av_frame_free(&frame);

    if (!changed) throw std::runtime_error("format change must be true for different channels");
    std::printf("[C12] format change channels PASS\n");
}

int main() {
    std::printf("=== P6C Frame Converter Tests ===\n\n");

    P6C_RUN(test_c09_convert_frame);
    P6C_RUN(test_c10_no_format_change);
    P6C_RUN(test_c11_format_change_sample_rate);
    P6C_RUN(test_c12_format_change_channels);

    if (g_failed == 0) {
        std::printf("\n=== P6C Converter: ALL PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6C Converter: %d FAILED ===\n", g_failed);
        return 1;
    }
}
