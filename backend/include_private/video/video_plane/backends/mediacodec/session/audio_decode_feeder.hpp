#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "video/video_plane/core/audio/audio_decoder.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

namespace kivo::video::mediacodec {

/// 解码线程的统计事实。**只有在 stop() 返回之后读取才有效**（线程已被 join）。
struct AudioDecodeFeedStats {
    std::int64_t packets_submitted = 0;  ///< 成功送进解码器的压缩包
    std::int64_t chunks_received = 0;    ///< 收到的 PCM 块数
    std::int64_t frames_decoded = 0;     ///< 解码出的 PCM 帧（含尚未写入环的部分）
    std::int64_t frames_written = 0;     ///< 真正写进环形缓冲的帧
    std::int64_t ring_full_waits = 0;    ///< 环满导致的等待次数（供大于求，正常背压）
    std::int64_t frames_dropped = 0;     ///< 因停止而丢弃的帧（非 0 必须在证据里显式出现）
    float decoded_peak = 0.0F;  ///< Actual decoded PCM amplitude.
    bool reached_eos = false;
    bool eos_flag_seen = false;          ///< 是否真的收到平台的 EOS 标志
    std::string error;                   ///< 空表示正常走完
};

/// 真实解码线程：解封装 → 音频解码 → 转 float → 写环形缓冲。
///
/// 与 `ToneRingProducer` 的区别只有一个——数据从哪来：
/// 那个造正弦并按倍速模拟供数，这个解真实音轨、以解码器产出的速度供数。
///
/// 供数节拍说明：**这里不需要 `sleep_until` 绝对时间轴**。
/// C2 的结论"解码线程必须按绝对时间轴供数"针对的是**没有环形缓冲**的直连模式；
/// 一旦中间有环，节奏就由"环满即停、环空即续"决定，生产快于消费会自然被环挡住，
/// 不会累积漂移，也不会人为引入延迟。
///
/// 线程约束：内部线程独占 source 与 decoder，两者都非线程安全，
/// 因此调用方在这条线程运行期间**不得**再触碰它们。
class AudioDecodeFeeder {
public:
    /// 转换格式不在这里传参，而是每次从 `decoder.output_format()` 现取 ——
    /// 那是唯一事实来源。容器声明的格式（sample-rate/channel-count）只是**期望值**，
    /// 拿它去算转换等于把"平台悄悄改了格式"这件事掩盖掉：
    /// 声道数一旦不符，交错数据的步长就错了，听感是噪声而不是"格式不对"。
    AudioDecodeFeeder(MediaExtractorSource& source,
                      core::audio::AudioDecoder& decoder,
                      core::audio::SpscAudioRingBuffer& ring);

    ~AudioDecodeFeeder();

    AudioDecodeFeeder(const AudioDecodeFeeder&) = delete;
    AudioDecodeFeeder& operator=(const AudioDecodeFeeder&) = delete;
    AudioDecodeFeeder(AudioDecodeFeeder&&) = delete;
    AudioDecodeFeeder& operator=(AudioDecodeFeeder&&) = delete;

    void start();

    /// 停止并等待线程退出。可重复调用。
    void stop();

    /// 等待环内可读帧数达到 `min_frames`，最多等 `timeout_ms`。
    ///
    /// 用途：必须在 `AaudioOutputSession::start()` **之前**把环灌起来。
    /// C2 已经吃过一次亏——开流之后才灌，开头那几十毫秒会被如实记成欠载，
    /// 掩盖后面真正要看的东西。返回是否达标。
    [[nodiscard]] bool wait_filled(std::int32_t min_frames, int timeout_ms) const;

    /// 解码线程是否已自行结束（EOS 或失败）。
    [[nodiscard]] bool finished() const { return finished_.load(std::memory_order_acquire); }

    [[nodiscard]] AudioDecodeFeedStats stats() const { return stats_; }

private:
    void loop();
    void drain_once(std::int64_t timeout_us, bool& got_data);
    void push_frames(const float* interleaved, std::int32_t frame_count);

    MediaExtractorSource& source_;
    core::audio::AudioDecoder& decoder_;
    core::audio::SpscAudioRingBuffer& ring_;

    std::vector<float> converted_;  ///< 转换暂存，仅在变大时分配

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> finished_{false};

    AudioDecodeFeedStats stats_;
};

}  // namespace kivo::video::mediacodec
