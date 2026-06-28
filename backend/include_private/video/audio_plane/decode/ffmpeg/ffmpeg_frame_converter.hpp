// ffmpeg_frame_converter.hpp — P6C backend: FFmpeg AVFrame → DecodedAudioFrame converter (private interface)
// Private interface: AVFrame* allowed here (avcodec layer). NO avformat.
#pragma once

#include <cstdint>

#include "kivo/video/audio_plane/decode/decoded_audio_frame.hpp"
#include "ffmpeg_decode_config.hpp"

// Forward declaration of FFmpeg AVFrame (avcodec layer).
struct AVFrame;

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::DecodedAudioFrame;
using internal::FFmpegDecodeConfig;

// FFmpegFrameConverter
// Converts FFmpeg AVFrame (avcodec decoded output) to P6A DecodedAudioFrame.
// Also detects mid-stream format changes by comparing AVFrame params against
// the previously-known FFmpegDecodeConfig.
class FFmpegFrameConverter {
public:
    FFmpegFrameConverter() = default;

    // Convert an AVFrame to a DecodedAudioFrame.
    // pool_slot_id: P6 pool slot where the decoded frame will be stored.
    DecodedAudioFrame convert(const AVFrame* frame, uint64_t pool_slot_id) const;

    // Returns true if the AVFrame's params differ from prev config (format change).
    bool detect_format_changed(const AVFrame* frame, const FFmpegDecodeConfig& prev) const;
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
