#include "video/video_plane/backends/mediacodec/decode/mediacodec_audio_decoder.hpp"

#include "video/video_plane/backends/mediacodec/decode/mediacodec_codec_name_resolver.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediacodec_output_pcm_format.hpp"

#include <cstring>

namespace kivo::video::mediacodec {
namespace {

constexpr std::int64_t kEosPtsUs = 0;

[[nodiscard]] std::string status_text(media_status_t status) {
    return "status=" + std::to_string(static_cast<int>(status));
}

}  // namespace

MediaCodecAudioDecoder::~MediaCodecAudioDecoder() {
    release();
}

void MediaCodecAudioDecoder::release() {
    if (codec_ != nullptr) {
        if (started_) {
            AMediaCodec_stop(codec_);
        }
        AMediaCodec_delete(codec_);
        codec_ = nullptr;
    }
    configured_ = false;
    started_ = false;
}

DecodeOutcome MediaCodecAudioDecoder::fail(const std::string& message) {
    return {DecodeOutcome::Code::kFailed, message};
}

DecodeOutcome MediaCodecAudioDecoder::configure(const std::string& mime,
                                                const AMediaFormat* format) {
    if (codec_ != nullptr) {
        return fail("audio decoder already configured");
    }
    if (format == nullptr) {
        return fail("format is null");
    }

    AMediaCodec* codec = AMediaCodec_createDecoderByType(mime.c_str());
    if (codec == nullptr) {
        return fail("no audio decoder for mime " + mime);
    }

    // 第三个参数为 nullptr：音频没有 Surface，输出就是 CPU 能直接读的 PCM 缓冲。
    const media_status_t status = AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
    if (status != AMEDIA_OK) {
        AMediaCodec_delete(codec);
        return fail("configure failed for " + mime + " (" + status_text(status) + ")");
    }

    codec_ = codec;
    configured_ = true;
    return {};
}

DecodeOutcome MediaCodecAudioDecoder::start() {
    if (codec_ == nullptr || !configured_) {
        return fail("start called before configure");
    }
    const media_status_t status = AMediaCodec_start(codec_);
    if (status != AMEDIA_OK) {
        return fail("AMediaCodec_start failed (" + status_text(status) + ")");
    }
    started_ = true;
    return {};
}

DecodeOutcome MediaCodecAudioDecoder::submit(const SampleView& sample, std::int64_t timeout_us) {
    if (codec_ == nullptr || !started_) {
        return fail("submit called before start");
    }
    if (sample.is_encrypted) {
        return fail("encrypted audio sample is not supported yet");
    }

    const ssize_t index = AMediaCodec_dequeueInputBuffer(codec_, timeout_us);
    if (index < 0) {
        if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            return {DecodeOutcome::Code::kTryAgain, "input queue full"};
        }
        return fail("dequeueInputBuffer returned " + std::to_string(index));
    }

    std::size_t capacity = 0;
    std::uint8_t* buffer = AMediaCodec_getInputBuffer(codec_, static_cast<std::size_t>(index),
                                                      &capacity);
    if (buffer == nullptr) {
        return fail("getInputBuffer returned null for index " + std::to_string(index));
    }
    if (sample.size > capacity) {
        return fail("sample size " + std::to_string(sample.size) +
                    " exceeds input buffer capacity " + std::to_string(capacity));
    }
    if (sample.size > 0) {
        std::memcpy(buffer, sample.data, sample.size);
    }

