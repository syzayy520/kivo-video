#pragma once

#include <memory>
#include "video/video_plane/core/audio/audio_decoder.hpp"

namespace kivo::video::ffmpeg {

/// AC3/E-AC3 compressed samples to PCM; no demux, audio device, or clock ownership.
class FfmpegAudioDecoder final : public core::audio::AudioDecoder {
public:
    FfmpegAudioDecoder();
    ~FfmpegAudioDecoder() override;
    FfmpegAudioDecoder(const FfmpegAudioDecoder&) = delete;
    FfmpegAudioDecoder& operator=(const FfmpegAudioDecoder&) = delete;

    core::decode::DecodeOutcome configure(const std::string& mime,
                                         const core::audio::PcmFormat& declared);
    core::decode::DecodeOutcome start() override;
    core::decode::DecodeOutcome submit(const core::decode::SampleView&, std::int64_t) override;
    core::decode::DecodeOutcome signal_end_of_stream(std::int64_t) override;
    core::decode::DecodeOutcome receive(core::audio::PcmChunk&, std::int64_t) override;
    core::decode::DecodeOutcome flush() override;
    void stop() override;
    std::string codec_name() const override;
    const core::audio::PcmFormat& output_format() const override;
    bool output_format_diverged() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kivo::video::ffmpeg
