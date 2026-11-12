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
    /// 做过 5.1→立体声下混的帧数。>0 说明确实折过（否则是 0）。
    std::int64_t downmixed_frames = 0;
    /// 下混后送进环的峰值。0 表示没下混过。
    float downmixed_peak = 0.0F;
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
    /// 声道数不在这里传参，而是每次从 `decoder.output_format()` 现取 ——
    /// 那是唯一事实来源。容器声明的格式（sample-rate/channel-count）只是**期望值**，
    /// 拿它去算转换等于把"平台悄悄改了格式"这件事掩盖掉：
    /// 声道数一旦不符，交错数据的步长就错了，听感是噪声而不是"格式不对"。
    ///
    /// **唯一的例外是 5.1 → 立体声**：见 `DownmixPolicy`。
    AudioDecodeFeeder(MediaExtractorSource& source,
                      core::audio::AudioDecoder& decoder,
                      core::audio::SpscAudioRingBuffer& ring);

    /// 是否允许在解码后、送入环之前做 5.1 → 立体声下混。
    ///
    /// 为什么需要它（这是 252 人声过小的根因，见 docs/evidence/p252/audio-downmix）：
    ///   真高清音轨是 5.1(side)，**人声几乎全部在 FC（中置声道）**。
    ///   实测 Shelter 的 TrueHD 轨：FC 占总能量 70.7%，FL/FR 各只有 12%/11%。
    ///   我们的 App 把 6 声道原样交给 AAudio，而设备 HAL 的输出端是立体声
    ///   （真机 dumpsys：我们的轨道 Chn mask 0x8000003F = 5.1 side，
    ///     输出线程 Mixer channel Mask 0x3 = front-left/front-right），
    ///   于是 6→2 的下混由**平台**做。平台那条路径把 FC 压到 −3 dB 计入，
    ///   对"人声几乎全在 FC"的素材，人声被明显压掉 —— 这正是用户听到的
    ///   "人声太小、听不清楚"。别的播放器（mpv/libmpv）都是
    ///   **自己**用 swresample 折成 2ch 再送设备，所以它们正常。
    ///
    /// 因此：**我们自己折**。用 AC3/Dolby 标准矩阵（FC 各 0.7071 进左右，
    /// 丢弃 LFE），折完再送 2 声道，平台就无需再动一次。
    ///
    /// 这里是**显式开关**而不是隐式重映射：只有 `kAllow` 时才折，
    /// 且只支持 5.1→立体声这一种组合，其它声道数组合仍然直接失败
    /// （与 PcmFormatConverter "明确不做声道映射"的规矩一致）。
    enum class DownmixPolicy {
        kDisallow = 0,  ///< 声道数必须与解码器输出一致（默认）
        kAllow = 1,     ///< 允许 5.1 → 立体声
    };

    /// 设定下混策略。必须在 `start()` 之前调用。
    void set_downmix_policy(DownmixPolicy policy) { downmix_policy_ = policy; }

    /// 实际发生过的下混次数（帧）。用于证据：>0 说明确实折过。
    [[nodiscard]] std::int64_t downmixed_frames() const {
        return stats_.downmixed_frames;
    }

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
    std::vector<float> downmixed_;  ///< 5.1→立体声下混后的暂存

    /// 下混策略。默认 kDisallow：声道数必须与解码器输出严格一致。
    DownmixPolicy downmix_policy_ = DownmixPolicy::kDisallow;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> finished_{false};

    AudioDecodeFeedStats stats_;
};

}  // namespace kivo::video::mediacodec
