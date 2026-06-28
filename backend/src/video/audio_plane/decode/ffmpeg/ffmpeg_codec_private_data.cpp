// ffmpeg_codec_private_data.cpp — P6C backend: codec private data adapter implementation
// avcodec layer only — NO avformat. Uses av_mallocz for extradata allocation.
#include "video/audio_plane/decode/ffmpeg/ffmpeg_codec_private_data.hpp"

#include <cstring>
// Forward-include the full FFmpeg header here (not in the private interface .hpp,
// to keep the interface header lean).
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
}

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

kivo::Expected<std::vector<uint8_t>, LoadError>
FFmpegCodecPrivateDataAdapter::load(uint64_t codec_private_data_ref) {
    // ref=0 means "no codec private data" — valid for codecs that don't need
    // extradata (MP3, AC3, DTS, etc.). Returns empty vector.
    // A non-zero ref would resolve to real bytes (fake: also empty for now).
    return std::vector<uint8_t>{};
}

void FFmpegCodecPrivateDataAdapter::apply_to_context(
    AVCodecContext* ctx, const std::vector<uint8_t>& data) const {
    if (!ctx || data.empty()) {
        return;
    }
    // Free any existing extradata.
    av_freep(&ctx->extradata);
    ctx->extradata_size = 0;

    // Allocate with AV_INPUT_BUFFER_PADDING_SIZE padding (required by avcodec).
    const size_t alloc_size = data.size() + AV_INPUT_BUFFER_PADDING_SIZE;
    ctx->extradata = static_cast<uint8_t*>(
        av_mallocz(alloc_size));
    if (!ctx->extradata) {
        return;  // allocation failed — leave extradata null
    }
    std::memcpy(ctx->extradata, data.data(), data.size());
    ctx->extradata_size = static_cast<int>(data.size());
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
