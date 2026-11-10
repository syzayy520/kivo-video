#pragma once

#include "video/video_plane/core/audio/pcm_format.hpp"
#include "video/video_plane/core/decode/decode_outcome.hpp"
#include "video/video_plane/core/decode/sample_view.hpp"

namespace kivo::video::core::audio {

/// Borrowed interleaved PCM; valid until the next receive/flush or destruction.
struct PcmChunk {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::int64_t presentation_time_us = -1;
    std::int32_t frame_count = 0;
};

/// Feeding contract only. Backend-specific configuration stays outside core.
class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;
    virtual decode::DecodeOutcome start() = 0;
    virtual decode::DecodeOutcome submit(const decode::SampleView&, std::int64_t timeout_us) = 0;
    virtual decode::DecodeOutcome signal_end_of_stream(std::int64_t pts_us) = 0;
    virtual decode::DecodeOutcome receive(PcmChunk&, std::int64_t timeout_us) = 0;
    virtual decode::DecodeOutcome flush() = 0;
    virtual void stop() = 0;
    virtual std::string codec_name() const = 0;
    virtual const PcmFormat& output_format() const = 0;
    virtual bool output_format_diverged() const = 0;
};

}  // namespace kivo::video::core::audio
