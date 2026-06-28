// ffmpeg_decode_config.hpp — P6C internal: decode config + context helpers
// INTERNAL header: includes FFmpeg headers (allowed under backend/src/.../internal/).
// avformat/demux is FORBIDDEN — avcodec layer only.
#pragma once

#include <cstdint>
#include <utility>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>

namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal {

// Map an AVSampleFormat to (bit_depth, is_planar).
//   AV_SAMPLE_FMT_S16  -> (16, false)   AV_SAMPLE_FMT_S16P -> (16, true)
//   AV_SAMPLE_FMT_S32  -> (32, false)   AV_SAMPLE_FMT_S32P -> (32, true)
//   AV_SAMPLE_FMT_FLT  -> (32, false)   AV_SAMPLE_FMT_FLTP -> (32, true)
//   AV_SAMPLE_FMT_DBL  -> (64, false)   AV_SAMPLE_FMT_DBLP -> (64, true)
//   default            -> (0, false)
//
// For float formats FFmpeg uses 32-bit storage and double uses 64-bit; we
// report the storage bit depth (not the mantissa precision) so callers can
// size intermediate buffers consistently.
inline std::pair<int32_t, bool> extract_sample_format(AVSampleFormat fmt) noexcept {
    switch (fmt) {
        case AV_SAMPLE_FMT_S16:  return {16, false};
        case AV_SAMPLE_FMT_S16P: return {16, true};
        case AV_SAMPLE_FMT_S32:  return {32, false};
        case AV_SAMPLE_FMT_S32P: return {32, true};
        case AV_SAMPLE_FMT_FLT:  return {32, false};
        case AV_SAMPLE_FMT_FLTP: return {32, true};
        case AV_SAMPLE_FMT_DBL:  return {64, false};
        case AV_SAMPLE_FMT_DBLP: return {64, true};
        default:                 return {0, false};
    }
}

// Decoded-stream configuration snapshot. Mirrors the subset of AVCodecContext
// fields that the P6C decoder needs to reason about the audio format. The
// fields are intentionally plain scalar types so the struct can be copied
// freely without touching FFmpeg resources.
struct FFmpegDecodeConfig {
    int32_t sample_rate_hz{0};
    int32_t channel_count{0};
    int32_t bit_depth{0};
    int64_t time_base_num{0};
    int64_t time_base_den{0};
    bool is_planar{false};
};

// Apply the config to an AVCodecContext. No allocation is performed; only
// scalar fields are written. A non-positive channel_count leaves ch_layout
// untouched (caller is responsible for having set it, e.g. via the demuxer).
inline void apply_to_context(AVCodecContext* ctx,
                             const FFmpegDecodeConfig& config) noexcept {
    if (ctx == nullptr) {
        return;
    }
    if (config.sample_rate_hz > 0) {
        ctx->sample_rate = config.sample_rate_hz;
    }
    if (config.channel_count > 0) {
        av_channel_layout_default(&ctx->ch_layout, config.channel_count);
    }
    if (config.time_base_den > 0) {
        ctx->time_base.num = static_cast<int>(config.time_base_num);
        ctx->time_base.den = static_cast<int>(config.time_base_den);
    }
}

// Derive a config snapshot from an AVCodecContext. bit_depth / is_planar are
// derived from ctx->sample_fmt via extract_sample_format().
inline FFmpegDecodeConfig extract_from_context(
    const AVCodecContext* ctx) noexcept {
    FFmpegDecodeConfig config{};
    if (ctx == nullptr) {
        return config;
    }
    config.sample_rate_hz = ctx->sample_rate;
    config.channel_count = ctx->ch_layout.nb_channels;
    config.time_base_num = ctx->time_base.num;
    config.time_base_den = ctx->time_base.den;
    auto [bits, planar] = extract_sample_format(ctx->sample_fmt);
    config.bit_depth = bits;
    config.is_planar = planar;
    return config;
}

// True when the context's current format differs from `prev` in sample rate,
// channel count, or sample format (bit_depth / is_planar). Used to decide
// whether a mid-stream format change requires a decoder reconfigure.
//
// Note: is_planar is folded into bit_depth comparison via the underlying
// AVSampleFormat; comparing bit_depth alone is insufficient (S16 vs S16P
// share bit_depth=16). We therefore also compare is_planar.
inline bool detect_format_changed(const AVCodecContext* ctx,
                                  const FFmpegDecodeConfig& prev) noexcept {
    if (ctx == nullptr) {
        return false;
    }
    auto [bits, planar] = extract_sample_format(ctx->sample_fmt);
    if (ctx->sample_rate != prev.sample_rate_hz) {
        return true;
    }
    if (ctx->ch_layout.nb_channels != prev.channel_count) {
        return true;
    }
    if (bits != prev.bit_depth) {
        return true;
    }
    if (planar != prev.is_planar) {
        return true;
    }
    return false;
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal
