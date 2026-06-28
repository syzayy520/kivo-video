// ffmpeg_codec_private_data.hpp — P6C backend: codec private data adapter (private interface)
// Private interface: AVCodecContext* allowed here (avcodec layer). NO avformat.
#pragma once

#include <cstdint>
#include <vector>

#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"

// Forward declaration of FFmpeg AVCodecContext (avcodec layer).
struct AVCodecContext;

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

// LoadError: why loading codec private data failed.
enum class LoadError : int32_t {
    InvalidRef = 0,
    NotFound = 1,
    Empty = 2,
};

// FFmpegCodecPrivateDataAdapter
// Bridges P6 codec private data handle references to FFmpeg AVCodecContext extradata.
// - load(): resolves a P6 data handle ref to raw bytes.
//   FAKE implementation: returns empty vector (real resolution from P6 data handle deferred).
// - apply_to_context(): sets ctx->extradata (av_mallocz with AV_INPUT_BUFFER_PADDING_SIZE),
//   ctx->extradata_size, and memcpy's the data.
class FFmpegCodecPrivateDataAdapter {
public:
    FFmpegCodecPrivateDataAdapter() = default;

    // Load codec private data bytes from a P6 data handle reference.
    // Returns empty vector for now (fake — real resolution deferred to integration).
    kivo::Expected<std::vector<uint8_t>, LoadError> load(uint64_t codec_private_data_ref);

    // Apply loaded codec private data to an AVCodecContext.
    // Allocates extradata with AV_INPUT_BUFFER_PADDING_SIZE padding,
    // copies the data, and sets extradata_size.
    void apply_to_context(AVCodecContext* ctx, const std::vector<uint8_t>& data) const;
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