    const media_status_t status = AMediaCodec_queueInputBuffer(
        codec_, static_cast<std::size_t>(index), 0, sample.size,
        static_cast<std::uint64_t>(sample.presentation_time_us), 0);
    if (status != AMEDIA_OK) {
        return fail("queueInputBuffer failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecAudioDecoder::signal_end_of_stream(std::int64_t presentation_time_us) {
    if (codec_ == nullptr || !started_) {
        return fail("signalEndOfStream called before start");
    }

    const ssize_t index = AMediaCodec_dequeueInputBuffer(codec_, 8000);
    if (index < 0) {
        if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            return {DecodeOutcome::Code::kTryAgain, "input queue full before EOS"};
        }
        return fail("dequeueInputBuffer for EOS returned " + std::to_string(index));
    }

    const media_status_t status = AMediaCodec_queueInputBuffer(
        codec_, static_cast<std::size_t>(index), 0, 0,
        static_cast<std::uint64_t>(presentation_time_us < 0 ? kEosPtsUs : presentation_time_us),
        AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
    if (status != AMEDIA_OK) {
        return fail("queueing EOS failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecAudioDecoder::refresh_output_format() {
    AMediaFormat* format = AMediaCodec_getOutputFormat(codec_);
    if (format == nullptr) {
        return fail("AMediaCodec_getOutputFormat returned null");
    }
    const core::audio::PcmFormat pcm = read_output_pcm_format(format);
    AMediaFormat_delete(format);

    if (!pcm.is_valid()) {
        return fail("decoder output is not usable PCM (rate=" + std::to_string(pcm.sample_rate) +
                    " channels=" + std::to_string(pcm.channel_count) +
                    " encoding=" + std::string(core::audio::pcm_encoding_name(pcm.encoding)) + ")");
    }

    // 只比采样率与声道：容器里没有 pcm-encoding 这一项，
    // 拿它做"是否背离"的判据会在每台设备上都误报。
    if (declared_.is_valid()) {
        if (pcm.sample_rate != declared_.sample_rate || pcm.channel_count != declared_.channel_count) {
            format_diverged_ = true;
        }
    }

    output_format_ = pcm;
    return {};
}

DecodeOutcome MediaCodecAudioDecoder::receive(PcmChunk& out, std::int64_t timeout_us) {
    out = PcmChunk{};
    if (codec_ == nullptr || !started_) {
        return fail("receive called before start");
    }

    AMediaCodecBufferInfo info{};
    const ssize_t index = AMediaCodec_dequeueOutputBuffer(codec_, &info, timeout_us);

    if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        const DecodeOutcome formatted = refresh_output_format();
        if (!formatted.ok()) {
            return formatted;
        }
        return {DecodeOutcome::Code::kTryAgain, "output format changed"};
    }
    if (index == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        return {DecodeOutcome::Code::kTryAgain, "output buffers changed"};
    }
    if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
        return {DecodeOutcome::Code::kTryAgain, "no pcm ready"};
    }
    if (index < 0) {
        return fail("dequeueOutputBuffer returned " + std::to_string(index));
    }

    const auto out_index = static_cast<std::size_t>(index);

    // 首块到达时才读输出格式：configure 之后的格式里还没有声道数与采样率。
    if (output_format_.encoding == core::audio::PcmEncoding::kUnknown) {
        const DecodeOutcome formatted = refresh_output_format();
        if (!formatted.ok()) {
            AMediaCodec_releaseOutputBuffer(codec_, out_index, false);
            return formatted;
        }
    }

    std::size_t capacity = 0;
    std::uint8_t* buffer = AMediaCodec_getOutputBuffer(codec_, out_index, &capacity);
    const bool eos = (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0;

    if (buffer == nullptr) {
        AMediaCodec_releaseOutputBuffer(codec_, out_index, false);
        return fail("getOutputBuffer returned null for index " + std::to_string(index));
    }
    if (info.offset < 0 || info.size < 0) {
        AMediaCodec_releaseOutputBuffer(codec_, out_index, false);
        return fail("decoder reported negative offset/size");
    }

    const auto offset = static_cast<std::size_t>(info.offset);
    const auto size = static_cast<std::size_t>(info.size);
    if (offset + size > capacity) {
        AMediaCodec_releaseOutputBuffer(codec_, out_index, false);
        return fail("output range " + std::to_string(offset) + "+" + std::to_string(size) +
                    " exceeds buffer capacity " + std::to_string(capacity));
    }

    if (size > 0) {
        if (staging_.size() < size) {
            staging_.resize(size);
        }
        std::memcpy(staging_.data(), buffer + offset, size);
    }

    // 数据已经拷走，可以立刻归还平台缓冲；render 标志对音频无意义，必须是 false。
    AMediaCodec_releaseOutputBuffer(codec_, out_index, false);

    const std::int32_t frame_size = output_format_.frame_size_bytes();
    if (size > 0 && (frame_size <= 0 || size % static_cast<std::size_t>(frame_size) != 0)) {
        return fail("decoder produced " + std::to_string(size) + " bytes, not a multiple of frame size " +
                    std::to_string(frame_size));
    }

    out.data = (size > 0) ? staging_.data() : nullptr;
    out.size = size;
    out.presentation_time_us = info.presentationTimeUs;
    out.frame_count = (frame_size > 0) ? static_cast<std::int32_t>(size / static_cast<std::size_t>(frame_size)) : 0;

    if (eos) {
        return {DecodeOutcome::Code::kEndOfStream, "decoder reached end of stream"};
    }
    return {};
}

DecodeOutcome MediaCodecAudioDecoder::flush() {
    if (codec_ == nullptr) {
        return fail("flush called before configure");
    }
    const media_status_t status = AMediaCodec_flush(codec_);
    if (status != AMEDIA_OK) {
        return fail("AMediaCodec_flush failed (" + status_text(status) + ")");
    }
    return {};
}

void MediaCodecAudioDecoder::stop() {
    if (codec_ == nullptr || !started_) {
        return;
    }
    AMediaCodec_stop(codec_);
    started_ = false;
}

std::string MediaCodecAudioDecoder::codec_name() const {
    return resolve_codec_name(codec_);
}

}  // namespace kivo::video::mediacodec
