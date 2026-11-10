#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "video/video_plane/backends/mediacodec/decode/decode_outcome.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"

#include "video/video_plane/core/audio/audio_decoder.hpp"

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

namespace kivo::video::mediacodec {

using core::audio::PcmChunk;

/// MediaCodec 同步模式音频解码器。
///
/// 职责边界：送压缩包、收 PCM 字节、报输出格式。
/// 明确不做：写环形缓冲、转 float、计时、重采样 —— 全在调用方。
///
/// 与视频解码器的差别只有一处，但很关键：视频走 `releaseOutputBuffer(render=true)`
/// 直接进 Surface（零拷贝），音频必须把样本读进 CPU 内存 —— 音频本来就要交给
/// 音频设备，不存在"零拷贝上屏"这回事。G5 的零拷贝约束只针对视频。
class MediaCodecAudioDecoder final : public core::audio::AudioDecoder {
public:
    MediaCodecAudioDecoder() = default;
    ~MediaCodecAudioDecoder() override;

    MediaCodecAudioDecoder(const MediaCodecAudioDecoder&) = delete;
    MediaCodecAudioDecoder& operator=(const MediaCodecAudioDecoder&) = delete;
    MediaCodecAudioDecoder(MediaCodecAudioDecoder&&) = delete;
    MediaCodecAudioDecoder& operator=(MediaCodecAudioDecoder&&) = delete;

    /// 创建并配置解码器。输出到 CPU 缓冲（不传 Surface）。
    ///
    /// @param mime 轨道 mime（如 "audio/mp4a-latm"），按类型创建，**不按 codec 名**
    /// @param format 轨道格式，必须是解封装器给出的那一份
    [[nodiscard]] DecodeOutcome configure(const std::string& mime, const AMediaFormat* format);

    [[nodiscard]] DecodeOutcome start() override;

    /// 送一包。TRY_AGAIN 表示输入队列满，调用方应先把已解出的 PCM 收走。
    [[nodiscard]] DecodeOutcome submit(const SampleView& sample, std::int64_t timeout_us) override;

    [[nodiscard]] DecodeOutcome signal_end_of_stream(std::int64_t presentation_time_us) override;

    /// 收一块 PCM。
    ///
    /// - `kOk`：`out` 已被填充；
    /// - `kEndOfStream`：解码结束。若 `out.size > 0`，这一块仍要先消费掉；
    /// - `kTryAgain`：暂无数据（含"输出格式刚变化"）。
    [[nodiscard]] DecodeOutcome receive(PcmChunk& out, std::int64_t timeout_us) override;

    [[nodiscard]] DecodeOutcome flush() override;

    void stop() override;

    /// 平台实际选中的 codec 名，仅用于证据与日志，**不得**用于逻辑判断。
    [[nodiscard]] std::string codec_name() const override;

    /// 最近一次读到的输出格式。收到首块 PCM 之前 `encoding` 为 `kUnknown`。
    [[nodiscard]] const core::audio::PcmFormat& output_format() const override { return output_format_; }

    /// 输出格式与容器声明不一致时置位（采样率/声道/编码任一不同）。
    /// 上层据此显式失败——采样率错了，音高与时长全错，不能"能出声就行"。
    [[nodiscard]] bool output_format_diverged() const override { return format_diverged_; }

    /// 给上层比对用的声明值。configure 时记录，不参与解码。
    [[nodiscard]] const core::audio::PcmFormat& declared_format() const { return declared_; }
    void set_declared_format(const core::audio::PcmFormat& declared) { declared_ = declared; }

private:
    DecodeOutcome fail(const std::string& message);
    DecodeOutcome refresh_output_format();
    void release();

    AMediaCodec* codec_ = nullptr;
    bool configured_ = false;
    bool started_ = false;

    core::audio::PcmFormat declared_{};
    core::audio::PcmFormat output_format_{};
    bool format_diverged_ = false;

    std::vector<std::uint8_t> staging_;  ///< 首次 receive 时按需增长，之后复用
};

}  // namespace kivo::video::mediacodec
