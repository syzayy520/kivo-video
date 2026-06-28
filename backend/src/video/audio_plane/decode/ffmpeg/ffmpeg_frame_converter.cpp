// ffmpeg_frame_converter.cpp — P6C backend: FFmpegFrameConverter implementation
// Converts AVFrame (avcodec) → P6A DecodedAudioFrame. NO avformat.
#include "video/audio_plane/decode/ffmpeg/ffmpeg_frame_converter.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::DecodedAudioFrame;
using internal::FFmpegDecodeConfig;

DecodedAudioFrame FFmpegFrameConverter::convert(
    const AVFrame* frame, uint64_t pool_slot_id) const {
    DecodedAudioFrame result;
    if (!frame) return result;
    result.sample_rate_hz = frame->sample_rate;
    result.channel_count = frame->ch_layout.nb_channels;
    result.pts = frame->pts;
    result.duration = frame->duration;
    result.pool_slot_id = pool_slot_id;
    result.is_key_frame = (frame->key_frame != 0);
    // bit_depth: derive from AVSampleFormat via internal helper (no extra link deps).
    auto [bits, planar] = internal::extract_sample_format(
        static_cast<AVSampleFormat>(frame->format));
    result.bit_depth = bits;
    return result;
}

bool FFmpegFrameConverter::detect_format_changed(
    const AVFrame* frame, const FFmpegDecodeConfig& prev) const {
    if (!frame) return false;
    if (frame->sample_rate != prev.sample_rate_hz) return true;
    if (frame->ch_layout.nb_channels != prev.channel_count) return true;
    auto [bits, planar] = internal::extract_sample_format(
        static_cast<AVSampleFormat>(frame->format));
    if (bits != prev.bit_depth) return true;
    if (planar != prev.is_planar) return true;
    return false;
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
