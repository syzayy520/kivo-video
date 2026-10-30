#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "video/video_plane/backends/mediacodec/decode/decode_outcome.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

namespace kivo::video::mediacodec {

/// 一块解码后的 PCM。数据指向解码器内部的暂存区，**下次 receive 前有效**。
///
/// 为什么要拷贝而不是把平台缓冲直接交出去：
/// 平台缓冲在 `releaseOutputBuffer` 之前才有效，而调用方要做"转 float → 写环形缓冲"
/// 两步，中间还可能因为环满而等待。把平台缓冲的生命周期跨到调用方手里，
/// 只要有一处忘记归还就是永久卡死。拷贝代价是 48kHz 立体声约 192KB/s，可忽略。
struct PcmChunk {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;                 ///< 字节数
    std::int64_t presentation_time_us = -1;
    std::int32_t frame_count = 0;         ///< 按当前输出格式换算的帧数
};

/// MediaCodec 同步模式音频解码器。
///
/// 职责边界：送压缩包、收 PCM 字节、报输出格式。
/// 明确不做：写环形缓冲、转 float、计时、重采样 —— 全在调用方。
///
/// 与视频解码器的差别只有一处，但很关键：视频走 `releaseOutputBuffer(render=true)`
/// 直接进 Surface（零拷贝），音频必须把样本读进 CPU 内存 —— 音频本来就要交给
/// 音频设备，不存在"零拷贝上屏"这回事。G5 的零拷贝约束只针对视频。
class MediaCodecAudioDecoder {
public:
    MediaCodecAudioDecoder() = default;
    ~MediaCodecAudioDecoder();

    MediaCodecAudioDecoder(const MediaCodecAudioDecoder&) = delete;
    MediaCodecAudioDecoder& operator=(const MediaCodecAudioDecoder&) = delete;
    MediaCodecAudioDecoder(MediaCodecAudioDecoder&&) = delete;
    MediaCodecAudioDecoder& operator=(MediaCodecAudioDecoder&&) = delete;

    /// 创建并配置解码器。输出到 CPU 缓冲（不传 Surface）。
    ///
    /// @param mime 轨道 mime（如 "audio/mp4a-latm"），按类型创建，**不按 codec 名**
    /// @param format 轨道格式，必须是解封装器给出的那一份
    [[nodiscard]] DecodeOutcome configure(const std::string& mime, const AMediaFormat* format);

    [[nodiscard]] DecodeOutcome start();

    /// 送一包。TRY_AGAIN 表示输入队列满，调用方应先把已解出的 PCM 收走。
    [[nodiscard]] DecodeOutcome submit(const SampleView& sample, std::int64_t timeout_us);

    [[nodiscard]] DecodeOutcome signal_end_of_stream(std::int64_t presentation_time_us);

    /// 收一块 PCM。
    ///
    /// - `kOk`：`out` 已被填充；
    /// - `kEndOfStream`：解码结束。若 `out.size > 0`，这一块仍要先消费掉；
    /// - `kTryAgain`：暂无数据（含"输出格式刚变化"）。
    [[nodiscard]] DecodeOutcome receive(PcmChunk& out, std::int64_t timeout_us);

    [[nodiscard]] DecodeOutcome flush();

    void stop();

    /// 平台实际选中的 codec 名，仅用于证据与日志，**不得**用于逻辑判断。
    [[nodiscard]] std::string codec_name() const;

    /// 最近一次读到的输出格式。收到首块 PCM 之前 `encoding` 为 `kUnknown`。
    [[nodiscard]] const core::audio::PcmFormat& output_format() const { return output_format_; }

    /// 输出格式与容器声明不一致时置位（采样率/声道/编码任一不同）。
    /// 上层据此显式失败——采样率错了，音高与时长全错，不能"能出声就行"。
    [[nodiscard]] bool output_format_diverged() const { return format_diverged_; }

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
